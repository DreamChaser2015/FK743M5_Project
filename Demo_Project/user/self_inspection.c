#include "self_inspection.h"
#include "debug.h"
#include "quadspi.h"
#include "main.h"
#include "qspi_w25q64.h"
#include "sdmmc.h"
#include "lcd_spi_200.h"
#include "user_sdram.h"

#define W25Qxx_NumByteToTest 32 * 1024 // 测试数据的长度，64K

int32_t QSPI_Status;

uint32_t W25Qxx_TestAddr = 0;
uint8_t W25Qxx_WriteBuffer[W25Qxx_NumByteToTest];
uint8_t W25Qxx_ReadBuffer[W25Qxx_NumByteToTest];

int8_t QSPI_W25Qxx_Test(void) // Flash读写测试
{
    uint32_t i = 0;               // 计数变量
    uint32_t ExecutionTime_Begin; // 开始时间
    uint32_t ExecutionTime_End;   // 结束时间
    uint32_t ExecutionTime;       // 执行时间
    float ExecutionSpeed;         // 执行速度

    // 擦除 >>>>>>>

    log_info("\r\n*****************************************************************************************************\r\n");

    ExecutionTime_Begin = HAL_GetTick();                       // 获取 systick 当前时间，单位ms
    QSPI_Status = QSPI_W25Qxx_BlockErase_32K(W25Qxx_TestAddr); // 擦除32K字节
    ExecutionTime_End = HAL_GetTick();                         // 获取 systick 当前时间，单位ms

    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin; // 计算擦除时间，单位ms

    if (QSPI_Status == QSPI_W25Qxx_OK)
    {
        log_info("\r\nW25Q64 擦除成功, 擦除32K字节所需时间: %d ms\r\n", ExecutionTime);
    }
    else
    {
        log_info("\r\n 擦除失败!!!!!  错误代码:%d\r\n", QSPI_Status);
        while (1)
            ;
    }

    // 写入 >>>>>>>

    for (i = 0; i < W25Qxx_NumByteToTest; i++) // 先将数据写入数组
    {
        W25Qxx_WriteBuffer[i] = i;
    }
    ExecutionTime_Begin = HAL_GetTick();                                                              // 获取 systick 当前时间，单位ms
    QSPI_Status = QSPI_W25Qxx_WriteBuffer(W25Qxx_WriteBuffer, W25Qxx_TestAddr, W25Qxx_NumByteToTest); // 写入数据
    ExecutionTime_End = HAL_GetTick();                                                                // 获取 systick 当前时间，单位ms

    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;      // 计算擦除时间，单位ms
    ExecutionSpeed = (float)W25Qxx_NumByteToTest / ExecutionTime; // 计算写入速度，单位 KB/S
    if (QSPI_Status == QSPI_W25Qxx_OK)
    {
        log_info("\r\n写入成功,数据大小：%d KB, 耗时: %d ms, 写入速度：%.2f KB/s\r\n", W25Qxx_NumByteToTest / 1024, ExecutionTime, ExecutionSpeed);
    }
    else
    {
        log_info("\r\n写入错误!!!!!  错误代码:%d\r\n", QSPI_Status);
        while (1)
            ;
    }

    // 读取 >>>>>>>

    ExecutionTime_Begin = HAL_GetTick();                                                            // 获取 systick 当前时间，单位ms
    QSPI_Status = QSPI_W25Qxx_ReadBuffer(W25Qxx_ReadBuffer, W25Qxx_TestAddr, W25Qxx_NumByteToTest); // 读取数据
    ExecutionTime_End = HAL_GetTick();                                                              // 获取 systick 当前时间，单位ms

    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;             // 计算擦除时间，单位ms
    ExecutionSpeed = (float)W25Qxx_NumByteToTest / ExecutionTime / 1024; // 计算读取速度，单位 MB/S

    if (QSPI_Status == QSPI_W25Qxx_OK)
    {
        log_info("\r\n读取成功,数据大小：%d KB, 耗时: %d ms, 读取速度：%.2f MB/s \r\n", W25Qxx_NumByteToTest / 1024, ExecutionTime, ExecutionSpeed);
    }
    else
    {
        log_info("\r\n读取错误!!!!!  错误代码:%d\r\n", QSPI_Status);
        while (1)
            ;
    }

    // 数据校验 >>>>>>>

    for (i = 0; i < W25Qxx_NumByteToTest; i++) // 验证读出的数据是否等于写入的数据
    {
        if (W25Qxx_WriteBuffer[i] != W25Qxx_ReadBuffer[i]) // 如果数据不相等，则返回0
        {
            log_info("\r\n数据校验失败!!!!!\r\n");
            while (1)
                ;
        }
    }
    log_info("\r\n校验通过!!!!! QSPI驱动W25Q64测试正常\r\n");

    // 读取整片Flash的数据，用以测试速度 >>>>>>>
    log_info("\r\n*****************************************************************************************************\r\n");
    log_info("\r\n上面的测试中，读取的数据比较小，耗时很短，加之测量的最小单位为ms，计算出的读取速度误差较大\r\n");
    log_info("\r\n接下来读取整片flash的数据用以测试速度，这样得出的速度误差比较小\r\n");
    log_info("\r\n开始读取>>>>\r\n");
    ExecutionTime_Begin = HAL_GetTick(); // 获取 systick 当前时间，单位ms

    for (i = 0; i < W25Qxx_FlashSize / (W25Qxx_NumByteToTest); i++) // 每次读取 W25Qxx_NumByteToTest 字节的数据
    {
        QSPI_Status = QSPI_W25Qxx_ReadBuffer(W25Qxx_ReadBuffer, W25Qxx_TestAddr, W25Qxx_NumByteToTest);
        W25Qxx_TestAddr = W25Qxx_TestAddr + W25Qxx_NumByteToTest;
    }
    ExecutionTime_End = HAL_GetTick(); // 获取 systick 当前时间，单位ms

    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;         // 计算擦除时间，单位ms
    ExecutionSpeed = (float)W25Qxx_FlashSize / ExecutionTime / 1024; // 计算读取速度，单位 MB/S

    if (QSPI_Status == QSPI_W25Qxx_OK)
    {
        log_info("\r\n读取成功,数据大小：%d MB, 耗时: %d ms, 读取速度：%.2f MB/s \r\n", W25Qxx_FlashSize / 1024 / 1024, ExecutionTime, ExecutionSpeed);
    }
    else
    {
        log_info("\r\n读取错误!!!!!  错误代码:%d\r\n", QSPI_Status);
        while (1)
            ;
    }

    return QSPI_W25Qxx_OK; // 测试通过
}

void SDCard_test(void)
{
    HAL_SD_CardInfoTypeDef CardInfo;
    HAL_StatusTypeDef stat;
    HAL_SD_Init(&hsd1);
    HAL_SD_InitCard(&hsd1);
    stat = HAL_SD_GetCardInfo(&hsd1, &CardInfo);

    log_info("*****************************************************************************************************\r\n");
    log_info("SD Card test:---------------------------\n");
    log_info("stat: %d\n", stat);
    log_info("CardType: %d\n", CardInfo.CardType);
    log_info("CardVersion: %d\n", CardInfo.CardVersion);
    log_info("RelCardAdd: %08X\n", CardInfo.RelCardAdd);
    log_info("BlockNbr: %d\n", CardInfo.BlockNbr);
    log_info("BlockSize: %d\n", CardInfo.BlockSize);
    log_info("LogBlockNbr: %d\n", CardInfo.LogBlockNbr);
    log_info("LogBlockSize: %d\n", CardInfo.LogBlockSize);
    //log_info("CardSpeed: %fM/S\n", CardInfo.CardSpeed / 1024 / 1024);
    log_info("CardSpeed: %dM/S\n", CardInfo.CardSpeed);

    // stat = HAL_SD_ReadBlocks(&hsd1, read_buf1, 0, 1, HAL_MAX_DELAY);
    // log_info("stat: %d\n", stat);
    // log_array(read_buf1, 512);

    HAL_Delay(100);
}

#if 0
/*************************************************************************************************
 *	函 数 名:	LCD_Test_Clear
 *
 *	函数功能:	清屏测试
 *************************************************************************************************/
void LCD_Test_Clear(void)
{
    uint8_t i = 0; // 计数变量

    LCD_SetTextFont(&CH_Font24); // 设置2424中文字体,ASCII字体对应为2412
    LCD_SetColor(LCD_BLACK);     // 设置画笔颜色

    for (i = 0; i < 8; i++)
    {
        switch (i) // 切换背景色
        {
        case 0:
            LCD_SetBackColor(LIGHT_RED);
            break;
        case 1:
            LCD_SetBackColor(LIGHT_GREEN);
            break;
        case 2:
            LCD_SetBackColor(LIGHT_BLUE);
            break;
        case 3:
            LCD_SetBackColor(LIGHT_YELLOW);
            break;
        case 4:
            LCD_SetBackColor(LIGHT_CYAN);
            break;
        case 5:
            LCD_SetBackColor(LIGHT_GREY);
            break;
        case 6:
            LCD_SetBackColor(LIGHT_MAGENTA);
            break;
        case 7:
            LCD_SetBackColor(LCD_WHITE);
            break;
        default:
            break;
        }
        LCD_Clear(); // 清屏
        LCD_DisplayText(13, 70, "STM32H7 刷屏测试");
        LCD_DisplayText(13, 106, "屏幕分辨率:240*320");
        LCD_DisplayText(13, 142, "控制器:ST7789");
        HAL_Delay(1000); // 延时
    }
}

/*************************************************************************************************
 *	函 数 名:	LCD_Test_Text
 *
 *	函数功能:	文本显示测试
 *
 *	说    明:	显示文本，包括各种字体大小的中文和ASCII字符
 *************************************************************************************************/
void LCD_Test_Text(void)
{
    LCD_SetBackColor(LCD_BLACK); //	设置背景色
    LCD_Clear();                 // 清屏

    LCD_SetColor(LCD_WHITE);
    LCD_SetAsciiFont(&ASCII_Font32);
    LCD_DisplayString(0, 0, "!#$'()*+,-.0123");
    LCD_SetAsciiFont(&ASCII_Font24);
    LCD_DisplayString(0, 32, "!#$'()*+,-.012345678");
    LCD_SetAsciiFont(&ASCII_Font20);
    LCD_DisplayString(0, 56, "!#$'()*+,-.0123456789:;<");
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_DisplayString(0, 76, "!#$'()*+,-.0123456789:;<=>?@AB");
    LCD_SetAsciiFont(&ASCII_Font12);
    LCD_DisplayString(0, 92, "!#$'()*+,-.0123456789:;<=>?@ABCDEFGHIJKL");

    LCD_SetColor(LCD_CYAN);
    LCD_SetAsciiFont(&ASCII_Font12);
    LCD_DisplayString(0, 104, "!#&'()*+,-.0123456789:;<=>?@ABCDEFGHIJKL");
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_DisplayString(0, 116, "!#&'()*+,-.0123456789:;<=>?@AB");
    LCD_SetAsciiFont(&ASCII_Font20);
    LCD_DisplayString(0, 132, "!#&'()*+,-.0123456789:;<");
    LCD_SetAsciiFont(&ASCII_Font24);
    LCD_DisplayString(0, 152, "!#&'()*+,-.012345678");
    LCD_SetAsciiFont(&ASCII_Font32);
    LCD_DisplayString(0, 176, "!#&'()*+,-.0123");

    LCD_SetTextFont(&CH_Font24); // 设置2424中文字体,ASCII字体对应为2412
    LCD_SetColor(LCD_YELLOW);    // 设置画笔，黄色
    //LCD_DisplayText(0, 216, "ASCII字符集");

    HAL_Delay(2000); // 延时等待
    LCD_Clear();     // 清屏

    LCD_SetTextFont(&CH_Font12); // 设置1212中文字体,ASCII字体对应为1206
    LCD_SetColor(0X8AC6D1);      // 设置画笔
    LCD_DisplayText(14, 10, "1212:反客科技");

    LCD_SetTextFont(&CH_Font16); // 设置1616中文字体,ASCII字体对应为1608
    LCD_SetColor(0XC5E1A5);      // 设置画笔
    LCD_DisplayText(14, 30, "1616:反客科技");

    LCD_SetTextFont(&CH_Font20); // 设置2020中文字体,ASCII字体对应为2010
    LCD_SetColor(0XFFB549);      // 设置画笔
    LCD_DisplayText(14, 60, "2020:反客科技");

    LCD_SetTextFont(&CH_Font24); // 设置2424中文字体,ASCII字体对应为2412
    LCD_SetColor(0XFF585D);      // 设置画笔
    LCD_DisplayText(14, 90, "2424:反客科技");

    LCD_SetTextFont(&CH_Font32); // 设置3232中文字体,ASCII字体对应为3216
    LCD_SetColor(0xFFB6B9);      // 设置画笔
    LCD_DisplayText(14, 130, "3232:反客科技");

    LCD_SetTextFont(&CH_Font24); // 设置2424中文字体,ASCII字体对应为2412
    LCD_SetColor(LCD_WHITE);
    LCD_DisplayText(14, 180, "中文显示");

    HAL_Delay(2000); // 延时等待
}
/************************************************************************************************
 *	函 数 名:	LCD_Test_Variable
 *
 *	函数功能:	变量显示，包括整数和小数
 *************************************************************************************************/
void LCD_Test_Variable(void)
{
    uint16_t i;    // 计数变量
    int32_t a = 0; // 定义整数变量，用于测试
    int32_t b = 0; // 定义整数变量，用于测试
    int32_t c = 0; // 定义整数变量，用于测试

    double p = 3.1415926;  // 定义浮点数变量，用于测试
    double f = -1234.1234; // 定义浮点数变量，用于测试

    LCD_SetBackColor(LCD_BLACK); //	设置背景色
    LCD_Clear();                 // 清屏

    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LIGHT_CYAN); // 设置画笔，蓝绿色
    LCD_DisplayText(0, 10, "正数:");
    LCD_DisplayText(0, 40, "负数:");

    LCD_SetColor(LIGHT_YELLOW); // 设置画笔，亮黄色
    LCD_DisplayText(0, 80, "填充空格:");
    LCD_DisplayText(0, 110, "填充0:");

    LCD_SetColor(LIGHT_RED); // 设置画笔	，亮红色
    LCD_DisplayText(0, 150, "正小数:");
    LCD_DisplayText(0, 180, "负小数:");

    for (i = 0; i < 100; i++)
    {
        LCD_SetColor(LIGHT_CYAN);                 // 设置画笔	，蓝绿色
        LCD_ShowNumMode(Fill_Space);              // 多余位填充空格
        LCD_DisplayNumber(80, 10, b + i * 10, 4); // 显示变量
        LCD_DisplayNumber(80, 40, c - i * 10, 4); // 显示变量

        LCD_SetColor(LIGHT_YELLOW); // 设置画笔，亮黄色

        LCD_ShowNumMode(Fill_Space);                // 多余位填充 空格
        LCD_DisplayNumber(130, 80, a + i * 150, 8); // 显示变量

        LCD_ShowNumMode(Fill_Zero);                  // 多余位填充0
        LCD_DisplayNumber(130, 110, b + i * 150, 8); // 显示变量

        LCD_SetColor(LIGHT_RED);                            // 设置画笔，亮红色
        LCD_ShowNumMode(Fill_Space);                        // 多余位填充空格
        LCD_DisplayDecimals(100, 150, p + i * 0.1, 6, 3);   // 显示小数
        LCD_DisplayDecimals(100, 180, f + i * 0.01, 11, 4); // 显示小数

        HAL_Delay(15);
    }
    HAL_Delay(2500);
}
/*************************************************************************************************
 *	函 数 名:	LCD_Test_Color
 *
 *	函数功能:	颜色测
 *************************************************************************************************/
void LCD_Test_Color(void)
{
    uint16_t i; // 计数变量
    uint16_t y;
    // 颜色测试>>>>>
    LCD_SetBackColor(LCD_BLACK); // 设置背景色
    LCD_Clear();                 // 清屏，刷背景色

    LCD_SetTextFont(&CH_Font20); // 设置字体
    LCD_SetColor(LCD_WHITE);     // 设置画笔颜色
    LCD_DisplayText(0, 0, "RGB三基色:");

    // 使用画线函数绘制三基色色条
    for (i = 0; i < 240; i++)
    {
        LCD_SetColor(LCD_RED - (i << 16));
        LCD_DrawLine_V(0 + i, 20, 10);
    }
    for (i = 0; i < 240; i++)
    {
        LCD_SetColor(LCD_GREEN - (i << 8));
        LCD_DrawLine_V(0 + i, 35, 10);
    }
    for (i = 0; i < 240; i++)
    {
        LCD_SetColor(LCD_BLUE - i);
        LCD_DrawLine_V(0 + i, 50, 10);
    }

    y = 70;
    LCD_SetColor(LIGHT_CYAN);
    LCD_FillRect(150, y + 5, 90, 10);
    LCD_DisplayString(0, y, "LIGHT_CYAN");
    LCD_SetColor(LIGHT_MAGENTA);
    LCD_FillRect(150, y + 20 * 1 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 1, "LIGHT_MAGENTA");
    LCD_SetColor(LIGHT_YELLOW);
    LCD_FillRect(150, y + 20 * 2 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 2, "LIGHT_YELLOW");
    LCD_SetColor(LIGHT_GREY);
    LCD_FillRect(150, y + 20 * 3 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 3, "LIGHT_GREY");
    LCD_SetColor(LIGHT_RED);
    LCD_FillRect(150, y + 20 * 4 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 4, "LIGHT_RED");
    LCD_SetColor(LIGHT_BLUE);
    LCD_FillRect(150, y + 20 * 5 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 5, "LIGHT_BLUE");

    LCD_SetColor(DARK_CYAN);
    LCD_FillRect(150, y + 20 * 6 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 6, "DARK_CYAN");
    LCD_SetColor(DARK_MAGENTA);
    LCD_FillRect(150, y + 20 * 7 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 7, "DARK_MAGENTA");
    LCD_SetColor(DARK_YELLOW);
    LCD_FillRect(150, y + 20 * 8 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 8, "DARK_YELLOW");
    LCD_SetColor(DARK_GREY);
    LCD_FillRect(150, y + 20 * 9 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 9, "DARK_GREY");
    LCD_SetColor(DARK_RED);
    LCD_FillRect(150, y + 20 * 10 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 10, "DARK_RED");
    LCD_SetColor(DARK_GREEN);
    LCD_FillRect(150, y + 20 * 11 + 5, 90, 10);
    LCD_DisplayString(0, y + 20 * 11, "DARK_GREEN");

    HAL_Delay(2000);
}

/*************************************************************************************************
 *	函 数 名:	LCD_Test_Grahic
 *
 *	函数功能:	2D图形绘制
 *************************************************************************************************/
void LCD_Test_Grahic(void)
{
    LCD_SetBackColor(LCD_BLACK); // 设置背景色
    LCD_Clear();                 // 清屏，刷背景色

    LCD_SetColor(LCD_WHITE);
    LCD_DrawRect(0, 0, 240, 320); // 绘制矩形

    LCD_SetColor(LCD_RED);
    LCD_FillCircle(140, 50, 30); // 填充圆形
    LCD_SetColor(LCD_GREEN);
    LCD_FillCircle(170, 50, 30);
    LCD_SetColor(LCD_BLUE);
    LCD_FillCircle(200, 50, 30);

    LCD_SetColor(LCD_YELLOW);
    LCD_DrawLine(26, 26, 113, 64); // 画直线
    LCD_DrawLine(35, 22, 106, 81); // 画直线
    LCD_DrawLine(45, 20, 93, 100); // 画直线
    LCD_DrawLine(52, 16, 69, 108); // 画直线
    LCD_DrawLine(62, 16, 44, 108); // 画直线

    LCD_SetColor(LIGHT_CYAN);
    LCD_DrawCircle(120, 170, 30); // 绘制圆形
    LCD_DrawCircle(120, 170, 20);

    LCD_SetColor(LIGHT_RED);
    LCD_DrawEllipse(120, 170, 90, 40); // 绘制椭圆
    LCD_DrawEllipse(120, 170, 70, 40); // 绘制椭圆
    LCD_SetColor(LIGHT_MAGENTA);
    LCD_DrawEllipse(120, 170, 100, 50); // 绘制椭圆
    LCD_DrawEllipse(120, 170, 110, 60);

    LCD_SetColor(DARK_RED);
    LCD_FillRect(50, 250, 50, 50); // 填充矩形
    LCD_SetColor(DARK_BLUE);
    LCD_FillRect(100, 250, 50, 50); // 填充矩形
    LCD_SetColor(LIGHT_GREEN);
    LCD_FillRect(150, 250, 50, 50); // 填充矩形
}
/*************************************************************************************************
 *	函 数 名:	LCD_Test_Image
 *
 *	函数功能:	图片显示测试
 *************************************************************************************************/
void LCD_Test_Image(void)
{
    LCD_SetBackColor(LCD_BLACK); //	设置背景色
    LCD_Clear();                 // 清屏

    LCD_SetColor(0xffF6E58D);
    LCD_DrawImage(19, 55, 83, 83, Image_Android_83x83); // 显示图片

    LCD_SetColor(0xffDFF9FB);
    LCD_DrawImage(141, 55, 83, 83, Image_Message_83x83); // 显示图片

    LCD_SetColor(0xff9DD3A8);
    LCD_DrawImage(19, 175, 83, 83, Image_Toys_83x83); // 显示图片

    LCD_SetColor(0xffFF8753);
    LCD_DrawImage(141, 175, 83, 83, Image_Video_83x83); // 显示图片

    HAL_Delay(2000);
}
/*************************************************************************************************
 *	函 数 名:	LCD_Test_Direction
 *
 *	函数功能:	更换显示方向
 *************************************************************************************************/
void LCD_Test_Direction(void)
{
    for (int i = 0; i < 4; i++)
    {
        LCD_SetBackColor(LCD_BLACK); //	设置背景色
        LCD_Clear();                 // 清屏
        LCD_SetTextFont(&CH_Font24);
        LCD_SetColor(0xffDFF9FB);
        switch (i) // 切换背景色
        {
        case 0:
            LCD_SetDirection(Direction_V);
            LCD_DisplayText(20, 20, "Direction_V");
            break;

        case 1:
            LCD_SetDirection(Direction_H);
            LCD_DisplayText(20, 20, "Direction_H");
            break;

        case 2:
            LCD_SetDirection(Direction_V_Flip);
            LCD_DisplayText(20, 20, "Direction_V_Flip");
            break;
        case 3:
            LCD_SetDirection(Direction_H_Flip);
            LCD_DisplayText(20, 20, "Direction_H_Flip");
            break;

        default:
            break;
        }
        LCD_SetColor(0xffF6E58D);
        LCD_DrawImage(19, 80, 83, 83, Image_Android_83x83); // 显示图片
        LCD_SetTextFont(&CH_Font32);
        LCD_SetColor(0xff9DD3A8);
        LCD_DisplayText(130, 90, "反客");
        LCD_DisplayText(130, 130, "科技");
        LCD_SetDirection(Direction_V);
        HAL_Delay(1000); // 延时
    }
}
#endif

void user_peripheral_self_inspection(void)
{
    /*sd card*/
    SDCard_test();

    /*qspi flash*/
    //QSPI_W25Qxx_Test();

    /*sdram*/
    //SDRAM_Test();

    /*spi lcd*/
    // LCD_Test_Clear();     // 清屏测试
    // LCD_Test_Text();      // 文本测试
    // LCD_Test_Variable();  // 变量显示，包括整数和小数
    // LCD_Test_Color();     // 颜色测试
    //LCD_Test_Grahic();    // 2D图形绘制
    //LCD_Test_Image();     // 图片显示
    //LCD_Test_Direction(); // 更换显示方向
    //LCD_SetDirection(Direction_H);

    log_info("*****************************************************************************************************\r\n");

    HAL_Delay(100);
}
