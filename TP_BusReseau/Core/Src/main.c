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
#include "cmsis_os.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
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
#define TASK_SHELL_STACK_DEPTH 256
#define TASK_SHELL_PRIORITY 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
h_shell_t h_shell;
xTaskHandle h_shell_handle;
uint8_t GET_T_FLAG = 0;
uint16_t bmp280_addr_shifted =  BMP280_ADDR << 1; // adresse du composant contenant l'ID, qu'on décale de 1 bit (voir énoncé)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t convertBufferToUint32(uint8_t buffer[3]) {
    return (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
}

int GET_T(h_shell_t * h_shell,int argc, char ** argv)
{
	//récupération de la température
	uint8_t temp_start_addr = 0xFA; // l'adresse de départ du registre température
	uint8_t temp_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
	uint8_t current_temp_addr = temp_start_addr;
	uint8_t temp_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

	for(int i = 0; i <3;i++)
	{

		HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, &current_temp_addr, 1, 1000); // on demande à récup valeur de l'adresse courante
		HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &temp_value, 1, 1000); // on récupère la valeur de calibration de l'adresse courante
		temp_value_buffer[i] = temp_value; // on la range dans le buffer
		current_temp_addr++; // on incrémente l'adresse
	}
	uint32_t temp_value_32  =	convertBufferToUint32(temp_value_buffer);


	printf("température non compensée %d \r\n",temp_value_32);
	return 0;
}

int __io_put_char(int chr)
{
	HAL_UART_Transmit(&huart2, (uint8_t*) &chr, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart3, (uint8_t*) &chr, 1, HAL_MAX_DELAY);

	return chr;
}


void task_shell(void * unused)
{
	shell_init(&h_shell);
	shell_add(&h_shell,'t', GET_T, "Une fonction qui lit la température youppii !!");
	shell_run(&h_shell);
	// shell_run() infinie donc la task ne se finie jamais
	//rappel : une tache ne doit jamais retourner
	// si on arrete une tache, il faut appeler vTaskDelete(0);

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
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  BaseType_t ret;
  ret = xTaskCreate(task_shell, "shell", TASK_SHELL_STACK_DEPTH, NULL, TASK_SHELL_PRIORITY, &h_shell_handle);
#ifdef TP1

	// on récupère ID
	uint8_t bmp280_id = 0;
	uint8_t bmp280_addr_id = 0xD0; // l'adresse du registre contenant l'ID
	HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, &bmp280_addr_id, 1, 1000); //on envoie l'adresse du registre qu'on veut récupérer
	HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &bmp280_id, 1, 1000);
	//--------------------------------------------------------------------------------------

	// on config

	uint8_t bmp280_addr_ctrl_meas = 0xF4; // l'adresse du registre contenant le control des mesures
	uint8_t bmp280_config_ctrl_meas = 0x57; // la valuer de la config a appliqué
	uint8_t bmp280_ctrl_meas_buffer[2] = {bmp280_addr_ctrl_meas, bmp280_config_ctrl_meas};
	uint8_t value = 0;
	//on envoie la config
	HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, bmp280_ctrl_meas_buffer, 2, 1000); //on envoie buffer avec adresse du registre
																							//puis valeur à y écrire
	//pour vérification
	HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &value, 1, 1000);

	//--------------------------------------------------------------------------------------
	//Récupération de létalonnage
	uint8_t calibration_buffer[BMP280_CALIBRATION_BUFFER_SIZE];
	uint8_t first_calibration_addr = 0x88; // valeur de la première adresse du registre calibration
	uint8_t current_calibration_addr = first_calibration_addr; // variable que l'on va incrémenter pour récupérer toutes les adresses
	uint8_t calibration_value;
	for(int i = 0; i <BMP280_CALIBRATION_BUFFER_SIZE;i++)
	{

		HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, &current_calibration_addr, 1, 1000); // on demande à récup valeur de l'adresse courante
		HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &calibration_value, 1, 1000); // on récupère la valeur de calibration de l'adresse courante
		calibration_buffer[i] = calibration_value; // on la range dans le buffer
		current_calibration_addr++; // on incrémente l'adresse
	}

	//récupération de la pression
	uint8_t pressure_start_addr = 0xF7; // l'adresse de départ du registre pression
	uint8_t pressure_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
	uint8_t current_pressure_addr = pressure_start_addr;
	uint8_t pressure_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

	for(int i = 0; i <3;i++)
	{

		HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, &current_pressure_addr, 1, 1000); // on demande à récup valeur de l'adresse courante
		HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &pressure_value, 1, 1000); // on récupère la valeur de calibration de l'adresse courante
		pressure_value_buffer[i] = pressure_value; // on la range dans le buffer
		current_pressure_addr++; // on incrémente l'adresse
	}
	//HAL_I2C_Mem_Read(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout)
	uint32_t pressure_value_32 = convertBufferToUint32(pressure_value_buffer);
	//--------------------------------------------------------------------------------------
	//récupération de la température
	uint8_t temp_start_addr = 0xFA; // l'adresse de départ du registre température
	uint8_t temp_value_buffer[3]; //chaque adresse sera stocké dans un byte puis on combinera les bytes
	uint8_t current_temp_addr = temp_start_addr;
	uint8_t temp_value; //chaque adresse sera stocké dans un byte puis on combinera les bytes

	for(int i = 0; i <3;i++)
	{

		HAL_I2C_Master_Transmit(&hi2c1, bmp280_addr_shifted, &current_temp_addr, 1, 1000); // on demande à récup valeur de l'adresse courante
		HAL_I2C_Master_Receive(&hi2c1, bmp280_addr_shifted, &temp_value, 1, 1000); // on récupère la valeur de calibration de l'adresse courante
		temp_value_buffer[i] = temp_value; // on la range dans le buffer
		current_temp_addr++; // on incrémente l'adresse
	}
	uint32_t temp_value_32  =	convertBufferToUint32(temp_value_buffer);
#endif

	//--------------------------------------------------------------------------------------
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
