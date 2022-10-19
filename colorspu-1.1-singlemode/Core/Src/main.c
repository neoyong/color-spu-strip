/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
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
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int sys_state = 0;

uint32_t huecolor(float hue) {
    int amp = 256;
    int normalized = (int) (hue / 360.0 * amp * 6);

    //find the region for this position
    int region = normalized / amp;

    //find the distance to the start of the closest region
    int x = normalized % amp;

    uint8_t r = 0, g = 0, b = 0;
    switch (region) {
        case 0:
            r = amp - 1;
            g = 0;
            b = 0;
            g += x;
            break;
        case 1:
            r = amp - 1;
            g = amp - 1;
            b = 0;
            r -= x;
            break;
        case 2:
            r = 0;
            g = amp - 1;
            b = 0;
            b += x;
            break;
        case 3:
            r = 0;
            g = amp - 1;
            b = amp - 1;
            g -= x;
            break;
        case 4:
            r = 0;
            g = 0;
            b = amp - 1;
            r += x;
            break;
        case 5:
            r = amp - 1;
            g = 0;
            b = amp - 1;
            b -= x;
            break;
    }
    return r + (g << 8) + (b << 16);
}

#define MAX_LED 17
#define PDM_SAMPLES 512

uint8_t LED_Data[MAX_LED][4];

void Set_LED(uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue) {

    LED_Data[LEDnum][0] = LEDnum;
    LED_Data[LEDnum][1] = Green;
    LED_Data[LEDnum][2] = Red;
    LED_Data[LEDnum][3] = Blue;
}

volatile uint8_t datasentflag = 0;

uint16_t pwmData[(24 * MAX_LED) + 50];

void WS2812_Send(void) {
    uint32_t indx = 0;
    uint32_t color;
    for (int i = 0; i < 50; i++) {
        pwmData[indx] = 0;
        indx++;
    }
    for (int i = 0; i < MAX_LED; i++) {
        color = ((LED_Data[i][1] << 16) | (LED_Data[i][2] << 8) | (LED_Data[i][3]));

        for (int i = 23; i >= 0; i--) {
            if ((color >> i) & 1) {
                pwmData[indx] = 53;  // 2/3 of 20
            } else pwmData[indx] = 27;  // 1/3 of 20

            indx++;
        }

    }


    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *) pwmData, indx);
    while (!datasentflag) {};
    datasentflag = 0;
}

int pdm2amp(const uint8_t *r, int s, int e) {
    int max_amp = 0;
    int amp = 0;
    for (int i = s; i < e; i++) {
        for (int j = 0; j < 8; j++) {
            if (((r[i] >> j) & 1) == 1) {
                amp++;
            }
        }
    }
    int dest = (amp - (e - s) * 8 / 2);
    if (dest < 0)dest = -dest;
    if (dest > MAX_LED)dest = MAX_LED;

    if (dest > max_amp) {
        max_amp = dest;
    }
    return max_amp;
}

int pdm2amp_rec(uint8_t *buf, int size, int step) {
    if (size > PDM_SAMPLES)size = PDM_SAMPLES;
    if (step > PDM_SAMPLES)step = PDM_SAMPLES;
    int amp = 0;
    for (int i = 0; i < PDM_SAMPLES - size; i += step) {
        int amp_t = pdm2amp(buf, i, i + size);
        if (amp_t > amp)amp = amp_t;
    }
    return amp;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
    datasentflag = 1;
}

uint8_t r[PDM_SAMPLES] = {0};

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {

}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {

}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
    sys_state = 1 - sys_state;
}


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

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_DMA_Init();
    MX_TIM1_Init();
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    HAL_SPI_DeInit(&hspi1);
    HAL_SPI_Init(&hspi1);
    HAL_SPI_Receive_DMA(&hspi1, (uint8_t *) r, PDM_SAMPLES);
//    int counter = 0;

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *) pwmData, (24 * MAX_LED) + 50);
    int max_amp = 0;


    while (1) {

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        //  DEPRECATED: non-precise frequency band matching
        //      volatile int amp1=0;
        //      volatile int amp2=0;
        //      volatile int amp3=0;
        //      volatile int amp4=0;
        //        for(int i=0;i<400-1;i++){
        //            int amp1_t=pdm2amp(r,PDM_SAMPLES/400*i,PDM_SAMPLES/400*(i+1));
        //            if(amp1_t>amp1)amp1=amp1_t;
        //        }
        //          for(int i=0;i<64-1;i++){
        //              int amp2_t=pdm2amp(r,PDM_SAMPLES/64*i,PDM_SAMPLES/64*(i+1));
        //              if(amp2_t>amp2)amp2=amp2_t;
        //          }
        //      for(int i=0;i<16-1;i++){
        //          int amp3_t=pdm2amp(r,PDM_SAMPLES/16*i,PDM_SAMPLES/16*(i+1));
        //          if(amp3_t>amp3)amp3=amp3_t;
        //      }
        //      for(int i=0;i<4-1;i++){
        //          int amp4_t=pdm2amp(r,PDM_SAMPLES/4*i,PDM_SAMPLES/4*(i+1));
        //          if(amp4_t>amp4)amp4=amp4_t;
        //      }
        //      volatile int amp5=pdm2amp(r,0,PDM_SAMPLES);
        //      volatile int amp=0;

        if (sys_state == 0) {
            int amp = 0;

            int amp1 = pdm2amp_rec(r, 12, 100);
            amp = amp > amp1 ? amp : amp1;

            int amp2 = pdm2amp_rec(r, 12 * 10, 100);
            amp = amp > amp2 ? amp : amp2;

            int amp3 = pdm2amp_rec(r, 12 * 20, 100);
            amp = amp > amp3 ? amp : amp3;

            int amp4 = pdm2amp_rec(r, 12 * 30, 100);
            amp = amp > amp4 ? amp : amp4;

            int amp5 = pdm2amp_rec(r, 12 * 40, 100);
            amp = amp > amp5 ? amp : amp5;

            //  DEPRECATED: non-precise frequency band matching
            //      int amp1= pdm2amp_rec(r,4,256);
            //      int amp2= pdm2amp_rec(r,8,64);
            //      int amp3= pdm2amp_rec(r,16,32);
            //      int amp4= pdm2amp_rec(r,64,16);
            //      int amp5= pdm2amp_rec(r,256,4);


            max_amp = amp;
            float color_step = 0;
            for (int i = 0; i < max_amp; i++) {
                uint32_t color = huecolor(color_step);
                float r_s = ((color >> 8) & 0xff) / 80.0;
                float g_s = (color & 0xff) / 80;
                float b_s = ((color >> 16) & 0xff) / 80;
                Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                color_step += (amp1 * 2 + amp2 + amp3 / 2.0 + amp4 / 4.0 + amp5 / 8.0) / 2;
            }
            for (int i = max_amp; i < MAX_LED; i++) {
                Set_LED(i, 0, 0, 0);
            }
            WS2812_Send();
        } else {
            for (int i = 0; i < MAX_LED; i++) {
                Set_LED(i, 0, 0, 0);
            }
            Set_LED(MAX_LED - 1, 0, 0, 1);
            WS2812_Send();
        }
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
    }

    /* HSI configuration and activation */
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1) {
    }

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_Enable();
    LL_RCC_PLL_EnableDomain_SYS();
    while (LL_RCC_PLL_IsReady() != 1) {
    }

    /* Set AHB prescaler*/
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    /* Sysclk activation on the main PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }

    /* Set APB1 prescaler*/
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(64000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
