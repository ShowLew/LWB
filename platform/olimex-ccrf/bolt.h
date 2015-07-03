/*
 * Copyright (c) 2015, Swiss Federal Institute of Technology (ETH Zurich).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author:  Reto Da Forno
 */

/**
 * @addtogroup  Platform
 * @{
 *
 * @defgroup    bolt BOLT
 * @{
 *
 * @file
 * @author
 *              Reto Da Forno
 *
 * This library provides functionality to configure and use the asynchronous
 * data interface.
 * Set BOLT_CONF_ON to 1 to use this library. By default, DMA usage is disabled
 * and the max. message length is 32 bytes. To adjust the max. message length, 
 * define BOLT_CONF_MAX_MSG_LEN in your configuration file.
 * @note The data transfer over the SPI can either be synchronous (blocking,
 * polling/busy wait) or asynchronous (interrupt/DMA-driven).
 * @remark This lib does not require a timer to work.
 */

#ifndef __BOLT_H__
#define __BOLT_H__

#if BOLT_CONF_ON

#ifndef BOLT_CONF_SPI
#define BOLT_CONF_SPI                   USCI_A0   /* USCI module (SPI_A0_BASE) */
#endif /* BOLT_CONF_SPI */

#ifndef BOLT_PIN_REQ
#define BOLT_PIN_TIMEREQ                PORT2, PIN1
#define BOLT_PIN_REQ                    PORT2, PIN2
#define BOLT_PIN_IND                    PORT2, PIN3
#define BOLT_PIN_ACK                    PORT2, PIN4
#define BOLT_PIN_MODE                   PORT2, PIN5
#endif /* BOLT_PIN_REQ */

#ifndef BOLT_CONF_TIMEREQ_TIMERID
#define BOLT_CONF_TIMEREQ_TIMERID       RTIMER_TA1_0
#endif /* BOLT_CONF_TIMEREQ_TIMERID */

#ifndef BOLT_CONF_MAX_MSG_LEN
#define BOLT_CONF_MAX_MSG_LEN           32              /* bytes */
#endif /* BOLT_CONF_MAX_MSG_LEN */

#ifndef BOLT_CONF_USE_DMA
#define BOLT_CONF_USE_DMA               0
#endif /* BOLT_CONF_USE_DMA */

#ifndef BOLT_CONF_TIMEREQ_ENABLE
#define BOLT_CONF_TIMEREQ_ENABLE        0
#endif /* BOLT_CONF_TIMEREQ_ENABLE */

#ifndef BOLT_CONF_SCLK_SPEED            /* serial clock speed */
#define BOLT_CONF_SCLK_SPEED            (SMCLK_SPEED)
#endif /* BOLT_CONF_SCLK_SPEED */

/**
 * @brief checks whether there is data to read from BOLT
 * @remark IND line high means data is available
 */
#define BOLT_DATA_AVAILABLE             PIN_GET_INPUT_BIT(BOLT_PIN_IND)

/**
 * @brief writes one message (num_bytes bytes of the buffer data) to the
 * asynchronous interface
 */
#define BOLT_WRITE(in_data, num_bytes) \
  { \
    if(bolt_acquire(BOLT_OP_WRITE)) { \
      bolt_start(in_data, &num_bytes); \
      bolt_release(); \
    } \
  }

/**
 * @brief reads one message from the asynchronous interface
 */
#define BOLT_READ(out_data, num_rcvd_bytes) \
  { \
    if(bolt_acquire(BOLT_OP_READ)) { \
      bolt_start(out_data, &num_rcvd_bytes); \
      bolt_release(); \
    } \
  }


/**
 * @brief the two possible data operations: read or write
 */
typedef enum {
  BOLT_OP_READ = 0,
  BOLT_OP_WRITE,
  NUM_OF_OPS
} bolt_op_mode_t;


/**
 * @brief initializes all required GPIO pins and peripherals to use the
 * asynchronous data interface
 *
 * Configures the GPIO pins AI_CTRL_IND, AI_CTRL_MODE, AI_CTRL_REQ and
 * AI_CTRL_ACK as well as the peripheral modules BOLT_CONF_SPI and the DMA (if
 * BOLT_USE_DMA is defined).
 */
void bolt_init(void);

/**
 * @brief requests an operation on the asynchronous data interface
 * @param[in] mode the operating mode, either OP_READ or OP_WRITE
 * @return    one if the request was successful (REQ pin was set), zero
 * otherwise
 *
 * Prepares a data transfer over the asynchronous interface by enabling the
 * SPI, setting up the DMA (if BOLT_USE_DMA is defined) and acquiring a lock
 * (set request pin high).
 */
uint8_t bolt_acquire(bolt_op_mode_t mode);

/**
 * @brief start an operation on the asynchronous data interface
 * @param[in,out] data A pointer to the data buffer (an input in write mode and
 * an output in read mode). Set this parameter to 0 if DMA mode is used.
 * @param[in,out] num_bytes The number of bytes to transmit (in write mode);
 * the number of transmitted bytes will be stored in num_bytes.
 * @return 1 if the data transfer was successful, 0 otherwise
 * @note this is a blocking call
 */
uint8_t bolt_start(uint8_t *data, uint16_t *num_bytes);

#if BOLT_CONF_USE_DMA
/**
 * @brief set the buffers for the DMA driven data transfer
 * @param[in] rx_buffer_addr address of the reception buffer
 * @param[in] tx_buffer_addr address of the transmit buffer
 * @remark this is an optional call, buffer addresses can also be passed to the
 * interface with the bolt_start_op() call
 */
void bolt_set_dma_buffers(uint16_t rx_buffer_addr, uint16_t tx_buffer_addr);
#endif

#if BOLT_CONF_TIMEREQ_ENABLE
/**
 * @brief set the callback function for the timestamp request functionality
 * @param[in] func a pointer to the callback function. Set this parameter to
 * 0 disable the timestamp request interrupt and to use polling instead
 */
void bolt_set_timereq_callback(void (*func)(void)); 

/**
 * @brief checks whether a timestamp request is pending and handles it if so
 * @return one if there is a timestamp request pending, zero otherwise
 */
uint8_t bolt_handle_timereq(uint8_t *out_buffer);
#endif /* BOLT_CONF_TIMEREQ_ENABLE */

/**
 * @brief release the asynchronous data interface and clean up
 *
 * Resets the REQ pin to put the asynchronous interface back into idle state
 * and disables the DMA and SPI.
 * @note Any ongoing operation on BOLT_CONF_SPI will be terminated immediately.
 */
void bolt_release(void);

#endif /* BOLT_CONF_ON */

#endif /* __BOLT_H__ */

/**
 * @}
 * @}
 */