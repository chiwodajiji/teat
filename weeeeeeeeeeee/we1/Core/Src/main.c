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
#include <stdio.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim2;

//* USER CODE BEGIN PV */
volatile uint8_t pause_flag = 0;       // 暫停旗標 (0: 執行, 1: 暫停)

// UART 接收相關變數
uint8_t lpuart_rx_data;                // 單字元接收緩衝
char rx_buffer[30];                    // 儲存完整指令字串
uint8_t rx_index = 0;

uint32_t countdown_time = 0;           // 設定的倒數時間（秒數）
uint8_t state = 0;                     // 0: 等待設定, 1: 就緒/暫停, 2: 執行中
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // 1. 啟動三個 PWM 通道 (TIM2 CH1: PA0, CH2: PA1, CH3: PB10)
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

  // 2. 啟動 LPUART1 的中斷接收（每次收 1 個 byte）
  HAL_UART_Receive_IT(&hlpuart1, &lpuart_rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
        // 狀態 0 或 1：等待接收指令或暫停狀態，燈保持全暗或不動
        if (state != 2) {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
            HAL_Delay(100);
            continue;
        }

        // 狀態 2：收到 START 後開始執行依序變暗邏輯（精準校正時間）

              uint32_t total_time_ms = countdown_time * 60 * 1000; // 例如 1 分鐘 = 60,000 ms

              // 總共有 600 步 (3顆燈 * 各200步)
              // 為了補償 HAL_Delay 的累積誤差，我們將總時間稍微乘以 0.9 左右來校正
              uint32_t calibrated_time_ms = (total_time_ms * 88) / 100;
              uint32_t step_delay = calibrated_time_ms / 600;
              if (step_delay < 1) step_delay = 1;

              // 1. 三顆燈瞬間全亮（取消額外的 hold_time，讓時間完全用在變暗過程）
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 999);
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 999);
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 999);

              // 2. 第一顆燈 (PA0) 漸漸變暗
              for (int32_t i = 999; i >= 0; i -= 5) {
                  while (pause_flag) { while(pause_flag); } // 支援隨時暫停
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, i);

                  for(uint32_t sd = 0; sd < step_delay; sd += 5) {
                      while (pause_flag) { while(pause_flag); }
                      HAL_Delay(5);
                  }
                  if (i == 0) break;
              }

              // 3. 第二顆燈 (PA1) 漸漸變暗
              for (int32_t i = 999; i >= 0; i -= 5) {
                  while (pause_flag) { while(pause_flag); }
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, i);

                  for(uint32_t sd = 0; sd < step_delay; sd += 5) {
                      while (pause_flag) { while(pause_flag); }
                      HAL_Delay(5);
                  }
                  if (i == 0) break;
              }

              // 4. 第三顆燈 (PB10) 漸漸變暗
              for (int32_t i = 999; i >= 0; i -= 5) {
                  while (pause_flag) { while(pause_flag); }
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, i);

                  for(uint32_t sd = 0; sd < step_delay; sd += 5) {
                      while (pause_flag) { while(pause_flag); }
                      HAL_Delay(5);
                  }
                  if (i == 0) break;
              }

              // 確保三顆燈完全熄滅
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);

              // 透過 UART 傳送時間結束訊息
              char finish_msg[] = "TIME_UP\r\n";
              HAL_UART_Transmit(&hlpuart1, (uint8_t*)finish_msg, sizeof(finish_msg) - 1, 100);

              // 回到等待狀態
              state = 0;
              countdown_time = 0;
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

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

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 169;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// 1. 外部按鈕中斷 (PC9) - 附帶 200ms 軟體消抖
uint32_t last_click_tick = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_9) {
        uint32_t current_tick = HAL_GetTick();
        if (current_tick - last_click_tick > 200) {
            pause_flag = !pause_flag; // 切換暫停 / 繼續
            last_click_tick = current_tick;
        }
    }
}

// 2. LPUART1 接收完成中斷 - 解析 "SET 數字" 與 "START"
// 2. LPUART1 接收完成中斷 - 穩定的字元收集與解析
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1) {
        // 如果收到換行或結尾，或是緩衝區滿了
        if (lpuart_rx_data == '\n' || lpuart_rx_data == '\r') {
            if (rx_index > 0) { // 確保不是空的換行
                rx_buffer[rx_index] = '\0'; // 加上字串結尾

                // 測試回傳：把收到的完整指令印回電腦畫面
                char debug_msg[50];
                int len = sprintf(debug_msg, "收到指令: [%s]\r\n", rx_buffer);
                HAL_UART_Transmit(&hlpuart1, (uint8_t*)debug_msg, len, 100);

                uint32_t temp_val = 0;
                if (sscanf(rx_buffer, "SET %lu", &temp_val) == 1) {
                    countdown_time = temp_val;
                    state = 1; // 進入就緒狀態
                }
                else if (strcmp(rx_buffer, "START") == 0) {
                    if (countdown_time > 0) {
                        state = 2; // 開始執行依序變暗
                    }
                }
                rx_index = 0; // 重置索引準備收下一句
            }
        }
        else {
            // 正常收集字元（過濾掉空白或控制字元以外的字）
            if (lpuart_rx_data >= 32 && lpuart_rx_data <= 126) {
                if (rx_index < sizeof(rx_buffer) - 1) {
                    rx_buffer[rx_index++] = lpuart_rx_data;
                }
            }
        }

        // 【極重要】不管怎樣，一定要重新啟動中斷接收下一個字元！
        HAL_UART_Receive_IT(&hlpuart1, &lpuart_rx_data, 1);
    }
}
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
