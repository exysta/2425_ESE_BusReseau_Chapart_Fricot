/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define TP1
#define BMP280_ADDR 0x77
#define MPU9250_ADDR 0x68
#define BMP280_CALIBRATION_BUFFER_SIZE 25
#define CAN
#define NO_PROBLEMO 0
#define YES_PROBLEMO 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t GET_T_FLAG = 0;
uint16_t bmp280_addr_shifted =  BMP280_ADDR << 1; // adresse du composant contenant l'ID, qu'on décale de 1 bit (voir énoncé)
uint8_t calibration_buffer[BMP280_CALIBRATION_BUFFER_SIZE];
// t_fine carries fine temperature as global value
int32_t t_fine;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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

	int GET_T()
	{
		//récupération de la température
		uint8_t temp_start_addr = 0xFA; // l'adresse de départ du registre température
		uint8_t temp_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
		uint8_t current_temp_addr = temp_start_addr;
		uint8_t temp_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

		for(int i = 0; i <3;i++)
		{

			HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, &current_temp_addr, 1, 1000); // on demande à récup valeur de l'adresse courante
			HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &temp_value, 1, 1000); // on récupère la valeur de calibration de l'adresse courante
			temp_value_buffer[i] = temp_value; // on la range dans le buffer
			current_temp_addr++; // on incrémente l'adresse
		}
		uint32_t temp_value_32  =	convertBufferToUint32(temp_value_buffer);
		temp_value_32 = bmp280_compensate_T_int32(temp_value_32);
		float temp_value_c = temp_value_32 * 0.0025f;
		uint32_t temp_value_c_scaled = (int)(temp_value_c * 100); //1234 = 12.34 degrés celsius

		printf("Temperature (à divisé par 100 ): %lu \r\n", temp_value_c_scaled);
		return 0;
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

#ifdef CAN

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


#endif

	int __io_put_char(int chr)
	{
		HAL_UART_Transmit(&huart2, (uint8_t*) &chr, 1, HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart3, (uint8_t*) &chr, 1, HAL_MAX_DELAY);

		return chr;
	}



	/* USER CODE END 0 */

	/**
	 * @brief  The application entry point.
	 * @retval int
	 */
	int main(void)
	{

		/* USER CODE BEGIN 1 */

		/* USER CODE END 1 */

		/* MCU Configuration--------------------------------------------------------*/

		/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
		HAL_Init();

		/* USER CODE BEGIN Init */

		/* USER CODE END Init */

		/* Configure the system clock */
		SystemClock_Config();

		/* USER CODE BEGIN SysInit */

		/* USER CODE END SysInit */

		/* Initialize all configured peripherals */
		MX_GPIO_Init();
		MX_USART2_UART_Init();
		MX_CAN1_Init();
		MX_USART3_UART_Init();
		MX_I2C3_Init();
		/* USER CODE BEGIN 2 */

#ifdef SENSOR_TEST
		// on récupère ID
		uint8_t bmp280_id = 0;
		uint8_t bmp280_addr_id = 0xD0; // l'adresse du registre contenant l'ID
		HAL_I2C_Master_Transmit(&hi2c3, bmp280_addr_shifted, &bmp280_addr_id, 1, 1000); //on envoie l'adresse du registre qu'on veut récupérer
		HAL_I2C_Master_Receive(&hi2c3, bmp280_addr_shifted, &bmp280_id, 1, 1000);
		//--------------------------------------------------------------------------------------




#endif

		uint8_t uart_transmission_end_flag = 0;
		uint8_t received_char;
		uint8_t receive_buffer[10];
		memset(receive_buffer, 0, sizeof(receive_buffer));

		bmp280_config();
		bmp280_etalonnage();

		uint8_t prompt[] = ">>> \r\n";
#ifdef CAN

		HAL_CAN_Start(&hcan1);

#endif
		/* USER CODE END 2 */

		/* Infinite loop */
		/* USER CODE BEGIN WHILE */
		while (1)
		{
			//		CAN_Send_AutomaticMode(0x54,0x01);
			//		HAL_Delay(1000);
			//		CAN_Send_AutomaticMode(0x54,0x00);
			//
			//		HAL_Delay(1000);
			printf(prompt);
			int buffer_index = 0;
			uart_transmission_end_flag = 0;

			while(uart_transmission_end_flag == 0)
			{
				HAL_UART_Receive(&huart2, &received_char, 1, HAL_MAX_DELAY);// pour raspberry
				if((received_char != '\r') && (received_char != '\n') )
				{
					receive_buffer[buffer_index] = received_char;
					buffer_index++;
				}
				else
				{
					if(strcmp((const char*)receive_buffer,"GET_T") == 0)
					{
						if(GET_T() != NO_PROBLEMO)
						{
							printf("problem while reading temp");
						}
					}
					if(strcmp((const char*)receive_buffer,"GET_P") == 0)
					{
						if(GET_P() != NO_PROBLEMO)
						{
							printf("problem while reading pressure");
						}
					}
					memset(receive_buffer, 0, sizeof(receive_buffer));
					uart_transmission_end_flag = 1;
				}
			}

			/* USER CODE END WHILE */

			/* USER CODE BEGIN 3 */
		}
		/* USER CODE END 3 */
	}

	/**
	 * @brief System Clock Configuration
	 * @retval None
	 */
	void SystemClock_Config(void)
	{
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};
		RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

		/** Configure the main internal regulator output voltage
		 */
		__HAL_RCC_PWR_CLK_ENABLE();
		__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

		/** Initializes the RCC Oscillators according to the specified parameters
		 * in the RCC_OscInitTypeDef structure.
		 */
		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
		RCC_OscInitStruct.HSEState = RCC_HSE_ON;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
		RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
		RCC_OscInitStruct.PLL.PLLM = 4;
		RCC_OscInitStruct.PLL.PLLN = 180;
		RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
		RCC_OscInitStruct.PLL.PLLQ = 2;
		RCC_OscInitStruct.PLL.PLLR = 2;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		/** Activate the Over-Drive mode
		 */
		if (HAL_PWREx_EnableOverDrive() != HAL_OK)
		{
			Error_Handler();
		}

		/** Initializes the CPU, AHB and APB buses clocks
		 */
		RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
				|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
		RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
		RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
		RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

		if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		{
			Error_Handler();
		}
	}

	/* USER CODE BEGIN 4 */

	/* USER CODE END 4 */

	/**
	 * @brief  This function is executed in case of error occurrence.
	 * @retval None
	 */
	void Error_Handler(void)
	{
		/* USER CODE BEGIN Error_Handler_Debug */
		/* User can add his own implementation to report the HAL error return state */
		__disable_irq();
		while (1)
		{
		}
		/* USER CODE END Error_Handler_Debug */
	}

#ifdef  USE_FULL_ASSERT
	/**
	 * @brief  Reports the name of the source file and the source line number
	 *         where the assert_param error has occurred.
	 * @param  file: pointer to the source file name
	 * @param  line: assert_param error line source number
	 * @retval None
	 */
	void assert_failed(uint8_t *file, uint32_t line)
	{
		/* USER CODE BEGIN 6 */
		/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
		/* USER CODE END 6 */
	}
#endif /* USE_FULL_ASSERT */
