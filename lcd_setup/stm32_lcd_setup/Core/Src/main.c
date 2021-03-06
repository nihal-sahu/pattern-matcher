#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "main.h"

#define ARDUINO_ADDRESS (0x33<<1)	//arduino address

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
void led_pattern_gen();
void sendData();
void receiveData();
uint8_t button_matching();
bool pattern_check();
void next_level();

uint8_t ArduinoDataBuffer[50] = {};				//received data buffer
uint8_t STM32DataBuffer[50] = {};				//sent data buffer
uint8_t led_pattern[100] = {};					//store the led pattern
uint8_t bttn_pattern[100] = {};					//store the users button presses

//points to position of received data in the i2c buffer
uint8_t *receivedData = (uint8_t*)&ArduinoDataBuffer[0];
//points to first element of buffer
uint8_t *sentData = (uint8_t*)&STM32DataBuffer[0];

uint16_t led_arr[3] = {GPIO_PIN_8, GPIO_PIN_6, GPIO_PIN_5};

uint16_t level = 3;
uint16_t counter = 0;



int main(void)
{

	//initialize i2c, gpio, and uart peripherals
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_I2C1_Init();
	srand(time(0));

	*sentData = 1;
	sendData();

	//WAIT FOR STM32 ON-BOARD BUTTON PRESS TO START GAME
	while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13));

	//game loop
	while (1)
	{
		//START PATTERN GENERATION

		//send level to arduino to display on 7 Segment
		*sentData = level;
		sendData();

		//light LED's and generate pattern
		for (uint16_t i = 0; i < level; ++i)
		{
			led_pattern_gen();
		}

		for (uint16_t i = 0; i < level; ++i)
		{
			bttn_pattern[i] = button_matching();
			HAL_Delay(250);
		}

		//if an error was found
		if (!pattern_check())
			break;

		//if an error wasn't found
		next_level();
		HAL_Delay(3000);
	}

}

void led_pattern_gen()
{
	uint8_t led = (rand() % (2 - 0 + 1)) + 0;

	HAL_Delay(500);
	HAL_GPIO_TogglePin(GPIOC, led_arr[led]);
	HAL_Delay(500);
	HAL_GPIO_TogglePin(GPIOC, led_arr[led]);

	led_pattern[counter] = led;
	counter++;
}

uint8_t button_matching()
{
	while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))
	{
		if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))
		{
			return 0;
		}
		else if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14))
		{
			return 1;
		}
		else if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13))
		{
			return 2;
		}
	}
}

bool pattern_check()
{
	for (int i = 0; i < level; ++i)
	{
		if (led_pattern[i] != bttn_pattern[i])
			return false;
	}

	return true;
}

void next_level()
{
	//reset user button and led patterns
	for (int i = 0; i < level; ++i)
	{
		led_pattern[i] = 0;
		bttn_pattern[i] = 0;
	}

	level++;
	counter = 0;
}

void sendData()
{
	//wait for i2c data to be sent
	while(HAL_I2C_Master_Transmit(&hi2c1, ARDUINO_ADDRESS, STM32DataBuffer, 1, 100) != HAL_OK);
}

void receiveData()
{
	//wait until some i2c data is received by the arduino
	while(HAL_I2C_Master_Receive(&hi2c1, ARDUINO_ADDRESS , ArduinoDataBuffer, 50, 100) != HAL_OK);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC5 PC6 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

void Error_Handler(void)
{

  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}

#endif /* USE_FULL_ASSERT */
