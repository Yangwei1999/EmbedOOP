## 考虑如果场景、一个 mcu 中有两个led1、led2，现在要去点灯，如何处理

```C
void led1_on()
{
    HAL_GPIO_WritePin(led1_port, led1_pin, 0);
}
void led1_off()
{
    HAL_GPIO_WritePin(led1_port, led1_pin, 1);
}
void led2_on()
{
    HAL_GPIO_WritePin(led1_port, led1_pin, 0);
}
void led2_off()
{
    HAL_GPIO_WritePin(led1_port, led1_pin, 1);
}
```

如果再有第三个led、实际上上面的代码又需要重复一遍，问题的原因是代码跟硬件写死了

比较好的修改方式是

```C
typedef struct {
	GPIO_TypeDef *port_addr;
	uint32_t pin_num;
	uint8_t on_state;
} led_gpio_t;

void led_gpio_on(led_gpio_t *led)
{
    HAL_GPIO_WritePin(led->port_addr, led->pin_num, led->on_state);
}
void led_gpio_off(led_gpio_t *led)
{
    HAL_GPIO_WritePin(led->port_addr, led->pin_num, 1 - led->on_state);
}
led_gpio_t led1 = {x, x, x};
led_gpio_on(&led1);
```

## 再考虑一个场景，如果led不是通过gpio控制，而是通过pwm波控制

```C
typedef struct {
	uint32_t brightness;
	TIM_HandleTypeDef *htim;
	uint32_t channel;
} led_pwm_t;

相同的pwm的处理方法 on/off 参考占空比为100/0
void led_pwm_set(led_pwm_t *led_pwm, uint32_t brightness) {
	uint32_t duty = brightness * 10;
	__HAL_TIM_SET_COMPARE(led_pwm->htim, led_pwm->channel, duty);
}

led_pwm_t led2 = {x, x, x};
led_pwm_on(&led2);
```

## 上述led结构体都是和硬件相关，我们加一些与硬件无关的标识（后面会用到、业务层感知）

```C
typedef struct {
    char *name;
    uint32_t state;
	GPIO_TypeDef *port_addr;
	uint32_t pin_num;
	uint8_t on_state;
} led_gpio_t;

typedef struct {
    char *name;
    uint32_t state;
	uint32_t brightness;
	TIM_HandleTypeDef *htim;
	uint32_t channel;
} led_pwm_t;

led_gpio_t led1 = {};
led_pwm_t led2={};

led_gpio_init(); 设备跟硬件绑定，上层只感知设备、不感知硬件
led_pwm_init(); 设备跟硬件绑定
```

实际上name 和 state 是与硬件无关的，是所有led设备都有的属性, 可以定义成一个基类

```C
struct led_base {
	char *name;
	uint8_t state;
};

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

子类init函数调用父类init函数 + 自己特有的部分
```

## 现在点on off 仍然需要不同的调用函数，例如

led_pwm_on/off led_gpio_on/off, 能否统一一个on/off 函数

可以简单写一个这样的函数，利用函数指针实现运行时绑定地址

```C
void on(funcptr func1, funcptr func2) {
    funt1();
}
```

更好的写法是，我们定义一个led_ops结构体，里面定义一些方法

/* 3. 函数指针定义（可以用前向声明的类型） */
typedef void (*led_on_fn)(led_base *led);
typedef void (*led_off_fn)(led_base *led);
typedef void (*led_set_brightness)(led_base *led, uint32_t brightness);

/* 4. 定义 led_ops */
struct led_ops {
led_on_fn on;
led_off_fn off;
led_set_brightness set_brightness;
};

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

这样
void led_on(led_base *led, led_ops* ops)
{
    ops->on(led);
}

进一步归一，把opts也放在led_base中，那么有

struct led_base {
    char *name;
    uint8_t state;
    led_ops *ops;
};

ops是在 led_pwm_init/led_gpio_init 中进行绑定、也就是子类初始化时进行绑定

这样
void led_on(led_base *led)
{
    基类对象不关注ops如何实现、进一步抽象
    led->ops->on(led);
}

## 业务层不关注具体硬件信息

```C
	g_led_alram = &led_green.base;
	g_led_error = &led_pwm2.base;
    // 硬件信息在板级初始化时绑定、
	led_gpio_init(&led_green, "led_green", LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
	led_pwm_init(&led_pwm2, "led_pwm2", &htim4, TIM_CHANNEL_1, 10);
	
	业务层点灯只需要
	led_on(g_led_alram)
	
	如果业务层需要拿下层信息、可以使用contain0_of . 第一个参数是基类对象、第二个是子类结构体声明、第三个是基础类成员名字
    led_gpio_t *test = __containerof(g_led_alram, led_gpio_t, base);
	printf(" gpio is %u \r\n", test->pin_num);
```

## 如果现在alarm led 换成pwm灯

仅需要修改一行
g_led_alram = &led_pwm.base;
led_pwm_init(&led_pwm, "led_pwm", &htim4, TIM_CHANNEL_1, 10);  // 这里完成硬件绑定


## 如果实现linux中的module_init的，使得上电自动执行操作

原理是把一系列函数地址放在某个固定位置，在初始化函数中遍历执行
.my_init :
{
. = ALIGN(4);
_smytest = .;
/*  *(.my_init)           /* .text sections (code) */*/
/*  KEEP(*(.my_init))*/
KEEP(*(.my_init*))
/*  *(.my_init*)          /* .text* sections (code) */*/

. = ALIGN(4);
_emytest = .;
} >FLASH

void print_1(void) {
printf("asdsad111\r\n");
}

void print_2(void) {
printf("asdsad2222\r\n");
}

__attribute__((section(".my_init"))) print_test key[2] = {print_1, print_2};

extern  uint32_t _smytest;
extern  uint32_t _emytest;

for (uint32_t i = 0; i <(&_emytest - &_smytest) ; i++) {
    p[i]();
}