
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stm32l4xx.h"
#include "hal_flash.h"
#include "hal_qspi_flash.h"
#include "usart.h"
#include "gpio.h"
#include "ota/ota.h"
#include "board.h"
#include "ota/recover_image.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
uint8_t TEXT_Buffer[] = {"123456789"};
uint8_t TEXT_Buffer2[] = {"987654321"};
#define TEST_SIZE sizeof(TEXT_Buffer)
uint32_t location=0x00050000;
/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static int flash_read(flash_type_e flash_type, void *buf, int32_t len, uint32_t offset)
{
    switch (flash_type)
    {
    case FLASH_OLDBIN_READ:
        return hal_flash_read(buf, len, OTA_DEFAULT_IMAGE_ADDR + offset);
    case FLASH_PATCH:
        return hal_spi_flash_read(buf, len, OTA_IMAGE_DOWNLOAD_ADDR + offset);
    case FLASH_UPDATE_INFO:
        return hal_spi_flash_read(buf, len, OTA_FLAG_ADDR1);
    default:
        printf("wrong flash type detected %d\n", flash_type);
        return -1;
    }
}

static int flash_write(flash_type_e flash_type, const void *buf, int32_t len, uint32_t offset)
{
    switch (flash_type)
    {
    case FLASH_NEWBIN_WRITE:
        return hal_spi_flash_erase_write(buf, len, OTA_IMAGE_DIFF_UPGRADE_ADDR + offset);
    case FLASH_PATCH:
        return hal_spi_flash_erase_write(buf, len, OTA_IMAGE_DOWNLOAD_ADDR + offset);
    case FLASH_UPDATE_INFO:
        return hal_spi_flash_erase_write(buf, len, OTA_FLAG_ADDR1);
    default:
        return -1;
    }
}

static int jump(uint32_t oldbin_size)
{
    int ret;

    printf("info: begin to jump to application\n");
    ret = board_jump2app();
    if (ret != 0)
    {
        printf("warning: jump to app failed, try to roll back now\n");
        (void)recover_set_update_fail();
        ret = board_rollback_copy(oldbin_size);
        if (ret != 0)
        {
            printf("fatal: roll back failed, system start up failed\n");
            _Error_Handler(__FILE__, __LINE__);
        }
    }
    printf("info: begin to try to jump to application again\n");
    ret = board_jump2app();
    if (ret != 0)
    {
        printf("fatal: roll back succeed, system start up failed\n");
        _Error_Handler(__FILE__, __LINE__);
    }

    return ret;
}

static int register_info(void)
{
    recover_info_s info;
    recover_assist_s assist;
    recover_flash_s flash;

    info.max_new_image_size = OTA_IMAGE_DIFF_UPGRADE_SIZE;
    info.max_old_image_size = OTA_IMAGE_DIFF_UPGRADE_SIZE;
    info.max_patch_size = OTA_IMAGE_DOWNLOAD_SIZE;
    info.old_image_addr = OTA_DEFAULT_IMAGE_ADDR;
    info.new_image_addr = OTA_IMAGE_DIFF_UPGRADE_ADDR;
    info.patch_addr = OTA_IMAGE_DOWNLOAD_ADDR;
    info.flash_erase_unit = 0x1000;
    info.recover_on_oldimage = 0;

    assist.func_printf = printf;
    assist.func_malloc = malloc;
    assist.func_free = free;

    flash.func_flash_read = flash_read;
    flash.func_flash_write = flash_write;

    return recover_init(&info, &assist, &flash);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    int ret;
    recover_upgrade_type_e upgrade_type = RECOVER_UPGRADE_NONE;
    uint32_t newbin_size = 0;
    uint32_t oldbin_size = 0;
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_USART1_UART_Init();
	
	hal_spi_flash_config();

	uint8_t datatemp[TEST_SIZE];
	//hal_flash_erase_write(TEXT_Buffer, TEST_SIZE, location);
	//hal_flash_read(datatemp, TEST_SIZE, location);
	//hal_flash_read(datatemp, TEST_SIZE, location);
	//printf("SPI  datatemp :%s\r\n",datatemp);
	
	hal_spi_flash_erase_write(TEXT_Buffer2, 0x1000, location);
	hal_spi_flash_read(datatemp, TEST_SIZE, location);
	printf("SPI  datatemp :%s\r\n",datatemp);

    printf("bootloader begin\n");
    ret = register_info();
    if (ret != 0)
        printf("warning: recover register failed\n");

    printf("info: begin to process upgrade\n");
    ret = recover_image(&upgrade_type, &newbin_size, &oldbin_size);
    if (oldbin_size == 0)
        oldbin_size = OTA_IMAGE_DOWNLOAD_SIZE;
    if (ret == 0)
    {
        switch (upgrade_type)
        {
        case RECOVER_UPGRADE_NONE:
            printf("info: normal start up\n");
            break;
        case RECOVER_UPGRADE_FULL:
            printf("info: full upgrade\n");
            ret = board_update_copy(oldbin_size, newbin_size, OTA_IMAGE_DOWNLOAD_ADDR);
            if (ret != 0)
            {
                printf("warning: [full] copy newimage to inner flash failed\n");
                (void)recover_set_update_fail();
            }
            break;
        case RECOVER_UPGRADE_DIFF:
            printf("info: diff upgrade\n");
            ret = board_update_copy(oldbin_size, newbin_size, OTA_IMAGE_DIFF_UPGRADE_ADDR);
            if (ret != 0)
            {
                printf("warning: [diff] copy newimage to inner flash failed\n");
                (void)recover_set_update_fail();
            }
            break;
        default:
            break;
        }
    }
    else
        printf("warning: upgrade failed with ret %d\n", ret);

    ret = jump(oldbin_size);

    return ret;


}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

   RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Configure LSE Drive Capability 
    */
  HAL_PWR_EnableBkUpAccess();

  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the main internal regulator output voltage 
    */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /**Enable MSI Auto calibration 
    */
  HAL_RCCEx_EnableMSIPLLMode();

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
