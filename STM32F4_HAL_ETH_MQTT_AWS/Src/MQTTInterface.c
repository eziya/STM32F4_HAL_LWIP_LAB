#include "MQTTInterface.h"
#include "stm32f4xx_hal.h"

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/platform.h"

#include <string.h>
#include "lwip.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <string.h>

#define DEBUG_LEVEL 1

//Amazon ECC 256 certificate
const char mbedtls_aws_root_certificate[] =
		"-----BEGIN CERTIFICATE-----\r\n"
		"MIIBtjCCAVugAwIBAgITBmyf1XSXNmY/Owua2eiedgPySjAKBggqhkjOPQQDAjA5\r\n"
		"MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6b24g\r\n"
		"Um9vdCBDQSAzMB4XDTE1MDUyNjAwMDAwMFoXDTQwMDUyNjAwMDAwMFowOTELMAkG\r\n"
		"A1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJvb3Qg\r\n"
		"Q0EgMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABCmXp8ZBf8ANm+gBG1bG8lKl\r\n"
		"ui2yEujSLtf6ycXYqm0fc4E7O5hrOXwzpcVOho6AF2hiRVd9RFgdszflZwjrZt6j\r\n"
		"QjBAMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMB0GA1UdDgQWBBSr\r\n"
		"ttvXBp43rDCGB5Fwx5zEGbF4wDAKBggqhkjOPQQDAgNJADBGAiEA4IWSoxe3jfkr\r\n"
		"BqWTrBqYaGFy+uGh0PsceGCmQ5nFuMQCIQCcAu/xlJyzlvnrxir4tiz+OpAUFteM\r\n"
		"YyRIHN8wfdVoOw==\r\n"
		"-----END CERTIFICATE-----\r\n";

//client certificate here
const char mbedtls_client_certificate[] =
		"-----BEGIN CERTIFICATE-----\r\n"

		"-----END CERTIFICATE-----\r\n";

//client private key here
const char mbedtls_client_key[] =
		"-----BEGIN RSA PRIVATE KEY-----\r\n"

		"-----END RSA PRIVATE KEY-----\r\n";


const size_t mbedtls_aws_root_certificate_len = sizeof(mbedtls_aws_root_certificate);
const size_t mbedtls_client_certificate_len = sizeof(mbedtls_client_certificate);
const size_t mbedtls_client_key_len = sizeof(mbedtls_client_key);

//if you want to use static memory, enable MBEDTLS_MEMORY_BUFFER_ALLOC_C
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#define MEMORY_HEAP_SIZE      (1024*64)
uint8_t alloc_buf[MEMORY_HEAP_SIZE];
#endif

mbedtls_net_context server_fd;
const char *pers = "mbedtls";

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;
mbedtls_x509_crt cli_cert;
mbedtls_pk_context cli_key;

//freertos calloc & free
#if !defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
void *mbedtls_calloc( size_t n, size_t size )
{
	const size_t poolSize = n * size;
	void *p = pvPortMalloc(poolSize);
	if (p != NULL)
	{
		memset(p, 0, poolSize);
	}
	return p;
}

void mbedtls_free( void *ptr )
{
	vPortFree(ptr);
}
#endif

static void my_debug(void *ctx, int level, const char *file, int line, const char *str) {
	((void) level);
	mbedtls_fprintf((FILE*) ctx, "%s:%04d: %s", file, line, str);
	fflush((FILE*) ctx);
}

int net_init(Network *n, char *host) {
	int ret;

	//if you want to use static memory, enable MBEDTLS_MEMORY_BUFFER_ALLOC_C
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_init(alloc_buf, sizeof(alloc_buf));
#endif

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

	//mbedtls_net_init(&server_fd); //MX_LWIP_Init() is called already in "StartDefaultTask"
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);

	mbedtls_x509_crt_init(&cacert);
	mbedtls_x509_crt_init(&cli_cert);
	mbedtls_pk_init(&cli_key);

	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*) pers,
			strlen(pers))) != 0) {
		return -1;
	}

	//parse root CA certificate
	ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char*) mbedtls_aws_root_certificate, mbedtls_aws_root_certificate_len);
	if (ret < 0) {
		printf("mbedtls_x509_crt_parse failed.\n");
		return -1;
	}

	//parse client certificate
	ret = mbedtls_x509_crt_parse(&cli_cert, (const unsigned char *) mbedtls_client_certificate, mbedtls_client_certificate_len);
	if (ret < 0) {
		printf("mbedtls_x509_crt_parse failed.\n");
		return -1;
	}

	//parse client private key
	ret = mbedtls_pk_parse_key(&cli_key, (const unsigned char *)mbedtls_client_key, mbedtls_client_key_len , (unsigned char const *)"", 0);
	if (ret < 0) {
		printf("mbedtls_pk_parse_key failed.\n");
		return -1;
	}

	//configure ssl
	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret < 0) {
		printf("mbedtls_ssl_config_defaults failed.\n");
		return -1;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

	//config client certificate & key
	ret = mbedtls_ssl_conf_own_cert(&conf, &cli_cert, &cli_key);
	if (ret < 0) {
		printf("mbedtls_ssl_conf_own_cert failed.\n");
		return -1;
	}

	//set timeout 1000ms, mbedtls_ssl_conf_read_timeout has problem with accurate timeout
	mbedtls_ssl_conf_read_timeout(&conf, 1000);

	//ssl setup
	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret < 0) {
		printf("mbedtls_ssl_setup failed.\n");
		return -1;
	}

	//set hostname
	ret = mbedtls_ssl_set_hostname(&ssl, host);
	if (ret < 0) {
		printf("mbedtls_ssl_set_hostname failed.\n");
		return -1;
	}

	//set bio
	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	//register functions for MQTT
	n->mqttread = net_read; //receive function
	n->mqttwrite = net_write; //send function
	n->disconnect = net_disconnect; //disconnection function

	return 0;
}

int net_connect(Network *n, char *host, char* port) {
	int ret;

	//connect
	ret = mbedtls_net_connect(&server_fd, host, port, MBEDTLS_NET_PROTO_TCP);
	if (ret < 0) {
		printf("mbedtls_net_connect failed.\n");
		return -1;
	}

	//handshake
	while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
			{
				printf("mbedtls_ssl_handshake certificate verification failed.\n");
			}
			else
			{
				printf("mbedtls_ssl_handshake failed.\n");
			}

			return -1;
		}
	}

	//verify
	ret = mbedtls_ssl_get_verify_result(&ssl);
	if (ret < 0) {
		printf("mbedtls_ssl_get_verify_result failed.\n");
		return -1;
	}

	return 0;
}

//receive data
int net_read(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	int ret;
	int received = 0;
	int error = 0;
	int complete = 0;

	//set timeout
	if (timeout_ms != 0) {
		mbedtls_ssl_conf_read_timeout(&conf, timeout_ms);
	}

	//read until received length is bigger than variable len
	do {
		ret = mbedtls_ssl_read(&ssl, buffer, len);
		if (ret > 0) {
			received += ret;
		} else if (ret != MBEDTLS_ERR_SSL_WANT_READ) {
			error = 1;
		}
		if (received >= len) {
			complete = 1;
		}
	} while (!error && !complete);

	return received;
}

//send data
int net_write(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	int ret;
	int written;

	//check all bytes are written
	for (written = 0; written < len; written += ret) {
		while ((ret = mbedtls_ssl_write(&ssl, buffer + written, len - written)) <= 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				return ret;
			}
		}
	}

	return written;
}

//disconnect ssl
void net_disconnect(Network *n) {
	int ret;

	do {
		ret = mbedtls_ssl_close_notify(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	mbedtls_ssl_session_reset(&ssl);
	mbedtls_net_free(&server_fd);
}

//clear resources
void net_clear() {
	mbedtls_net_free(&server_fd);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_x509_crt_free(&cli_cert);
	mbedtls_pk_free(&cli_key);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_free();
#endif
}

uint32_t MilliTimer;

//Timer functions
char TimerIsExpired(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}

void TimerCountdownMS(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

void TimerCountdown(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}

int TimerLeftMS(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}

void TimerInit(Timer *timer) {
	timer->end_time = 0;
}

