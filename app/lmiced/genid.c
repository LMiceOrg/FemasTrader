#include <eal/lmice_eal_hash.h>
#include <eal/lmice_eal_shm.h>
#include <eal/lmice_eal_event.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int ret;
    if(argc < 2) {
        printf("genid: Generate id by symbol name\n\n\tUsage: genid xxx\n");
        return 1;
    }
    for(ret = 1; ret < argc; ++ret) {
        const char* cmd = argv[ret];
        uint64_t hval;
        char symbol[32] ={0};
        char ename[32] ={0};
        char sname[32]={0};
        if(strlen(cmd) >=32) {
            printf("symbol[%s] is too long.\n");
            return 1;
        }
        memset(symbol, 0, 32);
        strcat(symbol, cmd);
        eal_hash64_fnv1a(cmd, 32);
        eal_event_hash_name(hval, ename);
        eal_shm_hash_name(hval, sname);

        printf("symbol:[%s] \n\tshm name[%s]\n\tevent name[%s]\n\n", symbol, sname, ename);
    }
    return 0;
}
