/**
 * @file st7789.c
 *
 * Mostly taken from lbthomsen/esp-idf-littlevgl github.
 */

#include "st7789.h"
#include "main.h"
#include "stdio.h"
#include "spi.h"
#include "cmsis_os.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "st7789"

//#define SPI_USE_BLOCK
//#define SPI_USE_IT
#define SPI_USE_DMA

#define USE_MUTEX 1
/**********************
 *      TYPEDEFS
 **********************/

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void st7789_set_orientation(uint8_t orientation);

static void st7789_send_color(void *data, uint16_t length);

/**********************
 *  STATIC VARIABLES
 **********************/
#if defined SPI_USE_DMA || defined SPI_USE_IT
osSemaphoreDef(spi5_dma_tx_cmplt);
osSemaphoreId mutex_spi5_tx;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void st7789_init(void)
{
#if defined SPI_USE_DMA || defined SPI_USE_IT
    mutex_spi5_tx = osSemaphoreNew(1, 1, osSemaphore(spi5_dma_tx_cmplt));
#endif

    static lcd_init_cmd_t st7789_init_cmds[] = {
        {0xCF, {0x00, 0x83, 0X30}, 3},
        {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
        {ST7789_PWCTRL2, {0x85, 0x01, 0x79}, 3},
        {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
        {0xF7, {0x20}, 1},
        {0xEA, {0x00, 0x00}, 2},
        {ST7789_LCMCTRL, {0x26}, 1},
        {ST7789_IDSET, {0x11}, 1},
        {ST7789_VCMOFSET, {0x35, 0x3E}, 2},
        {ST7789_CABCCTRL, {0xBE}, 1},
        {ST7789_MADCTL, {0x00}, 1}, // Set to 0x28 if your display is flipped
        {ST7789_COLMOD, {0x05}, 1},

#if 1
        {ST7789_INVON, {0}, 0}, // set inverted mode
#else
        {ST7789_INVOFF, {0}, 0}, // set non-inverted mode
#endif

        //{ST7789_RGBCTRL, {0x00, 0x1B}, 2},
        {0xF2, {0x08}, 1},
        {ST7789_GAMSET, {0x01}, 1},
        {ST7789_PVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17}, 14},
        {ST7789_NVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
        {ST7789_CASET, {0x00, 0x00, 0x00, 0xEF}, 4},
        {ST7789_RASET, {0x00, 0x00, 0x01, 0x3f}, 4},
        {ST7789_RAMWR, {0}, 0},
        {ST7789_GCTRL, {0x07}, 1},
        {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
        {ST7789_SLPOUT, {0}, 0x80},
        {ST7789_DISPON, {0}, 0x80},
        {0, {0}, 0xff},
    };

    log_info("ST7789 initialization.\n");

    // Send all the commands
    uint16_t cmd = 0;
    while (st7789_init_cmds[cmd].databytes != 0xff)
    {
        st7789_send_cmd(st7789_init_cmds[cmd].cmd);
        //st7789_send_data(&st7789_init_cmds[cmd].cmd, 1);
        st7789_send_data(st7789_init_cmds[cmd].data, st7789_init_cmds[cmd].databytes & 0x1F);
        if (st7789_init_cmds[cmd].databytes & 0x80)
        {
            //HAL_Delay(100);
            osDelay(100);
        }
        cmd++;
    }

    st7789_enable_backlight(true);

    // st7789_set_orientation(CONFIG_LV_DISPLAY_ORIENTATION);
    st7789_set_orientation(1); // 设置屏幕显示方向
}

void st7789_enable_backlight(bool backlight)
{
    if (backlight)
    {
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);
    }
}

/* The ST7789 display controller can drive 320*240 displays, when using a 240*240
 * display there's a gap of 80px, we need to edit the coordinates to take into
 * account that gap, this is not necessary in all orientations. */
void st7789_flush(lv_display_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint8_t data[4] = {0};

    uint16_t offsetx1 = area->x1;
    uint16_t offsetx2 = area->x2;
    uint16_t offsety1 = area->y1;
    uint16_t offsety2 = area->y2;

#if (CONFIG_LV_TFT_DISPLAY_OFFSETS)
    offsetx1 += CONFIG_LV_TFT_DISPLAY_X_OFFSET;
    offsetx2 += CONFIG_LV_TFT_DISPLAY_X_OFFSET;
    offsety1 += CONFIG_LV_TFT_DISPLAY_Y_OFFSET;
    offsety2 += CONFIG_LV_TFT_DISPLAY_Y_OFFSET;

#elif (LV_HOR_RES_MAX == 240) && (LV_VER_RES_MAX == 240)
#if (CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT)
    offsetx1 += 80;
    offsetx2 += 80;
#elif (CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED)
    offsety1 += 80;
    offsety2 += 80;
#endif
#endif

    /*Column addresses*/
    st7789_send_cmd(ST7789_CASET);
    data[0] = (offsetx1 >> 8) & 0xFF;
    data[1] = offsetx1 & 0xFF;
    data[2] = (offsetx2 >> 8) & 0xFF;
    data[3] = offsetx2 & 0xFF;
    st7789_send_data(data, 4);

    /*Page addresses*/
    st7789_send_cmd(ST7789_RASET);
    data[0] = (offsety1 >> 8) & 0xFF;
    data[1] = offsety1 & 0xFF;
    data[2] = (offsety2 >> 8) & 0xFF;
    data[3] = offsety2 & 0xFF;
    st7789_send_data(data, 4);

    /*Memory write*/
    st7789_send_cmd(ST7789_RAMWR);

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    st7789_send_color((void *)color_map, size * 2);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

void disp_wait_for_pending_transactions(void)
{
#if defined SPI_USE_DMA || defined SPI_USE_IT
    osSemaphoreAcquire(mutex_spi5_tx, osWaitForever);
#endif
}

void disp_spi_send_data(uint8_t * data, uint16_t length)
{
#if defined SPI_USE_BLOCK
    HAL_SPI_Transmit(&hspi5, data, length, 1000);
#elif defined SPI_USE_IT
    HAL_SPI_Transmit_IT(&hspi5, data, length);
#elif defined SPI_USE_DMA
    // NOTE: Cache 一致性问题
    // 如果使用了D-Cache且使用DMA发送数据，这里需要注意，发送前把D-Cache里的数据写回SRAM
    // SCB_CleanDCache_by_Addr : 清除指定D-cache数据，将D-cache数据写回sram，DMA发送时用。
    // SCB_InvalidateDCache_by_Addr : 使指定D-cache数据无效，需要重新从sram中加载到D-cache中，用在DMA接收。
    SCB_CleanDCache_by_Addr((uint32_t *)data, length);
    
    HAL_SPI_Transmit_DMA(&hspi5, data, length);
#endif
}

void disp_spi_send_colors(uint8_t * data, uint16_t length)
{
    // 大小端不一样的芯片上来调整颜色数组
    for (uint16_t i = 0; i < length; i += 2)
    {
        data[i] ^= data[i + 1];
        data[i + 1] ^= data[i];
        data[i] ^= data[i + 1];
    }
#if defined SPI_USE_BLOCK
    HAL_SPI_Transmit(&hspi5, data, length, 1000);
#elif defined SPI_USE_IT
    HAL_SPI_Transmit_IT(&hspi5, data, length);
#elif defined SPI_USE_DMA
    // NOTE: Cache 一致性问题
    // 如果使用了D-Cache且使用DMA发送数据，这里需要注意，发送前把D-Cache里的数据写回SRAM
    // SCB_CleanDCache_by_Addr : 清除指定D-cache数据，将D-cache数据写回sram，DMA发送时用。
    // SCB_InvalidateDCache_by_Addr : 使指定D-cache数据无效，需要重新从sram中加载到D-cache中，用在DMA接收。
    SCB_CleanDCache_by_Addr((uint32_t *)data, length);

    HAL_SPI_Transmit_DMA(&hspi5, data, length);
#endif
}

static uint8_t g_cmd[1] __attribute__((aligned(32)));

void st7789_send_cmd(uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    g_cmd[0] = cmd;
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    disp_spi_send_data(g_cmd, 1);
}

void st7789_send_data(void *data, uint16_t length)
{
    if (length)
    {
        disp_wait_for_pending_transactions();
        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
        disp_spi_send_data(data, length);
    }
}

static void st7789_send_color(void *data, uint16_t length)
{
    if (length)
    {
        disp_wait_for_pending_transactions();
        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
        disp_spi_send_colors(data, length);
    }
}

static void st7789_set_orientation(uint8_t orientation)
{
    // ESP_ASSERT(orientation < 4);

    const char *orientation_str[] = {
        "PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE", "LANDSCAPE_INVERTED"};

    // ESP_LOGI(TAG, "Display orientation: %s", orientation_str[orientation]);

    uint8_t data[] =
        {
#if CONFIG_LV_PREDEFINED_DISPLAY_TTGO
            0x60, 0xA0, 0x00, 0xC0
#else
            0xC0, 0x00, 0x60, 0xA0
#endif
        };

    // ESP_LOGI(TAG, "0x36 command value: 0x%02X", data[orientation]);

    st7789_send_cmd(ST7789_MADCTL);
    st7789_send_data((void *)&data[orientation], 1);
}

void disp_tx_cb(void)
{
    osSemaphoreRelease(mutex_spi5_tx);
}

