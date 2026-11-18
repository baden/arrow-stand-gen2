/**
 * @file    main.c
 * @brief   Entry point of the arrow-stand-gen2 target firmware.
 *
 * Flow:
 *   init -> wait for impact -> send packet -> sleep -> repeat
 *
 * The application logic lives in app.c; this file only wires it to the HAL.
 */

#include "config.h"
#include "app.h"

int main(void)
{
    app_init();

    for (;;) {
        app_run_once();
    }
}
