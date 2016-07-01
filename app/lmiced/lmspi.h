#ifndef LMSPI_H
#define LMSPI_H

#include <stdarg.h>
#include <stdint.h>

#include <string>

typedef void(*symbol_callback)(const char* symbol, const void* addr, int size);
class CLMSpi;
typedef void (CLMSpi::*csymbol_callback)(const char* symbol, const void* addr, int size);

class CLMSpi
{
public:
    CLMSpi(const char* name="Model", int poolsize=0);
    virtual ~CLMSpi();

    void logging(const char* format, ...);

    std::string gbktoutf8(char *pgbk);

    void subscribe(const char* symbol);

    void unsubscribe(const char* symbol);

    void publish(const char* symbol);

    void send(const char* symbol, const void* addr, int len);

    int register_callback(symbol_callback func, const char* symbol = NULL, void *udata = NULL);

    int register_cb(csymbol_callback func, const char* symbol = NULL) ;

    int join();

    int quit();

    int isquit();



private:
    void* m_priv;
};

#endif // LMSPI_H
