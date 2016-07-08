#include "fm2spi.h"
#include "lmice_trace.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

/* print useage of the app */
static void print_usage();

int main(int argc, char* argv) {
    int ret = 0;
    int i;
    char* cmd;
    char front_address[64] = "10.0.10.0";
    char user[64] = "";
    char password[64] = "";
    char broker[64] = "";
    char model_name[32]="fm2trader";
    char *investor = user;

    /// 解析命令行
    for(i=1; i<argc; ++i) {
        cmd = argv[i];
        if(strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            return 0;
        } else if(strcmp(cmd, "-f") == 0 ||
                  strcmp(cmd, "--front") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(front_address, 0, sizeof(front_address));
                strncpy(front_address, cmd, sizeof(front_address)-1);
            } else {
                lmice_error_print("Command(%s) require front address string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-u") == 0 ||
                  strcmp(cmd, "--user") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(user, 0, sizeof(user));
                strncpy(user, cmd, sizeof(user)-1);
                //投资人ID＝ 用户ID［去除前面的交易商ID］
                investor = user + strlen(broker);
            } else {
                lmice_error_print("Command(%s) require user id string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-p") == 0 ||
                  strcmp(cmd, "--password") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(password, 0, sizeof(password));
                strncpy(password, cmd, sizeof(password)-1);
            } else {
                lmice_error_print("Command(%s) require password string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-b") == 0 ||
                  strcmp(cmd, "--broker") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(broker, 0, sizeof(broker));
                strncpy(broker, cmd, sizeof(broker)-1);
                //投资人ID＝ 用户ID［去除前面的交易商ID］
                investor = user + strlen(broker);
            } else {
                lmice_error_print("Command(%s) require broker string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-n") == 0 ||
                  strcmp(cmd, "--name") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(model_name, 0, sizeof(model_name));
                strncpy(model_name, cmd, sizeof(model_name)-1);
            } else {
                lmice_error_print("Command(%s) require model name string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-c") == 0 ||
                  strcmp(cmd, "--cpuset") == 0) {
            if(i+1 < argc) {
                int cpuset[8];
                int setcount = 0;
                cmd = argv[i+1];

                do {
                    if(setcount >=8)
                        break;
                    cpuset[setcount] = atoi(cmd);
                    ++setcount;
                    while(*cmd != 0) {
                        if(*cmd != ',') {
                            ++cmd;
                        } else {
                            ++cmd;
                            break;
                        }
                    }
                } while(*cmd != 0);

                ret = lmspi_cpuset(NULL, cpuset, setcount);
                lmice_critical_print("set CPUset %d return %d\n", setcount, ret);

            } else {
                lmice_error_print("Command(%s) require cpuset param, separated by comma(,)\n", cmd);
            }
            ++i;
        }
    }

    /// 注册SPI

    // 产生一个 CUstpFtdcTraderApi 实例
    CUstpFtdcTraderApi *pt = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
    //产生一个事件处理的实例
    CFemas2TraderSpi * spi = new CFemas2TraderSpi(pt);

    // 设置交易信息
    spi->user_id(user);
    spi->password(password);
    spi->broker_id(broker);
    spi->investor_id(investor);
    spi->front_address(front_address);
    spi->model_name(model_name);

    spi->init_trader();

    // 注册一事件处理的实例
    pt->RegisterSpi(spi);

    // 订阅私有流
    //        TERT_RESTART:从本交易日开始重传
    //        TERT_RESUME:从上次收到的续传
    //        TERT_QUICK:只传送登录后私有流的内容
    pt->SubscribePrivateTopic(  USTP_TERT_QUICK);
    pt->SubscribePublicTopic (  USTP_TERT_QUICK);

    //设置心跳超时时间
    pt->SetHeartbeatTimeout(5);

    //注册前置机网络地址
    pt->RegisterFront(front_address);

    //初始化
    pt->Init();

    /// 开始运行
    spi->join();

    // 释放useapi实例
    pt->Release();

    //释放SPI实例
    delete spi;

    return 0;
}


void print_usage() {
    printf("fm2trader:\t\t-- Femas2.0 Trader app --\n"
           "\t-h, --help\t\tshow this message\n"
           "\t-n, --name\t\tset module name\n"
           "\t-f, --front\t\tset front address\n"
           "\t-u, --user\t\tset user id \n"
           "\t-p, --password\t\tset password\n"
           "\t-b, --broker\t\tset borker id\n"
           "\t-c, --cpuset\t\tset cpuset \n"
           )
}
