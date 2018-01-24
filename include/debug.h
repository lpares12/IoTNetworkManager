/**
* @file
* @author  Lluc Pares
* @date    18/01/2018
* @brief   ESP8266 debugging method
*
* API used for debugging with ESP8266. To make sure the device
* hasn't crashed
**/

#include "user_interface.h"

/**
 * @brief Method used for debugging. Blinks LED in GPIO2 to
 * make sure we can make sure esp8266 is active.
 */
static void ICACHE_FLASH_ATTR blink_cb(void *arg);

void ICACHE_FLASH_ATTR init_debug_blink();