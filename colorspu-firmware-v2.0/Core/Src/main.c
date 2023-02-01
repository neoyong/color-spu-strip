/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * This is the final firmware for Color SPU v1.1r1
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
double color_speed=0.02;
double brightness=256;
double brightness_dest=40;

unsigned long long sleep_counter=0;
unsigned long long key_counter=0;

int key_pending=0;
int last_sys_state=0;


#define MAX_LED 17
#define PDM_SAMPLES 512

#define SYS_DEEP_SLEEP -2
#define SYS_AUTO_SLEEP -1
#define SYS_SOUND_PICKUP2 0
#define SYS_SOUND_PICKUP1 1
#define SYS_GRADIENT2 2
#define SYS_GRADIENT1 3


#define KEY_PENDING_TIME 700
#define AUTOSLEEP_TIME 2800
#define AUTOSLEEP_THRESHOLD 4
#define AUTOWAKEUP_THRESHOLD 11

#define SYS_NORMAL_MODE_START 0
#define SYS_DEFAULT_MODE 0
#define MAX_SYS_STATE 3

#define MODE_BUTTON_PIN GPIO_PIN_5
#define COLOR_BUTTON_PIN GPIO_PIN_11
#define BRIGHTNESS_BUTTON_PIN GPIO_PIN_7

int key_pair=-1;
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
    //Button Down
    if(sys_state<SYS_NORMAL_MODE_START){
        sys_state=last_sys_state;
        if(sys_state<SYS_NORMAL_MODE_START)sys_state=SYS_DEFAULT_MODE;
        return;
    }else{
        key_pending=1;
        key_pair=GPIO_Pin;
        key_counter=HAL_GetTick();
    }
}


void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
    //Button Up

    sleep_counter=0;
    if(key_pending==0)return;
    if(key_pair!=GPIO_Pin){
        key_pair=-1;
        key_pending=0;
        return;
    }
    if(GPIO_Pin==MODE_BUTTON_PIN) {
        //MODE
        if (sys_state >= SYS_NORMAL_MODE_START) {
            if (sys_state != MAX_SYS_STATE) {
                sys_state++;
            }else{
                sys_state = SYS_DEFAULT_MODE;
            }
        }
    }else if(GPIO_Pin==BRIGHTNESS_BUTTON_PIN) {
        //BRIGHTNESS
        if (sys_state >= SYS_NORMAL_MODE_START) {
            if(brightness_dest<60)brightness_dest += 60;
            else brightness_dest += 120;
            if (brightness_dest >= 200)brightness_dest = 10;
        }
        //60 120 180
        //20 80 140 200
    }else if(GPIO_Pin==COLOR_BUTTON_PIN) {
        //COLOR
        if (sys_state >= SYS_NORMAL_MODE_START) {
            color_speed += 0.1;
            if (color_speed >= 0.3)color_speed = 0.02;

            //0.02 0.12 0.22
        }
    }
    key_pending = 0;
    key_pair=-1;
}


uint32_t huecolor(float hue) {
    if(hue>360)hue=0;
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
    return (r&0xff) + ((g&0xff) << 8) + ((b&0xff) << 16);
}




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

        for (int j = 23; j >= 0; j--) {
            if ((color >> j) & 1) {
                pwmData[indx] = 53;  // 2/3 of 20
            } else pwmData[indx] = 27;  // 1/3 of 20

            indx++;
        }

    }


    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *) pwmData, indx);
    while (!datasentflag) {};
    datasentflag = 0;
}

int abs(int a){
    if(a<0)return -a;
    return a;
}

int pdm2amp(const uint8_t *r, int s, int e) {
    int amp = 0;
    for (int i = s; i < e; i++) {
        for (int j = 0; j < 8; j++) {
            if (((r[i] >> j) & 1) == 1) {
                amp++;
            }
        }
    }
    int dest = (amp - (e - s) * 8 / 2);
    if(dest>0){
        if (abs(dest) > MAX_LED)dest = MAX_LED;
    }else{
        if (abs(dest) > MAX_LED)dest = -MAX_LED;
    }
    return dest;
}

int pdm2amp_rec(uint8_t *buf, int size, int step) {
    if (size > PDM_SAMPLES)size = PDM_SAMPLES;
    if (step > PDM_SAMPLES)step = PDM_SAMPLES;
    int amp = 0;
    for (int i = 0; i < PDM_SAMPLES - size; i += step) {
        int amp_t = pdm2amp(buf, i, i + size);
        if (abs(amp_t) > abs(amp))amp = amp_t;
    }
    return amp;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
    datasentflag = 1;
}

uint8_t r[PDM_SAMPLES] = {0};






struct AmpInfo{
    int max_amp;
    int amp1,amp2,amp3,amp4,amp5
};
struct AmpInfo calculateAmp(){
    int amp = 0;
    int amp1 = pdm2amp_rec(r, 12, 100);
    amp = abs(amp) > abs(amp1) ? amp : amp1;

    int amp2 = pdm2amp_rec(r, 12 * 10, 100);
    amp = abs(amp) > abs(amp2) ? amp : amp2;

    int amp3 = pdm2amp_rec(r, 12 * 20, 100);
    amp = abs(amp) > abs(amp3) ? amp : amp3;

    int amp4 = pdm2amp_rec(r, 12 * 30, 100);
    amp = abs(amp) > abs(amp4) ? amp : amp4;

    int amp5 = pdm2amp_rec(r, 12 * 40, 100);
    amp = abs(amp) > abs(amp5) ? amp : amp5;

    struct AmpInfo amps;
    amps.max_amp=amp;
    amps.amp1=amp1; amps.amp2=amp2; amps.amp3=amp3; amps.amp4=amp4; amps.amp5=amp5;
    return amps;
}
int sysSleep(){
    for (int i = 0; i < MAX_LED; i++) {
        Set_LED(i, 0, 0, 0);
    }
    Set_LED(MAX_LED - 1, 0, 1, 1);
    WS2812_Send();
}


int sysDeepSleep(){
    for (int i = 0; i < MAX_LED; i++) {
        Set_LED(i, 0, 0, 0);
    }
    Set_LED(MAX_LED - 1, 0, 1, 0);
    WS2812_Send();
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


    double hue0 = 0;
    double hue1 = 0;

    while (1) {

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        if(key_pending&&HAL_GetTick()-key_counter>KEY_PENDING_TIME){
            key_pending=0;
            last_sys_state=sys_state;
            sys_state=SYS_DEEP_SLEEP;
            continue;
        }

        int region0=0,region1=0;


            if(sys_state==SYS_SOUND_PICKUP1){
                if(brightness<brightness_dest)brightness+=0.6;
                if(brightness>brightness_dest)brightness-=0.6;
//// auto sleep if amp<4
                struct AmpInfo amps=calculateAmp();
                if(amps.max_amp<=AUTOSLEEP_THRESHOLD){
                    sleep_counter++;
                    if(sleep_counter>AUTOSLEEP_TIME){
                        sleep_counter=0;
                        last_sys_state=SYS_SOUND_PICKUP1;
                        sys_state = SYS_AUTO_SLEEP;
                        continue;
                    }
                }else{
                    sleep_counter=0;
                }
                region0=MAX_LED/2-abs(amps.max_amp/2);
                region1=MAX_LED/2+abs(amps.max_amp/2)+1;
                if(region0<0)region0=0;
                if(region1>MAX_LED)region1=MAX_LED;

                float color_start=(color_speed-0.02)*10*120;
                float color_step = 0+color_start;

                for (int i = 0; i < region0; i++) {
                    Set_LED(i, 0, 0, 0);
                }
                for (int i = MAX_LED/2; i >= region0; i--) {
                    uint32_t color = huecolor(color_step);
                    float r_s = ((color >> 8) & 0xff) / brightness;
                    float g_s = (color & 0xff) / brightness;
                    float b_s = ((color >> 16) & 0xff) / brightness;
                    Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                    color_step += (abs(amps.amp1) * 2
                                                    + abs(amps.amp2)
                                                    + abs(amps.amp3 / 2.0)
                                                    + abs(amps.amp4 / 4.0)
                                                    + abs(amps.amp5 / 8.0));
                }

                color_step = 15+color_start;
                for (int i = MAX_LED/2; i < region1; i++) {
                    uint32_t color = huecolor(color_step);
                    float r_s = ((color >> 8) & 0xff) / brightness;
                    float g_s = (color & 0xff) / brightness;
                    float b_s = ((color >> 16) & 0xff) / brightness;
                    Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                    color_step += (abs(amps.amp1 * 2)
                                                    + abs(amps.amp2)
                                                    + abs(amps.amp3 / 2.0)
                                                    + abs(amps.amp4 / 4.0)
                                                    + abs(amps.amp5 / 8.0));
                }
                for (int i = region1; i < MAX_LED; i++) {
                    Set_LED(i, 0, 0, 0);
                }
                WS2812_Send();

            }
            else if(sys_state==SYS_SOUND_PICKUP2) {
                if(brightness<brightness_dest)brightness+=0.6;
                if(brightness>brightness_dest)brightness-=0.6;
// auto sleep if amp<4
                struct AmpInfo amps=calculateAmp();
                if (abs(amps.max_amp) <= AUTOSLEEP_THRESHOLD) {
                    sleep_counter++;
                    if (sleep_counter > AUTOSLEEP_TIME) {
                        sleep_counter = 0;
                        last_sys_state=SYS_SOUND_PICKUP2;
                        sys_state = SYS_AUTO_SLEEP;
                        continue;
                    }
                }
                float color_start=(color_speed-0.02)*10*120;
                float color_step = 15+color_start;
                for (int i = 0; i < abs(amps.max_amp); i++) {
                    uint32_t color = huecolor(color_step);
                    float r_s = ((color >> 8) & 0xff) / brightness;
                    float g_s = (color & 0xff) / brightness;
                    float b_s = ((color >> 16) & 0xff) / brightness;
                    Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                    if (color_step-color_start < 100)
                        color_step += (abs(amps.amp1 * 2)
                                       + abs(amps.amp2)
                                       + abs(amps.amp3 / 2.0)
                                       + abs(amps.amp4 / 4.0)
                                       + abs(amps.amp5 / 8.0));
                }
                for (int i = abs(amps.max_amp); i < MAX_LED; i++) {
                    Set_LED(i, 0, 0, 0);
                }
                WS2812_Send();
            }else if(sys_state==SYS_GRADIENT1){
                if(brightness<brightness_dest)brightness+=0.2;
                if(brightness>brightness_dest)brightness-=0.2;
                for (int i = 0; i < MAX_LED; i++) {

                    uint64_t color = huecolor(hue0);
                    float r_s = ((color >> 8) & 0xff) / brightness;
                    float g_s = (color & 0xff) / brightness;
                    float b_s = ((color >> 16) & 0xff) / brightness;
                    Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                }
                hue0 += color_speed;
                if (hue0 >= 360)hue0 = 0;
                WS2812_Send();
            }else if(sys_state==SYS_GRADIENT2){
                if(brightness<brightness_dest)brightness+=0.2;
                if(brightness>brightness_dest)brightness-=0.2;
                hue1 = hue0;
                for (int i = 0; i < MAX_LED; i++) {

                    uint64_t color = huecolor(hue1);
                    float r_s = ((color >> 8) & 0xff) / brightness;
                    float g_s = (color & 0xff) / brightness;
                    float b_s = ((color >> 16) & 0xff) / brightness;
                    Set_LED(i, (int) r_s, (int) g_s, (int) b_s);
                    hue1 += 7;
                    if (hue1 >= 360)hue1 = 0;
                }
                hue0 += color_speed;
                if (hue0 >= 360)hue0 = 0;
                WS2812_Send();
            }else if(sys_state==SYS_AUTO_SLEEP){
                    struct AmpInfo amps=calculateAmp();
                    sysSleep();
                    // auto wakeup if amp>9
                    if(amps.max_amp>AUTOWAKEUP_THRESHOLD){
                        sys_state = last_sys_state;
                    }
                    continue;
            }
            else if(sys_state==SYS_DEEP_SLEEP){
                sysDeepSleep();
                continue;
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
