#include "debug.h"

#include "osapi.h"
#include "os_type.h"
#include "gpio.h"


// Variables for blinking LED
static os_timer_t blink_timer;
static uint8_t led_state=0;

static void ICACHE_FLASH_ATTR blink_cb(void *arg)
{
    led_state = !led_state;
    GPIO_OUTPUT_SET(2, led_state);
}

void ICACHE_FLASH_ATTR init_debug_blink()
{
	// Initialize the GPIO subsystem.
    gpio_init();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

    os_timer_disarm(&blink_timer);
    os_timer_setfn(&blink_timer, (os_timer_func_t *)blink_cb, (void *)0);
    os_timer_arm(&blink_timer, 1000, 1);
}