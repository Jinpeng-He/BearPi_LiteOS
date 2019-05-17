/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#include <string.h>
#include "hal_flash.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32_hal_legacy.h"

#ifdef HAL_FLASH_MODULE_ENABLED

#define FLASH_SECTOR_ILEGAL      0xFFFFFFFF
#define ADDR_FLASH_SECTOR_0      ((uint32_t)0x08000000) /* Base address of Sector 0, 2 Kbytes   */
#define ADDR_FLASH_SECTOR_END    ((uint32_t)0x08040000) /* End address of Sector 256 */

//extern void    FLASH_PageErase(uint32_t PageAddress);

static uint32_t prv_flash_get_sector(uint32_t addr)
{
    uint32_t sector = 0;

    if ((addr < ADDR_FLASH_SECTOR_END) && (addr >= ADDR_FLASH_SECTOR_0))
    {
        sector = ~(FLASH_PAGE_SIZE - 1) & (addr - ADDR_FLASH_SECTOR_0);
			  sector /= FLASH_PAGE_SIZE;
    }
    else
    {
        sector = FLASH_SECTOR_ILEGAL;
    }

    return sector;
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}


int hal_flash_read(void* buf, int32_t len, uint32_t location)
{
    if (NULL == buf
        || len < 0
        || len >= ADDR_FLASH_SECTOR_END - ADDR_FLASH_SECTOR_0)
    {
        return -1;
    }

    if (FLASH_SECTOR_ILEGAL != GetPage(location)
        && FLASH_SECTOR_ILEGAL != GetPage(location + len))
    {
        memcpy(buf, (uint8_t*)location, len);
        return 0;
    }
    else
    {
        return -1;
    }

}

int hal_flash_erase(uint32_t addr, int32_t len)
{
    uint32_t begin_page;
    uint32_t end_page;
    uint32_t i;

    if (len < 0 || len >= ADDR_FLASH_SECTOR_END - ADDR_FLASH_SECTOR_0)
    {
        return -1;
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return -1;
    }

    begin_page = GetPage(addr);
    end_page = GetPage(addr + len);
		
//		begin_sector = (addr - ADDR_FLASH_SECTOR_0) / FLASH_PAGE_SIZE * FLASH_PAGE_SIZE;
//    end_sector = (addr - ADDR_FLASH_SECTOR_0 + len - 1) / FLASH_PAGE_SIZE * FLASH_PAGE_SIZE;

    if (FLASH_SECTOR_ILEGAL == begin_page
        || FLASH_SECTOR_ILEGAL == end_page)
    {
        return -1;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

    for (i = begin_page; i <= end_page; ++i)
    {
        FLASH_PageErase(i ,FLASH_BANK_1);
			  
    }
		CLEAR_BIT(FLASH->CR, FLASH_CR_PER);///////////////////////
    HAL_FLASH_Lock();
    return 0;
}

int hal_flash_write(const void* buf, int32_t len, uint32_t* location)
{
    int i;
    uint8_t* pbuf;
    uint32_t location_cur;

    if (NULL == buf
        || NULL == location
        || len < 0
        || len >= ADDR_FLASH_SECTOR_END - ADDR_FLASH_SECTOR_0)
    {
        return -1;
    }

    location_cur = *location;
    pbuf = (uint8_t*)buf;

    if (FLASH_SECTOR_ILEGAL == prv_flash_get_sector(location_cur)
        || FLASH_SECTOR_ILEGAL == prv_flash_get_sector(location_cur + len))
    {
        return -1;
    }
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    //len /= 2;
		HAL_FLASH_Unlock();
    for (i = 0; i < len; /*++i*/i = i + 8)
    {
        if ( HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(uint32_t)(location_cur+i), *(uint64_t *)(pbuf + i)) == HAL_OK)
        {
           if(*(uint64_t *)(pbuf + i) != *(uint64_t*)(location_cur+i))
						{
							/* Flash content doesn't match SRAM content */

								return -1;
						}
        }
        else
        {
            return -1;
        }
    }
    *location += len;
		HAL_FLASH_Lock();

    return 0;
}
//int hal_flash_write(const uint8_t* buf, int32_t len, uint32_t* location)
//{
//    int i,j;
//		int8_t status = 0;
//    uint32_t location_cur;
//    uint8_t write_data = 0, temp_data = 0;
//    if (NULL == buf
//        || NULL == location
//        || len < 0
//        || len >= ADDR_FLASH_SECTOR_END - ADDR_FLASH_SECTOR_0)
//    {
//        return -1;
//    }

//    location_cur = *location;

//    if (FLASH_SECTOR_ILEGAL == prv_flash_get_sector(location_cur)
//        || FLASH_SECTOR_ILEGAL == prv_flash_get_sector(location_cur + len))
//    {
//        return -1;
//    }
//    //len /= 2;
//		HAL_FLASH_Unlock();
//        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

//    if (len < 1)
//    {
//        return -1;
//    }

//    for (i = 0; i < len;)
//    {
//        if ((len - i) < 8)
//        {
//            for (j = 0; (len - i) > 0; i++, j++)
//            {
//                temp_data = *buf;
//                write_data = (write_data) | (temp_data << 8*j);

//                buf ++;
//            }
//        }
//        else
//        {
//            for (j = 0; j < 8; j++, i++)
//            {
//                temp_data = *buf;
//                write_data = (write_data) | (temp_data << 8*j);
//                buf ++;
//            }
//        }

//        /* write data */
//        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, location_cur, write_data) == HAL_OK)
//        {
//            /* Check the written value */
//            if (*(uint64_t*)location_cur != write_data)
//            {
//                status = -1;
//                goto exit;
//            }
//        }
//        else
//        {
//            status = -1;
//            goto exit;
//        }
//        
//        temp_data = 0;
//        write_data = 0;

//        location_cur +=8;
//    }
//		
//exit:
//		HAL_FLASH_Lock();
//    if (status != 0)
//    {
//        return status;
//    }
//    return 0;
//}
int hal_flash_erase_write(const void* buf, int32_t len, uint32_t location)
{
    if (NULL == buf)
    {
        return -1;
    }

    if (hal_flash_erase(location, len) != 0)
    {
        (void)HAL_FLASH_Lock();
        return -1;
    }

    if (hal_flash_write(buf, len, &location) != 0)
    {
        (void)HAL_FLASH_Lock();
        return -1;
    }

    return 0;
}

void hal_flash_lock(void)
{
    (void)HAL_FLASH_Lock();
}

#endif /* HAL_FLASH_MODULE_ENABLED */

