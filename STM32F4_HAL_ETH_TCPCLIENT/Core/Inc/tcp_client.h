/*
 * tcp_client.h
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#ifndef INC_TCP_CLIENT_H_
#define INC_TCP_CLIENT_H_

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#include <stdbool.h>
#include <string.h>

#define SERVER_IP1  192
#define SERVER_IP2  168
#define SERVER_IP3  1
#define SERVER_IP4  227
#define SERVER_PORT	1234

typedef enum {REQ = 0, RESP = 1} packet_type;

struct time_packet
{
  uint8_t head; //0xAE
  uint8_t type; //0:REQ, 1:RESP
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t dummy[247]; //you may add more information
  uint8_t tail; //0xEA
};//256 bytes

void app_start_get_time(void);

#endif /* INC_TCP_CLIENT_H_ */
