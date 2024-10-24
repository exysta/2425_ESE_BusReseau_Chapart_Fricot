/*
 * drv_uart.c
 *
 *  Created on: Oct 18, 2024
 *      Author: exysta
 */
#include <stdlib.h>
#include "usart.h"
#include "drv_uart.h"
#include "cmsis_os.h"

#ifdef DRV_UART_MODE_SIMPLE
	uint8_t drv_uart_receive(char * pData, uint16_t size)
{
	HAL_UART_Receive(&UART_DEVICE, (uint8_t*)pData, size,HAL_MAX_DELAY);
	return size;
}

uint8_t drv_uart_transmit(char * pData, uint16_t size)
{
	HAL_UART_Transmit(&UART_DEVICE, (uint8_t*)pData, size,HAL_MAX_DELAY);
	return size;
}
#endif

#ifdef DRV_UART_MODE_INTERRUPT

static TaskHandle_t uartRxTaskHandle = NULL;  // Task handle for RX notification
static TaskHandle_t uartTxTaskHandle = NULL;  // Task handle for TX notification
static volatile uint8_t uartRxComplete = 0;   // Flag for RX completion
static volatile uint8_t uartTxComplete = 0;   // Flag for TX completion

// Non-blocking UART Receive function
uint8_t drv_uart_receive(char *pData, uint16_t size)
{
    // Register the current task as the one to notify when RX is complete
    uartRxTaskHandle = xTaskGetCurrentTaskHandle();

    // Start UART reception in interrupt mode (non-blocking)
    HAL_UART_Receive_IT(&UART_DEVICE_RPI, (uint8_t*)pData, size);

    // Task will check completion via notification, so return immediately
    return size;
}

// Non-blocking UART Transmit function
uint8_t drv_uart_transmit(char *pData, uint16_t size)
{
    // Register the current task as the one to notify when TX is complete
    uartTxTaskHandle = xTaskGetCurrentTaskHandle();

    // Start UART transmission in interrupt mode (non-blocking)
    HAL_UART_Transmit_IT(&UART_DEVICE_PC, (uint8_t*)pData, size);
    HAL_UART_Transmit_IT(&UART_DEVICE_RPI, (uint8_t*)pData, size);

    // Task will check completion via notification, so return immediately
    return size;
}

// UART receive complete callback (called by HAL when receive interrupt occurs)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // Check if the interrupt was triggered for the correct UART
    if (huart->Instance == UART_DEVICE_PC.Instance ||huart->Instance == UART_DEVICE_RPI.Instance  )
    {
        // Notify the task waiting for RX completion
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (uartRxTaskHandle != NULL)
        {
            vTaskNotifyGiveFromISR(uartRxTaskHandle, &xHigherPriorityTaskWoken);
            uartRxTaskHandle = NULL;  // Clear task handle after notifying
        }

        // Context switch if necessary
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// UART transmit complete callback (called by HAL when transmit interrupt occurs)
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // Check if the interrupt was triggered for the correct UART
    if (huart->Instance == UART_DEVICE_PC.Instance ||huart->Instance == UART_DEVICE_RPI.Instance  )
    {
        // Notify the task waiting for TX completion
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (uartTxTaskHandle != NULL)
        {
            vTaskNotifyGiveFromISR(uartTxTaskHandle, &xHigherPriorityTaskWoken);
            uartTxTaskHandle = NULL;  // Clear task handle after notifying
        }

        // Context switch if necessary
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Function to check if UART receive is complete (blocking until notification received)
uint8_t drv_uart_waitReceiveComplete(void)
{
    // Wait for RX task notification (blocking if necessary)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return 1;
}

// Function to check if UART transmit is complete (blocking until notification received)
uint8_t drv_uart_waitTransmitComplete(void)
{
    // Wait for TX task notification (blocking if necessary)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return 1;
}

#endif
