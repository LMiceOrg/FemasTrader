
#include "strategy_ins.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

strategy_ins *g_ins = NULL;

void print_usage(void) {
    printf("strategyht [option] [value]\n\n"
           "\t-h, --help\t\tshow this message\n"
           "\t-n, --name\t\tset module name\n"
           //"\t-s, --silent\t\trun in silent mode[backend]\n"
           "\t-c, --cpuset\t\tSet cpuset for this app(, separated)\n"
		   "\t-v, --value\t\tyesterday value of corresponding account\n"
           "\t-p, --position\t\tmax position can hold\n"
           "\t-l, --loss\t\tmax loss can make\n"
           );
}

void timeout_handler(int sig)
{
	if(sig == SIGALRM )
	{
		if( g_ins != NULL )
			g_ins->exit();
	}
}

void signal_handler(int sig) {
    if(sig == SIGTERM || sig == SIGINT)
    {
    	if( g_ins != NULL)
			g_ins->exit();
	}
}

int main(int argc, char **argv)
{
	STRATEGY_CONF st_conf;
	memset(&st_conf, 0, sizeof(STRATEGY_CONF));

	/** Process command line */
	int i;
	for(i=0; i<argc; i++) 
	{
        char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            return 0;
        } 
		else if(strcmp(cmd, "-c") == 0 ||
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
                    while(*cmd != 0) {
                        if(*cmd != ',') {
                            ++cmd;
                        } else {
                            ++cmd;
                            break;
                        }
                    }
                } while(*cmd != 0);

                int ret = lmspi_cpuset(NULL, cpuset, setcount);
                lmice_critical_print("set CPUset %d return %d\n", setcount, ret);

            } else {
                lmice_error_print("Command(%s) require cpuset param, separated by comma(,)\n", cmd);
            }
            ++i;
        }
		else if(strcmp(cmd, "-n") == 0 ||
                  strcmp(cmd, "--name") == 0) 
        {
            if(i+1<argc) {
                cmd = argv[++i];
                memset(st_conf.m_model_name, 0, sizeof(st_conf.m_model_name));
                strcpy(st_conf.m_model_name, cmd);
            } 
			else 
			{
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 1;
            }
        } 
		else if(strcmp(cmd, "-v") == 0 ||
                  strcmp(cmd, "--value") == 0) 
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_close_value = atof(cmd);
            } 
			else 
			{
                lmice_error_print("Command(%s) require yesterday close value(double)\n", cmd);
                return 1;
            }
        } 
		else if(strcmp(cmd, "-p") == 0 ||
                  strcmp(cmd, "--postition") == 0) 
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_max_pos = atoi(cmd);
            } 
			else 
			{
                lmice_error_print("Command(%s) require max position can hold(uint)\n", cmd);
                return 1;
            }
        } 
		else if(strcmp(cmd, "-l") == 0 ||
                  strcmp(cmd, "--loss") == 0) 
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_max_loss = atof(cmd);
            } 
			else 
			{
                lmice_error_print("Command(%s) require max loss can make(double)\n", cmd);
                return 1;
            }
        } 
    } /* end-for: argc*/

	signal(SIGALRM, timeout_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	while(1)
	{
		sleep(1);
	}

	lmice_info_print("strategy for ht is running...\n");
	strategy_ins instance( &st_conf );
	g_ins = &instance;
	instance.run();
	
}

