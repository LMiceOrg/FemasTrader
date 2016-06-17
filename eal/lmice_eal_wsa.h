#ifndef LMICE_EAL_WSA_H
#define LMICE_EAL_WSA_H

/* Windows Socket Library API */

#include "lmice_trace.h"
#include "lmice_eal_common.h"
#include "lmice_eal_hash.h"
#include "lmice_eal_iocp.h"

#include <stdint.h>

#define EAL_WSA_INVALID_INSTID (uint64_t)-1

enum eal_esa_net_tcp
{
    EAL_WSA_TCP_CLIENT_MODE,
    EAL_WSA_TCP_SERVER_MODE,
    EAL_WSA_UDP_CLIENT_MODE,
    EAL_WSA_UDP_SERVER_MODE,
    EAL_WSA_MULTICAST_MODE
};

struct eal_wsa_handle_s
{
    uint64_t inst_id;
    SOCKET nfd;
    int addrlen;
#if defined(WIN64)
    int padding0;
#endif
    SOCKADDR_STORAGE addr;
};
typedef struct eal_wsa_handle_s eal_wsa_handle;

#define EAL_WSA_HANDLE_LIST_LENGTH 128
struct eal_wsa_handle_list_s
{
    volatile int64_t lock;
    struct eal_wsa_handle_list_s *next;
#if !defined(_WIN64)
    uint32_t padding0;
#endif
    eal_wsa_handle data[EAL_WSA_HANDLE_LIST_LENGTH];
};
typedef struct eal_wsa_handle_list_s eal_wsa_handle_list;


forceinline int eal_wsa_create_handle_list(eal_wsa_handle_list** pb)
{
    *pb = (eal_wsa_handle_list*)malloc( sizeof(eal_wsa_handle_list) );
    if(*pb == NULL)
        return -1;
    memset(*pb, 0, sizeof(eal_wsa_handle_list));
    return 0;
}

forceinline void eal_wsa_destroy_handle_list(eal_wsa_handle_list* pb)
{
    eal_wsa_handle_list *next = NULL;
    do {
        next = pb->next;
        free(pb);
        pb = next;
    } while(pb != NULL);
}


forceinline int eal_wsa_append_handle(eal_wsa_handle_list* pb, uint64_t inst_id, eal_wsa_handle** data)
{
    size_t i=0;
    eal_wsa_handle_list *next = NULL;
    eal_wsa_handle* cur = NULL;
    do {

        next = pb;
        for(i=0; i< EAL_WSA_HANDLE_LIST_LENGTH; ++i)
        {
            cur = &(pb->data[i]);
            if(cur->inst_id == 0)
            {
                memset(cur, 0, sizeof(eal_wsa_handle));
                cur->inst_id = inst_id;
                cur->addrlen = sizeof(cur->addr);
                *data = cur;
                return 0;
            }
        }
        pb = next->next;
    } while(pb != NULL);


    if(pb == NULL)
    {
        eal_wsa_create_handle_list(&pb);
        next->next = pb;

        cur = &(pb->data[0]);
        memset(cur, 0, sizeof(eal_wsa_handle));
        cur->inst_id = inst_id;
        cur->addrlen = sizeof(cur->addr);
        *data = cur;

    }

    return 0;

}

forceinline void eal_wsa_remove_handle(eal_wsa_handle* bt)
{
    bt->inst_id = 0;
}


struct eal_wsa_service_param_s
{
    /* [in] 0 client-mode, 1 server-mode  */
    int mode;
    /* [in/out] ipv4, ipv6 */
    int inet;
    /* processed by getaddrinfo-bind routine
     * [in] local socket port
     * [in] local socket address
     * Empty - any available port
     *       - any available address
     */
    char local_port[8];
    char local_addr[64];
    /* processed by
     * TCP-connect routine,
     * UDP-send routine
     * MC-add-membership routine
     * [in] remote socket port
     * [in] remote socket address
     * Empty - any available port
     *       - any available address
     */
    char remote_port[8];
    char remote_addr[64];

    /* [in] global wsa handle list */
    eal_wsa_handle_list *hlist;
    /* [in] global iocp data list */
    eal_iocp_data_list *ilist;
    /* [out] the wsa handle to be created from global list*/
    eal_wsa_handle* hd ;
    /* [out] the iocp data to be created from global list */
    eal_iocp_data* data;
    /* [in] the i/o event-process handle */
    HANDLE  cp;

};
typedef struct eal_wsa_service_param_s eal_wsa_service_param;



typedef DWORD  (WINAPI *eal_wsa_tcp_accept_thread_proc_t) ( LPVOID lpParameter);

DWORD WINAPI eal_wsa_tcp_accept_thread_proc( LPVOID lpParameter);

forceinline int eal_create_tcp_accept_thread(eal_wsa_service_param * pm,
                                                    eal_wsa_tcp_accept_thread_proc_t thread_proc)
{
    int ret = 0;
    HANDLE thd;
    thd = CreateThread(NULL,
                       0,
                       thread_proc,
                       pm,
                       0,
                       NULL);
    if(thd == NULL)
    {
        ret = 1;
    }
    CloseHandle(thd);
    return ret;
}

forceinline void eal_wsa_hash(int mode, const char* local_addr, int laddr_len,
                                     char* local_port,  int lport_len,
                                     char* remote_addr, int raddr_len,
                                     char* remote_port, int rport_len, uint64_t* inst_id)
{
    *inst_id = eal_hash64_fnv1a(&mode, sizeof(mode));
    *inst_id = eal_hash64_more_fnv1a(local_addr, laddr_len,  *inst_id);
    *inst_id = eal_hash64_more_fnv1a(local_port, lport_len,  *inst_id);
    *inst_id = eal_hash64_more_fnv1a(remote_addr, raddr_len, *inst_id);
    *inst_id = eal_hash64_more_fnv1a(remote_port, rport_len, *inst_id);

}

forceinline int eal_wsa_tcp_connect(eal_wsa_service_param* pm)
{
    UNREFERENCED_PARAM(pm);
}


forceinline int eal_wsa_create_tcp_service(eal_wsa_service_param* pm)
{
    int ret = 0;
    struct addrinfo *local = NULL;
    struct addrinfo *remote = NULL;
    struct addrinfo *rp = NULL;
    struct addrinfo *bp = NULL;
    struct addrinfo hints;
    HANDLE hdl = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* TCP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // 创建用来和套接字关联的单句柄数据信息结构
    eal_wsa_append_handle(pm->hlist, EAL_WSA_INVALID_INSTID, &pm->hd);

    /* getaddrinfo() returns a list of address structures.
                 Try each address until we successfully bind(2).
                 If socket(2) (or bind(2)) fails, we (close the socket
                 and) try the next address.
    */
    ret = getaddrinfo(pm->local_addr, pm->local_port, &hints, &local);
    if (ret != 0) {
        lmice_error_print("getaddrinfo[%s]: %d\n", pm->local_addr, ret);
        eal_wsa_remove_handle(pm->hd);
        return -1;
    }

    for (bp = local; bp != NULL; bp = bp->ai_next) {
        ret = -1;
        pm->inet = bp->ai_family;
        pm->hd->nfd = WSASocket(bp->ai_family,
                                bp->ai_socktype,
                                bp->ai_protocol,
                                NULL,
                                0,
                                WSA_FLAG_OVERLAPPED);

        if (INVALID_SOCKET == pm->hd->nfd)
            continue;

        /* 绑定SOCKET到地址 */
        ret = bind(pm->hd->nfd, bp->ai_addr, bp->ai_addrlen);
        if (ret == SOCKET_ERROR)
        {
            ret = WSAGetLastError();
            lmice_error_print("Bind failed. Error[%u]\n", ret);
            closesocket(pm->hd->nfd);
            continue;
        }

        if(pm->mode == EAL_WSA_TCP_CLIENT_MODE) {

            ret = getaddrinfo(pm->remote_addr, pm->remote_port, &hints, &remote);
            if (ret != 0) {
                ret = WSAGetLastError();
                lmice_error_print("getaddrinfo[%s]: %d\n", pm->remote_addr, ret);
                closesocket(pm->hd->nfd);
                freeaddrinfo(local);
                eal_wsa_remove_handle(pm->hd);
                return -1;
            }
            for (rp = remote; rp != NULL; rp = rp->ai_next) {
                /* 连接 remote server */
                ret = connect(pm->hd->nfd, rp->ai_addr, rp->ai_addrlen);
                //ret = WSAConnect(pm->nfd, rp->ai_addr, rp->ai_addrlen, NULL, NULL, NULL);
                if(ret == SOCKET_ERROR) {
                    ret = WSAGetLastError();
                    lmice_error_print("Connect failed. Error[%u]\n", ret);
                    continue;
                }

                /** 连接成功 */
                memcpy(&(pm->hd->addr), rp->ai_addr, rp->ai_addrlen);
                pm->hd->addrlen = rp->ai_addrlen;

                /* 更新inst_id */
                eal_wsa_hash(pm->mode,
                             pm->local_addr, 64, pm->local_port,8,
                             pm->remote_addr, 64, pm->remote_port, 8,
                             &(pm->hd->inst_id) );

                /* 将接受套接字和完成端口关联 */
                hdl = CreateIoCompletionPort((HANDLE)(pm->hd->nfd),
                                             pm->cp,
                                             (ULONG_PTR)pm->hd,
                                             0);
                if(hdl == NULL)
                {
                    lmice_error_print("CreateIoCompletionPort failed\n");
                    closesocket(pm->hd->nfd);
                    freeaddrinfo(remote);
                    freeaddrinfo(local);
                    eal_wsa_remove_handle(pm->hd);
                    return -1;
                }

                /* 开始在接受套接字上处理I/O使用重叠I/O机制 */
                eal_iocp_append_data(pm->ilist, pm->hd->inst_id, &pm->data);
                pm->data->operation = EAL_IOCP_TCP_RECV;

                /* 在新建的套接字上投递一个异步WSARecv */
                WSARecv(pm->hd->nfd,
                        &(pm->data->data),
                        1,
                        &(pm->data->recv_bytes),
                        &(pm->data->flags),
                        &(pm->data->overlapped),
                        NULL);
                break;
            } /* end-for: rp */

            freeaddrinfo(remote);


        } else if(pm->mode == EAL_WSA_TCP_SERVER_MODE) {

            /* 将SOCKET设置为监听模式 */
            ret = listen(pm->hd->nfd, 10);
            if(ret == SOCKET_ERROR) {
                ret = WSAGetLastError();
                lmice_error_print("Listen failed. Error[%u]\n", ret);
                closesocket(pm->hd->nfd);
                continue;
            }

            ret = eal_create_tcp_accept_thread(pm, eal_wsa_tcp_accept_thread_proc);


        }
        ret = 0;
        break;

    } /* end-for: bp */

    freeaddrinfo(local);

    return ret;
}

forceinline int eal_wsa_create_udp_handle(eal_wsa_service_param* pm)
{
    int ret = 0;
    struct addrinfo *local = NULL;
    struct addrinfo *bp = NULL;
    struct addrinfo hints;
    HANDLE hdl = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Stream socket */
    hints.ai_flags = 0;             /* For wildcard IP address */
    hints.ai_protocol = 0;          /* TCP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // 创建用来和套接字关联的单句柄数据信息结构
    eal_wsa_append_handle(pm->hlist, EAL_WSA_INVALID_INSTID, &pm->hd);

    /* getaddrinfo() returns a list of address structures.
                 Try each address until we successfully bind(2).
                 If socket(2) (or bind(2)) fails, we (close the socket
                 and) try the next address.
    */
    ret = getaddrinfo(pm->local_addr, pm->local_port, &hints, &local);
    if (ret != 0) {
        lmice_error_print("getaddrinfo[%s]: %d\n", pm->local_addr, ret);
        eal_wsa_remove_handle(pm->hd);
        return -1;
    }

    for (bp = local; bp != NULL; bp = bp->ai_next) {
        ret = -1;
        pm->inet = bp->ai_family;
        pm->hd->nfd = WSASocket(bp->ai_family,
                                bp->ai_socktype,
                                bp->ai_protocol,
                                NULL,
                                0,
                                WSA_FLAG_OVERLAPPED);

        if (INVALID_SOCKET == pm->hd->nfd)
            continue;

        /* 绑定SOCKET到地址 */
        ret = bind(pm->hd->nfd, bp->ai_addr, bp->ai_addrlen);
        if (ret == SOCKET_ERROR) {
            ret = WSAGetLastError();
            lmice_error_print("Bind failed. Error[%u]\n", ret);
            closesocket(pm->hd->nfd);
            continue;
        }

        /* 更新 id */
        eal_wsa_hash(pm->mode,
                     pm->local_addr, 64,
                     pm->local_port, 8,
                     NULL, 0,
                     NULL, 0,
                     &(pm->hd->inst_id) );

        /* 将接受套接字和完成端口关联 */
        hdl = CreateIoCompletionPort((HANDLE)(pm->hd->nfd),
                                     pm->cp,
                                     (ULONG_PTR)pm->hd,
                                     0);
        if(hdl == NULL)
        {
            lmice_error_print("CreateIoCompletionPort failed\n");
            closesocket(pm->hd->nfd);
            freeaddrinfo(local);
            eal_wsa_remove_handle(pm->hd);
            return -1;
        }

        /* 准备数据 */
        eal_iocp_append_data(pm->ilist, pm->hd->inst_id, &pm->data);

        /* 接收数据 */
        WSARecvFrom(pm->hd->nfd,
                    &(pm->data->data),
                    1,
                    &(pm->data->recv_bytes),
                    &(pm->data->flags),
                    (struct sockaddr*)&(pm->hd->addr),
                    &(pm->hd->addrlen),
                    &(pm->data->overlapped),
                    NULL);
        ret = 0;
        break;

    }
    freeaddrinfo(local);
    return ret;
}

forceinline int eal_wsa_create_mc_handle(eal_wsa_service_param* pm)
{
    int ret = 0;
    struct addrinfo *local = NULL;
    struct addrinfo *bp = NULL;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_IP;          /* IP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // 创建用来和套接字关联的单句柄数据信息结构
    eal_wsa_append_handle(pm->hlist, EAL_WSA_INVALID_INSTID, &pm->hd);

    /* getaddrinfo() returns a list of address structures.
                 Try each address until we successfully bind(2).
                 If socket(2) (or bind(2)) fails, we (close the socket
                 and) try the next address.
    */
    ret = getaddrinfo(pm->local_addr, pm->local_port, &hints, &local);
    if (ret != 0) {
        lmice_error_print("getaddrinfo[%s]: %d\n", pm->local_addr, ret);
        eal_wsa_remove_handle(pm->hd);
        return -1;
    }

    for (bp = local; bp != NULL; bp = bp->ai_next) {
        ret = -1;
        pm->inet = bp->ai_family;
        pm->hd->nfd = WSASocket(bp->ai_family,
                                bp->ai_socktype,
                                bp->ai_protocol,
                                NULL,
                                0,
                                WSA_FLAG_OVERLAPPED);

        if (INVALID_SOCKET == pm->hd->nfd)
            continue;

        /* 绑定SOCKET到地址 */
        ret = bind(pm->hd->nfd, bp->ai_addr, bp->ai_addrlen);
        if (ret == SOCKET_ERROR)
        {
            ret = WSAGetLastError();
            lmice_error_print("Bind failed. Error[%d]\n", ret);
            closesocket(pm->hd->nfd);
            continue;
        }

        /** 加入组播组 */
        struct ip_mreq req;
        req.imr_interface.s_addr = ((struct sockaddr_in*)bp->ai_addr)->sin_addr.s_addr;
        req.imr_multiaddr.s_addr = inet_addr(pm->remote_addr);
        ret = setsockopt(pm->hd->nfd ,
                         IPPROTO_IP ,
                         IP_ADD_MEMBERSHIP ,
                         (const char *)&req ,
                         sizeof(req));
        if(ret != 0)
        {
            ret = WSAGetLastError();
            lmice_error_print("setsockopt failed. Error[%d]\n", ret);
            closesocket(pm->hd->nfd);
            continue;
        }

        /* 更新 id (bind address, multicast group address) */
        eal_wsa_hash(pm->mode,
                     pm->local_addr, 64,
                     pm->local_port, 8,
                     pm->remote_addr, 64,
                     pm->remote_port, 8,
                     &(pm->hd->inst_id) );

        ret = 0;
        break;

    } /* end-for: bp */

    freeaddrinfo(local);

    return ret;
}

forceinline int eal_wsa_bind_handle_cp(eal_wsa_service_param* pm)
{
    HANDLE hdl = NULL;

    /* 将接受套接字和完成端口关联 */
    hdl = CreateIoCompletionPort((HANDLE)(pm->hd->nfd),
                                 pm->cp,
                                 (ULONG_PTR)pm->hd,
                                 0);
    if(hdl == NULL) {
        lmice_error_print("CreateIoCompletionPort failed\n");
        closesocket(pm->hd->nfd);
        pm->hd->nfd = 0;
        eal_wsa_remove_handle(pm->hd);
        return -1;
    }
}

forceinline int eal_wsa_arecv_data(eal_wsa_service_param* pm)
{
    int ret = 0;

    /* 准备数据 */
    eal_iocp_append_data(pm->ilist, pm->hd->inst_id, &pm->data);

    /* 接收数据 */
    ret = WSARecvFrom(pm->hd->nfd,
                &(pm->data->data),
                1,
                &(pm->data->recv_bytes),
                &(pm->data->flags),
                (struct sockaddr*)&(pm->hd->addr),
                &(pm->hd->addrlen),
                &(pm->data->overlapped),
                NULL);

    return ret;
}

forceinline int  eal_wsa_init(void)
{
    /* 请求2.2版本的WinSock库 */
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    int ret;

    /* 加载socket动态链接库 */
    ret = WSAStartup(version, &data);

    /* 检查套接字库是否申请成功 */
    if (0 != ret){
        lmice_error_print("WSAStartup Windows Socket Library Error[%u]!\n", ret);
        return -1;
    }

    /* 检查是否申请了所需版本的套接字库 */
    if(LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2){
        WSACleanup();
        lmice_error_print("Request Windows Socket Version 2.2 Error!\n");
        return -1;
    }

    return 0;
}

forceinline void  eal_wsa_finit(void)
{
    WSACleanup();
}

#endif /** LMICE_EAL_WSA_H */

