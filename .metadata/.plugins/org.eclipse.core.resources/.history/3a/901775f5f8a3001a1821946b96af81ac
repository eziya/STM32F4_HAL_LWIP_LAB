/*
 *  SSL client demonstration program
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/platform.h"
#include "lwip.h"
#include "lwip/api.h"

#include "ssl_client.h"

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

#define SERVER_PORT "443"
#define SERVER_NAME "www.google.com"
#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

#define DEBUG_LEVEL 1

const char mbedtls_google_root_certificate[] = "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\r\n"
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\r\n"
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\r\n"
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\r\n"
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\r\n"
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\r\n"
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\r\n"
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\r\n"
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\r\n"
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\r\n"
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\r\n"
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\r\n"
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\r\n"
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\r\n"
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\r\n"
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\r\n"
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\r\n"
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\r\n"
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\r\n"
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\r\n"
    "-----END CERTIFICATE-----\r\n";

const size_t mbedtls_google_root_certificate_len = sizeof(mbedtls_google_root_certificate);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#define MEMORY_HEAP_SIZE      65536
uint8_t alloc_buf[MEMORY_HEAP_SIZE];
#endif

mbedtls_net_context server_fd;
uint32_t flags;
unsigned char buf[1024];
const char *pers = "ssl_client1";

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;

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

static void my_debug(void *ctx, int level, const char *file, int line, const char *str)
{
  ((void) level);

  mbedtls_fprintf((FILE*) ctx, "%s:%04d: %s", file, line, str);
  fflush((FILE*) ctx);
}

void StartSSLClientTask(void const *argument)
{
  int ret = 1, len;

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
  mbedtls_memory_buffer_alloc_init( alloc_buf, sizeof(alloc_buf) );
#endif

#if defined(MBEDTLS_DEBUG_C)
  mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

  /*
   * 0. Initialize the RNG and the session data
   */
  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  mbedtls_printf("\n  . Seeding the random number generator...");
  fflush( stdout);

  mbedtls_entropy_init(&entropy);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*) pers, strlen(pers))) != 0)
  {
    mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
    goto exit;
  }

  mbedtls_printf(" ok\n");

  /*
   * 0. Initialize certificates
   */
  mbedtls_printf("  . Loading the CA root certificate ...");
  fflush( stdout);

  ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char*) mbedtls_google_root_certificate, mbedtls_google_root_certificate_len);
  if(ret < 0)
  {
    mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", (unsigned int) -ret);
    goto exit;
  }

  mbedtls_printf(" ok (%d skipped)\n", ret);

  /*
   * waiting for valid ip address
   */
  while(1)
  {
    if(gnetif.ip_addr.addr == 0 || gnetif.netmask.addr == 0 || gnetif.gw.addr == 0) //system has no valid ip address
    {
      osDelay(1000);
      continue;
    } else
    {
      printf("DHCP/Static IP O.K.\n");
      break;
    }
  }

  /*
   * 1. Setup stuff
   */
  mbedtls_printf("  . Setting up the SSL/TLS structure...");
  fflush( stdout);

  if((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
  {
    mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
    goto exit;
  }

  mbedtls_printf(" ok\n");

  /* OPTIONAL is not optimal for security,
   * but makes interop easier in this simplified example */
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
  mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

  if((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
  {
    mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
    goto exit;
  }

  if((ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME)) != 0)
  {
    mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
    goto exit;
  }

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  while(1)
  {
    /*
     * 2. Start the connection
     */
    mbedtls_printf("  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT);
    fflush( stdout);

    while(1)
    {
  	if((ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) == 0)
  	{
  	  break;
  	} else
  	{
  	  mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
  	  osDelay(100);
  	}
    }

    mbedtls_printf(" ok\n");

    /*
     * 4. Handshake
     */
    mbedtls_printf("  . Performing the SSL/TLS handshake...");

    while((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
      if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int) -ret);
        goto exit;
      }
    }
    mbedtls_printf(" ok\n");

    /*
     * 5. Verify the server certificate
     */
    mbedtls_printf("  . Verifying peer X.509 certificate...");

    /* In real life, we probably want to bail out when ret != 0 */
    if((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
    {
      char vrfy_buf[512];
      mbedtls_printf(" failed\n");
      mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
      mbedtls_printf("%s\n", vrfy_buf);
    } else
    {
      mbedtls_printf(" ok\n");
    }

    /*
     * 6. Write the GET request
     */
    mbedtls_printf("  > Write to server:");
    fflush( stdout);

    len = sprintf((char*) buf, GET_REQUEST);

    while((ret = mbedtls_ssl_write(&ssl, buf, len)) <= 0)
    {
      if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
        goto exit;
      }
    }

    len = ret;
    //mbedtls_printf(" %d bytes written\n\n%s", len, (char*) buf);

    /*
     * 7. Read the HTTP response
     */
    mbedtls_printf("  < Read from server:");
    fflush( stdout);

    do
    {
      len = sizeof(buf) - 1;
      memset(buf, 0, sizeof(buf));
      ret = mbedtls_ssl_read(&ssl, buf, len);

      if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        continue;

      if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        break;

      if(ret < 0)
      {
        mbedtls_printf("failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
        break;
      }

      if(ret == 0)
      {
        mbedtls_printf("\n\nEOF\n\n");
        break;
      }

      len = ret;
      //mbedtls_printf(" %d bytes read\n\n%s", len, (char*) buf);
    } while(1);

    /*
     * 8. Close session
     */
    do
    {
      mbedtls_ssl_close_notify(&ssl);
    } while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    ret = mbedtls_ssl_session_reset(&ssl);
    if(ret != 0)
    {
      mbedtls_printf("failed\n  ! mbedtls_ssl_session_reset returned %d\n\n", ret);
      break;
    }

    mbedtls_net_free(&server_fd);

    //Toggle LED
    mbedtls_printf("xPortGetFreeHeapSize: %d\n\n", xPortGetFreeHeapSize());
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
    osDelay(5000);
  }

  exit:
  mbedtls_printf("error on mbedtls...\n\n");

  mbedtls_net_free(&server_fd);
  mbedtls_x509_crt_free(&cacert);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
  mbedtls_memory_buffer_alloc_free();
#endif

  vTaskDelete(NULL);
}
