/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "fatfs.h"
#include "lvgl.h"
#include "usbd_cdc_if.h"
#include "fmc.h"
#include "debug.h"
#include "user_sdram.h"
#include "qspi_w25q64.h"
#include "lcd_spi_200.h"
#include "self_inspection.h"
#include "rtc.h"
#include "lv_port_lcd_stm32_template.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DISPLAY_WIDTH 480
/*Static or global buffer(s). The second buffer is optional*/
#define BYTE_PER_PIXEL 2 //(LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
// TODO: Declare your own BUFF_SIZE appropriate to your system.
//#define BUFF_SIZE (DISPLAY_WIDTH * 10 * BYTE_PER_PIXEL)
//static uint8_t buf_1[BUFF_SIZE] __attribute__((aligned(32)));
//static uint8_t buf_2[BUFF_SIZE] __attribute__((aligned(32)));
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask0 */
osThreadId_t myTask0Handle;
const osThreadAttr_t myTask0_attributes = {
  .name = "myTask0",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint16_t *px_map);

uint8_t FatFs_FileTest(void)
{
  uint8_t i = 0;
  uint16_t BufferSize = 0;
  FIL MyFile;
  UINT MyFile_Num;
  BYTE MyFile_WriteBuffer[] = "STM32H743 SD card test";
  BYTE MyFile_ReadBuffer[64];
  FRESULT MyFile_Res;

  log_info("-------------FatFs file creat and write test---------------\r\n");

  MyFile_Res = f_open(&MyFile, "0:/2.txt", FA_CREATE_ALWAYS | FA_WRITE);
  if (MyFile_Res == FR_OK)
  {
    log_info("file open success, ready to write...\r\n");

    MyFile_Res = f_write(&MyFile, MyFile_WriteBuffer, sizeof(MyFile_WriteBuffer), &MyFile_Num);
    if (MyFile_Res == FR_OK)
    {
      log_info("write success, content:\r\n");
      log_info("%s\r\n", MyFile_WriteBuffer);
    }
    else
    {
      log_info("file write fail!\r\n");
      f_close(&MyFile);
      return ERROR;
    }
    f_close(&MyFile);
  }
  else
  {
    log_info("Cannot creat/open file! %d\r\n", MyFile_Res);
    f_close(&MyFile);
    return ERROR;
  }

  log_info("-------------FatFs file read test---------------\r\n");

  BufferSize = sizeof(MyFile_WriteBuffer) / sizeof(BYTE);
  MyFile_Res = f_open(&MyFile, "0:/2.txt", FA_OPEN_EXISTING | FA_READ);
  MyFile_Res = f_read(&MyFile, MyFile_ReadBuffer, BufferSize, &MyFile_Num);
  if (MyFile_Res == FR_OK)
  {
    log_info("file read success...\r\n");

    for (i = 0; i < BufferSize; i++)
    {
      if (MyFile_WriteBuffer[i] != MyFile_ReadBuffer[i])
      {
        log_info("Check fail\r\n");
        f_close(&MyFile);
        return ERROR;
      }
    }
    log_info("Check success, read content is:\r\n");
    log_info("%s\r\n", MyFile_ReadBuffer);
  }
  else
  {
    log_info("Cannot read file! \r\n");
    f_close(&MyFile);
    return ERROR;
  }

  f_close(&MyFile);
  return SUCCESS;
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask0(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationTickHook(void);

/* USER CODE BEGIN 3 */
void vApplicationTickHook(void)
{
  /* This function will be called by each tick interrupt if
  configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
  added here, but the tick hook is called from an interrupt context, so
  code must not attempt to block, and only the interrupt safe FreeRTOS API
  functions can be used (those that end in FromISR()). */

  lv_tick_inc(1); // lvgl heart beat
}

/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTask0 */
  myTask0Handle = osThreadNew(StartTask0, NULL, &myTask0_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1000);
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    //CDC_Transmit_FS("Hello world\n", 12);

    // RTC_TimeTypeDef time;
    // RTC_DateTypeDef date;
    // HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BCD);
    // HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BCD);
    // log_info("%d/%d/%d  %d:%d:%d\n", date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask0 */
/**
 * @brief Function implementing the myTask0 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask0 */
void StartTask0(void *argument)
{
  /* USER CODE BEGIN StartTask0 */
  // FRESULT res;
  // res = f_mount(&SDFatFS, "0:/", 1);
  // if (res == FR_OK)
  // {
  // 	log_info("mount file system succes\n");

  // 	FatFs_FileTest();
  // }
  // else
  // {
  // 	log_info("mount file system fail, error code: %d\n", res);
  // }

  debug_init();
  log_info("Hello STM32H743_TEST, %s %s\n", __DATE__, __TIME__);
  QSPI_W25Qxx_Init();
  SDRAM_Initialization_Sequence(&hsdram1);

  user_peripheral_self_inspection();

  // GUI init
  lv_init();
  //lv_display_t *display = lv_display_create(320, 240);                                          /*Create the display*/
  //lv_display_set_flush_cb(display, my_disp_flush);                                              /*Set a flush callback to draw to the display*/
  //lv_display_set_buffers(display, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL); /*Set an initialized buffer*/
  lv_port_disp_init();

  // Change the active screen's background color
  //lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);
  //lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);

  /*Create a spinner*/
  lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
  lv_obj_set_size(spinner, 100, 100);
  // lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_center(spinner);
  lv_spinner_set_anim_params(spinner, 10000, 200);

  static lv_style_t style_bg;
  static lv_style_t style_indic;

  lv_style_init(&style_bg);
  lv_style_set_border_color(&style_bg, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_border_width(&style_bg, 2);
  lv_style_set_pad_all(&style_bg, 6); /*To make the indicator smaller*/
  lv_style_set_radius(&style_bg, 6);
  lv_style_set_anim_duration(&style_bg, 1000);

  lv_style_init(&style_indic);
  lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
  lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_radius(&style_indic, 3);

  lv_obj_t *bar = lv_bar_create(lv_screen_active());
  lv_obj_remove_style_all(bar); /*To have a clean start*/
  lv_obj_add_style(bar, &style_bg, 0);
  lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

  lv_obj_set_size(bar, 200, 40);
  // lv_obj_center(bar);
  lv_obj_align(bar, LV_ALIGN_CENTER, 0, -100);
  lv_bar_set_value(bar, 0, LV_ANIM_ON);

  static uint16_t x = 0;
  /* Infinite loop */
  for (;;)
  {
    osDelay(10);
    lv_task_handler();

    x++;
    if (x > 500)
    {
      x = 0;
    }
    lv_bar_set_value(bar, x / 5, LV_ANIM_ON);
  }
  /* USER CODE END StartTask0 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

