#include "lmice_core.h"
#include "lmice_eal_endian.h"
#include "lmice_eal_common.h"
#include "lmice_trace.h"

/*
int lmice_process_cmdline(int argc, char * const *argv)
{
    return 0;
}


int lmice_exec_command(lmice_environment_t *env)
{
    return 0;
}
*/


int eal_env_init(lmice_environment_t* env)
{
    eal_core_get_properties( &(env->lcore), &(env->memory), &(env->net_bandwidth) );

    return 0;
}
