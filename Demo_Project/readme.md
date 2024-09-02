# 注意事项



## SD卡

要注意SD卡支持的最大读写速率，如果SDMMC的时钟速率配置的太高，超过的SD卡支持的最大速率，初始化sd卡或者读写的时候会出错。

需要调整SDMMC的时钟和分频因子。



## QSPI W25Q64

QSPI的IO口速率要配置为Very High



## SDRAM

FMC配置好之后，需要按照W9825的初始化序列对其初始化，之后才能访问对应的地址。



## USB



## SPI5 LCD 

LCD_BL背光引脚和LCD_DC指令数据切换引脚需要配置为输出



## keil lvgl pack

