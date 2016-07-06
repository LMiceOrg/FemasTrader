#include <lmspi.h>  /* LMICED spi */
#include <pcap.h>   /* libpcap  */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <eal/lmice_trace.h>    /* EAL tracing */
#include <eal/lmice_bloomfilter.h> /* EAL Bloomfilter */

#include "guavaproto.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

pcap_t* pcapHandle = NULL;

lm_bloomfilter_t* bflter = NULL;

/* netmd package callback */
void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet);

/* netmd worker thread */
int netmd_pcap_thread(lmspi_t spi, const char *devname, const char *packet_filter);

/* stop netmd worker */
void netmd_pcap_stop(int sig);

/* publish data */
forceinline void netmd_pub_data(lmspi_t spi, const char* symbol, const void* addr, int len);

/* create bloom filter for net md */
void netmd_bf_create(void);

/* delete bloom filter */
void netmd_bf_delete(void);

/* print usage of app */
void print_usage(void);

/* Daemon */
int init_daemon(int silent);

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
           "\t-d, --device\t\tset adapter device name\n"
           "\t-u, --uid\t\tset user id when running\n"
           "\t-s, --silent\t\trun in silent mode[backend]\n"
           "\t-f, --filter\t\tfilter to run pcap\n"
           "\t-p, --position\t\tposition to catch symbol\n"
           "\t-b, --bytes\t\tpackage size limitation\n"
           "\n"
           );
}

void netmd_bf_create(void) {
    uint64_t n = 512;
    uint32_t m = 0;
    uint32_t k = 0;
    double f = 0.0001;
    /* 512 items, and with 0.0001 false positive */
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

static int position = 11;
static int bytes = sizeof(struct guava_udp_normal)+sizeof(struct guava_udp_head);

int main(int argc, char* argv[]) {
    lmspi_t spi;
    uid_t uid = -1;
    int ret;
    int i;
    int silent = 0;

    int m_sock;

    char devname[64] = "p6p1";
    char filter[128] = "udp and port 30100";


    (void)argc;
    (void)argv;

    {
        struct ip_mreq mreq;
        struct sockaddr_in local_addr;
        int flag=1;
        m_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(5001);	//multicast port
        bind(m_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));


        mreq.imr_multiaddr.s_addr = inet_addr("238.0.1.2");	//multicast group ip
        mreq.imr_interface.s_addr = inet_addr("10.10.21.191");

        setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    }

    /** Process command line */
    for(i=0; i< argc; ++i) {
        char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            return 0;
        } else if(strcmp(cmd, "-d") == 0 ||
                  strcmp(cmd, "--device") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                if(strlen(cmd)>63) {
                    lmice_error_print("Adapter device name is too long(>63)\n");
                    return 1;
                }
                memset(devname, 0, 64);
                strncpy(devname, cmd, strlen(cmd));
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
        }
    } /* end-for: argc*/

    lmice_critical_print("NetMD -- a md app --\n");

    /** Silence mode */
    init_daemon(silent);

    /** Create LMiced spi */
    spi = lmspi_create("[md]netmd", -1);
    lmice_info_print("[md]netmd startting in adapter[%s]. filter[%s]..\n", devname, filter);
    if(uid != -1) {
        ret = seteuid(getuid());
        if(ret != 0) {
            ret = errno;
            lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
            return ret;
        }
    }

    /** Create bloom filter */
    netmd_bf_create();

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
    netmd_bf_delete();

    lmice_critical_print("[md]netmd exit\n");


    return 0;
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

forceinline void netmd_pub_data(lmspi_t spi, const char* sym, const void* addr, int len) {
    char symbol[32] = {0};
    eal_bf_hash_val key;

    /* construct symbol */
    strcat(symbol, "[netmd]");
    strcat(symbol, sym);

    /* calc bf key */
    eal_bf_key(bflter, symbol, 32, key);

    /* check if it's already published */
    if(eal_bf_find(bflter, key) == 0) {
        lmspi_send(spi, symbol, addr, len);
    } else {
        /* publish the new symbol */
        lmspi_publish(spi, symbol);
        eal_bf_add(bflter, key);
        usleep(1000);
        lmspi_send(spi, symbol, addr, len);
    }
}
