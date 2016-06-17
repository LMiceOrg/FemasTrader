#include "lmice_eal_wsa.h"
#include "lmice_eal_iocp.h"
#include "lmice_eal_hash.h"

eal_wsa_handle_list *hlist;
eal_iocp_data_list *ilist;

DWORD WINAPI eal_wsa_tcp_accept_thread_proc( LPVOID lpParameter)
{
    int ret = 0;
    HANDLE hdl= NULL;
    eal_wsa_service_param * pm = (eal_wsa_service_param *)lpParameter;
    eal_wsa_handle* hd = NULL;
    eal_iocp_data* data = NULL;
    char host[64];
    char serv[8];
    int hlen = 64;
    int slen = 8;

    for(;;) {
        // 创建用来和套接字关联的单句柄数据信息结构
        eal_wsa_append_handle(pm->hlist, EAL_WSA_INVALID_INSTID, &hd);

        // 接收连接，并分配完成端，这儿可以用AcceptEx()
        hd->nfd = accept(pm->hd->nfd, (struct sockaddr*)&hd->addr, &hd->addrlen);
        if(SOCKET_ERROR == hd->nfd){	// 接收客户端失败
            ret = WSAGetLastError();
            lmice_error_print("Accept Socket Error[%d] \n", ret);
            eal_wsa_remove_handle(hd);
            break;
        }

        getnameinfo((struct sockaddr*)&hd->addr, hd->addrlen,
                    host, hlen, serv, slen, 0);

        /* 更新inst_id */
        eal_wsa_hash(pm->mode,
                     pm->local_addr, 64, pm->local_port,8,
                     host, 64, serv, 8,
                     &(hd->inst_id) );


        // 将接受套接字和完成端口关联
        hdl = CreateIoCompletionPort((HANDLE)(hd->nfd),
                               pm->cp,
                               (ULONG_PTR)hd,
                               0);
        if(hdl == NULL)
        {
            lmice_error_print("CreateIoCompletionPort failed\n");
            eal_wsa_remove_handle(hd);
            break;
        }

        // 开始在接受套接字上处理I/O使用重叠I/O机制
        eal_iocp_append_data(ilist, hd->inst_id, &data);

        // 在新建的套接字上投递一个异步WSARecv，这些I/O请求完成后，工作者线程会为I/O请求提供服务
        WSARecv(hd->nfd, &(data->data), 1, &data->recv_bytes, &data->flags, &(data->overlapped), NULL);

    }
    return 0;
}
