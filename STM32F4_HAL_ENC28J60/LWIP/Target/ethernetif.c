/**
  ******************************************************************************
  * @file    LwIP/LwIP_TCP_Echo_Server/Src/ethernetif.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    13-March-2015
  * @brief   This file implements Ethernet network interface drivers for lwIP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "lwip/opt.h"
#include "lwip/lwip_timers.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "enc28j60.h"
#include <string.h>

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */
#include "enc28j60.h"
/* USER CODE END 0 */

/* Private define ------------------------------------------------------------*/

/* Network interface name */
#define IFNAME0 'e'
#define IFNAME1 'n'

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN 2 */
ENC_HandleTypeDef EncHandle;
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */

/* USER CODE END 3 */

/* Private functions ---------------------------------------------------------*/

static void ENC_MSPInit(ENC_HandleTypeDef *henc)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOs clocks */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* ENC28J60_CS ----------------------> PB12 */
  /*Configure GPIO pins : PB12 */
  GPIO_InitStructure.Pin = GPIO_PIN_12;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Deselect ENC28J60 module */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void HAL_ENC_MspDeInit(ENC_HandleTypeDef *henc)
{
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/*******************************************************************************
 LL Driver Interface ( LwIP stack --> ETH)
 *******************************************************************************/
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x80;
  netif->hwaddr[2] = 0xE1;
  netif->hwaddr[3] = 0x00;
  netif->hwaddr[4] = 0x00;
  netif->hwaddr[5] = 0x00;

  EncHandle.Init.MACAddr = netif->hwaddr;
  EncHandle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
  EncHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  EncHandle.Init.InterruptEnableBits = EIE_LINKIE;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  ENC_MSPInit(&EncHandle);

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* Start the EN28J60 module */
  if(ENC_Start(&EncHandle))
  {
    /* Set the MAC address */
    ENC_SetMacAddr(&EncHandle);

    /* Set netif link flag */
    netif->flags |= NETIF_FLAG_LINK_UP;
  }
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  /* TODO use netif to check if we are the right ethernet interface */
  err_t errval;
  struct pbuf *q;
  uint32_t framelength = 0;

  /* Prepare ENC28J60 Tx buffer */
  errval = ENC_RestoreTXBuffer(&EncHandle, p->tot_len);
  if(errval != ERR_OK)
  {
    return errval;
  }

  /* copy frame from pbufs to driver buffers and send packet */
  for(q = p;q != NULL;q = q->next)
  {
    ENC_WriteBuffer(q->payload, q->len);
    framelength += q->len;
  }

  if(framelength != p->tot_len)
  {
    return ERR_BUF;
  }

  EncHandle.transmitLength = p->tot_len;
  /* Actual transmission is triggered in main loop */

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf* low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  struct pbuf *q;
  uint16_t len;
  uint8_t *buffer;
  uint32_t bufferoffset = 0;

  if(!ENC_GetReceivedFrame(&EncHandle))
  {
    return NULL;
  }

  /* Obtain the size of the packet and put it into the "len" variable. */
  len = EncHandle.RxFrameInfos.length;
  buffer = (uint8_t*) EncHandle.RxFrameInfos.buffer;

  if(len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if(p != NULL)
  {
    bufferoffset = 0;

    for(q = p;q != NULL;q = q->next)
    {
      /* Copy data in pbuf */
      memcpy((uint8_t*) ((uint8_t*) q->payload), (uint8_t*) ((uint8_t*) buffer + bufferoffset), q->len);
      bufferoffset = bufferoffset + q->len;
    }
  }

  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif)
{
  err_t err;
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);

  /* no packet could be read, silently ignore this */
  if(p == NULL)
    return;

  /* entry point to the LwIP stack */
  err = netif->input(p, netif);

  if(err != ERR_OK)
  {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
    pbuf_free(p);
    p = NULL;
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "stm32idisco";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/* USER CODE BEGIN 6 */

/**
 * @brief  Returns the current time in milliseconds
 *         when LWIP_TIMERS == 1 and NO_SYS == 1
 * @param  None
 * @retval Time
 */
u32_t sys_jiffies(void)
{
  return HAL_GetTick();
}

/**
 * @brief  Returns the current time in milliseconds
 *         when LWIP_TIMERS == 1 and NO_SYS == 1
 * @param  None
 * @retval Time
 */
u32_t sys_now(void)
{
  return HAL_GetTick();
}

/* USER CODE END 6 */

/**
 * @brief  This function sets the netif link status.
 * @note   This function should be included in the main loop to poll
 *         for the link status update
 * @param  netif: the network interface
 * @retval None
 */
uint32_t EthernetLinkTimer = 0;

void ethernetif_set_link(struct netif *netif)
{
  /* Handle ENC28J60 interrupt */
  ENC_IRQHandler(&EncHandle);

  /* Check whether the link is up or down*/
  if(((EncHandle.LinkStatus) & PHSTAT2_LSTAT) != 0)
  {
    netif_set_link_up(netif);
  }
  else
  {
    netif_set_link_down(netif);
  }

  /* Reenable interrupts */
  ENC_EnableInterrupts(EIE_INTIE);
}

/* USER CODE BEGIN 7 */

/* USER CODE END 7 */

#if LWIP_NETIF_LINK_CALLBACK
/**
 * @brief  Link callback function, this function is called on change of link status
 *         to update low level driver configuration.
 * @param  netif: The network interface
 * @retval None
 */
void ethernetif_update_config(struct netif *netif)
{
  if(netif_is_link_up(netif))
  {
    /* Restart the EN28J60 module */
    low_level_init(netif);
  }

  ethernetif_notify_conn_changed(netif);
}

/* USER CODE BEGIN 8 */
/**
 * @brief  This function notify user about link status changement.
 * @param  netif: the network interface
 * @retval None
 */
__weak void ethernetif_notify_conn_changed(struct netif *netif)
{
  /* NOTE : This is function could be implemented in user file
   when the callback is needed,
   */

}
/* USER CODE END 8 */
#endif /* LWIP_NETIF_LINK_CALLBACK */

/* USER CODE BEGIN 9 */
void ethernet_transmit(void) {
  ENC_Transmit(&EncHandle);
}
/* USER CODE END 9 */

