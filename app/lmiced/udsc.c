#include "udss.h"

#include "lmice_trace.h"
#include "eal/lmice_eal_time.h"

#include <errno.h>

#define SOCK_FILE "/var/run/lmiced.socket"

int main(int argc, char* argv[]) {
    int err = 0;
    int ret;
    int len;
    lmice_sub_t*psub;
    lmice_trace_info_t *info;
    uds_msg* pmsg;
    create_uds_msg(&pmsg);

    ret = init_uds_client(SOCK_FILE, pmsg);
    lmice_critical_print("Init socket local %d\n", pmsg->sock);
    lmice_critical_print("Init socket local %s\n", pmsg->local_un.sun_path);
    lmice_critical_print("Init socket remote %s\n", pmsg->remote_un.sun_path);
    {
        int64_t tm;
        get_system_time(&tm);
        lmice_warning_print("Time:%lld\n", tm);
    }
    psub = (lmice_sub_t*)pmsg->data;
    info = &(psub->info);
    info->pid = getpid();
    info->tid = eal_gettid();
    time(&info->tm);
    get_system_time(&info->systime);
    info->type = EM_LMICE_SUB_TYPE;
    sprintf(psub->symbol, "cu1701");

    pmsg->size = sizeof(lmice_sub_t);

    lmice_info_print("Sub cu1701\n");
    ret = sendto(pmsg->sock, pmsg->data, pmsg->size, 0, (struct sockaddr*)&(pmsg->remote_un), sizeof pmsg->remote_un);
    if(ret == -1) {
        err = errno;
        lmice_critical_print("sendto failed [%d].\n", err);
    }
    lmice_info_print("Sub cu1701\n");
    ret = sendto(pmsg->sock, pmsg->data, pmsg->size, 0, (struct sockaddr*)&(pmsg->remote_un), sizeof pmsg->remote_un);
    if(ret == -1) {
        err = errno;
        lmice_critical_print("sendto failed [%d].\n", err);
    }
//    {

//        ret = recvfrom(pmsg->sock, pmsg->data, sizeof pmsg->data, 0,(struct sockaddr*)&(pmsg->remote_un), &len);
//        lmice_info_print("recvfrom size[%d] done[%d]\n",len, ret);

//    }

    finit_uds_msg(pmsg);
    {
        int64_t tm;
        get_system_time(&tm);
        lmice_warning_print("Time:%lld\n", tm);
    }

    return 0;
}
