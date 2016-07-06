
#include "strategy_ins.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

strategy_ins *g_ins = NULL;

void timeout_handler(int sig)
{
	if(sig == SIGALRM)
	{
		g_ins->ins_exit();
		delete g_ins;
	}
}

void signal_handler(int sig) {
    if(sig == SIGTERM || sig == SIGINT)
    {
		if(exit)
		{
			g_ins->ins_exit();
			delete g_ins;
		}
	}
}

int main(int argc, char **argv)
{
	lmice_info_print("strategy for ht is running...\n");
	strategy_ins instance;
	g_ins = &instance;
	//signal(SIGALRM, timeout_handler);
	//signal(SIGTERM, signal_handler);
	//signal(SIGINT, signal_handler);
	instance.run();
}

