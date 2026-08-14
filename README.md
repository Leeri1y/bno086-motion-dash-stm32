# BNO086 MotionDash — STM32F407VET6 + FreeRTOS 移植工程

将原 Arduino 工程 `BNO086_MotionDash`（BNO086 九轴姿态运动仪表盘）完整移植到
**STM32F407VET6 + STM32CubeMX + Keil(MDK-ARM) + FreeRTOS**，功能与显示效果等价。

- 原工程：零知派标准板 STM32F103RBT6 + 软件 I2C(SoftWire) + Adafruit ST7789/GFX + SparkFun BNO08x
- 移植后：STM32F407VET6 + 硬件 I2C1 + 硬件 SPI1 + HAL + FreeRTOS（2 任务 + EXTI 按键中断）

---

## 1. 引脚分配（BNO086 MotionDash → STM32F407VET6）

| 功能 | 引脚 | 复用 |
|---|---|---|
| BNO086 SCL | PB6 | I2C1_SCL (AF4) |
| BNO086 SDA | PB7 | I2C1_SDA (AF4) |
| BNO086 INT | PC6 | GPIO 输入（上拉，轮询） |
| BNO086 RST | PC7 | GPIO 输出 |
| ST7789 SCK | PA5 | SPI1_SCK (AF5) |
| ST7789 MOSI | PA7 | SPI1_MOSI (AF5) |
| ST7789 CS | PC4 | GPIO 输出 |
| ST7789 DC | PC5 | GPIO 输出 |
| ST7789 RST | PA4 | GPIO 输出 |
| ST7789 BL(背光) | PA8 | GPIO 输出 |
| 按键 NEXT | PB0 | EXTI0（上下沿中断，上拉） |
| 按键 ACT | PB1 | EXTI1（上下沿中断，上拉） |
| 调试/VOFA+ 串口 | PA9(TX)/PA10(RX) | USART1 (AF7), 115200 |

时钟：HSE 8 MHz → PLL → SYSCLK 168 MHz（APB1=42 MHz，APB2=84 MHz）。

---

## 2. 目录结构

```
STM32F407_FreeRTOS/
├── BNO086_MotionDash.ioc           # CubeMX 工程（参考/重新生成用）
├── MDK-ARM/
│   ├── BNO086_MotionDash.uvprojx   # Keil 工程（AC6/armclang，可直接编译）
│   └── BNO086_MotionDash.sct       # 散列加载文件（512KB flash / 128KB RAM）
├── Core/
│   ├── Inc/  (main.h, stm32f4xx_hal_conf.h, stm32f4xx_it.h, FreeRTOSConfig.h)
│   └── Src/  (main.c, stm32f4xx_it.c, stm32f4xx_hal_msp.c,
│              stm32f4xx_hal_timebase_tim.c, system_stm32f4xx.c)
│   └── Startup/ (startup_stm32f407xx.s, ARM 语法，AC6 自动识别 armasm)
├── Drivers/
│   ├── STM32F4xx_HAL_Driver/       # STM32CubeF4 V1.28.3 HAL
│   ├── CMSIS/                      # CMSIS Core + Device
│   └── BSP/
│       ├── bno08x/                 # BNO086 驱动 (SH2/SHTP + HAL I2C 适配)
│       ├── st7789/                 # ST7789 驱动 + 5x7 字库
│       └── console/                # UART 调试 / VOFA+ 输出
├── App/                            # 应用层（原 .ino 的状态机 + 页面 + 按键）
└── Middlewares/FreeRTOS/           # FreeRTOS V10.3.1 (GCC/ARM_CM4F 移植)
```

---

## 3. 直接编译（推荐）

1. 用 Keil uVision5（需已安装 **ARM Compiler 6 / armclang**，MDK 5.28 及以上默认自带）
   打开 `MDK-ARM/BNO086_MotionDash.uvprojx`。
2. 首次打开若提示设备包（DFP），确认已安装 `Keil.STM32F4xx_DFP`（任意较新版本即可）。
3. 直接 `Build`（F7）。生成 `Objects/BNO086_MotionDash.axf/.hex`。
4. 用 ST-Link/J-Link 或串口 ISP 烧录；`Options → Debug` 已按 ST-Link 默认配置，
   可按需改为自己的调试器。

> 说明：工程已自带 HAL/CMSIS/FreeRTOS 源码，**不依赖 CubeMX 也能直接编译**。

---

## 4. 用 CubeMX 重新生成（可选）

`.ioc` 复现了上面的引脚/时钟/外设配置，可用来重新生成 HAL 骨架：

1. CubeMX 打开 `BNO086_MotionDash.ioc`，`Project Manager` 里 Toolchain 选
   `MDK-ARM`，点 `GENERATE CODE`。
2. 生成后 CubeMX 会覆盖 `Core/` 并生成自己的 `freertos.c`（创建两个任务）。
   - 把 `Drivers/BSP/` 与 `App/` 加入生成工程的对应分组；
   - 将生成的 `freertos.c` 中两个任务的执行体改为调用
     `app_imu_task(arg)` / `app_ui_task(arg)`（或直接用本仓库自带的
     `main.c` 里的任务创建代码，见第 6 节）；
   - 在 `stm32f4xx_it.c` 的 `EXTI0_IRQHandler/EXTI1_IRQHandler` 里调用
     `btnNextISR()` / `btnActISR()`。

> 最简单的方式仍是直接使用本仓库自带的 Keil 工程；`.ioc` 主要用于改引脚时重新生成。

---

## 5. FreeRTOS 架构

- **IMU 任务 `imu`（优先级 3）**：轮询 BNO086（每次最多 `IMU_POLL_MAX_PER_LOOP` 个事件），
  处理来自 UI 的命令（切换 report 订阅 / Tare / 睡眠 / 唤醒），并负责
  “I2C 错误自愈”和“长时间无数据 → 总线恢复 + 重新订阅”看门狗。
- **UI 任务 `ui`（优先级 2）**：完整复刻原 `loop()` 状态机
  （`APP_MENU/APP_PAGE/APP_ABOUT/APP_SLEEP`）+ 显示刷新 + VOFA+ 输出。
  按键通过二元信号量唤醒，显示按 `DISPLAY_REFRESH_MS=100ms` 节拍刷新。
- **按键**：PB0/PB1 配 EXTI 上下沿中断，ISR 内做消抖/长短按判定，
  `xSemaphoreGiveFromISR` 唤醒 UI 任务（消抖 150ms、长按 600ms，与原工程一致）。
- **共享数据**：`IMUSnapshot` 由互斥锁保护（IMU 任务写、UI 任务读）。
- **HAL 时基**：FreeRTOS 占用 SysTick，HAL 的 `HAL_Delay/HAL_GetTick` 由
  **TIM6** 提供（`stm32f4xx_hal_timebase_tim.c`）。

---

## 6. 关键移植对照

| 原 Arduino | 移植后 |
|---|---|
| `SoftWire`(软件 I2C) | HAL 硬件 I2C1（100kHz，`NoStretch=DISABLE` 允许时钟拉伸） |
| `imuWire.i2cBusRecovery()`(打 SCL 脉冲) | `bno_i2c_recover()`：复位 I2C1 外设 + 位带 SCL 脉冲 + 重初始化 |
| `imuWire.sclStretchTimeout` 自愈 | I2C 事务超时置 `s_i2c_error`，IMU 任务检测后自愈 |
| SparkFun `BNO08x` + SH2/SHTP | 原样移植 SH2 内核(sh2/shtp/sh2_util/sh2_SensorValue) + `sh2_hal.c`(HAL I2C) + `bno08x` 包装类 |
| `Adafruit_ST7789` + `Adafruit_GFX` | `st7789` 驱动 + 内置 5x7 字库，文字度量与 Adafruit 逐像素一致 |
| `millis()` | `HAL_GetTick()`（TIM6 时基） |
| `delay()` | 调度器启动前 `HAL_Delay`，启动后 `vTaskDelay` |
| `Serial.print` / `String` | `uart_console.c` 的 `dbg()`/`vofaSend3/4()` + 定长 `char[]` 缓存 |
| `attachInterrupt(CHANGE)` | EXTI 上下沿 + `Button::onInterrupt()` |

---

## 7. 串口输出（调试 + VOFA+）

- USART1，115200-8-N-1。
- 调试日志：`[TAG] DEBUG: ...`（原工程的 `[TAG]调试: ...` 改为 ASCII 以免 Keil 源码编码问题）。
- VOFA+ Firewater 协议：欧拉角页输出 `roll,pitch,yaw`（角度，1 位小数），
  旋转矢量页输出 `i,j,k,real`（4 位小数），只在收到新 report 时输出一次。

---

## 8. 注意事项

- 编译目标为 **AC6 (armclang)**（MDK 5.28+ 默认自带；语言标准 C99 + C++03）。
  若使用 AC5(armcc)，需改用 `portable/RVDS/ARM_CM4F` 的 FreeRTOS 移植（本仓库未包含 AC5 版本）。
- `BNO086 INT`(PC6) 为**轮询**输入（与原工程一致），不占用 EXTI。
- 若换用 0x4A 地址的 BNO086，改 `main.h` 的 `BNO_I2C_ADDR` 与 `Config.h` 即可。
- 本移植遵循原工程版权说明，仅限个人非商业学习使用。
