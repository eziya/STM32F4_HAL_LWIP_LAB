/*
 * tcp_echo.h
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#ifndef INC_TCP_ECHO_H_
#define INC_TCP_ECHO_H_

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

/* 서버 상태값 */
enum tcp_echoserver_states
{
	ES_NONE = 0, ES_ACCEPTED, ES_RECEIVED, ES_CLOSING
};

/* 상태 저장용 구조체 */
struct tcp_echoserver_struct
{
	u8_t state; /* 서버 상태값 */
	u8_t retries; /* 안쓰임 */
	struct tcp_pcb *pcb; /* PCB 포인터 */
	struct pbuf *p; /* 송수신 버퍼 포인터 */
};

void tcp_echoserver_init(void);

#endif /* INC_TCP_ECHO_H_ */
