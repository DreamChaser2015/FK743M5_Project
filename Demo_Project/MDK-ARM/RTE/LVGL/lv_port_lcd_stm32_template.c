/**
 * @file lv_port_lcd_stm32_template.c
 *
 * Example implementation of the LVGL LCD display drivers on the STM32 platform
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
/* Include STM32Cube files here, e.g.:
#include "stm32f7xx_hal.h"
*/

#include "lv_port_lcd_stm32_template.h"
#include "./src/drivers/display/st7789/lv_st7789.h"
#include "spi.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
/*********************
 *      DEFINES
 *********************/
 #define MY_DISP_HOR_RES    240
 #define MY_DISP_VER_RES    320
 
 
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif

#define BUS_SPI5_POLL_TIMEOUT 0x1000U

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lcd_color_transfer_ready_cb(SPI_HandleTypeDef * hspi);
static int32_t lcd_io_init(void);
static void lcd_send_cmd(lv_display_t * disp, const uint8_t * cmd, size_t cmd_size, const uint8_t * param,
                         size_t param_size);
static void lcd_send_color(lv_display_t * disp, const uint8_t * cmd, size_t cmd_size, uint8_t * param,
                           size_t param_size);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_display_t * lcd_disp;
static volatile int lcd_bus_busy = 0;

osSemaphoreDef(spi5_dma_tx_cmplt);
osSemaphoreId mutex_spi5_tx;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /* Initialize LCD I/O */
    if(lcd_io_init() != 0)
        return;
    
    mutex_spi5_tx = osSemaphoreNew(1, 0, osSemaphore(spi5_dma_tx_cmplt));

    /* Create the LVGL display object and the ST7789 LCD display driver */
    lcd_disp = lv_st7789_create(MY_DISP_HOR_RES, MY_DISP_VER_RES, LV_LCD_FLAG_NONE, lcd_send_cmd, lcd_send_color);
    
    // ST7789_INVON 颜色反转
    {  
        uint8_t para[] = {0x00};
        uint8_t cmd = 0x21;
        lcd_send_cmd(lcd_disp, &cmd, 1, 0, 0);
    }
    
    lv_display_set_rotation(lcd_disp, LV_DISPLAY_ROTATION_90);     /* set landscape orientation */
    

    /* Example: two dynamically allocated buffers for partial rendering */
    #if 0
    lv_color_t * buf1 = NULL;
    lv_color_t * buf2 = NULL;

    uint32_t buf_size = MY_DISP_HOR_RES * MY_DISP_VER_RES / 10 * lv_color_format_get_size(lv_display_get_color_format(
                                                                                              lcd_disp));

    buf1 = lv_malloc(buf_size);
    if(buf1 == NULL) {
        LV_LOG_ERROR("display draw buffer malloc failed");
        return;
    }

    buf2 = lv_malloc(buf_size);
    if(buf2 == NULL) {
        LV_LOG_ERROR("display buffer malloc failed");
        lv_free(buf1);
        return;
    }
    lv_display_set_buffers(lcd_disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    #else
    #define DISPLAY_WIDTH 480
    /*Static or global buffer(s). The second buffer is optional*/
    #define BYTE_PER_PIXEL 2 //(LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
    // TODO: Declare your own BUFF_SIZE appropriate to your system.
    #define BUFF_SIZE (DISPLAY_WIDTH * 10 * BYTE_PER_PIXEL)
    static uint8_t buf_1[BUFF_SIZE] __attribute__((aligned(32)));
    static uint8_t buf_2[BUFF_SIZE] __attribute__((aligned(32)));
    lv_display_set_buffers(lcd_disp, buf_1, buf_2, 8 * 1024, LV_DISPLAY_RENDER_MODE_PARTIAL);
    #endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
void disp_wait_for_pending_transactions(void)
{
    osSemaphoreAcquire(mutex_spi5_tx, osWaitForever);
}

/* Callback is called when background transfer finished */
static void lcd_color_transfer_ready_cb(SPI_HandleTypeDef * hspi)
{
    /* CS high */
    //HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    lcd_bus_busy = 0;
    lv_display_flush_ready(lcd_disp);
}

/* Initialize LCD I/O bus, reset LCD */
static int32_t lcd_io_init(void)
{
    /* Register SPI Tx Complete Callback */
    //HAL_SPI_RegisterCallback(&hspi5, HAL_SPI_TX_COMPLETE_CB_ID, lcd_color_transfer_ready_cb);

    /* reset LCD */
    //HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    //HAL_Delay(100);
    //HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    //HAL_Delay(100);

    //HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    //HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);

    return HAL_OK;
}

/* Platform-specific implementation of the LCD send command function. In general this should use polling transfer. */
static void lcd_send_cmd(lv_display_t * disp, const uint8_t * cmd, size_t cmd_size, const uint8_t * param,
                         size_t param_size)
{
    LV_UNUSED(disp);
    while(lcd_bus_busy);    /* wait until previous transfer is finished */
    
    /* Set the SPI in 8-bit mode */
    hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&hspi5);
    
    /* DCX low (command) */
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    /* CS low */
    //HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    /* send command */
    if(HAL_SPI_Transmit(&hspi5, cmd, cmd_size, BUS_SPI5_POLL_TIMEOUT) == HAL_OK) {
        if (param_size)
        {
            /* DCX high (data) */
            HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
            /* for short data blocks we use polling transfer */
            HAL_SPI_Transmit(&hspi5, (uint8_t *)param, (uint16_t)param_size, BUS_SPI5_POLL_TIMEOUT);
            /* CS high */
            //HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        }
    }
}

/* Platform-specific implementation of the LCD send color function. For better performance this should use DMA transfer.
 * In case of a DMA transfer a callback must be installed to notify LVGL about the end of the transfer.
 */
static void lcd_send_color(lv_display_t * disp, const uint8_t * cmd, size_t cmd_size, uint8_t * param,
                           size_t param_size)
{
    LV_UNUSED(disp);
    
    // 大小端不一样的芯片上来调整颜色数组
    //for (uint16_t i = 0; i < param_size; i += 2)
    //{
    //    param[i] ^= param[i + 1];
    //    param[i + 1] ^= param[i];
    //    param[i] ^= param[i + 1];
    //}
    
    while(lcd_bus_busy);    /* wait until previous transfer is finished */
    
    /* Set the SPI in 8-bit mode */
    hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&hspi5);
    
    /* DCX low (command) */
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    /* CS low */
    //HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    /* send command */
    if(HAL_SPI_Transmit(&hspi5, cmd, cmd_size, BUS_SPI5_POLL_TIMEOUT) == HAL_OK) {
        if (param_size)
        {
            // NOTE: Cache 一致性问题
            // 如果使用了D-Cache且使用DMA发送数据，这里需要注意，发送前把D-Cache里的数据写回SRAM
            // SCB_CleanDCache_by_Addr : 清除指定D-cache数据，将D-cache数据写回sram，DMA发送时用。
            // SCB_InvalidateDCache_by_Addr : 使指定D-cache数据无效，需要重新从sram中加载到D-cache中，用在DMA接收。
            SCB_CleanDCache_by_Addr((uint32_t *)param, param_size);
            
            /* DCX high (data) */
            HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
            /* for color data use DMA transfer */
            /* Set the SPI in 16-bit mode to match endianess */
            hspi5.Init.DataSize = SPI_DATASIZE_16BIT;
            HAL_SPI_Init(&hspi5);
            
            //HAL_SPI_Transmit(&hspi5, param, (uint16_t)param_size / 2, BUS_SPI5_POLL_TIMEOUT);
            //HAL_SPI_Transmit_IT(&hspi5, param, (uint16_t)param_size / 2);
            HAL_SPI_Transmit_DMA(&hspi5, param, (uint16_t)param_size / 2);
            
            disp_wait_for_pending_transactions();   // 中断模式或DMA模式，等待数据发送完成
            
            /* NOTE: CS will be reset in the transfer ready callback */
        }
        
        //lv_display_flush_ready(lcd_disp);
    }
}

void disp_tx_cb(void)
{
    lcd_bus_busy = 0;
    lv_display_flush_ready(lcd_disp);
    osSemaphoreRelease(mutex_spi5_tx);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
