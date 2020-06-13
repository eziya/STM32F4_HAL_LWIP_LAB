/*
 * wizInterface.h
 *
 *  Created on: 2020. 5. 31.
 *      Author: eziya76@gmail.com
 */

#ifndef INC_WIZINTERFACE_H_
#define INC_WIZINTERFACE_H_

#include "main.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "dhcp.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//#define USE_DHCP

#define DHCP_SOCKET		0	//dhcp socket 0
#define CLIENT_SOCKET	2	//tcp client socket 1

void WIZ_SPI_Select(void);
void WIZ_SPI_Deselect(void);
void WIZ_SPI_TxByte(uint8_t byte);
uint8_t WIZ_SPI_RxByte(void);
void WIZ_SPI_TxBuffer(uint8_t *buffer, uint16_t len);
void WIZ_SPI_RxBuffer(uint8_t *buffer, uint16_t len);

bool WIZ_ChipInit(void);
bool WIZ_NetworkInit(void);

#endif /* INC_WIZINTERFACE_H_ */
