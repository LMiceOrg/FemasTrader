#include "lmspi.h"
#include <time.h>
void callback (const char* symbol, const void* addr, uint32_t size)
{
    int64_t t;
    get_system_time(&t);
    lmice_info_print("callback symbol %s as size %d, at %lx\n", symbol, size, addr);
    lmice_warning_print("The data is: %s\n", addr);
    lmice_warning_print("The fun callback send at %ld\n", t);
}

int main() {
    CLMSpi spi("DemoModel");
    lmice_info_print("sub demo\n");
    spi.subscribe("demo");
    sleep(1);
    lmice_info_print("pub demo\n");
    spi.publish("demo");

    lmice_info_print("reg callback\n");
    spi.register_callback(callback);
    sleep(1);
    usleep(100000);
    lmice_info_print("senddata demo\n");
    for(size_t i=0; i< 10; ++i) {
    spi.send("demo", NULL, 0);
    sleep(1);
    }

    return 0;

}
