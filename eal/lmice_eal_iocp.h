#ifndef LMICE_EAL_IOCP_H
#define LMICE_EAL_IOCP_H

#include "lmice_trace.h"
#include "lmice_eal_common.h"

#include <stdint.h>

/**
    IOCP需要用到的动态链接库
pragma comment(lib, "Kernel32.lib")
*/

#define eal_iocp_handle HANDLE

/* iocp 操作指令 */
enum eal_iocp_operation_e
{
    EAL_IOCP_TCP_RECV,
    EAL_IOCP_TCP_SEND,
    EAL_IOCP_MC_RECV,
    EAL_IOCP_MC_SEND,
    EAL_IOCP_UDP_RECV,
    EAL_IOCP_UDP_SEND
};

/* IO数据区 长度 */
#define EAL_IOCP_BUFFER_SIZE  (64 * 1024 - 64 )
/* 数组 长度 */
#define EAL_IOCP_BUFFER_LENGTH 64

/* iocp 数据结构 */
struct eal_iocp_data_s
{
    uint64_t id;
    void* pdata;
    int operation;
    int quit_flag;
    WSABUF data;
    OVERLAPPED overlapped;
    DWORD recv_bytes;
    DWORD flags;

    char buff[EAL_IOCP_BUFFER_SIZE];
};
typedef struct eal_iocp_data_s eal_iocp_data;

struct eal_iocp_data_list_s
{
    volatile int64_t lock;
    struct eal_iocp_data_list_s* next;
    eal_iocp_data data_array[EAL_IOCP_BUFFER_LENGTH];
};
typedef struct eal_iocp_data_list_s eal_iocp_data_list;

/**
  eal_iocp_create_data_list
  eal_iocp_destroy_data_list
  eal_iocp_append_data
  eal_iocp_remove_data
 */ 

forceinline int eal_iocp_create_data_list(eal_iocp_data_list** pb)
{
    *pb = (eal_iocp_data_list*)malloc( sizeof(eal_iocp_data_list) );
    if(*pb == NULL)
        return -1;
    memset(*pb, 0, sizeof(eal_iocp_data_list));
    return 0;
}

forceinline void eal_iocp_destroy_data_list(eal_iocp_data_list* pb)
{
    eal_iocp_data_list *next = NULL;
    do {
        next = pb->next;
        free(pb);
        pb = next;
    } while(pb != NULL);
}


forceinline int eal_iocp_append_data(eal_iocp_data_list* pb, uint64_t inst_id, eal_iocp_data** data)
{
    size_t i=0;
    eal_iocp_data_list *next = NULL;
    eal_iocp_data* cur = NULL;
    do {

        next = pb;
        for(i=0; i< EAL_IOCP_BUFFER_LENGTH; ++i) {
            cur = &(pb->data_array[i]);
            if(cur->id == 0) {
                cur->id = inst_id;
                cur->data.len = EAL_IOCP_BUFFER_SIZE;
                cur->data.buf = cur->buff;
                *data = cur;
                return 0;
            }
        }
        pb = next->next;
    } while(pb != NULL);


    if(pb == NULL) {
        eal_iocp_create_data_list(&pb);
        next->next = pb;

        cur = &(pb->data_array[0]);
        cur->id = inst_id;
        cur->data.len = EAL_IOCP_BUFFER_SIZE;
        cur->data.buf = cur->buff;
        *data = cur;

    }

    return 0;

}

forceinline void eal_iocp_remove_data(eal_iocp_data* bt)
{
    bt->id = 0;
}


struct eal_iocp_buffer_s
{
    uint64_t inst_id;
    char buffer[EAL_IOCP_BUFFER_SIZE];

};
typedef struct eal_iocp_buffer_s eal_iocp_bt;

struct eal_iocp_buffer_list_s
{
    volatile int64_t lock;
    struct eal_iocp_buffer_list_s* next;
    eal_iocp_bt buffer_array[EAL_IOCP_BUFFER_LENGTH];
};
typedef struct eal_iocp_buffer_list_s eal_iocp_buff_list;

forceinline int eal_iocp_create_buffer_list(eal_iocp_buff_list** pb)
{
    *pb = (eal_iocp_buff_list*)malloc( sizeof(eal_iocp_buff_list) );
    if(*pb == NULL)
        return -1;
    memset(*pb, 0, sizeof(eal_iocp_buff_list));
    return 0;
}

forceinline  void eal_iocp_destroy_buffer_list(eal_iocp_buff_list* pb)
{
    eal_iocp_buff_list *next = NULL;
    do {
        next = pb->next;
        free(pb);
        pb = next;
    } while(pb != NULL);
}

forceinline int eal_append_iocp_buffer(eal_iocp_buff_list* pb, uint64_t inst_id, char** buffer)
{
    size_t i=0;
    eal_iocp_buff_list *next = NULL;
    eal_iocp_bt* cur = NULL;
    do {

        next = pb;
        for(i=0; i< EAL_IOCP_BUFFER_LENGTH; ++i)
        {
            cur = &(pb->buffer_array[i]);
            if(cur->inst_id == 0)
            {
                cur->inst_id = inst_id;
                *buffer = cur->buffer;
                return 0;
            }
        }
        pb = next->next;
    } while(pb != NULL);


    if(pb == NULL)
    {
        eal_iocp_create_buffer_list(&pb);
        next->next = pb;

        cur = &(pb->buffer_array[0]);
        cur->inst_id = inst_id;
        *buffer = cur->buffer;

    }

    return 0;

}

forceinline void eal_remove_iocp_bt(eal_iocp_bt* bt)
{
    bt->inst_id = 0;
}

forceinline void eal_remove_iocp_buffer(char* buffer)
{
    eal_iocp_bt* bt = (eal_iocp_bt*)(buffer - sizeof(bt->inst_id) );
    bt->inst_id = 0;
}

forceinline int eal_append_iocp_handle(eal_iocp_handle cp, eal_iocp_handle sock, uint64_t id) {
    HANDLE hdl;
    /* 将接受套接字和完成端口关联 */
    hdl = CreateIoCompletionPort(sock,
                                 cp,
                                 (ULONG_PTR)id,
                                 0);
    return hdl == NULL?1:0;
}

forceinline int eal_create_iocp_handle(HANDLE* cp)
{
    int ret = 0;
    DWORD err = 0;

    *cp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0);
    /* 检查创建IO内核对象失败*/
    if (NULL == *cp) {
        err = GetLastError();
        lmice_error_print("CreateIoCompletionPort failed. Error[%ul]\n", err);
        ret = -1;
    }
    return ret;
}

typedef DWORD  (WINAPI eal_iocp_worker_thread_type) (LPVOID ctx);
forceinline void eal_create_iocp_worker(int size, eal_iocp_worker_thread_type worker, HANDLE cp)
{
    HANDLE  tfd = NULL;
    DWORD   tid = 0;
    int     i   = 0;
    for( i=0;i < size; ++i){
        tfd = CreateThread(NULL, 0, worker, cp, 0, &tid);
        CloseHandle(tfd);
    }
}

#endif /** LMICE_EAL_IOCP_H */

