/*
 * mqtt_task.c
 *
 *  Created on: 2020. 5. 19.
 *      Author: eziya76@gmail.com
 */

#include "main.h"
#include "mqtt_task.h"

//mqtt subscribe task
void MqttClientSubTask(void const *argument)
{
	while(1)
	{
		if(!mqttClient.isconnected)
		{
			//try to connect to the broker
			MQTTDisconnect(&mqttClient);
			MqttConnectBroker();
			osDelay(1000);
		}
		else
		{
			MQTTYield(&mqttClient, 1000); //handle timer
			osDelay(100);
		}
	}
}

//mqtt publish task
void MqttClientPubTask(void const *argument)
{
	const char* str = "MQTT message from STM32";
	MQTTMessage message;

	while(1)
	{
		if(mqttClient.isconnected)
		{
			message.payload = (void*)str;
			message.payloadlen = strlen(str);

			MQTTPublish(&mqttClient, "test", &message); //publish a message
		}
		osDelay(1000);
	}
}

int MqttConnectBroker()
{
	int ret;

	net_clear();
	ret = net_init(&net);
	if(ret != MQTT_SUCCESS)
	{
		printf("net_init failed.\n");
		return -1;
	}

	ret = net_connect(&net, BROKER_IP, MQTT_PORT);
	if(ret != MQTT_SUCCESS)
	{
		printf("net_connect failed.\n");
		return -1;
	}

	MQTTClientInit(&mqttClient, &net, 1000, sndBuffer, sizeof(sndBuffer), rcvBuffer, sizeof(rcvBuffer));

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 3;
	data.clientID.cstring = "STM32F4";
	data.username.cstring = "STM32F4";
	data.password.cstring = "";
	data.keepAliveInterval = 60;
	data.cleansession = 1;

	ret = MQTTConnect(&mqttClient, &data);
	if(ret != MQTT_SUCCESS)
	{
		printf("MQTTConnect failed.\n");
		return ret;
	}

	ret = MQTTSubscribe(&mqttClient, "test", QOS0, MqttMessageArrived);
	if(ret != MQTT_SUCCESS)
	{
		printf("MQTTSubscribe failed.\n");
		return ret;
	}

	printf("MqttConnectBroker O.K.\n");
	return MQTT_SUCCESS;
}

void MqttMessageArrived(MessageData* msg)
{
	HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin); //toggle pin when new message arrived

	MQTTMessage* message = msg->message;
	memset(msgBuffer, 0, sizeof(msgBuffer));
	memcpy(msgBuffer, message->payload,message->payloadlen);

	printf("MQTT MSG[%d]:%s\n", (int)message->payloadlen, msgBuffer);
}
