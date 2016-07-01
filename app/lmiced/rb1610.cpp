#include "lmspi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lmice_eal_time.h"
#include "lmice_trace.h"
#include <time.h>
void callback (const char* symbol, const void* addr, int32_t size)
{
    int64_t t;
    get_system_time(&t);
    lmice_warning_print("%s[%lu] send: %s\n", symbol, t, (char *)addr);
}

int main() {
    CLMSpi spi("DemoModel");
    lmice_info_print("sub rb1610\n");
    spi.subscribe("[md]rb1610");
    sleep(1);
    lmice_info_print("reg callback\n");
    spi.register_callback(callback);
    sleep(1);

    printf("\nPress Ctrl+C to quit\n");
    spi.join();
    return 0;

}
