/*
 * shell.c
 *
 *  Created on: Oct 1, 2023
 *      Author: nicolas
 */
#include "usart.h"
#include "shell.h"
#include "can.h"
#include "i2c.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#define BMP280_ADDR 0x77
#define MPU9250_ADDR 0x68
#define BMP280_CALIBRATION_BUFFER_SIZE 25
#define CAN
#define POSITIVE_ROTATION 0
#define NEGATIVE_ROTATION 1
#define BASE_TEMP 25



uint8_t prompt[]="lulu_chaha@bus_reseau>>";
uint8_t started[]=
		"\r\n*-----------------------------*"
		"\r\n| Welcome luluchacha |"
		"\r\n*-----------------------------*"
		"\r\n";
uint8_t newline[]="\r\n";
uint8_t backspace[]="\b \b";
uint8_t cmdNotFound[]="Command not found\r\n";
uint8_t brian[]="Brian is in the kitchen\r\n";
uint8_t uartRxReceived;
uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];
uint8_t uartTxBuffer[UART_TX_BUFFER_SIZE];
uint16_t bmp280_addr_shifted =  BMP280_ADDR << 1; // adresse du composant contenant l'ID, qu'on décale de 1 bit (voir énoncé)
uint8_t calibration_buffer[BMP280_CALIBRATION_BUFFER_SIZE];
uint32_t temp;
// t_fine carries fine temperature as global value
int32_t t_fine;
uint8_t motor_coeff;

char	 	cmdBuffer[CMD_BUFFER_SIZE];
int 		idx_cmd;
char* 		argv[MAX_ARGS];
int		 	argc = 0;
char*		token;
int 		newCmdReady = 0;

int control_motor()
{
	GET_T(&temp);
	int new_temp = temp - BASE_TEMP*10;
	uint8_t sens;
	if(new_temp <0)
	{
		sens = 0;
	}
	else
	{
		sens = 1;
	}
	uint32_t motor_angle = abs(new_temp) * motor_coeff;
	CAN_Send_AutomaticMode(motor_angle,sens);
}


int set_angle(char **argv,int argc)
{
	int angle = atoi(argv[1]);//speed in expected in % of max speed
	if(argc != 2)
	{
		uint8_t error_message[] = "Error : angle function expect exactly 1 parameter \r\n";
		HAL_UART_Transmit(&huart2, error_message, sizeof(error_message), HAL_MAX_DELAY);
		return 1;
	}

	else if(angle > 180)//on vérifie qu'on met pas l'angle au dessus du max
	{
		uint8_t error_message[] = "angle function must not exceed 180  \r\n";
		HAL_UART_Transmit(&huart2, error_message, sizeof(error_message), HAL_MAX_DELAY);
	}
	uint32_t real_angle;
	uint8_t sens;
	if(angle <0)
	{
		real_angle = -angle;
		sens = NEGATIVE_ROTATION;
	}
	else
	{
		real_angle = angle;
		sens = POSITIVE_ROTATION;
	}
	CAN_Send_AutomaticMode(real_angle,sens);
	uint8_t * success_message = "angle set to %lu \r\n";
	//snprintf((char *)success_message, sizeof(success_message), "speed set to %lu of max value \r\n", (unsigned long)speed);
	HAL_UART_Transmit(&huart2, success_message, sizeof(success_message), HAL_MAX_DELAY);

}

void CAN_Send_AutomaticMode(uint8_t angle, uint8_t sign) {
	CAN_TxHeaderTypeDef txHeader;
	uint8_t data[2];         // Data array for angle and sign
	uint32_t txMailbox;

	// Limit the angle to 180 degrees max
	if (angle > 180) angle = 180;

	// Frame configuration for Automatic Mode
	txHeader.StdId = 0x61;           // ID for Automatic Mode angle setting
	txHeader.ExtId = 0x1ABCDE;       // Not used here
	txHeader.IDE = CAN_ID_STD;       // Standard CAN ID
	txHeader.RTR = CAN_RTR_DATA;     // Data frame
	txHeader.DLC = 2;                // 2 bytes: D0 (angle), D1 (sign)
	txHeader.TransmitGlobalTime = DISABLE;

	// Data configuration
	data[0] = angle;                 // D0: Desired angle (0 to 180)
	data[1] = sign;                  // D1: Angle sign (0x00 for positive, 0x01 for negative)

	// Send the frame
	if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, data, &txMailbox) != HAL_OK) {
		// Error handling
		Error_Handler();
	}
}


uint32_t convertBufferToUint32(uint8_t buffer[3]) {
	return (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
}

int bmp280_config()
{
	// on config

	uint8_t bmp280_addr_ctrl_meas = 0xF4; // l'adresse du registre contenant le control des mesures
	uint8_t bmp280_config_ctrl_meas = 0x57; // la valuer de la config a appliqué
	uint8_t bmp280_ctrl_meas_buffer[2] = {bmp280_addr_ctrl_meas, bmp280_config_ctrl_meas};
	uint8_t value = 0;
	//on envoie la config
	//on envoie buffer avec adresse du registre
	//puis valeur à y écrire
	if( HAL_OK != HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, bmp280_ctrl_meas_buffer, 2, 1000))
	{
		return 1;
	}
	//pour vérification
	HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &value, 1, 1000);
	return 0;
}

int bmp280_etalonnage()
{
	//--------------------------------------------------------------------------------------
	//Récupération de létalonnage
	uint8_t first_calibration_addr = 0x88; // valeur de la première adresse du registre calibration
	uint8_t current_calibration_addr = first_calibration_addr; // variable que l'on va incrémenter pour récupérer toutes les adresses
	uint8_t calibration_value;
	for(int i = 0; i <BMP280_CALIBRATION_BUFFER_SIZE;i++)
	{
		if( HAL_OK != HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, &current_calibration_addr, 1, 1000))  // on demande à récup valeur de l'adresse courante

		{
			return 1;
		}
		if( HAL_OK != HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &calibration_value, 1, 1000))  // on demande à récup valeur de l'adresse courante
		{
			return 1;
		}
		// on récupère la valeur de calibration de l'adresse courante
		calibration_buffer[i] = calibration_value; // on la range dans le buffer
		current_calibration_addr++; // on incrémente l'adresse
	}
	return 0;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value

int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((int32_t)calibration_buffer[0]<<1))) * ((int32_t)calibration_buffer[1])) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)calibration_buffer[0])) * ((adc_T>>4) - ((int32_t)calibration_buffer[0]))) >> 12) *
			((int32_t)calibration_buffer[2])) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t bmp280_compensate_P_int64(int32_t adc_P)
{
	int64_t var1, var2, p;
	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)calibration_buffer[8];
	var2 = var2 + ((var1*(int64_t)calibration_buffer[7])<<17);
	var2 = var2 + (((int64_t)calibration_buffer[6])<<35);
	var1 = ((var1 * var1 * (int64_t)calibration_buffer[5])>>8) + ((var1 * (int64_t)calibration_buffer[4])<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calibration_buffer[3])>>33;
	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((int64_t)calibration_buffer[11]) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)calibration_buffer[10]) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)calibration_buffer[9])<<4);
	return (uint32_t)p;
}

int GET_T(uint32_t * temp)
{
	//récupération de la température
	uint8_t temp_start_addr = 0xFA; // l'adresse de départ du registre température
	uint8_t temp_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
	uint8_t current_temp_addr = temp_start_addr;
	uint8_t temp_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

	for(int i = 0; i <3;i++)
	{

		HAL_StatusTypeDef test1=HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, &current_temp_addr, 1, 1000);

		HAL_StatusTypeDef test2=HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &temp_value, 1, 1000);

		temp_value_buffer[i] = temp_value; // on la range dans le buffer
		current_temp_addr++; // on incrémente l'adresse
	}
	uint32_t temp_value_32  =	convertBufferToUint32(temp_value_buffer);
	temp_value_32 = bmp280_compensate_T_int32(temp_value_32);
	float temp_value_c = temp_value_32 * 0.0025f;
	*temp = (int)(temp_value_c * 100); //1234 = 12.34 degrés celsius

	return 0;
}

void PRINT_T()
{
	GET_T(&temp);
	printf("Temperature (à divisé par 10 ): %lu \r\n", temp);

}

int GET_P()
{
	//récupération de la pression
	uint8_t pressure_start_addr = 0xF7; // l'adresse de départ du registre pression
	uint8_t pressure_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
	uint8_t current_pressure_addr = pressure_start_addr;
	uint8_t pressure_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

	for(int i = 0; i <3;i++)
	{

		if(HAL_OK != HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, &current_pressure_addr, 1, 1000)) // on demande à récup valeur de l'adresse courante
		{
			return 1;
		}
		if(HAL_OK != HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &pressure_value, 1, 1000)) // on demande à récup valeur de l'adresse courante
		{
			return 1;
		}
		// on récupère la valeur de calibration de l'adresse courante
		pressure_value_buffer[i] = pressure_value; // on la range dans le buffer
		current_pressure_addr++; // on incrémente l'adresse
	}
	//HAL_I2C_Mem_Read(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout)
	uint32_t pressure_value_32 = convertBufferToUint32(pressure_value_buffer);
	pressure_value_32 = bmp280_compensate_P_int64(pressure_value_32);
	printf("pression non compensée %lu \r\n",pressure_value_32);
	return 0;
}

void GET_K()
{
	printf("K=%u\r\n",motor_coeff);
}

void SET_K()
{

}

void Shell_Init(void){
	memset(argv, NULL, MAX_ARGS*sizeof(char*));
	memset(cmdBuffer, NULL, CMD_BUFFER_SIZE*sizeof(char));
	memset(uartRxBuffer, NULL, UART_RX_BUFFER_SIZE*sizeof(char));
	memset(uartTxBuffer, NULL, UART_TX_BUFFER_SIZE*sizeof(char));

	bmp280_config();
	bmp280_etalonnage();
	HAL_CAN_Start(&hcan1);
	motor_coeff = 1;

	HAL_UART_Transmit(&huart2, started, strlen((char *)started), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, prompt, strlen((char *)prompt), HAL_MAX_DELAY);
	HAL_UART_Receive_IT(&huart2, uartRxBuffer, UART_RX_BUFFER_SIZE);

}


// Main shell loop
void Shell_Loop(void) {
	if (uartRxReceived) {  // Check if UART data was received
		switch (uartRxBuffer[0]) {
		case ASCII_CR:  // Newline character, process the command
			HAL_UART_Transmit(&huart2, newline, sizeof(newline), HAL_MAX_DELAY);
			cmdBuffer[idx_cmd] = '\0';
			argc = 0;
			token = strtok(cmdBuffer, " ");
			while (token != NULL) {
				argv[argc++] = token;
				token = strtok(NULL, " ");
			}
			idx_cmd = 0;
			newCmdReady = 1;
			break;

		case ASCII_BACK:  // Backspace, delete last character
			if (idx_cmd > 0) {  // Ensure we don't go below 0
				cmdBuffer[--idx_cmd] = '\0';
				HAL_UART_Transmit(&huart2, backspace, sizeof(backspace), HAL_MAX_DELAY);
			}
			break;

		default:  // Add new character to command buffer
			if (idx_cmd < sizeof(cmdBuffer) - 1) {  // Avoid overflow
				cmdBuffer[idx_cmd++] = uartRxBuffer[0];
				HAL_UART_Transmit(&huart2, uartRxBuffer, 1, HAL_MAX_DELAY);
			}
			break;
		}
		uartRxReceived = 0;  // Reset the received flag after processing
	}

	if (newCmdReady) {
		if (strcmp(argv[0], "WhereisBrian?") == 0) {
			HAL_UART_Transmit(&huart2, brian, sizeof(brian), HAL_MAX_DELAY);
		} else if (strcmp(argv[0], "help") == 0) {
			int uartTxStringLength = snprintf((char *)uartTxBuffer, UART_TX_BUFFER_SIZE, "Print all available functions here\r\n");
			HAL_UART_Transmit(&huart2, uartTxBuffer, uartTxStringLength, HAL_MAX_DELAY);
		} else if (strcmp(argv[0], "angle") == 0) {
			set_angle(argv, argc);
		}
		else if (strcmp(argv[0], "GET_T") == 0) {
			PRINT_T(&temp);
		}
		else {
			HAL_UART_Transmit(&huart2, cmdNotFound, sizeof(cmdNotFound), HAL_MAX_DELAY);
		}
		HAL_UART_Transmit(&huart2, prompt, sizeof(prompt), HAL_MAX_DELAY);
		newCmdReady = 0;
	}
}

// Callback function to handle UART receive completion
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {  // Ensure the callback is for the correct UART instance
		uartRxReceived = 1;  // Set the flag to indicate data is ready to be processed
		// Restart the UART receive interrupt
		HAL_UART_Receive_IT(&huart2, uartRxBuffer, 1);  // Read 1 byte at a time
	}
}
