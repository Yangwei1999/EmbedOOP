# AgentAI 工程说明

## 项目概览

这是一个基于 `STM32F407` 的嵌入式实验工程，当前主要包含两部分内容：

- 基于 `W25Q64JV` 的外部 Flash 组件接入
- 一套使用 C 语言模拟面向对象的 LED 抽象设计

工程的重点不只是“点亮一个 LED”，而是探索这样一个问题：

如果一个 MCU 上存在多种不同实现方式的 LED 设备，例如：

- 直接通过 `GPIO` 控制的 LED
- 通过 `PWM` 占空比控制亮度的 LED

那么如何让业务层只关心“这是一个 LED”，而不关心底层到底是 GPIO 还是 PWM？

当前代码给出的答案是：

- 抽取一个公共基类 `led_base`
- 为不同硬件实现定义不同“子类”
- 通过 `ops` 方法表完成统一的 `on/off` 调用
- 让业务层只依赖 `led_base *`

---

## 工程结构

下面是当前工程中和设计相关的主要目录：

```text
AgentAI/
├── Components/
│   └── W25Q64JV/              # W25Q64JV Flash 组件
├── Core/
│   ├── Inc/
│   │   ├── led_base.h         # LED 基类与方法表定义
│   │   ├── led_gpio.h         # GPIO LED 类型定义
│   │   ├── led_pwm.h          # PWM LED 类型定义
│   │   ├── gpio.h             # GPIO 初始化声明
│   │   ├── tim.h              # TIM/PWM 初始化声明
│   │   └── main.h
│   └── Src/
│       ├── main.c             # 主流程、统一 LED 调用、.my_init 演示
│       ├── led_base.c         # 基类公共逻辑
│       ├── led_gpio.c         # GPIO LED 实现
│       ├── led_pwm.c          # PWM LED 实现
│       ├── gpio.c             # GPIO 外设初始化
│       ├── tim.c              # TIM/PWM 外设初始化
│       └── TopAlarm.c         # 上层模块占位文件
├── docs/
│   ├── W25Q64JV_notes.md      # Flash 相关记录
│   └── refactor_history.md    # 提交重构历史整理
├── STM32F407XX_FLASH.ld       # 链接脚本，包含 .my_init 段
├── CMakeLists.txt             # 顶层 CMake
└── AgentAI.ioc                # CubeMX 工程
```

---

## 设计目标

这个工程里的 LED 抽象主要解决 3 个问题：

### 1. 避免为每个 LED 重复写一套函数

如果直接写：

```c
void led1_on(void);
void led1_off(void);
void led2_on(void);
void led2_off(void);
```

那么每增加一个 LED，就要增加一组几乎重复的代码，问题在于：

- 控制逻辑重复
- 硬件信息被写死
- 业务代码与底层硬件强耦合

### 2. 允许不同硬件实现共存

LED 不一定都由 GPIO 控制，也可能通过 PWM 控制亮度。

因此需要一种方式，让下面两种设备都能接入同一套上层模型：

- `led_gpio_t`
- `led_pwm_t`

### 3. 让业务层只面向抽象编程

业务层真正关心的是：

- 告警灯亮
- 错误灯灭

而不是：

- 这个灯在哪个 GPIO 口
- 这个灯使用的是哪个 TIM 通道

所以底层需要在初始化时完成“设备与硬件的绑定”，上层只保留抽象对象指针。

---

## LED 抽象设计

### 第一层：GPIO LED 的最小抽象

如果只考虑 GPIO LED，可以把硬件信息封装为一个结构体：

```c
typedef struct {
    GPIO_TypeDef *port_addr;
    uint32_t pin_num;
    uint8_t on_state;
} led_gpio_t;
```

然后提供通用控制函数：

```c
void led_gpio_on(led_gpio_t *led);
void led_gpio_off(led_gpio_t *led);
```

这样做的价值是：

- 不再把端口和引脚写死在函数名里
- 一个函数可以控制多个不同实例
- 新增 LED 只需要新增对象，不需要复制逻辑

---

### 第二层：抽取 LED 的公共属性

随着 `GPIO LED` 和 `PWM LED` 同时出现，可以发现有些属性与硬件类型无关，例如：

- 名字 `name`
- 状态 `state`

因此当前工程中定义了基类 [`Core/Inc/led_base.h`](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/Core/Inc/led_base.h)：

```c
struct led_base {
    char *name;
    uint8_t state;
    led_ops *ops;
};
```

这里的 `led_base` 表达的是“一个 LED 设备的共性”，而不是某种具体实现。

对应的两个具体设备类型为：

```c
typedef struct {
    led_base base;
    GPIO_TypeDef *port_addr;
    uint32_t pin_num;
    uint8_t on_state;
} led_gpio_t;

typedef struct {
    led_base base;
    uint32_t brightness;
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} led_pwm_t;
```

这种写法本质上是在 C 语言中模拟“继承”：

- `led_base` 是基类
- `led_gpio_t` / `led_pwm_t` 是派生类型
- `base` 作为首成员，便于统一转换和上层持有

---

### 第三层：使用方法表统一行为

仅仅有基类还不够，因为 GPIO LED 和 PWM LED 的控制方式不同：

- GPIO LED 天然支持 `on/off`
- PWM LED 更适合 `set_brightness`

为了解决这个问题，工程引入了 `led_ops` 方法表：

```c
typedef void (*led_on_fn)(led_base *led);
typedef void (*led_off_fn)(led_base *led);
typedef void (*led_set_brightness)(led_base *led, uint32_t brightness);

struct led_ops {
    led_on_fn on;
    led_off_fn off;
    led_set_brightness set_brightness;
};
```

不同设备各自绑定不同的实现：

```c
static led_ops gpio_ops = {
    .on = led_gpio_on,
    .off = led_gpio_off,
    .set_brightness = NULL,
};

static led_ops pwm_ops = {
    .on = NULL,
    .off = NULL,
    .set_brightness = led_pwm_set,
};
```

在各自的初始化函数中绑定到基类：

- `led_gpio_init()` 中绑定 `gpio_ops`
- `led_pwm_init()` 中绑定 `pwm_ops`

这样，上层只要拿到 `led_base *`，就可以通过统一入口操作设备。

---

### 第四层：统一的 `led_on()` / `led_off()`

当前 [`Core/Src/main.c`](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/Core/Src/main.c) 中实现了统一入口：

```c
void led_on(led_base *led) {
    led_ops *ops = led->ops;
    if (ops->on != NULL) {
        ops->on(led);
        return;
    }

    if (ops->set_brightness != NULL) {
        ops->set_brightness(led, 100);
        return;
    }
}
```

`led_off()` 逻辑类似：

- 如果设备提供 `off`，直接调用
- 否则如果设备支持 `set_brightness`，则用 `0` 模拟关闭

这意味着业务层可以统一写成：

```c
led_on(g_led_alram);
led_off(g_led_error);
```

而不需要关心：

- `g_led_alram` 实际是 `led_gpio_t`
- `g_led_error` 实际是 `led_pwm_t`

---

## 当前代码中的初始化方式

当前 `main.c` 中，两个全局抽象指针分别表示业务层看到的两个 LED：

```c
led_base *g_led_error = NULL;
led_base *g_led_alram = NULL;
```

初始化时先创建具体设备对象，再把其 `base` 暴露给上层：

```c
led_gpio_t led_green = {0};
led_pwm_t led_pwm2;

g_led_alram = &led_green.base;
g_led_error = &led_pwm2.base;

led_gpio_init(&led_green, "led_green", LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
led_pwm_init(&led_pwm2, "led_pwm2", &htim4, TIM_CHANNEL_1, 10);
```

这种组织方式有两个好处：

- 硬件绑定只发生在板级初始化阶段
- 上层逻辑只持有 `led_base *`

如果以后要把告警灯从 GPIO 改成 PWM，理论上只需要替换绑定对象，而不需要大改业务调用。

---

## 通过 `__containerof` 从基类反推子类

因为 `led_base` 是子结构体的第一个成员，所以可以在需要时从基类指针恢复出真实类型。

当前代码中有示例：

```c
led_gpio_t *test = __containerof(g_led_alram, led_gpio_t, base);
printf(" gpio is %u \r\n", test->pin_num);
```

这个技巧的意义是：

- 上层默认只依赖抽象
- 当某些模块确实需要访问底层特有字段时，仍然可以安全回退到具体类型

这种写法和 Linux 内核中的对象组织方式很接近。

---

## PWM LED 的实现思路

PWM LED 在当前工程中的核心结构为：

```c
typedef struct {
    led_base base;
    uint32_t brightness;
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} led_pwm_t;
```

亮度设置函数定义在 [`Core/Src/led_pwm.c`](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/Core/Src/led_pwm.c)：

```c
void led_pwm_set(led_pwm_t *led_pwm, uint32_t brightness) {
    uint32_t duty = brightness * 10;
    __HAL_TIM_SET_COMPARE(led_pwm->htim, led_pwm->channel, duty);
}
```

这里的约定是把亮度近似看作 `0~100`，再映射到当前定时器周期配置下的比较值。

当前 `tim.c` 中已经配置了：

- `TIM2`
- `TIM4`

其中 `main.c` 里实际使用的是 `TIM4_CH1`。

---

## `.my_init` 机制说明

除了 LED 抽象外，这个工程还实验了一个类似 Linux `module_init` 的自动注册思路。

核心思想是：

- 把若干函数指针放到链接脚本指定的段里
- 程序启动后遍历这段地址范围
- 自动执行其中注册的函数

当前链接脚本 [`STM32F407XX_FLASH.ld`](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/STM32F407XX_FLASH.ld) 中定义了 `.my_init` 段：

```ld
.my_init :
{
  . = ALIGN(4);
  _smytest = .;
  KEEP(*(.my_init*))
  . = ALIGN(4);
  _emytest = .;
} >FLASH
```

在 `main.c` 中，把函数数组放进这个段：

```c
typedef void (*print_test)(void);

void print_1(void) { ... }
void print_2(void) { ... }

__attribute__((section(".my_init"))) print_test key[2] = {print_1, print_2};
```

然后通过 `_smytest` 和 `_emytest` 遍历调用：

```c
print_test *p = &_smytest;
for (uint32_t i = 0; i < (&_emytest - &_smytest); i++) {
    p[i]();
}
```

这个机制适合后续扩展为：

- 自动驱动注册
- 自动初始化任务
- 板级模块自注册

目前工程里仍以实验和验证思路为主。

---

## 当前主要源码说明

### `Core/Src/main.c`

主要负责：

- HAL 初始化
- GPIO / SPI / UART / TIM 初始化
- 创建 LED 实例
- 绑定业务层抽象指针
- 演示 `.my_init` 自动执行
- 在循环中统一控制 LED

### `Core/Src/led_base.c`

主要负责：

- 初始化基类公共字段
- 销毁基类状态
- 获取 LED 名称

### `Core/Src/led_gpio.c`

主要负责：

- GPIO LED 的硬件绑定
- GPIO LED 的 `on/off`
- 绑定 `gpio_ops`

### `Core/Src/led_pwm.c`

主要负责：

- PWM LED 的硬件绑定
- 亮度设置
- 启动 PWM
- 绑定 `pwm_ops`

### `Core/Src/TopAlarm.c`

当前仅作为上层模块占位文件，说明设计方向已经在尝试从 `main.c` 中继续分离业务层逻辑，但目前尚未形成完整实现。

---

## 当前状态与说明

从当前代码来看，这个工程更偏向“架构验证”和“设计实验”，而不是一个已经完全收口的产品级框架。

目前比较明确的状态有：

- LED 的抽象链路已经基本打通
- GPIO LED 和 PWM LED 都已经有独立实现
- 上层通过 `led_base *` 统一调用的思路已经成立
- `.my_init` 段注册机制已经有最小演示

同时也可以看到一些仍在演进中的痕迹：

- `Core/Inc/led_base_general.h` 目前基本是历史过渡文件
- `TopAlarm.c` 仍是占位状态
- `main.c` 中还保留了较多实验性打印与注释
- `led_base.state`、`led_pwm_t.brightness` 等字段的状态同步逻辑还比较简单

这也是这个工程的特点：它非常适合继续作为“嵌入式面向对象抽象实验场”来迭代。

---

## 一个简化理解

如果用一句话概括当前的 LED 设计，可以理解为：

1. 用 `led_base` 表达“所有 LED 的共性”
2. 用 `led_gpio_t` 和 `led_pwm_t` 表达“不同硬件实现”
3. 用 `led_ops` 表达“这个对象该怎么执行 on/off/brightness”
4. 用 `led_base *` 让业务层只依赖抽象

也就是把：

- “如何控制这盏灯”

和

- “这盏灯在业务里扮演什么角色”

拆成了两个层次。

---

## 相关文档

- 原始设计记录：[Readme.md](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/Readme.md)
- 提交重构梳理：[docs/refactor_history.md](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/docs/refactor_history.md)
- W25Q64JV 记录：[docs/W25Q64JV_notes.md](/Users/yangwei/Desktop/STM32/AIAsisFlash/AgentAI/docs/W25Q64JV_notes.md)

---

## 后续可继续完善的方向

- 把 `led_on()` / `led_off()` 从 `main.c` 下沉到独立模块
- 给 `TopAlarm.c` 补上真正的业务层状态机或告警控制逻辑
- 进一步规范 `state` 与 `brightness` 的同步语义
- 为 `.my_init` 封装统一的注册宏，而不是手写段属性
- 补充更明确的板级初始化与调用示例

这份 `readme2.md` 的目标是把当前代码中的设计思路、目录结构和关键机制梳理清楚，便于后续继续演进，而不替代你原始 `Readme.md` 里的思考痕迹。
