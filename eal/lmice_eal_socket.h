#ifndef LMICE_EAL_SOCKET_H
#define LMICE_EAL_SOCKET_H

#include <stdint.h>

#include "lmice_eal_common.h"

#if defined(_WIN32)
#define eal_socket_t SOCKET
#elif defined(__APPLE__) || defined(__linux__)
#define eal_socket_t int
#endif

enum lmice_socket_data_e
{
    LMICE_SOCKET_DATA_NOTUSE = 0,
    LMICE_SOCKET_DATA_USING = 1,
    LMICE_SOCKET_DATA_LIST_SIZE = 32
};



struct lmice_socket_data_s
{
    uint64_t inst_id;
    int state;
    eal_socket_t nfd;
    SOCKADDR_STORAGE addr;
};
typedef struct lmice_socket_data_s lm_socket_dt;

struct lmice_socket_head_s
{
    uint64_t inst_id;
    uint64_t del_pos;
    lm_socket_dt *next;
};
typedef struct lmice_socket_head_s lm_socket_ht;

void forceinline create_socket_data_list(lm_socket_dt** cl)
{
    *cl = (lm_socket_dt*)malloc( sizeof(lm_socket_dt)*LMICE_SOCKET_DATA_LIST_SIZE );
    memset(*cl, 0, sizeof(lm_socket_dt)*LMICE_SOCKET_DATA_LIST_SIZE );
}

void forceinline delete_socket_data_list(lm_socket_dt* cl)
{
    lm_socket_ht* head = NULL;
    lm_socket_dt* next = NULL;
    do {
        head = (lm_socket_ht*)cl;
        next = head->next;
        free(cl);
        cl = next;
    } while(cl != NULL);
}

int forceinline create_socket_data(lm_socket_dt* cl, lm_socket_dt** val)
{
    lm_socket_ht* head = NULL;
    lm_socket_dt* cur = NULL;
    size_t i = 0;

    do {
        head = (lm_socket_ht*)cl;
        for( i= 1; i < LMICE_SOCKET_DATA_LIST_SIZE; ++i) {
            cur = &cl[i];
            if(cur->state == LMICE_SOCKET_DATA_NOTUSE) {
                cur->state = LMICE_SOCKET_DATA_USING;
                *val = cur;
                return 0;
            }
        }
        cl = head->next;
    } while(cl != NULL);

    if(cl == NULL)
    {
        create_socket_data_list(&cl);
        head->next = cl;

        cur = &cl[1];
        cl[1].state = LMICE_SOCKET_DATA_USING;
        *val = &cl[1];
    }
    return 0;
}

int forceinline delete_socket_data(lm_socket_dt* cl, const lm_socket_dt* val)
{
    lm_socket_ht* head = NULL;
    lm_socket_dt* cur = NULL;
    size_t i = 0;
    head = (lm_socket_ht*)cl;
    do {
        for( i= 1; i < LMICE_SOCKET_DATA_LIST_SIZE; ++i) {
            cur = cl+i;
            if(cur->state == LMICE_SOCKET_DATA_USING &&
                    cur == val) {
                cur->state = LMICE_SOCKET_DATA_NOTUSE;
                return 0;
            }
        }
        cl = head->next;
    } while(cl != NULL);

    return 1;
}


#endif /** LMICE_EAL_SOCKET_H */

