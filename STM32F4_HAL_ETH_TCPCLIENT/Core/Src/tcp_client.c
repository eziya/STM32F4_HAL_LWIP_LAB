/*
 * tcp_client.c
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#include <tcp_client.h>

static struct tcp_pcb *pcb_client; //client pcb
static ip_addr_t server_addr; //server ip

struct time_packet packet; //256 bytes time_packet structure
uint16_t nRead = 0; //read buffer index
uint16_t nWritten = 0; //write buffer index

/* callback functions */
static err_t tcp_callback_connected(void *arg, struct tcp_pcb *pcb_new, err_t err);
static err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_callback_received(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb);
static void tcp_callback_error(void *arg, err_t err);

/* functions */
static void app_open_conn(void); //open function
static void app_close_conn(void); //close function
static void app_send_data(void); //send function

/*
 * app_start_get_time
 * initiate process (tcp_connect => tcp_callback_connected => tcp_write => tcp_callback_received => tcp_close)
 */
void app_start_get_time(void)
{
  app_open_conn();
}

/*
 * app_open_connection
 * create a client pcb & call tcp_connect
 */
static void app_open_conn(void)
{
  err_t err;

  if (pcb_client == NULL)
  {
    pcb_client = tcp_new();
    if (pcb_client == NULL) //lack of memory
    {
      memp_free(MEMP_TCP_PCB, pcb_client);
      pcb_client = NULL;
      HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET); //error led
    }
  }

  IP4_ADDR(&server_addr, SERVER_IP1, SERVER_IP2, SERVER_IP3, SERVER_IP4); //server ip
  err = tcp_connect(pcb_client, &server_addr, SERVER_PORT, tcp_callback_connected); //connect

  if(err == ERR_ISCONN) //already connected
  {
    app_close_conn();
  }
}

/*
 * tcp_callback_connected
 * callback when connected, client sends a request to the server
 */
static err_t tcp_callback_connected(void *arg, struct tcp_pcb *pcb_new, err_t err)
{
  LWIP_UNUSED_ARG(arg);

  if (err != ERR_OK) //error when connect to the server
  {
    return err;
  }

  tcp_setprio(pcb_new, TCP_PRIO_NORMAL); //set priority for the client pcb

  tcp_arg(pcb_new, 0); //no argument is used
  tcp_sent(pcb_new, tcp_callback_sent); //register send callback
  tcp_recv(pcb_new, tcp_callback_received);  //register receive callback
  tcp_err(pcb_new, tcp_callback_error); //register error callback
  tcp_poll(pcb_new, tcp_callback_poll, 0); //register poll callback

  app_send_data(); //send a request

  return ERR_OK;
}

/*
 * app_send_data
 * send the request to the server
 */
static void app_send_data(void)
{
  memset(&packet, 0, sizeof(struct time_packet));
  packet.head = 0xAE; //head
  packet.type = REQ; //request type
  packet.tail = 0xEA; //tail

  nWritten = 0; //clear index
  //tcp_write(pcb_client, &packet,sizeof(struct time_packet), TCP_WRITE_FLAG_COPY); //use copied data
  tcp_write(pcb_client, &packet,sizeof(struct time_packet), 0); //use pointer, should not changed until receive ACK
}

/*
 * tcp_callback_sent
 * callback when data sending is finished, control leds
 */
static err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(tpcb);
  LWIP_UNUSED_ARG(len);

  nWritten += len;

  if(nWritten < sizeof(struct time_packet)) //need to flush remain data
  {
    tcp_output(pcb_client); //flush
  }
  else if(nWritten > sizeof(struct time_packet)) //invalid length of sent data
  {
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET); //error led
    app_close_conn();
  }
  else
  {
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); //blink green when sent O.K
  }

  return ERR_OK;
}

/*
 * tcp_callback_received
 * callback when data is received, validate received data and parse it
 */
static err_t tcp_callback_received(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  err_t ret_err;

  if (p == NULL) //pbuf is null when session is closed
  {
    app_close_conn();
    ret_err = ERR_OK;
  }
  else if (err != ERR_OK) //ERR_ABRT is returned when called tcp_abort
  {
    tcp_recved(tpcb, p->tot_len); //advertise window size

    pbuf_free(p); //free pbuf
    ret_err = err;
  }
  else //receiving data
  {
    tcp_recved(tpcb, p->tot_len); //advertise window size

    memcpy(&packet + nRead, p->payload, p->len);
    nRead += p->len;

    if(nRead == sizeof(struct time_packet) && packet.type == RESP) //if received length is valid
    {
      nRead = 0;

      printf("%04d-%02d-%02d %02d:%02d:%02d\n",
             packet.year + 2000,
             packet.month, packet.day, packet.hour, packet.minute, packet.second); //print time information

      app_close_conn(); //close connection
    }
    else if(nRead > sizeof(struct time_packet))
    {
      nRead = 0;
      app_close_conn(); //close connection
    }

    pbuf_free(p); //free pbuf
    ret_err = ERR_OK;
  }

  return ret_err;
}

/*
 * app_close_conn
 * close connection & clear callbacks
 */
static void app_close_conn(void)
{
  /* clear callback functions */
  tcp_arg(pcb_client, NULL);
  tcp_sent(pcb_client, NULL);
  tcp_recv(pcb_client, NULL);
  tcp_err(pcb_client, NULL);
  tcp_poll(pcb_client, NULL, 0);

  tcp_close(pcb_client);    //close connection
  pcb_client = NULL;


}

/*
 *  error callback
 *  call when there's an error, turn on an error led
 */
static void tcp_callback_error(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET); //error loed
}

/*
 * poll callback
 * called when lwip is idle, do something such as watchdog reset
 */
static err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb)
{
  return ERR_OK;
}
