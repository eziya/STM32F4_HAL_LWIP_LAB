/**
 ******************************************************************************
 * @file    enc28j60.c
 * @author  Christian Schoffit, portions from Gregory Nutt:
 *          Copyright (C) 2010-2012, 2014 Gregory Nutt. All rights reserved.
 *          Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * @version V1.0.0
 * @date    02-June-2015
 * @brief   This file provides a set of functions needed to manage the ENC28J60
 *          Stand-Alone Ethernet Controller with SPI Interface.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 Christian Schoffit</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Christian Schoffit nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include "enc28j60.h"
#include "main.h"
#include "spi.h"
#include "gpio.h"

/* Poll timeout */
#define ENC_POLLTIMEOUT 50

/* Packet Memory ************************************************************/
/* Packet memory layout */
#define ALIGNED_BUFSIZE ((CONFIG_NET_ETH_MTU + 255) & ~255)

/* Work around Errata #5 (spurious reset of ERXWRPT to 0) by placing the RX */
#define PKTMEM_RX_START 0x0000                            /* RX buffer must be at addr 0 for errata 5 */
#define PKTMEM_RX_END   (PKTMEM_END-ALIGNED_BUFSIZE)      /* RX buffer length is total SRAM minus TX buffer */
#define PKTMEM_TX_START (PKTMEM_RX_END+1)                 /* Start TX buffer after */
#define PKTMEM_TX_ENDP1 (PKTMEM_TX_START+ALIGNED_BUFSIZE) /* Allow TX buffer for two frames */

/* Misc. Helper Macros ******************************************************/
#define enc_rdgreg(ctrlreg) enc_rdgreg2(ENC_RCR | GETADDR(ctrlreg))
#define enc_wrgreg(ctrlreg, wrdata) enc_wrgreg2(ENC_WCR | GETADDR(ctrlreg), wrdata)
#define enc_bfcgreg(ctrlreg,clrbits) enc_wrgreg2(ENC_BFC | GETADDR(ctrlreg), clrbits)
#define enc_bfsgreg(ctrlreg,setbits) enc_wrgreg2(ENC_BFS | GETADDR(ctrlreg), setbits)

/* platform-dependent functions */
#define SPIx_TIMEOUT  10
#define SPIx_CS   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);udelay(1)
#define SPIx_DS   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);udelay(1)

static uint8_t SPIx_TxRx(uint8_t cmd)
{
  HAL_SPI_TransmitReceive(&hspi2, &cmd, &cmd, 1, SPIx_TIMEOUT);
  return cmd;
}

static void SPIx_TxBuf(uint8_t *m2s, uint8_t *s2m, uint16_t bufflen)
{
  SPIx_CS;

  if((s2m == NULL) && (m2s != NULL))
  {
    HAL_SPI_Transmit(&hspi2, m2s, bufflen, SPIx_TIMEOUT);
  }
  else if(m2s == NULL)
  {
    HAL_SPI_Receive(&hspi2, s2m, bufflen, SPIx_TIMEOUT);
  }
  else
  {
    HAL_SPI_TransmitReceive(&hspi2, m2s, s2m, bufflen, SPIx_TIMEOUT);
  }

  SPIx_DS;
}

/* Initialize STM32 watchdog timer */
static void init_udelay(void)
{
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
  CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;
  DWT->CYCCNT = 0;

  /* 3 NO OPERATION instructions */
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");

}

/* Software delay in us */
void udelay(uint32_t us)
{
  uint32_t cycles = (SystemCoreClock/1000000L)*us;
  uint32_t start = DWT->CYCCNT;
  volatile uint32_t cnt;

  do
  {
    cnt = DWT->CYCCNT - start;
  } while(cnt < cycles);

}
/* platform-dependent functions */

/* static function prototypes */
static void enc_setbank(ENC_HandleTypeDef *handle, uint8_t bank);
static uint8_t enc_rdgreg2(uint8_t cmd);
static void enc_wrgreg2(uint8_t cmd, uint8_t wrdata);
static bool enc_waitgreg(uint8_t ctrlreg, uint8_t bits, uint8_t value);
static bool enc_waitwhilegreg(uint8_t ctrlreg, uint8_t bits, uint8_t value);
static uint8_t enc_rdbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg);
static void enc_wrbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg, uint8_t wrdata);
static bool enc_waitbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg, uint8_t bits, uint8_t value);
static uint16_t enc_rdphy(ENC_HandleTypeDef *handle, uint8_t phyaddr);
static void enc_wrphy(ENC_HandleTypeDef *handle, uint8_t phyaddr, uint16_t phydata);
static void enc_rdbuffer(void *buffer, int16_t buflen);
static void enc_linkstatus(ENC_HandleTypeDef *handle);

/* Send the single byte system reset command (SRC). */
void enc_reset(ENC_HandleTypeDef *handle)
{
  /* Send the system reset command. */
  SPIx_CS;
  SPIx_TxRx(ENC_SRC);
  SPIx_DS;

  handle->bank = 0; /* Initialize the trace on the current selected bank */
  HAL_Delay(2); /* >1000 us, conforms to errata #2 */
}

/* Initialize the enc28j60 and configure the needed hardware resources */
bool enc_start(ENC_HandleTypeDef *handle)
{
  uint8_t regval;

  /* Initialize watchdog timer */
  init_udelay();

  /* System reset */
  enc_reset(handle);

  /* Use bank 0 */
  enc_setbank(handle, 0);

  /* Check if we are actually communicating with the ENC28J60. */
  regval = enc_rdbreg(handle, ENC_EREVID);
  if(regval == 0x00 || regval == 0xff)
  {
    return false;
  }

  /* Initialize ECON2: Enable address auto increment. */
  enc_wrgreg(ENC_ECON2, ECON2_AUTOINC /* | ECON2_VRPS*/);

  /* Initialize receive buffer. */
  handle->nextpkt = PKTMEM_RX_START;
  enc_wrbreg(handle, ENC_ERXSTL, PKTMEM_RX_START & 0xff);
  enc_wrbreg(handle, ENC_ERXSTH, PKTMEM_RX_START >> 8);

  /* Set the receive data pointer */
  /* Errata 14 */
  enc_wrbreg(handle, ENC_ERXRDPTL, PKTMEM_RX_END & 0xff);
  enc_wrbreg(handle, ENC_ERXRDPTH, PKTMEM_RX_END >> 8);

  /* Set the receive buffer end. */
  enc_wrbreg(handle, ENC_ERXNDL, PKTMEM_RX_END & 0xff);
  enc_wrbreg(handle, ENC_ERXNDH, PKTMEM_RX_END >> 8);

  /* Set transmit buffer start. */
  handle->transmitLength = 0;
  enc_wrbreg(handle, ENC_ETXSTL, PKTMEM_TX_START & 0xff);
  enc_wrbreg(handle, ENC_ETXSTH, PKTMEM_TX_START >> 8);

  /* Set filter mode: unicast OR broadcast AND crc valid */
  enc_wrbreg(handle, ENC_ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

  do
  {
    HAL_Delay(10); /* Wait for 10 ms to let the clock be ready */
    regval = enc_rdbreg(handle, ENC_ESTAT);
  }while((regval & ESTAT_CLKRDY) == 0);

  /* Enable MAC receive */
  enc_wrbreg(handle, ENC_MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);

  /* Enable automatic padding and CRC operations */
  if(handle->Init.DuplexMode == ETH_MODE_HALFDUPLEX)
  {
    enc_wrbreg(handle, ENC_MACON3,
        ((handle->Init.ChecksumMode == ETH_CHECKSUM_BY_HARDWARE) ? MACON3_PADCFG0 | MACON3_TXCRCEN : 0) | MACON3_FRMLNEN);
    enc_wrbreg(handle, ENC_MACON4, MACON4_DEFER); /* Defer transmission enable */

    /* Set Non-Back-to-Back Inter-Packet Gap */
    enc_wrbreg(handle, ENC_MAIPGL, 0x12);
    enc_wrbreg(handle, ENC_MAIPGH, 0x0c);

    /* Set Back-to-Back Inter-Packet Gap */
    enc_wrbreg(handle, ENC_MABBIPG, 0x12);
  }
  else
  {
    /* Set filter mode: unicast OR broadcast AND crc valid AND Full Duplex */
    enc_wrbreg(handle, ENC_MACON3,
        ((handle->Init.ChecksumMode == ETH_CHECKSUM_BY_HARDWARE) ? MACON3_PADCFG0 | MACON3_TXCRCEN : 0) | MACON3_FRMLNEN | MACON3_FULDPX);

    /* Set Non-Back-to-Back Inter-Packet Gap */
    enc_wrbreg(handle, ENC_MAIPGL, 0x12);

    /* Set Back-to-Back Inter-Packet Gap */
    enc_wrbreg(handle, ENC_MABBIPG, 0x15);
  }

  /* Set the maximum packet size which the controller will accept */
  enc_wrbreg(handle, ENC_MAMXFLL, (CONFIG_NET_ETH_MTU + 18) & 0xff);
  enc_wrbreg(handle, ENC_MAMXFLH, (CONFIG_NET_ETH_MTU + 18) >> 8);

  /* Setup up PHCON1 & 2 */
  if(handle->Init.DuplexMode == ETH_MODE_HALFDUPLEX)
  {
    enc_wrphy(handle, ENC_PHCON1, 0x00);
    enc_wrphy(handle, ENC_PHCON2, PHCON2_HDLDIS); /* errata 9 workaround */
  }
  else
  {
    enc_wrphy(handle, ENC_PHCON1, PHCON1_PDPXMD); /* errata 10 workaround */
    enc_wrphy(handle, ENC_PHCON2, 0x00);
  }

  /* Process interrupt settings */
  if(handle->Init.InterruptEnableBits & EIE_LINKIE)
  {
    /* Enable link change interrupt in PHY module */
    enc_wrphy(handle, ENC_PHIE, PHIE_PGEIE | PHIE_PLNKIE);
  }

  /* Reset all interrupt flags */
  enc_bfcgreg(ENC_EIR, EIR_ALLINTS);

  regval = handle->Init.InterruptEnableBits;
  if(regval)
  {
    /* Ensure INTIE is set when at least an interruption is selected */
    regval |= EIE_INTIE;
  }
  /* Enable selected interrupts in ethernet controller module */
  enc_bfsgreg(ENC_EIE, regval);

  /* Enable the receiver */
  enc_bfsgreg(ENC_ECON1, ECON1_RXEN);

  return true;
}

/* Set the MAC address to the configured value. */
void enc_set_MAC(ENC_HandleTypeDef *handle)
{
  enc_wrbreg(handle, ENC_MAADR1, handle->Init.MACAddr[0]);
  enc_wrbreg(handle, ENC_MAADR2, handle->Init.MACAddr[1]);
  enc_wrbreg(handle, ENC_MAADR3, handle->Init.MACAddr[2]);
  enc_wrbreg(handle, ENC_MAADR4, handle->Init.MACAddr[3]);
  enc_wrbreg(handle, ENC_MAADR5, handle->Init.MACAddr[4]);
  enc_wrbreg(handle, ENC_MAADR6, handle->Init.MACAddr[5]);
}

/* Prepare TX buffer */
int8_t enc_prepare_txbuffer(ENC_HandleTypeDef *handle, uint16_t len)
{
  uint16_t txend;
  uint8_t control_write[2];

  /* Wait while TX is busy */
  if(!enc_waitgreg(ENC_ECON1, ECON1_TXRTS, 0))
  {
    return ERR_TIMEOUT;
  }

  /* Set transmit buffer start (is this necessary?). */
  enc_wrbreg(handle, ENC_ETXSTL, PKTMEM_TX_START & 0xff);
  enc_wrbreg(handle, ENC_ETXSTH, PKTMEM_TX_START >> 8);

  /* Reset the write pointer to start of transmit buffer */
  enc_wrbreg(handle, ENC_EWRPTL, PKTMEM_TX_START & 0xff);
  enc_wrbreg(handle, ENC_EWRPTH, PKTMEM_TX_START >> 8);

  txend = PKTMEM_TX_START + len;

  if(txend + 8 > PKTMEM_TX_ENDP1)
  {
    return ERR_MEM;
  }

  enc_wrbreg(handle, ENC_ETXNDL, txend & 0xff);
  enc_wrbreg(handle, ENC_ETXNDH, txend >> 8);

  control_write[0] = ENC_WBM;
  control_write[1] = PKTCTRL_PCRCEN | PKTCTRL_PPADEN | PKTCTRL_PHUGEEN;
  SPIx_TxBuf(control_write, control_write, 2);

  return ERR_OK;
}

/* Write a buffer of data. */
void enc_wrbuffer(void *buffer, uint16_t buflen)
{
  SPIx_CS;
  SPIx_TxRx(ENC_WBM);
  SPIx_TxBuf(buffer, NULL, buflen);
}

/* Start hardware transmission. */
void enc_transmit(ENC_HandleTypeDef *handle)
{
  if(handle->transmitLength != 0)
  {
    handle->retries = 16;
    do
    {
      enc_bfsgreg(ENC_ECON1, ECON1_TXRST);
      enc_bfcgreg(ENC_ECON1, ECON1_TXRST);
      enc_bfcgreg(ENC_EIR, EIR_TXERIF | EIR_TXIF);

      /* Start transmission */
      enc_bfsgreg(ENC_ECON1, ECON1_TXRTS);

      /* Wait for end of transmission */
      enc_waitwhilegreg(ENC_EIR, EIR_TXIF | EIR_TXERIF, 0);

      /* Stop transmission */
      enc_bfcgreg(ENC_ECON1, ECON1_TXRTS);

      {
        uint16_t addtTsv4;
        uint8_t tsv4, regval;

        /* read tsv */
        addtTsv4 = PKTMEM_TX_START + handle->transmitLength + 4;

        enc_wrbreg(handle, ENC_ERDPTL, addtTsv4 & 0xff);
        enc_wrbreg(handle, ENC_ERDPTH, addtTsv4 >> 8);

        enc_rdbuffer(&tsv4, 1);
        regval = enc_rdgreg(ENC_EIR);
        if(!(regval & EIR_TXERIF) || !(tsv4 & TSV_LATECOL))
        {
          break;
        }
      }
      handle->retries--;
    }while(handle->retries > 0);
    handle->transmitLength = 0;
  }
}

/* Check if we have received packet, and if so, retrieve them. */
bool enc_get_received_frame(ENC_HandleTypeDef *handle)
{
  uint8_t rsv[6];
  uint16_t pktlen;
  uint16_t rxstat;
  uint8_t pktcnt;

  bool result = true;

  pktcnt = enc_rdbreg(handle, ENC_EPKTCNT);
  if(pktcnt == 0)
  {
    return false;
  };

  /* Set the read pointer to the start of the received packet (ERDPT) */
  enc_wrbreg(handle, ENC_ERDPTL, (handle->nextpkt) & 0xff);
  enc_wrbreg(handle, ENC_ERDPTH, (handle->nextpkt) >> 8);
  enc_rdbuffer(rsv, 6);

  handle->nextpkt = (uint16_t) rsv[1] << 8 | (uint16_t) rsv[0];
  pktlen = (uint16_t) rsv[3] << 8 | (uint16_t) rsv[2];
  rxstat = (uint16_t) rsv[5] << 8 | (uint16_t) rsv[4];

  /* Check if the packet was received OK */
  if((rxstat & RXSTAT_OK) == 0)
  {
    result = false;
  }
  else
  { /* Check for a usable packet length (4 added for the CRC) */
    if(pktlen > (CONFIG_NET_ETH_MTU + 4) || pktlen <= (ETH_HDRLEN + 4))
    {
      result = false;
    }
    else
    {
      handle->RxFrameInfos.length = pktlen - 4;
      enc_rdbuffer(handle->RxFrameInfos.buffer, handle->RxFrameInfos.length);
    }
  }

  /* Errata 14 (on se sert de rxstat comme variable temporaire */
  rxstat = handle->nextpkt;
  if(rxstat == PKTMEM_RX_START)
  {
    rxstat = PKTMEM_RX_END;
  }
  else
  {
    rxstat--;
  }
  enc_wrbreg(handle, ENC_ERXRDPTL, rxstat && 0xff);
  enc_wrbreg(handle, ENC_ERXRDPTH, rxstat >> 8);

  /* Decrement the packet counter indicate we are done with this packet */
  enc_bfsgreg(ENC_ECON2, ECON2_PKTDEC);

  return result;
}

/* Enable individual ENC28J60 interrupts */
void enc_enable_interrupts(uint8_t bits)
{
  enc_bfsgreg(ENC_EIE, bits);
}

/* Perform interrupt handling logic outside of the interrupt handler */
void enc_irq_handler(ENC_HandleTypeDef *handle)
{
  uint8_t eir;

  enc_bfcgreg(ENC_EIE, EIE_INTIE);

  /* Read EIR for interrupt flags */
  eir = enc_rdgreg(ENC_EIR) & EIR_ALLINTS;

  /* PKTIF is not reliable, check PKCNT instead */
  if(enc_rdbreg(handle, ENC_EPKTCNT) != 0)
  {
    /* Manage EIR_PKTIF by software */
    eir |= EIR_PKTIF;
  }

  /* Store interrupt flags in handle */
  handle->interruptFlags = eir;

  /* If link status has changed, read it */
  if((eir & EIR_LINKIF) != 0) /* Link change interrupt */
  {
    enc_linkstatus(handle); /* Get current link status */
    enc_rdphy(handle, ENC_PHIR); /* Clear the LINKIF interrupt */
  }

  /* Reset ENC28J60 interrupt flags, except PKTIF form which interruption is deasserted when PKTCNT reaches 0 */
  enc_bfcgreg(ENC_EIR, EIR_ALLINTS);

  /* Enable Ethernet interrupts */
  /* done after effective process on interrupts enc_bfsgreg(ENC_EIE, EIE_INTIE); */
}

/* Set the bank for these next control register access. */
static void enc_setbank(ENC_HandleTypeDef *handle, uint8_t bank)
{
  if(bank != handle->bank)
  {
    /* Select bank 0 (just so that all of the bits are cleared) */
    enc_bfcgreg(ENC_ECON1, ECON1_BSEL_MASK);

    if(bank != 0)
    {
      enc_bfsgreg(ENC_ECON1, (bank << ECON1_BSEL_SHIFT));
    }

    handle->bank = bank;
  }
}

/* Read a global register (EIE, EIR, ESTAT, ECON2, or ECON1). */
static uint8_t enc_rdgreg2(uint8_t cmd)
{
  uint8_t cmdpdata[2];
  cmdpdata[0] = cmd;
  SPIx_TxBuf(cmdpdata, cmdpdata, 2);

  return cmdpdata[1];
}

/* Write to a global register (EIE, EIR, ESTAT, ECON2, or ECON1). */
static void enc_wrgreg2(uint8_t cmd, uint8_t wrdata)
{
  uint8_t cmdpdata[2];
  cmdpdata[0] = cmd;
  cmdpdata[1] = wrdata;
  SPIx_TxBuf(cmdpdata, NULL, 2);
}

/* Wait until grouped register bit(s) take a specific value */
static bool enc_waitgreg(uint8_t ctrlreg, uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t rddata;

  do
  {
    rddata = enc_rdgreg(ctrlreg);
    elapsed = HAL_GetTick() - start;
  }while((rddata & bits) != value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) == value;
}

/* Wait while grouped register bit(s) have a specific value */
static bool enc_waitwhilegreg(uint8_t ctrlreg, uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t rddata;

  do
  {
    rddata = enc_rdgreg(ctrlreg);
    elapsed = HAL_GetTick() - start;
  }while((rddata & bits) == value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) != value;
}

/* Read from a banked control register using the RCR command. */
static uint8_t enc_rdbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg)
{
  uint8_t data[3];

  enc_setbank(handle, GETBANK(ctrlreg));
  data[0] = ENC_RCR | GETADDR(ctrlreg);
  SPIx_TxBuf(data, data, (ISPHYMAC(ctrlreg)) ? 3 : 2);
  return (ISPHYMAC(ctrlreg)) ? data[2] : data[1];
}

/* Write to a banked control register using the WCR command. */
static void enc_wrbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg, uint8_t wrdata)
{
  uint8_t data[2];
  enc_setbank(handle, GETBANK(ctrlreg));
  data[0] = ENC_WCR | GETADDR(ctrlreg);
  data[1] = wrdata;
  SPIx_TxBuf(data, NULL, 2);
}

/* Wait until banked register bit(s) take a specific value */
static bool enc_waitbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg, uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t rddata;

  do
  {
    rddata = enc_rdbreg(handle, ctrlreg);
    elapsed = HAL_GetTick() - start;
  }while((rddata & bits) != value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) == value;
}

/* Read 16-bits of PHY data. */
static uint16_t enc_rdphy(ENC_HandleTypeDef *handle, uint8_t phyaddr)
{
  uint16_t data = 0;

  enc_wrbreg(handle, ENC_MIREGADR, phyaddr);
  enc_wrbreg(handle, ENC_MICMD, MICMD_MIIRD);
  udelay(12);

  if(enc_waitbreg(handle, ENC_MISTAT, MISTAT_BUSY, 0x00))
  {
    enc_wrbreg(handle, ENC_MICMD, 0x00);
    data = (uint16_t) enc_rdbreg(handle, ENC_MIRDL);
    data |= (uint16_t) enc_rdbreg(handle, ENC_MIRDH) << 8;
  }

  return data;
}

/* write 16-bits of PHY data. */
static void enc_wrphy(ENC_HandleTypeDef *handle, uint8_t phyaddr, uint16_t phydata)
{
  enc_wrbreg(handle, ENC_MIREGADR, phyaddr);
  enc_wrbreg(handle, ENC_MIWRL, phydata);
  enc_wrbreg(handle, ENC_MIWRH, phydata >> 8);
  udelay(12);
  enc_waitbreg(handle, ENC_MISTAT, MISTAT_BUSY, 0x00);
}

/* Read a buffer of data. */
static void enc_rdbuffer(void *buffer, int16_t buflen)
{
  SPIx_CS;
  SPIx_TxRx(ENC_RBM);
  SPIx_TxBuf(NULL, buffer, buflen);
}

/* The current link status can be obtained from the PHSTAT1.LLSTAT or PHSTAT2.LSTAT.*/
static void enc_linkstatus(ENC_HandleTypeDef *handle)
{
  handle->LinkStatus = enc_rdphy(handle, ENC_PHSTAT2);
}

