// sudo python -m serial.tools.miniterm /dev/ttyUSB0 76800
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "debug.h"
#include "network_manager.h"

static void ICACHE_FLASH_ATTR initDone(void)
{
    nmInit();
}

//Init function 
void ICACHE_FLASH_ATTR
user_init(void)
{
    init_debug_blink();

    nmConfig();

    // Call init_done method after system init is done
    system_init_done_cb(initDone);
}
