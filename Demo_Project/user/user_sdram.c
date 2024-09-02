#include "user_sdram.h"
#include "debug.h"

/* W9825G6 SDRAM 芯片 */

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
    FMC_SDRAM_CommandTypeDef Command;
    __IO uint32_t tmpmrd = 0;

    /* Configure a clock configuration enable command */
    Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;  // 开启SDRAM时钟
    Command.CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT); // 发送控制指令
    HAL_Delay(1);                                           // 延时等待

    /* Configure a PALL (precharge all) command */
    Command.CommandMode = FMC_SDRAM_CMD_PALL;        // 预充电命令
    Command.CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT); // 发送控制指令

    /* Configure a Auto-Refresh command */
    Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE; // 使用自动刷新
    Command.CommandTarget = FMC_COMMAND_TARGET_BANK;      // 选择要控制的区域
    Command.AutoRefreshNumber = 8;                        // 自动刷新次数
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT); // 发送控制指令

    /* Program the external memory mode register */
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2 |
             SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
             SDRAM_MODEREG_CAS_LATENCY_3 |
             SDRAM_MODEREG_OPERATING_MODE_STANDARD |
             SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;   // 加载模式寄存器命令
    Command.CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = tmpmrd;

    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT); // 发送控制指令

    HAL_SDRAM_ProgramRefreshRate(hsdram, 918); // 配置刷新率
}

void SDRAM_Test(void)
{
    uint32_t i = 0;        // 计数变量
    uint16_t ReadData = 0; // 读取到的数据
    uint8_t ReadData_8b;

    uint32_t ExecutionTime_Begin; // 开始时间
    uint32_t ExecutionTime_End;   // 结束时间
    uint32_t ExecutionTime;       // 执行时间
    float ExecutionSpeed;         // 执行速度

    log_info("\r\n*****************************************************************************************************\r\n");
    log_info("\r\n进行速度测试>>>\r\n");

    // 写入 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    ExecutionTime_Begin = HAL_GetTick(); // 获取 systick 当前时间，单位ms

    for (i = 0; i < SDRAM_Size / 2; i++)
    {
        *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i) = (uint16_t)i; // 写入数据
    }
    ExecutionTime_End = HAL_GetTick();                                       // 获取 systick 当前时间，单位ms
    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;                 // 计算擦除时间，单位ms
    ExecutionSpeed = (float)SDRAM_Size / 1024 / 1024 / ExecutionTime * 1000; // 计算速度，单位 MB/S

    log_info("\r\n以16位数据宽度写入数据，大小：%d MB，耗时: %d ms, 写入速度：%.2f MB/s\r\n", SDRAM_Size / 1024 / 1024, ExecutionTime, ExecutionSpeed);

    // 读取	>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    ExecutionTime_Begin = HAL_GetTick(); // 获取 systick 当前时间，单位ms

    for (i = 0; i < SDRAM_Size / 2; i++)
    {
        ReadData = *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i); // 从SDRAM读出数据
    }
    ExecutionTime_End = HAL_GetTick();                                       // 获取 systick 当前时间，单位ms
    ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;                 // 计算擦除时间，单位ms
    ExecutionSpeed = (float)SDRAM_Size / 1024 / 1024 / ExecutionTime * 1000; // 计算速度，单位 MB/S

    log_info("\r\n读取数据完毕，大小：%d MB，耗时: %d ms, 读取速度：%.2f MB/s\r\n", SDRAM_Size / 1024 / 1024, ExecutionTime, ExecutionSpeed);

    // 数据校验 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    log_info("\r\n*****************************************************************************************************\r\n");
    log_info("\r\n进行数据校验>>>\r\n");

    for (i = 0; i < SDRAM_Size / 2; i++)
    {
        ReadData = *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i); // 从SDRAM读出数据
        if (ReadData != (uint16_t)i)                            // 检测数据，若不相等，跳出函数,返回检测失败结果。
        {
            log_info("\r\nSDRAM测试失败！！\r\n");
            return ERROR; // 返回失败标志
        }
    }

    log_info("\r\n16位数据宽度读写通过，以8位数据宽度写入数据\r\n");
    for (i = 0; i < 255; i++)
    {
        *(__IO uint8_t *)(SDRAM_BANK_ADDR + i) = (uint8_t)i;
    }
    log_info("写入完毕，读取数据并比较...\r\n");
    for (i = 0; i < 255; i++)
    {
        ReadData_8b = *(__IO uint8_t *)(SDRAM_BANK_ADDR + i);
        if (ReadData_8b != (uint8_t)i) // 检测数据，若不相等，跳出函数,返回检测失败结果。
        {
            log_info("8位数据宽度读写测试失败！！\r\n");
            log_info("请检查NBL0和NBL1的连接\r\n");
            return ERROR; // 返回失败标志
        }
    }
    log_info("8位数据宽度读写通过\r\n");
    log_info("SDRAM读写测试通过，系统正常\r\n");
    return SUCCESS; // 返回成功标志
}
