#include <lmspi.h>  /* LMICED spi */
#include <pcap.h>   /* libpcap  */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <eal/lmice_trace.h>    /* EAL tracing */
#include <eal/lmice_bloomfilter.h> /* EAL Bloomfilter */
#include <eal/lmice_eal_hash.h>
#include <eal/lmice_eal_thread.h>

#include "guavaproto.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#if defined(__linux__)
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>

static int netmd_set_cpuset(int* cpuid, int count) {
    int i;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for(i=0; i<count; ++i) {
        CPU_SET(*(cpuid+i), &mask );
    }
    return sched_setaffinity(0, sizeof(mask), &mask );
}
#endif
pcap_t* pcapHandle = NULL;

lm_bloomfilter_t* bflter = NULL;


/* netmd package callback */
static void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet);

/* netmd worker thread */
static int netmd_pcap_thread(lmspi_t spi, const char *devname, const char *packet_filter);

/* stop netmd worker */
static void netmd_pcap_stop(int sig);

/* publish data */
forceinline void netmd_pub_data(lmspi_t spi, const char* symbol, const void* addr, int len);

/* create bloom filter for net md */
static void netmd_bf_create(void);

/* delete bloom filter */
static void netmd_bf_delete(void);

/* print usage of app */
static void print_usage(void);

/* Daemon */
static int init_daemon(int silent);

static int position = 11;
static int bytes = sizeof(struct guava_udp_normal)+sizeof(struct guava_udp_head);
static char md_name[32];
#define MAX_KEY_LENGTH 512
static uint64_t keylist[MAX_KEY_LENGTH];
static volatile int keypos;

int main(int argc, char* argv[]) {
    lmspi_t spi;
    uid_t uid = -1;
    int ret;
    int i;
    int silent = 0;

    int mc_sock = 0;
    int mc_port = 30100;/* 5001*/
    char mc_group[32]="233.54.1.100"; /*238.0.1.2*/
    char mc_bindip[32]="192.168.208.16";/*10.10.21.191*/

    char devname[64] = "p6p1";
    char filter[128] = "udp and port 30100";

    (void)argc;
    (void)argv;

    keypos = 0;
    memset(md_name, 0, sizeof(md_name));
    strncpy(md_name, "[netmd]", sizeof(md_name)-1);

    /** Process command line */
    for(i=0; i< argc; ++i) {
        char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            return 0;
        } else if(strcmp(cmd, "-n") == 0 ||
                  strcmp(cmd, "--name") == 0) {
            if(i+1<argc) {
                cmd=argv[i+1];
                memset(md_name, 0, sizeof(md_name));
                strncat(md_name, cmd, sizeof(md_name)-1);
            } else {
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-d") == 0 ||
                  strcmp(cmd, "--device") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                if(strlen(cmd)>63) {
                    lmice_error_print("Adapter device name is too long(>63)\n");
                    return 1;
                }
                memset(devname, 0, sizeof(devname));
                strncat(devname, cmd, sizeof(devname)-1);
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-s") == 0 ||
                  strcmp(cmd, "--silent") == 0) {
            silent = 1;
        } else if(strcmp(cmd, "-u") == 0 ||
                  strcmp(cmd, "--uid") == 0) {
            if(i+1<argc){
                cmd = argv[i+1];
                uid = atoi(cmd);
                ret = seteuid(uid);
                if(ret != 0) {
                    ret = errno;
                    lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
                    return ret;
                }
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-f") == 0 ||
                  strcmp(cmd, "--filter") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                memset(filter, 0, sizeof(filter));
                ret = strlen(cmd);
                if(ret > 128) {
                    lmice_error_print("filter string is too long(>127)\n");
                    return ret;
                }
                strncpy(filter, cmd, ret);
            } else {
                lmice_error_print("Command(%s) require filter string\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-p") == 0 ||
                  strcmp(cmd, "--position") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                position = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require position number\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-b") == 0 ||
                  strcmp(cmd, "--bytes") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                bytes = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require package size number\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-m") == 0 ||
                 strcmp(cmd, "--multicast") == 0) {
            if(i+3<argc) {
                cmd = argv[i+1];
                memset(mc_group, 0, sizeof(mc_group));
                strncpy(mc_group, cmd, 32);
                cmd=argv[i+2];
                memset(mc_bindip, 0, sizeof(mc_bindip));
                strncpy(mc_bindip, cmd, 32);
                cmd=argv[i+3];
                mc_port = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require mc_group, bind_ip port param(3)\n", cmd);
                return 1;
            }
            i+=3;
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
                    while(*cmd != NULL) {
                        if(*cmd != ',') {
                            ++cmd;
                        } else {
                            ++cmd;
                            break;
                        }
                    }
                } while(*cmd != NULL);

                ret = netmd_set_cpuset(cpuset, setcount);
                lmice_critical_print("set CPUset %d return %d\n", setcount, ret);

            } else {
                lmice_error_print("Command(%s) require cpuset param, separated by comma(,)\n", cmd);
            }
            ++i;
        }
    } /* end-for: argc*/

    lmice_critical_print("NetMD -- a md app --\n");

    /** Silence mode */
    init_daemon(silent);

    /** Join MCast group */
    {
        struct ip_mreq mreq;
        struct sockaddr_in local_addr;
        int flag=1;
        mc_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(mc_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(mc_port);	//multicast port
        bind(mc_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));


        mreq.imr_multiaddr.s_addr = inet_addr(mc_group);	//multicast group ip
        mreq.imr_interface.s_addr = inet_addr(mc_bindip);

        setsockopt(mc_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    }

    /** Create LMiced spi */
    spi = lmspi_create(md_name, -1);
    lmice_info_print("%s startting in adapter[%s]. filter[%s]..\n", md_name, devname, filter);
    if(uid != -1) {
        ret = seteuid(getuid());
        if(ret != 0) {
            ret = errno;
            lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
            return ret;
        }
    }

    /** Create bloom filter */
    //netmd_bf_create();

    /** Register signal handler */
    lmspi_signal(spi, netmd_pcap_stop);



    /** Run in main thread */
    netmd_pcap_thread(spi, devname, filter);
    /*
     * for(ret=0; ret<10; ++ret) {
        char buff[32] = {0};
        sprintf(buff, "data is %d\n", ret);
        netmd_pub_data(spi, "rb1610", buff, 32);
        usleep(500000);
    }
    */

    /** Exit and maintain resource */
    lmspi_quit(spi);
    lmspi_delete(spi);
    //netmd_bf_delete();

    lmice_critical_print("%s exit\n", md_name);


    return 0;
}


int init_daemon(int silent) {
    if(!silent)
        return 0;

    if( daemon(0, 1) != 0) {
        return -2;
    }
    return 0;
}

void print_usage(void) {
    printf("NetMD -- a md app --\n\n"
           "\t-h, --help\t\tshow this message\n"
           "\t-n, --name\t\tset module name\n"
           "\t-d, --device\t\tset adapter device name\n"
           "\t-u, --uid\t\tset user id when running\n"
           "\t-s, --silent\t\trun in silent mode[backend]\n"
           "\t-f, --filter\t\tfilter to run pcap\n"
           "\t-p, --position\t\tposition to catch symbol\n"
           "\t-b, --bytes\t\tpackage size limitation\n"
           "\t-m, --multicast\t\tmulticast group, bind ip, port\n"
           "\t-c, --cpuset\t\tSet cpuset for this app(, separated)\n"
           "\n"
           );
}

void netmd_bf_create(void) {
    uint64_t n = MAX_KEY_LENGTH;
    uint32_t m = 0;
    uint32_t k = 0;
    double f = 0.0001;
    /* MAX_KEY_LENGTH items, and with 0.0001 false positive */
    eal_bf_calculate(n, f, &m, &k, &f);

    bflter = (lm_bloomfilter_t*)malloc(sizeof(lm_bloomfilter_t)+m);
    memset(bflter, 0, sizeof(lm_bloomfilter_t)+m);
    bflter->n = n;
    bflter->f = f;
    bflter->m = m;
    bflter->k = k;
    bflter->addr = (char*)bflter+ sizeof(lm_bloomfilter_t);

    lmice_critical_print("Bloomfilter initialized:\n\tn:%lu\n\tm:%u\n\tk:%u\n\tf:%5.15lf\n",
                         n,m,k,f);

}

void netmd_bf_delete(void) {
    free(bflter);
    bflter = NULL;
}


/* netmd worker thread */
int netmd_pcap_thread(lmspi_t spi, const char *devname, const char* packet_filter) {
    pcap_t *adhandle;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_int netmask =0xffffff;
    /* setup the package filter  */
    /* const char packet_filter[] = "udp and port 30100"; */
    struct bpf_program fcode;

    /* Open the adapter */
    if ( (adhandle= pcap_open_live(devname, // name of the device
                                   65536, // portion of the packet to capture.
                                   // 65536 grants that the whole packet will be captured on all the MACs.
                                   1, // promiscuous mode
                                   1000, // read timeout
                                   errbuf // error buffer
                                   ) ) == NULL)
    {
        lmice_error_print("\nUnable to open the adapter[%s], as %s\n", devname, errbuf);
        return -1;
    } else {
        lmice_info_print("pcap_open_live done\n");
    }

    /*set buffer size 1MB, use default */
    pcap_set_buffer_size(adhandle, 1024*1024);


    /*compile the filter*/
    if(pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) <0 ){
        lmice_error_print("\nUnable to compile the packet filter. Check the syntax.\n");
        return -1;
    }
    /*set the filter*/
    if(pcap_setfilter(adhandle, &fcode)<0){
        lmice_error_print("\nError setting the filter.\n");
        return -1;
    }

    pcapHandle = adhandle;

    /* start the capture */
    lmice_critical_print("pcap startting...\n");
    pcap_loop(adhandle, -1, netmd_pcap_callback, (u_char*)spi);
    lmice_critical_print("pcap stopped.\n");
}

void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet) {
    eal_bf_hash_val key;
    char symbol[32] = {0};
    lmspi_t spi = (lmspi_t)arg;
    const char* data = (const char*)(packet+42);
    struct guava_udp_head* gh = (struct guava_udp_head*)data;
    /*
     * struct guava_udp_head* gh = (struct guava_udp_head*)(packet+42);
    struct guava_udp_normal *gn = (struct guava_udp_normal*)(packet+42+sizeof(guava_udp_head));
    struct guava_udp_summary *gs = (struct guava_udp_summary*)(packet+42+sizeof(guava_udp_head));
    */

    (void)pkthdr;
    (void)arg;

    /** check package state */
    if(pkthdr->len < 42+bytes )
        return;

    /** process guava head */
    switch(gh->m_quote_flag) {
    case 0x0:   /*0 无time sale, 无lev1 */
        break;
    case 0x1:   /*1 有time sale 无lev1 */
        break;
    case 0x2:   /*2 无time sale, 有lev1 */
        break;
    case 0x3:   /*3 有time sale, 有lev1 */
        break;
    case 0x4:   /*4 summary信息 */
        break;
    default:
        break;
    }


    /** pub data */
    netmd_pub_data(spi, data + position, data, pkthdr->len - 42);


}

void netmd_pcap_stop(int sig) {
    if(pcapHandle) {
        pcap_breakloop(pcapHandle);
        pcapHandle = NULL;
    }
}

static int key_compare(const void* key, const void* obj) {
    const uint64_t* hval = (const uint64_t*)key;
    const uint64_t* val = (const uint64_t*)obj;

    if(hval == val)
        return 0;
    else if(hval < val)
        return -1;
    else
        return 1;
}

forceinline int key_find_or_create(const char* symbol) {
    uint64_t hval;
    uint64_t *key;
    hval = eal_hash64_fnv1a(symbol, 32);
    key = (uint64_t*)bsearch(&hval, keylist, keypos, 8, key_compare);
    if(key == NULL) {
        /* create new element */
        if(keypos < MAX_KEY_LENGTH) {
            keylist[keypos] = hval;
            ++keypos;
            mergesort(keylist, keypos, 8, key_compare);
            return 1;
        } else {
            lmice_warning_print("Add key[%s] failed as list is full\n", symbol);
            return -1;
        }
    }

    return 0;

}

forceinline void netmd_pub_data(lmspi_t spi, const char* sym, const void* addr, int len) {
    char symbol[32] = {0};
    int ret;

    /* construct symbol */
    strncat(symbol, md_name, 16);
    strncat(symbol, sym, 16);

    /* calc bf key */
    ret = key_find_or_create(symbol);
    if(ret == 0) {
        lmspi_send(spi, symbol, addr, len);
    } else if(ret == 1) {
        lmspi_publish(spi, symbol);
        lmspi_send(spi, symbol, addr, len);
    }

}
