#include "lmspi.h"

#include "lmice_trace.h"
#include "lmice_eal_time.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
void callback (const char* symbol, const void* addr, int32_t size)
{
int64_t t = 0;
        int64_t t2 = 0;
        get_system_time(&t);
        sscanf((const char*)addr, "shm send at %ld\n", &t2);
        lmice_warning_print("C:%s[%lu][%lx] recv %d  timediff=%ld\n", symbol, (uint64_t)addr%8, addr, size, t-t2);
    //lmice_info_print("callback symbol %s as size %d, at %lx\n", symbol, size, addr);
    //lmice_warning_print("%s data is: %s\n",symbol, (const char*)addr);
    //sscanf((const char*)addr, "shm send at %ld\n", &t2);
    //lmice_warning_print("The fun callback send at %ld\n", t);
    //lmice_warning_print("G:%s[%lu][%u] send %ld, recv %ld, total:%ld\n", symbol, (uint64_t)addr%8, size, t2, t, t-t2);
    lmice_info_print("global C callback %s\n", (const char*) addr);
}

class test :public CLMSpi
{
private:
    double m_f;
public:
    test():CLMSpi("test", 0)
    {}
    double f() {
        return m_f;
    }
    void f(double d) {
        m_f=d;
    }

    void cb(const char* symbol, const void* addr, int size) {

        int64_t t = 0;
        int64_t t2 = 0;
        get_system_time(&t);
	sscanf((const char*)addr, "shm send at %ld\n", &t2);
        lmice_warning_print("C:%s[%lu][%lx] recv %d  timediff=%ld\n", symbol, (uint64_t)addr%8, addr, size, t-t2);
    }
};

int main() {
    test spi;
    //CLMSpi spi("DemoModel");
    lmice_info_print("sub demo\n");
    spi.subscribe("[netmd]rb1610");
    sleep(1);
    //lmice_info_print("pub demo\n");
    spi.publish("[netmd]rb1610");
    spi.f(123);

    lmice_info_print("reg callback\n");
    //spi.register_cb((csymbol_callback)&test::cb, "[netmd]rb1610");
    spi.register_callback(callback, "[netmd]rb1610");
    sleep(1);
    //return 0;
    lmice_info_print("senddata demo\n");
    for(size_t i=0; i< 10; ++i) {
        char buff[64];
        int64_t t;
        int size;
        get_system_time(&t);
        memset(buff, 0, 64);
        size = sprintf(buff, "shm send at %ld\n", t);

        spi.send("[netmd]rb1610", buff, size);
        sleep(1);
    }

    printf("Press Ctrl+C to quit.\n");
    spi.join();

    spi.delete_spi();

    return 0;

}
