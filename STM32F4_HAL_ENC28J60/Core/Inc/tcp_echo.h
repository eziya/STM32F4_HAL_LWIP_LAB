/*
 * tcp_echo.h
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#ifndef INC_TCP_ECHO_H_
#define INC_TCP_ECHO_H_

#include "gpio.h"

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#define  ECHO_SERVER_LISTEN_PORT	7

/* server states */
enum tcp_echoserver_states
{
	ES_NONE = 0,
	ES_ACCEPTED,
	ES_RECEIVED,
	ES_CLOSING
};

/* server info */
struct tcp_echoserver_struct
{
	uint8_t state; //ES_NOE, ES_ACCEPTED, ES_RECEIVED, ES_CLOSING
	uint8_t retries; //retry counter
	struct tcp_pcb *pcb; //PCB 포인터
	struct pbuf *p; //송수신 버퍼 포인터
};

err_t app_echoserver_init(void);

#endif /* INC_TCP_ECHO_H_ */
