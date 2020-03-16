/*
 * ntp_client.h
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#ifndef INC_NTP_CLIENT_H_
#define INC_NTP_CLIENT_H_

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define NTP_SERVER_HOST_NAME  ""
#define NTP_SERVER_PORT	7

/* client states */
enum ntp_client_states
{
	ES_NONE = 0,
	ES_CONNECTED,
	ES_DATA,
	ES_CLOSING
};

/* ntp client info */
struct ntp_client_struct
{
	uint8_t state; //ES_NOE, ES_ACCEPTED, ES_RECEIVED, ES_CLOSING
	struct tcp_pcb *pcb; //PCB 포인터
	struct pbuf *ptx; //송신 버퍼 포인터
	struct pbuf *prx; //수신 버퍼 포인터
};

err_t app_ntp_gettime(void);

#endif /* INC_NTP_CLIENT_H_ */
