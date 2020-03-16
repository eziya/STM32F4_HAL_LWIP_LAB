/*
 * ntp_client.c
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#include <ntp_client.h>
#include <stdbool.h>

static struct tcp_pcb *pcb_ntp_client;		//ntp client pcb
static ip_addr_t ntp_server_addr;
static bool dns_resolved = false;

/* tcp callback functions */
static err_t tcp_callback_connected(void *arg, struct tcp_pcb *pcb_new, err_t err);
static err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_callback_received(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb);
static void tcp_callback_error(void *arg, err_t err);

/* callback functions */
static void dns_callback_found(const char *name, const ip_addr_t *ipaddr, void *arg);

/* functions */
static err_t app_open_connection(struct tcp_pcb *tpcb); //open function
static void app_close_connection(struct tcp_pcb *tpcb, struct ntp_client_struct *cs); //close function
static void app_send_data(struct tcp_pcb *tpcb, struct ntp_client_struct *cs); //send function
static void app_receive_data(struct tcp_pcb *tpcb, struct ntp_client_struct *cs); //receive function

/* initialize ntp client */
err_t app_ntp_gettime(void)
{
  err_t ret_err;

  ret_err = app_open_connection(pcb_ntp_client);
  if (ret_err != ERR_OK)
  {
    return ret_err;
  }

  return ERR_OK;
}

/* dns callback */
static void dns_callback_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
  LWIP_UNUSED_ARG(name);
  LWIP_UNUSED_ARG(arg);

  if (ipaddr == NULL) //fail to resolve host name
  {
    dns_resolved = false;
  }
  else
  {
    dns_resolved = true;
  }
}

/* connected callback */
static err_t tcp_callback_connected(void *arg, struct tcp_pcb *pcb_new, err_t err)
{
  if (err != ERR_OK) //error when connected
  {
    app_close_connection(pcb_new, NULL); //close session
    return err;
  }

  tcp_setprio(pcb_new, TCP_PRIO_NORMAL); //set priority for new pcb

  struct ntp_client_struct *cs;
  cs = (struct ntp_client_struct*) mem_malloc(sizeof(struct ntp_client_struct)); //allocate ntp_client_struct

  if (cs == NULL) //lack of memory
  {
    app_close_connection(pcb_new, NULL); //close connection
    return ERR_MEM;
  }

  cs->state = ES_CONNECTED; //connected
  cs->pcb = pcb_new; //set pcb pointer
  cs->ptx = NULL; //clear buffer pointer
  cs->prx = NULL; //clear buffer pointer

  tcp_arg(pcb_new, cs); //send cs structure as an argument
  tcp_sent(pcb_new, tcp_callback_sent); //register send callback
  tcp_recv(pcb_new, tcp_callback_received);	//register receive callback
  tcp_err(pcb_new, tcp_callback_error); //register error callback
  tcp_poll(pcb_new, tcp_callback_poll, 0); //register poll callback

  return ERR_OK;
}

/* send callback */
static err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct ntp_client_struct *cs;
  LWIP_UNUSED_ARG(len);

  cs = (struct ntp_client_struct*) arg;

  if (cs->ptx != NULL) //if there's data to send
  {
    app_send_data(tpcb, cs); //send data
  }

  return ERR_OK;
}

/* receive callback */
static err_t tcp_callback_received(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct ntp_client_struct *cs;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL", arg != NULL); //check argument
  cs = (struct ntp_client_struct*) arg;

  if (p == NULL) //callback is called but there's no data
  {
    cs->state = ES_CLOSING;
    if (cs->ptx == NULL)
    {
      app_close_connection(tpcb, cs); //close connection
    }
    else //if there's remained data to send
    {
      app_send_data(tpcb, cs); //send remained data
    }
    ret_err = ERR_OK;
  }
  else if (err != ERR_OK) //ERR_ABRT
  {
    if (p != NULL) //buffer is not empty
    {
      cs->prx = NULL;
      cs->ptx = NULL;
      pbuf_free(p); //clear buffer
    }
    ret_err = err;
  }
  else if (cs->state == ES_CLOSING) //receiving data when connection is closing
  {
    tcp_recved(tpcb, p->tot_len); //advertise window size
    cs->ptx = NULL;
    cs->prx = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  else //receiving data
  {
    tcp_recved(tpcb, p->tot_len);

    if (cs->prx == NULL) //no data
    {
      cs->prx = p; //set buffer pointer
    }
    else //buffer is not empty
    {
      struct pbuf *ptr = cs->prx; //head buffer
      pbuf_chain(ptr, p); //append tail buffer to the head, this buffer will be handled by poll callback
    }

    app_receive_data(tpcb, cs); //handle received data
    ret_err = ERR_OK;
  }

  return ret_err;
}

/* error callback */
static void tcp_callback_error(void *arg, err_t err)
{
  struct ntp_client_struct *cs;
  LWIP_UNUSED_ARG(err);

  cs = (struct ntp_client_struct*) arg;
  if (cs != NULL)
  {
    mem_free(cs);	//free cs structure
  }

  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET); //turn on blue LED when there's error.
}

/* poll callback */
static err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb)
{
  struct ntp_client_struct *cs;
  cs = (struct ntp_client_struct*) arg;

  if (cs == NULL) //if there's no cs structure
  {
    tcp_abort(tpcb); //abort connection
    return ERR_ABRT;
  }

  if (cs->prx != NULL) //if there's data to send
  {
    app_receive_data(tpcb, cs); //handle received data
  }

  if (cs->ptx != NULL)
  {
    app_send_data(tpcb, cs);
  }

  return ERR_OK;
}

static err_t app_open_connection(struct tcp_pcb *tpcb)
{
  err_t err;

  if (pcb_ntp_client == NULL)
  {
    pcb_ntp_client = tcp_new(); //allocate pcb memory

    if (pcb_ntp_client == NULL)
    {
      //lack of memory
      memp_free(MEMP_TCP_PCB, pcb_ntp_client);
      return ERR_MEM;
    }
  }

  err =
      dns_gethostbyname(NTP_SERVER_HOST_NAME, &ntp_server_addr, dns_callback_found, NULL);
  if (err != ERR_OK)
  {
    return err;
  }

  err =
      tcp_connect(pcb_ntp_client, &ntp_server_addr, NTP_SERVER_PORT, tcp_callback_connected);
  return err;
}

/* close connection */
static void app_close_connection(struct tcp_pcb *tpcb, struct ntp_client_struct *cs)
{
  /* clear callback functions */
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  if (cs != NULL)
  {
    mem_free(cs);   //free cs structure
  }

  tcp_close(tpcb);    //close connection
}

/* send data */
static void app_send_data(struct tcp_pcb *tpcb, struct ntp_client_struct *cs)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  // while no error, data to send, data size is smaller than the size of the send buffer
  while ((wr_err == ERR_OK) && (cs->ptx != NULL)
      && (cs->ptx->len <= tcp_sndbuf(tpcb)))
  {
    ptr = cs->ptx;
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, TCP_WRITE_FLAG_COPY); //send data

    if (wr_err == ERR_OK)
    {
      u8_t freed;

      cs->ptx = ptr->next;

      if (cs->ptx != NULL) //there's chained buffer to send
      {
        pbuf_ref(cs->ptx);	//increase reference counter
      }

      do
      {
        freed = pbuf_free(ptr);		//free old buffer
      }
      while (freed == 0);
    }
    else
    {
      cs->ptx = ptr;  //fail to send, recover buffer pointer
    }
  }
}

static void app_receive_data(struct tcp_pcb *tpcb, struct ntp_client_struct *cs)
{
  struct pbuf *ptr;

  while (cs->prx != NULL)
  {
    u8_t freed;
    ptr = cs->prx;

    //payload 에 있는 버퍼에 담는다.

    cs->prx = ptr->next;
    if (cs->prx != NULL)
    {
      pbuf_ref(cs->prx);
    }

    do
    {
      freed = pbuf_free(ptr);
    }
    while (freed == 0);
  }

  //버퍼를 확인해서 48바이트 ntp 패킷이 완성되어 있으면 처리한다.
}
