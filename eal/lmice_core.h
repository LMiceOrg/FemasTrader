#ifndef LMICE_CORE_H
#define LMICE_CORE_H
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include "lmice_eal_common.h"
#include "lmice_trace.h"

struct lmice_environment_s
{
    void* addr;
    void* board;
    int32_t pid;
    int32_t ppid;
    FILE* logfd;
    uint32_t padding0;
    uint32_t lcore;
    uint32_t memory;
    uint32_t net_bandwidth;
};
typedef struct lmice_environment_s lmice_environment_t;



#if defined(_WIN32)

#include <Iphlpapi.h>
/* Library iphlpapi.lib */
forceinline int eal_get_net_bandwidth(uint32_t *net_bandwidth)
{
    DWORD dret = 0;
    DWORD buff_len = 0;
    IP_INTERFACE_INFO *iif = NULL;
    MIB_IFROW row;
    LONG i = 0;

    dret = GetInterfaceInfo(NULL, &buff_len);
    if(dret == ERROR_INSUFFICIENT_BUFFER) {
        iif = (PIP_INTERFACE_INFO)malloc(buff_len);
        GetInterfaceInfo(iif, &buff_len);
    } else {
        lmice_critical_print("get_net_bandwidth call GetInterfaceInfo failed[%u]\n", dret );
        return 1;
    }

    for(i=0; i< iif->NumAdapters; ++i) {

        row.dwIndex = iif->Adapter[i].Index;
        if(GetIfEntry(&row) == NO_ERROR) {
            /*
            _wprintf_p(L"Name: %s\t\n", row.wszName);
            printf("Desc:%s\n[%d] State:%u\tMtu:%u\tSpeed:%u\n\n", row.bDescr, iif->Adapter[i].Index,
                   row.dwOperStatus, row.dwMtu, row.dwSpeed);
            */
            *net_bandwidth = row.dwSpeed;
            break;
        }
    }
    free(iif);
    return 0;
}

forceinline void eal_core_get_properties(uint32_t* lcore, uint32_t* mem, uint32_t* net_bandwidth)
{
    ULONGLONG memkilo = 0;
    SYSTEM_INFO mySysInfo;

    /* get lcore num */
    GetSystemInfo(&mySysInfo);
    *lcore = mySysInfo.dwNumberOfProcessors;

    /* get memory in Milibytes */
    GetPhysicallyInstalledSystemMemory(&memkilo);
    *mem = memkilo / 1024;

    /* get device [0] bandwidth */
    eal_get_net_bandwidth(net_bandwidth);

}
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


forceinline void eal_core_get_net_bandwidth(const char* nif, uint32_t* bandwidth) {
    int ret;
    FILE* fp = NULL;
    char cmd[64];
    char mode[2];
    char* p;
    double rate;
    memset(cmd, 0, 64);
    snprintf(cmd, 63, "ifconfig -v %s|grep -e \"link\\srate\"", nif);
    mode[0] = 'r';
    mode[1] = '\0';
    fp = popen(cmd, mode);
    if(fp == NULL)
    {
        ret = errno;
        lmice_critical_print("get_net_bandwidth call popen(%s) failed[%d]\n",cmd, ret );
    }
    memset(cmd, 0, 64);
    fread(cmd, 1, 64, fp);
    rate = atof(cmd+12);
    for(p=cmd+12; p!= '\0'; ++p)
    {
        if(strncmp(p,"Gbps", 4) == 0 ||
                strncmp(p,"gbps", 4) == 0) {
            rate *= 1024*1024;
            break;
        }else if(strncmp(p, "Mbps", 4) == 0 ||
                 strncmp(p, "mbps", 4) == 0) {
            rate *= 1024;
            break;
        } else if(strncmp(p, "Kbps", 4) == 0 ||
                  strncmp(p, "kbps", 4) == 0) {
            break;
        }
    }
    *bandwidth = (uint32_t)rate;
    /*
    lmice_critical_print("link rate result :%s\t%lf\n", cmd+12, rate );
    */
    pclose(fp);

}

forceinline void eal_core_get_properties(uint32_t* lcore, uint32_t* mem, uint32_t* net_bandwidth)
{
    int ret;
    int mib[4];
    uint64_t memsize = 0;
    size_t len = 4;


    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;
    len =4;
    ret = sysctl(mib, 2, lcore, &len, NULL, 0);

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    len = 8;
    sysctl(mib, 2, &memsize, &len, NULL, 0);
    if(ret != -1) {
        *mem = (uint32_t) (memsize / (1024*1024) );
    }

    eal_core_get_net_bandwidth("en0", net_bandwidth);
}

#elif defined(__linux__)

#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/ethtool.h>
#include <string.h>
#include <stdlib.h>

forceinline int get_net_bandwidth(const char* nif, uint32_t* bandwidth) {
    int sock;
        struct ifreq ifr;
        struct ethtool_cmd edata;
        int rc;

        sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            lmice_error_print("get_net_bandwidth call socket(DGRAM) failed\n");
            return 1;
        }

        strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
        ifr.ifr_data = &edata;

        edata.cmd = ETHTOOL_GSET;

        rc = ioctl(sock, SIOCETHTOOL, &ifr);
        if (rc < 0) {
            lmice_error_print("get_net_bandwidth call ioctl(SIOCETHTOOL) failed\n");
            return 1;
        }
        switch (ethtool_cmd_speed(&edata)) {
            case SPEED_10: *bandwidth = 10*1024; break;
            case SPEED_100: *bandwidth = 100*1024; break;
            case SPEED_1000: *bandwidth = 1000*1024; break;
            case SPEED_2500: *bandwidth = 2500*1024; break;
            case SPEED_10000: *bandwidth = 100000*1024; break;
            default: *bandwidth = 0;
        }

        return (0);
}

#else

#error("No implementation of core functionalities.")
#endif


int eal_env_init(lmice_environment_t* env);

/**
 * @brief lmice_process_cmdline
 * @param argc arg count
 * @param argv arv list
 * @return 0 if success;else error occurred.
 */
int lmice_process_cmdline(int argc, char * const *argv);
int lmice_process_signal(lmice_environment_t* env);
int lmice_exec_command(lmice_environment_t* env);
int lmice_master_init(lmice_environment_t* env);
int lmice_signal_init(lmice_environment_t* env);
void lmice_master_loop(lmice_environment_t* env);
#endif /** LMICE_CORE_H */
