#ifndef LMTSPI_H
#define LMTSPI_H

#if defined(_WIN32) /* Windows */

#if defined(LMSPI_PROJECT)
#define SPICFUN __declspec(dllexport)
#else
#define SPICFUN __declspec(dllimport)
#endif

#elif defined(__MACH__) || defined(__linux__)

#if defined(LMSPI_PROJECT)
#define SPICFUNv __attribute__((visibility("default")))
#else
#define SPICFUN __attribute__((visibility("hidden")))
#endif

#endif

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>

#include "lmspi.h"

#ifdef __cplusplus

/* multi-threads SPI */
class CLMTSpi
{
public:
    void create(const char* name="Model", int poolsize=0);
    void destroy(void);

    void logging(const char* format, ...);

    void subscribe(const char* symbol);

    void unsubscribe(const char* symbol);

    void publish(const char* symbol);

    void unpublish(const char* symbol);

    void send(const char* symbol, const void* addr, int len);

    int register_callback(symbol_callback func, const char* symbol = NULL);

    int register_cb(csymbol_callback func, const char* symbol = NULL) ;

    void register_signal(sig_t sigfunc);

    int join();

    int quit();

    int isquit();



private:
    void* m_priv;
};
#endif

#endif /** LMTSPI_H */
