#ifndef LMSPI_H
#define LMSPI_H
/*

#pragma execution_character_set("utf-8")
#pragma setlocale("chs")

*/

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


#ifdef __cplusplus
#include <string>
extern "C" {
#endif


/* C subscribe callback type */
typedef void(*symbol_callback)(const char* symbol, const void* addr, int size);

/* C interface type*/
typedef void* lmspi_t;

SPICFUN lmspi_t lmspi_create(const char* name, int poolsize);
SPICFUN void lmspi_delete(lmspi_t spi);

// 资源注册
SPICFUN int lmspi_publish(lmspi_t spi, const char* symbol);
SPICFUN int lmspi_subscribe(lmspi_t spi, const char* symbol);
// 资源注销
SPICFUN int lmspi_unpublish(lmspi_t spi, const char* symbol);
SPICFUN int lmspi_unsubscribe(lmspi_t spi, const char* symbol);

// 基于资源的接收回调函数管理
SPICFUN int lmspi_register_recv(lmspi_t spi, const char* symbol, symbol_callback *callback);
SPICFUN int lmspi_unregister_recv(lmspi_t spi, const char* symbol);


// 阻塞运行与资源回收
SPICFUN int lmspi_join(lmspi_t spi);

// 退出运行
SPICFUN void lmspi_quit(lmspi_t spi);

// 日志
SPICFUN void lmspi_logging(lmspi_t spi, const char* format, ...);

// 发布数据
SPICFUN void lmspi_send(lmspi_t spi, const char* symbol, const void* addr, int len);

// 信号函数
SPICFUN void lmspi_signal(lmspi_t spi, sig_t sigfunc);

//设置进程亲缘性
SPICFUN int lmspi_cpuset(lmspi_t spi, const int* cpuset, int cpucount);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* C++ subscribe callback */
class CLMSpi;
typedef void (CLMSpi::*csymbol_callback)(const char* symbol, const void* addr, int size);

/* LMice daemon SPI
 * A discrete event dynamic system service provider interface
 *
 * poolsize: 0(default)
 * single-thread callback SPI
 * callback in single thread, thread-safe
 * main-thread: init and pub/sub, join or not
 * spi-thread: event-wait and callback
 *
 * poolsize: -1
 * in thread SPI
 * callback within the main thread
 * work:main thread
 * stop:signal(interrupt or terminate)
 *
 * main-thread: init, pub/sub, event-wait and callback
 *
 * poolsize : >1
 * multi-threaded callback SPI
 * callback(s) in multiple threads, thread-safe and re-entry
 * work: event thread, and worker thread
 * stop: signal or quit
 *
 * main-thread: init, pub/sub
 * spi-thread:  event-wait and in-queue
 * work-thread[ps]: de-dqueue, callback
 */
class CLMSpi
{
public:
    CLMSpi(const char* name="Model", int poolsize=0);
    void delete_spi(void);
    //virtual ~CLMSpi();

    void logging(const char* format, ...);

    std::string gbktoutf8(char *pgbk);

    void subscribe(const char* symbol);

    void unsubscribe(const char* symbol);

    void publish(const char* symbol);

    void unpublish(const char* symbol);

    void send(const char* symbol, const void* addr, int len);

    void get_symbol(const char* symbol, const void* * addr, int* count);

    int register_callback(symbol_callback func, const char* symbol = NULL);

    int register_cb(csymbol_callback func, const char* symbol = NULL) ;

    void register_signal(sig_t sigfunc);

    int join();

    int quit();

    int isquit();

    int set_cpuset(const int* cpuset, int setcount);



private:
    void* m_priv;
};

#endif

#endif // LMSPI_H
