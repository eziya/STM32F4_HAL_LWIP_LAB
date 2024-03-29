/*
 * echoserver.c
 *
 *  Created on: 2020. 5. 31.
 *      Author: eziya76@gmail.com
 */

#include <echoserver.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "wizInterface.h"

#include <stdio.h>

#define LISTEN_PORT	7 	//server port

uint8_t buffer[8];		//client sends 8 bytes

//tcp client task
void StartWizEchoServerTask(void const *argument) {

	int32_t ret;
	uint8_t remoteIP[4];
	uint16_t remotePort;

	if(!WIZ_ChipInit())
	{
		printf("WIZ_ChipInit failed.\n");
		vTaskDelete(NULL);	//clear task
	}

	if(!WIZ_NetworkInit())
	{
		printf("WIZ_NetworkInit failed.\n");
		vTaskDelete(NULL);	//clear task
	}


	while (1) {
		//create socket
		ret = socket(CLIENT_SOCKET, Sn_MR_TCP, LISTEN_PORT, SF_TCP_NODELAY);

		if (ret < 0) {
			HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
			printf("socket failed{%ld}.\n", ret);
			close(CLIENT_SOCKET);
			osDelay(100);
			continue;
		}

		//check initialization
		while(getSn_SR(CLIENT_SOCKET) != SOCK_INIT)
		{
			osDelay(10);
		}

		printf("listening....\n");
		ret = listen(CLIENT_SOCKET);
		if (ret < 0) {
			HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
			printf("listen failed{%ld}.\n", ret);
			close(CLIENT_SOCKET);
			osDelay(100);
			continue;
		}

		//check listening status
		while(getSn_SR(CLIENT_SOCKET) == SOCK_LISTEN)
		{
			osDelay(10);
		}

		if(getSn_SR(CLIENT_SOCKET) == SOCK_ESTABLISHED)
		{
			//client accepted
			printf("accepted....\n");

			//get remote information
			getsockopt(CLIENT_SOCKET, SO_DESTIP, remoteIP);
			getsockopt(CLIENT_SOCKET, SO_DESTPORT, (uint8_t*)&remotePort);
			printf("remote IP[PORT]:%03d.%03d.%03d.%03d[%05d]\n",
					remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);

			//receive data
			ret = recv(CLIENT_SOCKET, buffer, sizeof(buffer));
			if (ret < 0) {
				HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
				printf("recv failed.{%ld}\n", ret);
				close(CLIENT_SOCKET); //unexpected close
				continue;
			}

			printf("received...\n");

			//send back data
			ret = send(CLIENT_SOCKET, buffer, sizeof(buffer));
			if (ret < 0) {
				HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
				printf("send failed{%ld}.\n", ret);
				close(CLIENT_SOCKET); //unexpected close
				continue;
			}

			printf("sent back...\n");
			HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
		}
		else
		{
			printf("getSn_SR() != SOCKET_ESTABLISHED.\n");
		}

		//close socket
		close(CLIENT_SOCKET);
		printf("closed...\n");
	}

	vTaskDelete(NULL);	//clear task
}
