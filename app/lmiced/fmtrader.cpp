#include "USTPFtdcTraderApi.h"
#include "fmspi.h"

#include "udss.h"

#include "lmice_trace.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include<signal.h>

#include <unistd.h>
#include <sys/types.h>

#define SOCK_FILE "/var/run/lmiced.socket"


volatile int g_quit_flag = 0;

void signal_handler(int sig) {
    if(sig == SIGTERM || sig == SIGINT)
        g_quit_flag = 1;
}

class CClient {
public:
    CClient() {
        create_uds_msg((void**)&sid);
        init_uds_client(SOCK_FILE, sid);
        logging("LMice Femas Trader running %d:%d", getuid(), getpid());
    }

    ~CClient() {
        logging("LMice Femas Trader stopped.\n\n");
        finit_uds_msg(sid);
    }

    void logging(const char* format, ...) {
        va_list argptr;

        va_start(argptr, format);
        sid->size = vsprintf(sid->data, format, argptr);
        va_end(argptr);

        send_uds_msg(sid);
    }

    void showtips(void) {
        int ver[2];
        const char* sver;

        sver = CUstpFtdcTraderApi::GetVersion(ver[0], ver[1]);
        lmice_critical_print("Femas Trader (%s) version:%d.%d", sver, ver[0], ver[1]);
        logging("Femas Trader (%s) version:%d.%d", sver, ver[0], ver[1]);
    }

private:
    uds_msg* sid;
};



int main(int argc, char* argv[]) {
    int ret = 0;

    // 产生一个 CUstpFtdcTraderApi 实例
    CUstpFtdcTraderApi *pTrader = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
    g_puserapi=pTrader;

    CTraderSpi spi(pTrader);
    spi.Init();

    /** 注册一事件处理的实例 */
    pTrader->RegisterSpi(&spi);

    /** 设置飞马平台服务的地址 */
    // 可以注册多个地址备用
    spi.logging("register front addr: %s", spi.GetFrontAddr());
    pTrader->RegisterFront(spi.GetFrontAddr());
    /** 订阅私有流 */
    // USTP_TERT_RESTART:从本交易日开始重传
    // USTP_TERT_RESUME:从上次收到的续传
    // USTP_TERT_QUICK:只传送登录后私有流的内容
    pTrader->SubscribePrivateTopic(USTP_TERT_QUICK);
    /** 订阅公共流 */
    // USTP_TERT_RESTART:从本交易日开始重传
    // USTP_TERT_RESUME:从上次收到的续传
    // USTP_TERT_QUICK:只传送登录后公共流的内容
    pTrader->SubscribePublicTopic(USTP_TERT_QUICK);


    /** 使客户端开始与后台服务建立连接 */
    spi.logging("Init trader\n");
    const fmconf_t* cfg = spi.GetConf();
    lmice_warning_print("UserID=[%s],BrokerID=[%s],Password=[%s]\n", cfg->g_UserID,cfg->g_BrokerID,cfg->g_Password);
    pTrader->Init();



    /** Listen and wait to end */
    signal(SIGINT, signal_handler);
    /*signal(SIGCHLD,SIG_IGN);  ignore child */
    /* signal(SIGTSTP,SIG_IGN);  ignore tty signals */
    signal(SIGTERM,signal_handler); /* catch kill signal */

    while(1) {
        if(g_quit_flag == 1) {
            break;
        }
        usleep(500000);
    }

    pTrader->Release();

    lmice_warning_print("Release\n");

    return 0;
}
