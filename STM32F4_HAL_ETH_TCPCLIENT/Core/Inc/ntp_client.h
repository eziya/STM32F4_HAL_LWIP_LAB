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

struct ntp_packet
{
  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.
  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.
  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.
  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.
  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.
  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.
  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.
};                          // Total: 384 bits or 48 bytes.

err_t app_ntp_gettime(void);

#endif /* INC_NTP_CLIENT_H_ */
