/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	float acceleration;
	volatile unsigned int minStepInterval;
	void (*dirFunc)(int);
	void (*stepFunc)();

	uint8_t homing;
	unsigned int c0;
	volatile int stepPosition;

	volatile int dir; // 1 or -1
	volatile int dir_inv; //set 1 to reverse the dir

	volatile int pull_off; //set pull_off distance

	volatile int seeking_vel;
	volatile int homing_vel;

	volatile unsigned int totalSteps;

	volatile char movementDone;
	volatile unsigned int rampUpStepCount;
	volatile unsigned int n; // index
	volatile float d; // current interval length
	volatile unsigned long di; // d -> unsigned  di
	volatile unsigned int stepCount;
}stepperInfo;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_STEPPERS 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
volatile stepperInfo steppers[NUM_STEPPERS];
uint8_t end_stop_state = 0;
uint8_t homing_flag = 0;
uint8_t timer1_busy = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	//stepper 0 end stop senssor
	if(GPIO_Pin == GPIO_PIN_9 ){
		if(!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9)){
			end_stop_state |= 1;
		}
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9)){
			end_stop_state &= ~1;
		}
	}
}

void Step0(){
	//pull+
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);

	//pull-
	//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);

}
void Dir0(int dir){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, dir);
}

void resetStepperInfo(stepperInfo * si){
	si->n = 0;
	si->d = 0;
	si->di = 0;
	si->stepCount = 0;
	si->rampUpStepCount = 0;
	si->totalSteps = 0;
	si->stepPosition = 0;
	si->movementDone = 0;
}


void resetStepper(volatile stepperInfo* si){
	si->c0 = si->acceleration;
	si->d = si->c0;
	si->di = si->d;
	si->stepCount = 0;
	si->n = 0;
	si->rampUpStepCount = 0;
	si->movementDone = 0;
}

volatile uint8_t remainingSteppersFlag = 0;

void prepareMovement(int whichMotor, int steps){
	if(steps == 0){
		return;
	}
	volatile stepperInfo* si = &steppers[whichMotor];
	if(si->dir_inv){
		si->dirFunc( steps < 0 ? 0 : 1);
		si->dir = steps > 0 ? -1:1;
	}else{
		si->dirFunc( steps < 0 ? 1 : 0);
		si->dir = steps > 0 ? 1:-1;
	}

	si->totalSteps = abs(steps);
	resetStepper(si);
	remainingSteppersFlag |= (1 << whichMotor);
}

void prepareAbsoluteMovement(int whichMotor, int absolute_steps){
	volatile stepperInfo* si = &steppers[whichMotor];
	int steps = absolute_steps - si->stepPosition;
	if(steps == 0){
		return;
	}
	si->dirFunc( steps < 0 ? 1 : 0);
	si->dir = steps > 0 ? 1:-1;
	si->totalSteps = abs(steps);
	resetStepper(si);
	remainingSteppersFlag |= (1 << whichMotor);
}

volatile uint8_t nextStepperFlag = 0;

void setNextInterruptInterval(){

	unsigned int mind = 65535;
	for (int i = 0; i < NUM_STEPPERS; i++){
		if( ((1 << i) & remainingSteppersFlag) && steppers[i].di < mind ){
			mind = steppers[i].di;
		}
	}

	nextStepperFlag = 0;
	for (int i = 0; i < NUM_STEPPERS; i ++){
		if( ((1 << i) & remainingSteppersFlag) && steppers[i].di == mind )
			nextStepperFlag |= (1 << i);
	}

	if (remainingSteppersFlag == 0){
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 65500);
	}

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, mind);
	//__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 100);

}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){

	HAL_TIM_OC_Stop_IT(&htim1, TIM_CHANNEL_1);
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
	unsigned int tmpCtr = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
	//__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 65500);

	for (int i = 0; i < NUM_STEPPERS; i++){
		if ( ! ( (1 << i) & remainingSteppersFlag )){
			continue;
		}

		if( homing_flag & (1 << i)){
			if(end_stop_state & (1 << i)){
				remainingSteppersFlag &= ~(1 << i);
				continue;
			}
		}

		if ( ! (nextStepperFlag & (1 << i)) ){
			steppers[i].di -= tmpCtr;
			continue;
		}

		volatile stepperInfo* s = &steppers[i];



		if( (s->stepCount) < (s->totalSteps) ){
			s->stepFunc();
			s->stepCount++;
			s->stepPosition += s->dir;
			if ( (s->stepCount) >= (s->totalSteps) ){
				s->movementDone = 1;
				remainingSteppersFlag &= ~(1 << i);
			}
		}

		if (s->rampUpStepCount == 0){
			s->n++;
			s->d = s->d - (2*s->d) / (4*s->n +1);
			if (s->d <= s->minStepInterval ){
				s->d = s->minStepInterval;
				s->rampUpStepCount = s->stepCount;
			}
			if (s->stepCount >= (s->totalSteps / 2) ){
				s->rampUpStepCount = s->stepCount;
			}
		} else if ( s->stepCount >= s->totalSteps - s->rampUpStepCount) {
			s->d = (s->d * (4 * s->n + 1)) / (4 * s->n + 1 -2);
			s->n--;
		}
		s->di = s->d;
	}


	setNextInterruptInterval();
	//__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 100);
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	HAL_TIM_OC_Start_IT(&htim1, TIM_CHANNEL_1);



}

void sensorStateInit(){
	if( ! HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9)){
		end_stop_state |= 1;
	}
}

void runAndWait(){
	setNextInterruptInterval();
	while(remainingSteppersFlag);
}

void stepperHoming(int whichMotor){
	homing_flag |= (1 << whichMotor);
	//seeking
	steppers[whichMotor].minStepInterval = steppers[whichMotor].seeking_vel;
	prepareMovement(whichMotor, -100000000);
	runAndWait();
	HAL_Delay(500);
	//pull-off

	homing_flag &= ~(1 << whichMotor);
	prepareMovement(whichMotor, steppers[whichMotor].pull_off);
	runAndWait();
	HAL_Delay(500);
	//homing

	homing_flag |= (1 << whichMotor);
	steppers[whichMotor].minStepInterval = steppers[whichMotor].homing_vel;
	prepareMovement(whichMotor, -100000000);
	runAndWait();
	HAL_Delay(500);
	//pull-off
	homing_flag &= ~(1 << whichMotor);
	prepareMovement(whichMotor, steppers[whichMotor].pull_off);
	runAndWait();
	HAL_Delay(500);

	steppers[whichMotor].stepPosition = 0;
	steppers[whichMotor].homing = 1;
	steppers[whichMotor].minStepInterval = 100;
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  steppers[0].dirFunc = Dir0;
  steppers[0].stepFunc = Step0;
  steppers[0].acceleration = 10;
  steppers[0].minStepInterval = 10000;
  steppers[0].homing = 0;
  steppers[0].dir_inv = 1;
  steppers[0].seeking_vel = 100;
  steppers[0].homing_vel = 200;
  steppers[0].pull_off = 500;

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  HAL_TIM_OC_Start_IT(&htim1, TIM_CHANNEL_1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  prepareAbsoluteMovement(0, 10000);
	  runAndWait();
	  HAL_Delay(100);
	  prepareAbsoluteMovement(0, -10000);
	  runAndWait();
	  HAL_Delay(100);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 1920-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 10;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC10 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
