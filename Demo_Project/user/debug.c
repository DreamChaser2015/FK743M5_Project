#include "debug.h"
#include "main.h"
#include "stdio.h"
#include "usart.h"
#include "cmsis_os.h"
#include "stdarg.h"

osSemaphoreDef(dbg_mutex);
osSemaphoreId g_dbg_mutex;

static uint8_t debug_buf[256] __attribute__((aligned(32)));

void debug_init(void)
{
    g_dbg_mutex = osSemaphoreNew(1, 1, osSemaphore(dbg_mutex));
}

void log_info(const char * format, ...)
{
    int len;
    va_list arglist;

    osSemaphoreAcquire(g_dbg_mutex, osWaitForever);

    va_start(arglist, format);
    len = vsprintf(debug_buf, format, arglist);
    va_end(arglist);
    
    // NOTE: Cache 一致性问题
    // 如果使用了D-Cache且使用DMA发送数据，这里需要注意，发送前把D-Cache里的数据写回SRAM
    // SCB_CleanDCache_by_Addr : 清除指定D-cache数据，将D-cache数据写回sram，DMA发送时用。
    // SCB_InvalidateDCache_by_Addr : 使指定D-cache数据无效，需要重新从sram中加载到D-cache中，用在DMA接收。
    SCB_CleanDCache_by_Addr((uint32_t *)debug_buf, len);

    HAL_UART_Transmit_DMA(&huart1, debug_buf, len);
}

void log_array(const char * buf, uint32_t len)
{
    uint8_t i, tmp;

    while (len)
    {
        tmp = len > 16 ? 16 : len;

        osSemaphoreAcquire(g_dbg_mutex, osWaitForever);

        for (i = 0; i < tmp; i++)
        {
            sprintf(debug_buf + i * 3, "%02X ", buf[i]);
        }
        sprintf(debug_buf + i * 3, "\n");

        SCB_CleanDCache_by_Addr((uint32_t *)debug_buf, i * 3 + 1);

        HAL_UART_Transmit_DMA(&huart1, debug_buf, i * 3 + 1);

        len -= tmp;
    } 
}

void debug_tx_cb(void)
{
    osSemaphoreRelease(g_dbg_mutex);
}
