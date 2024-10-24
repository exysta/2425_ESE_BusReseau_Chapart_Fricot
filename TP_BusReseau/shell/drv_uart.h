/*
 * drv_uart.h
 *
 *  Created on: Oct 18, 2024
 *      Author: exysta
 */

#ifndef DRV_UART_H_
#define DRV_UART_H_

// UART Configuration
#define UART_DEVICE_PC huart2
#define UART_DEVICE_RPI huart3


// UART MODE
//#define DRV_UART_MODE_SIMPLE
#define DRV_UART_MODE_INTERRUPT


#ifdef DRV_UART_MODE_INTERRUPT

//char received_char;
#endif

uint8_t drv_uart_receive(char * pData, uint16_t size);
uint8_t drv_uart_transmit(char * pData, uint16_t size);
uint8_t drv_uart_waitReceiveComplete(void);
uint8_t drv_uart_waitTransmitComplete(void);

#endif /* DRV_UART_H_ */
