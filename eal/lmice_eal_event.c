#include "lmice_eal_event.h"
#include "lmice_trace.h"
#include "lmice_eal_hash.h"
#include <string.h>
#include <errno.h>

forceinline
void hash_to_nameA(uint64_t hval, char* name)
{
    int i=0;
    const char* hex_list="0123456789ABCDEF";
    for(i=0; i<8; ++i)
    {
        name[i*2] = hex_list[ *( (uint8_t*)&hval+i) >> 4];
        name[i*2+1] = hex_list[ *( (uint8_t*)&hval+i) & 0xf ];
    }
}

int eal_event_hash_name(uint64_t hval, char *name)
{
#if defined(_WIN32)
    memcpy(name, "Global\\ev", 9);
    hash_to_nameA(hval, name+9);
    name[25]='\0';
#else
    memcpy(name, "ev", 2);
    hash_to_nameA(hval, name+2);
    name[18]='\0';
#endif


    return 0;
}

int eal_event_zero(lmice_event_t *e)
{
    memset(e, 0, sizeof(lmice_event_t));
    return 0;
}

#if defined(_WIN32)


int eal_event_open(lmice_event_t* e)
{
    e->fd = OpenEventA( EVENT_ALL_ACCESS, FALSE, e->name);
    if(e->fd == NULL)
    {
        DWORD hr = GetLastError();
        lmice_error_print("Open event[%s] failed[%lu]\n", e->name, hr);
        e->fd = 0;
        return 1;
    }
    lmice_debug_print("event[%s] created as[%p]\n", e->name, e->fd);
    return 0;
}

int eal_event_create(lmice_event_t* e)
{
    e->fd = CreateEventA(
                NULL,
                FALSE,
                FALSE,
                e->name);
    if(e->fd == NULL)
    {
        DWORD hr = GetLastError();
        lmice_error_print("Create event[%s] failed[%lu]\n", e->name, hr);
        e->fd = 0;
        return 1;
    }
    /*
     * lmice_debug_print("event[%s] created as[%d]\n", e->name, (uint64_t)e->fd);
     */
    return 0;
}



int eal_event_awake(evtfd_t fd)
{
    BOOL ret = 1;
    if(fd)
        ret = SetEvent(fd);
    return ret != 0 ? 0 : 1;
}

int eal_event_destroy(lmice_event_t *e)
{
    BOOL ret = 1;
    if(e->fd)
    {
        ret = CloseHandle( e->fd );
        e->fd = 0;
    }
    return ret != 0 ? 0 : 1;
}

int eal_event_close(evtfd_t fd)
{
    BOOL ret = 1;
    if(fd)
    {
        ret = CloseHandle( fd );
    }
    return ret != 0 ? 0 : 1;
}

int eal_event_wait_one(evtfd_t fd)
{
    DWORD hr = WaitForSingleObject(fd,
                                   INFINITE);
    return hr;
}

#elif defined(__APPLE__) || defined(__linux__)

int eal_event_create(lmice_event_t* e) {
    int ret = 0;
    e->fd = sem_open(e->name, O_CREAT|O_EXCL, 0766, 0);
    if(e->fd == SEM_FAILED) {
        sem_unlink(e->name);

        ret = 1;
        e->fd = 0;
        lmice_critical_print("eal_event_create[%s] failed[%d]\n", e->name, errno);
    }
    return ret;
}

int eal_event_destroy(lmice_event_t* e) {

    int ret = 0;
    ret = sem_unlink(e->name);
    if(ret != 0) {
        lmice_critical_print("eal_event_destroy[%s] failed[%d].\n", e->name, errno);
    }
    return ret;
}

int eal_event_open(lmice_event_t* e) {
    int ret = 0;
    e->fd = sem_open(e->name, O_CREAT, 0666, 0);
    if(e->fd == SEM_FAILED) {
        e->fd = 0;
        ret = 1;
        lmice_critical_print("eal_event_open[%s] failed[%d].\n", e->name, errno);
    }
    return ret;
}

int eal_event_awake(evtfd_t fd) {
    int ret = 0;
    ret = sem_post(fd);
    return ret;
}

int eal_event_close(evtfd_t fd) {
    int ret = 0;
    ret = sem_close(fd);
    fd = 0;
    return ret;
}

int eal_event_wait_one(evtfd_t fd)
{
    return sem_wait(fd);
}

int eal_event_wait_timed(evtfd_t fd, int millisec)
{
    int ret;
    int cnt = 0;
    do {
        ret = sem_trywait(fd);
        switch(ret)
        {
        case 0:
            return ret;
        case EAGAIN:
            cnt += 1000;
            sleep(1);
            break;
        case EDEADLK:
        case EINTR:
        case EINVAL:
            return ret;
        }

    } while(cnt >= millisec);
    return ret;
}

#endif
