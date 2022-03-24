/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum DataRate{
	DATA_RATE_ONE_SHOT = 0b00000000,
	DATA_RATE_1Hz = 0b00010000,
	DATA_RATE_7Hz = 0b00100000,
	DATA_RATE_12_5Hz = 0b00110000,
	DATA_RATE_25Hz = 0b01000000
}DataRate;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PRESSURE_SENSOR_ADDR 0b10111010
#define HUMIDITY_TEMPERATURE_SENSOR_ADDR 0b10111110

#define PRESSURE_POWER_DOWN 0b10000000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t i=0;
uint8_t ret;
uint8_t data;
uint8_t rawPressureData[3];
int32_t pressureData;
float pressure;
uint8_t rawTemperatureData[2];
int16_t temperatureData;
float temperature;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart3,(uint8_t*)&ch,1,HAL_MAX_DELAY);
	return ch;
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
  MX_USART3_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */

  data = (uint8_t)(DATA_RATE_1Hz | PRESSURE_POWER_DOWN);
  HAL_I2C_Mem_Write(&hi2c1, PRESSURE_SENSOR_ADDR, 0x20, 1, &data, 1, 50);
  data = 0;
  HAL_I2C_Mem_Write(&hi2c1, PRESSURE_SENSOR_ADDR, 0x10, 1, &data, 1, 50);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  HAL_Delay(1000);

	  HAL_I2C_Mem_Read(&hi2c1, PRESSURE_SENSOR_ADDR, 0x28, 1, &rawPressureData[0], 1, 50);
	  HAL_I2C_Mem_Read(&hi2c1, PRESSURE_SENSOR_ADDR, 0x29, 1, &rawPressureData[1], 1, 50);
	  HAL_I2C_Mem_Read(&hi2c1, PRESSURE_SENSOR_ADDR, 0x2A, 1, &rawPressureData[2], 1, 50);

	  HAL_I2C_Mem_Read(&hi2c1, PRESSURE_SENSOR_ADDR, 0x2B, 1, &rawTemperatureData[0], 1, 50);
	  HAL_I2C_Mem_Read(&hi2c1, PRESSURE_SENSOR_ADDR, 0x2C, 1, &rawTemperatureData[1], 1, 50);

	  pressureData = rawPressureData[0];
	  pressureData |= (rawPressureData[1] << 8);
	  if(rawPressureData[2] & 0x80){
		  pressureData |= (rawPressureData[2] << 16);
		  pressureData |= (0xFF << 24);
	  }else{
		  pressureData |= (rawPressureData[2] << 16);
	  }
	  pressure = pressureData/4096.0;

	  temperatureData = rawTemperatureData[0] + (rawTemperatureData[1] << 8);
	  temperature = 42.5 + temperatureData/480.0;

	  if(ret != HAL_OK){
		  printf("[CONNECTION ERROR] Unable to get data");
	  }
	  else{
		  printf("%d\n\r", data);
	  }

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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
