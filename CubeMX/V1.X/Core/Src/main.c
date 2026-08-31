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
#include "connect-4-board.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MODER_REG_INPUT   0x00000000U  // [1:0] = 00 == Input
#define MODER_REG_OUTPUT  0x55555555U  // [1:0] = 01 == Output

// These represent GPIO pin numbers in port B, which could change between hardware versions
#define ROW_6_POS         0U
#define ROW_5_POS         1U
#define ROW_4_POS         2U
#define ROW_3_POS         3U
#define ROW_2_POS         4U
#define ROW_1_POS         5U

#define COL_1_POS         6U
#define COL_2_POS         7U
#define COL_3_POS         8U
#define COL_4_POS         9U
#define COL_5_POS         10U
#define COL_6_POS         11U
#define COL_7_POS         12U

// ODR bits (16 bit reg)
// Bit operations are taken care of by compiler, so at runtime it will be a constant
#define ROW_6_ODR         ((uint16_t)(1U << ROW_6_POS))

#define COL_1_ODR         ((uint16_t)(1U << COL_1_POS))

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

/* USER CODE BEGIN PV */

volatile uint8_t player_button_input = 0U; // Stores pressed button ID (1-7). 0 means no input.
volatile bool input_received = false;      // Flag to signal the game loop

volatile uint32_t gpiob_pin_modes[42] = {
    (ROW_6_MODER | COL_1_MODER) & MODER_REG_OUTPUT, // Bitboard 0 (Bottom Left)
    (ROW_5_MODER | COL_1_MODER) & MODER_REG_OUTPUT, // Bitboard 1
    (ROW_4_MODER | COL_1_MODER) & MODER_REG_OUTPUT, // Bitboard 2
    (ROW_3_MODER | COL_1_MODER) & MODER_REG_OUTPUT, // etc
    (ROW_2_MODER | COL_1_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_1_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_2_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_2_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_3_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_3_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_4_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_4_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_5_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_5_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_6_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_6_MODER) & MODER_REG_OUTPUT,

    (ROW_6_MODER | COL_7_MODER) & MODER_REG_OUTPUT,
    (ROW_5_MODER | COL_7_MODER) & MODER_REG_OUTPUT,
    (ROW_4_MODER | COL_7_MODER) & MODER_REG_OUTPUT,
    (ROW_3_MODER | COL_7_MODER) & MODER_REG_OUTPUT,
    (ROW_2_MODER | COL_7_MODER) & MODER_REG_OUTPUT,
    (ROW_1_MODER | COL_7_MODER) & MODER_REG_OUTPUT
};

volatile uint16_t gpiob_output_data[42] = {0U};  // Cast to uint32_t with 0-extension before writing into reg

uint64_t game_board = 0U;
uint64_t p1_moves = 0U;
uint8_t move_num = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

static void unified_button_handler(uint32_t pending_register) {
    
    EXTI->RPR1 = pending_register;
  
    if ((!input_received) && (pending_register != 0U)) {
        // Find interrupt source by counting trailing zeros to get lowest bit index
        player_button_input = (uint8_t)__builtin_ctz(pending_register);
        input_received = true;
    }
}

// Expected function signatures for interrupt callbacks
void EXTI0_1_IRQHandler(void) {
    unified_button_handler(EXTI->RPR1);
}

void EXTI2_3_IRQHandler(void) {
    unified_button_handler(EXTI->RPR1);
}

void EXTI4_15_IRQHandler(void) {
    unified_button_handler(EXTI->RPR1);
}

uint8_t bitboard_to_buffer_index(uint8_t bitboard_index) {
    return bitboard_index - (bitboard_index / 7);
}

// Leverage existing macros and shift relative to their positions
// Change operations to /6 and %6 if using buffer index
uint16_t get_row_odr(uint8_t bitboard_index) {
    // Bitboard index % 7 gives row 0 (bottom) to 5 (top)
    return (uint16_t)(ROW_6_ODR << (bitboard_index % 7));
}

uint16_t get_col_odr(uint8_t bitboard_index) {
    // Bitboard index / 7 gives col 0 to 6
    return (uint16_t)(COL_1_ODR << (bitboard_index / 7));
}

void set_color_row(uint8_t bitboard_position) {
    gpiob_output_data[bitboard_to_buffer_index(bitboard_position)] = get_row_odr(bitboard_position);
}

void set_color_col(uint8_t bitboard_position) {
    gpiob_output_data[bitboard_to_buffer_index(bitboard_position)] = get_col_odr(bitboard_position);
}

void end_player_win(void) {

}

void end_ai_win(void) {

}

void clear_board(void) {
    for (uint8_t i = 0U; i < 42U; ++i) {
        gpiob_output_data[i] = 0U;
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  // Power Supply Voltage Monitoring Configuration
  if ((FLASH->OPTR & FLASH_OPTR_BOR_LEV) != OB_BOR_LEVEL_1) {
      FLASH_OBProgramInitTypeDef OptionsBytesStruct = {0};

      HAL_FLASH_Unlock();
      HAL_FLASH_OB_Unlock();

      // Hardcoded payload for BOR Level 1
      OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
      OptionsBytesStruct.USERType   = OB_USER_BOR_LEV;
      OptionsBytesStruct.USERConfig = OB_BOR_LEVEL_1;

      // Program and reload option bytes
      if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) == HAL_OK) {
          // Reboots device with new 2.2V threshold
          HAL_FLASH_OB_Launch(); 
      }

      // Option bytes programming has failed
      // Fallback safety locks
      HAL_FLASH_OB_Lock();
      HAL_FLASH_Unlock();

      // TODO: Implement error handling
  }

  // Enable BOR/PVD Periodic Sampling
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // DMA channel driving GPIOB ODR
  hdma_tim2_ch1.Instance = DMA1_Channel1;
  hdma_tim2_ch1.Init.Request = DMA_REQUEST_TIM2_UP;
  hdma_tim2_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_tim2_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_tim2_ch1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_tim2_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim2_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_tim2_ch1.Init.Mode = DMA_CIRCULAR;
  hdma_tim2_ch1.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma_tim2_ch1) != HAL_OK) { 
      Error_Handler(); 
  }

  // DMA channel driving GPIOB MODER
  hdma_tim2_ch2.Instance = DMA1_Channel2;
  hdma_tim2_ch2.Init.Request = DMA_REQUEST_TIM2_UP;
  hdma_tim2_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_tim2_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_tim2_ch2.Init.MemInc = DMA_MINC_ENABLE;
  hdma_tim2_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_tim2_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_tim2_ch2.Init.Mode = DMA_CIRCULAR;
  hdma_tim2_ch2.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma_tim2_ch2) != HAL_OK) { 
      Error_Handler();
  }

  // Point DMA to screen buffers and the respective PortB registers
  HAL_DMA_Start(&hdma_tim2_ch2, (uint32_t)gpiob_pin_modes, (uint32_t)&(GPIOB->MODER), 42);
  HAL_DMA_Start(&hdma_tim2_ch1, (uint32_t)gpiob_output_data, (uint32_t)&(GPIOB->ODR), 42);

  // Enable the Timer to trigger DMA updates
  __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_UPDATE);
    
  HAL_TIM_Base_Start(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    while (move_num < 42)
    {
      if (move_num % 2 == 0)
      {
        uint8_t column;
        int location;

        for (;;) {
          player_button_input = 0;
          input_received = false;

          while (!input_received) {
            __WFI(); // TODO: Give AI additional computation time during wait
          }

          if (player_button_input < 1 || player_button_input > 7) {
            continue;
          }
          column = player_button_input - 1;

          if (is_playable(game_board, column)) {
            break;
          }
        }

        location = play_move(&game_board, column);
        if (location < 0) {
          break;
        }

        set_color_row(location);

        p1_moves |= (1ULL << location);
        move_num++;

        if (is_won(p1_moves)) {
          break;
        }
      }
      else
      {
        int bot_column = choose_best_ai_move(game_board, p1_moves);
        if (bot_column < 0) {
          break;
        }

        int location = play_move(&game_board, (unsigned char)bot_column);
        if (location < 0) {
          break;
        }

        set_color_col(location);

        move_num++;

        if (is_won(game_board ^ p1_moves)) {
          break;
        }
      }
    }

    if (is_won(p1_moves)) {
      end_player_win();
    }
    else if (is_won(game_board ^ p1_moves)) {
      end_ai_win();
    }

    game_board = 0;
    p1_moves = 0;
    move_num = 0;

    clear_board();

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 124;
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

  // Must add manually since CubeMX sees no GPIOB pins used
  __HAL_RCC_GPIOB_CLK_ENABLE();

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
