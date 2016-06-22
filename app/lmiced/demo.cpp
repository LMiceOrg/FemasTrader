#include "lmspi.h"

#include <time.h>
void callback (const char* symbol, const void* addr, uint32_t size)
{
    int64_t t;
    int64_t t2;
    get_system_time(&t);
    //lmice_info_print("callback symbol %s as size %d, at %lx\n", symbol, size, addr);
    //lmice_warning_print("%s data is: %s\n",symbol, (const char*)addr);
    sscanf((const char*)addr, "shm send at %ld\n", &t2);
    //lmice_warning_print("The fun callback send at %ld\n", t);
    lmice_warning_print("%s[%lu][%u] send %ld, recv %ld, total:%ld\n", symbol, (uint64_t)addr%8, size, t2, t, t-t2);
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
        char buff[64];
        int64_t t;
        int ret;
        get_system_time(&t);
        memset(buff, 0, 64);
        ret = sprintf(buff, "shm send at %ld\n", t);

        spi.send2("demo", buff, ret);
        sleep(1);
    }

    return 0;

}
