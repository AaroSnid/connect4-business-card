/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MODER_REG_INPUT   0x00000000  // [1:0] = 00 == Input
#define MODER_REG_OUTPUT  0x55555555  // [1:0] = 01 == Output

// These represent GPIO pin numbers in port B, which could change between hardware versions
#define ROW_6_POS         0
#define ROW_5_POS         1
#define ROW_4_POS         2
#define ROW_3_POS         3
#define ROW_2_POS         4
#define ROW_1_POS         5

#define COL_1_POS         6
#define COL_2_POS         7
#define COL_3_POS         8
#define COL_4_POS         9
#define COL_5_POS         10
#define COL_6_POS         11
#define COL_7_POS         12

// ODR bits (16 bit reg)
// Bit operations are taken care of by compiler, so at runtime it will be a constant
#define ROW_6_ODR         ((uint16_t)(1U << ROW_6_POS))
#define ROW_5_ODR         ((uint16_t)(1U << ROW_5_POS))
#define ROW_4_ODR         ((uint16_t)(1U << ROW_4_POS))
#define ROW_3_ODR         ((uint16_t)(1U << ROW_3_POS))
#define ROW_2_ODR         ((uint16_t)(1U << ROW_2_POS))
#define ROW_1_ODR         ((uint16_t)(1U << ROW_1_POS))

#define COL_1_ODR         ((uint16_t)(1U << COL_1_POS))
#define COL_2_ODR         ((uint16_t)(1U << COL_2_POS))
#define COL_3_ODR         ((uint16_t)(1U << COL_3_POS))
#define COL_4_ODR         ((uint16_t)(1U << COL_4_POS))
#define COL_5_ODR         ((uint16_t)(1U << COL_5_POS))
#define COL_6_ODR         ((uint16_t)(1U << COL_6_POS))
#define COL_7_ODR         ((uint16_t)(1U << COL_7_POS))

// MODER bits (32 bit reg)
#define ROW_6_MODER       ((uint32_t)(0x3UL << (ROW_6_POS * 2)))
#define ROW_5_MODER       ((uint32_t)(0x3UL << (ROW_5_POS * 2)))
#define ROW_4_MODER       ((uint32_t)(0x3UL << (ROW_4_POS * 2)))
#define ROW_3_MODER       ((uint32_t)(0x3UL << (ROW_3_POS * 2)))
#define ROW_2_MODER       ((uint32_t)(0x3UL << (ROW_2_POS * 2)))
#define ROW_1_MODER       ((uint32_t)(0x3UL << (ROW_1_POS * 2)))

#define COL_1_MODER       ((uint32_t)(0x3UL << (COL_1_POS * 2)))
#define COL_2_MODER       ((uint32_t)(0x3UL << (COL_2_POS * 2)))
#define COL_3_MODER       ((uint32_t)(0x3UL << (COL_3_POS * 2)))
#define COL_4_MODER       ((uint32_t)(0x3UL << (COL_4_POS * 2)))
#define COL_5_MODER       ((uint32_t)(0x3UL << (COL_5_POS * 2)))
#define COL_6_MODER       ((uint32_t)(0x3UL << (COL_6_POS * 2)))
#define COL_7_MODER       ((uint32_t)(0x3UL << (COL_7_POS * 2)))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define EXTI_LINES_1_7_MASK    (EXTI_IMR1_IM1 | EXTI_IMR1_IM2 | EXTI_IMR1_IM3 | \
                                EXTI_IMR1_IM4 | EXTI_IMR1_IM5 | EXTI_IMR1_IM6 | \
                                EXTI_IMR1_IM7)

#define EXTI_LINES_1_7_PENDING (EXTI_RPR1_RPIF1 | EXTI_RPR1_RPIF2 | EXTI_RPR1_RPIF3 | \
                                EXTI_RPR1_RPIF4 | EXTI_RPR1_RPIF5 | EXTI_RPR1_RPIF6 | \
                                EXTI_RPR1_RPIF7)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim2_ch1;
DMA_HandleTypeDef hdma_tim2_ch2;

PCD_HandleTypeDef hpcd_USB_DRD_FS;

/* USER CODE BEGIN PV */

volatile uint8_t player_button_input = 0; // Stores pressed button ID (1-7). 0 means no input.
volatile bool input_received = false;      // Flag to signal the game loop

volatile uint32_t gpiob_pin_modes[42] = {
  (ROW_1_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_1_MODER | COL_7_MODER) & MODER_REG_OUTPUT,

  (ROW_2_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_2_MODER | COL_7_MODER) & MODER_REG_OUTPUT,

  (ROW_3_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_3_MODER | COL_7_MODER) & MODER_REG_OUTPUT,

  (ROW_4_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_4_MODER | COL_7_MODER) & MODER_REG_OUTPUT,

  (ROW_5_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_5_MODER | COL_7_MODER) & MODER_REG_OUTPUT,

  (ROW_6_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
  (ROW_6_MODER | COL_7_MODER) & MODER_REG_OUTPUT
};

volatile uint16_t gpiob_output_data[42] = {0};  // Cast to uint32_t with 0-extension before writing into reg

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

// Clear leftover pending bits and enable interrupts lines [1:7] 
void TurnControl_EXTI_StartPlayerTurn(void) {
    EXTI->RPR1 = EXTI_LINES_1_7_PENDING;
    EXTI->IMR1 |= EXTI_LINES_1_7_MASK;
}

static void Unified_Button_Handler(uint32_t pending_register) {
    // Mask lines [1:7] to ignore subsequent presses and contact bounce
    EXTI->IMR1 &= ~EXTI_LINES_1_7_MASK;

    uint32_t valid_buttons = pending_register & EXTI_LINES_1_7_MASK;

    if (valid_buttons != 0) {
        // Find interrupt number by counting trailing zeros to get lowest bit index
        player_button_input = (uint8_t)__builtin_ctz(valid_buttons);
        input_received = true;
    }
}

void EXTI0_1_IRQHandler(void) {
    Unified_Button_Handler(EXTI->RPR1);
}

void EXTI2_3_IRQHandler(void) {
    Unified_Button_Handler(EXTI->RPR1);
}

void EXTI4_15_IRQHandler(void) {
    Unified_Button_Handler(EXTI->RPR1);
}

inline void set_odr_color(uint8_t bit_position, uint16_t forward_enable, uint16_t backwards_enable){
    gpiob_output_data[bit_position] = (forward_enable | backwards_enable);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* ---------------- Power Supply Voltage Monitoring Configuration ---------------- */
  if ((FLASH->OPTR & FLASH_OPTR_BOR_LEV) != OB_BOR_LEVEL_1)
  {
      FLASH_OBProgramInitTypeDef OptionsBytesStruct = {0};

      HAL_FLASH_Unlock();
      HAL_FLASH_OB_Unlock();

      // Hardcoded payload for BOR Level 1
      OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
      OptionsBytesStruct.USERType   = OB_USER_BOR_LEV;
      OptionsBytesStruct.USERConfig = OB_BOR_LEVEL_1;

      // 4. Program and reload the option bytes
      if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) == HAL_OK)
      {
          // Reboots device with new 2.2V threshold
          HAL_FLASH_OB_Launch(); 
      }

      // Option bytes programming has failed
      // Fallback safety locks
      HAL_FLASH_OB_Lock();
      HAL_FLASH_Unlock();

      // TODO: Implement error handling
  }

  /* ----------- Enable BOR/PVD Periodic Sampling ----------- */
  // Enable Power interface clock
  SET_BIT(RCC->APBENR1, RCC_APBENR1_PWREN);
    
  // Enable ultra-low-power sampling
  SET_BIT(PWR->CR3, PWR_CR3_ENULP);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_PCD_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // Point DMA to screen buffers and the respective PortB registers
  HAL_DMA_Start(&hdma_tim2_ch2, (uint32_t)gpiob_pin_modes, (uint32_t)&(GPIOB->MODER), 42);
  HAL_DMA_Start(&hdma_tim2_ch1, (uint32_t)gpiob_output_data, (uint32_t)&(GPIOB->BSRR), 42);

  // Enable the Timer to trigger DMA updates
  __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_UPDATE);
    
  HAL_TIM_Base_Start(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* Example implementation for getting the user input
    *    player_button_input = 0;
    *    input_received = false;
    *    TurnControl_EXTI_StartPlayerTurn();
    *    
    *    while (!input_received) {
    *        __WFI(); // Wait for interrupt instruction
    *    }
    */

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 39;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 10;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_DRD_FS.Instance = USB_DRD_FS;
  hpcd_USB_DRD_FS.Init.dev_endpoints = 8;
  hpcd_USB_DRD_FS.Init.speed = USBD_FS_SPEED;
  hpcd_USB_DRD_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_DRD_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_DRD_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : INPUT_1_Pin INPUT_2_Pin INPUT_3_Pin INPUT_4_Pin
                           INPUT_5_Pin INPUT_6_Pin INPUT_7_Pin */
  GPIO_InitStruct.Pin = INPUT_1_Pin|INPUT_2_Pin|INPUT_3_Pin|INPUT_4_Pin
                          |INPUT_5_Pin|INPUT_6_Pin|INPUT_7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_INPUT_Pin */
  GPIO_InitStruct.Pin = USB_INPUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(USB_INPUT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
#ifdef USE_FULL_ASSERT
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
