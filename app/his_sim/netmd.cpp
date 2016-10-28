#include <lmspi.h>  /* LMICED spi */
#include <pcap.h>   /* libpcap  */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <eal/lmice_trace.h>    /* EAL tracing */
#include <eal/lmice_bloomfilter.h> /* EAL Bloomfilter */
#include <eal/lmice_eal_hash.h>
#include <eal/lmice_eal_thread.h>
#include <eal/lmice_eal_spinlock.h>

#include "fm3in1.h"
#include "guavaproto.h"
#include "strategy_ins.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <vector>
#include <map>
#include <string>

using namespace std;

pcap_t* pcapHandle = NULL;

lm_bloomfilter_t* bflter = NULL;

/* strategy instance */
strategy_ins *g_ins = NULL;
char trading_instrument[32];
char g_cur_sysid[31];

std::vector<std::string> array_ref_ins;
std::vector<std::string> array_feature_name; 

/* timer sturct */
struct itimerval g_tick;


/* trader */

CFemas2TraderSpi* g_spi;

CUR_STATUS g_cur_status;
CUR_STATUS_P g_cur_st=&g_cur_status;
STRATEGY_CONF st_conf;
STRATEGY_CONF_P g_conf= &st_conf;
CUstpFtdcInputOrderField g_order;
CUstpFtdcOrderActionField g_order_action;
int g_order_action_count= 0;

ChinaL1Msg msg_data;
int64_t g_begin_time = 0;
int64_t g_end_time = 0;
struct timeval g_pkg_time;

double g_thresh_num = 0.3;

int g_done_flag = 1;
double g_signal_multiplier = 1;


/* netmd package callback */
static void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet);

/* netmd worker thread */
static int netmd_pcap_thread(const char *devname, const char *packet_filter);

/* stop netmd worker */
static void netmd_pcap_stop(int sig);

/* publish data */
forceinline void netmd_pub_data(const char* symbol, const void* addr, int len);

/* create bloom filter for net md */
static void netmd_bf_create(void);

/* delete bloom filter */
static void netmd_bf_delete(void);

/* print usage of app */
static void print_usage(void);

/* Daemon */
static int init_daemon(int silent);

/* Unit test */
static void unit_test(void);



/* get pcap file list */
static int hs_get_file_list( string path, vector<string> &pcap_file_list );
static int hs_load_file( string filename, vector<P_HS_MD_T> &pcap_md_list );
static int hs_wirte_log( string filename, int check_write );
static int hs_write_result( string filename );
static int hs_write_stastic();
static void hs_reset_global(void);
static double calc_result( vector<P_HS_MD_T> pcap_md_list );

static map<string, P_HS_MD_T> g_valid_md;

static int position = 11;
static int bytes = sizeof(struct guava_udp_normal)+sizeof(struct guava_udp_head);
static char md_name[32];
static char his_dir[256];
static char res_dir[256];

double g_trade_buy_price = 0;
double g_trade_sell_price = 0;
double g_flatten_threshold = 4;

double g_order_buy_price = 0;
double g_order_sell_price = 0;
int g_active_buy_orders = 0;
int g_active_sell_orders = 0;

#define HS_LOG_BUFFER_LEN 65536
char log_buffer[HS_LOG_BUFFER_LEN];
int log_pos = 0;

char signal_log_buffer[HS_LOG_BUFFER_LEN];
int signal_log_pos = 0;
int signal_log_line = 0;

char stastic_buffer[HS_LOG_BUFFER_LEN];
int stastic_pos = 0;

double session_fee = 0;
int session_trading_times = 0;
/*
#define MAX_KEY_LENGTH 512
static uint64_t keylist[MAX_KEY_LENGTH];
static volatile int keypos;
static volatile int64_t netmd_pd_lock = 0;
*/

static inline void guava_md(int mc_port, const char* mc_group, const char* mc_bindip);
void guava_md_stop(int sig);


static std::string log_file_name;
int main(int argc, char* argv[]) {
    uid_t uid = -1;
    int ret;
    int i;
    int silent = 0;
    int test = 0;

    int mc_sock = 0;
    int mc_port = 30100;/* 5001*/
    char mc_group[32]="233.54.1.100"; /*238.0.1.2*/
    char mc_bindip[32]="192.168.208.16";/*10.10.21.191*/

    char devname[64] = "p6p1";
    char filter[128] = "udp and port 30100";

    /** Trader */
    char front_address[64] = "10.0.10.0";
    char user[64] = "";
    char password[64] = "";
    char broker[64] = "";
    char *investor = user;
    char exchange_id[32]= "SHIF";
	
    (void)argc;
    (void)argv;

    memset( &g_cur_status, 0, sizeof(CUR_STATUS));
    //to be add,add trade insturment message and update to st
    g_cur_status.m_md.fee_rate = 0.000101 + 0.0000006;
    g_cur_status.m_md.m_up_price = 65535;
    g_cur_status.m_md.m_down_price = 0;
    g_cur_status.m_md.m_last_price = 0;
    g_cur_status.m_md.m_multiple = 10;
	g_cur_status.m_md.m_price_tick = 1;

	const double rebate_rate = 0.3;
	
    memset(&st_conf, 0, sizeof(STRATEGY_CONF));
	st_conf.m_max_pos = 3;

    //netmd_pd_lock = 0;
    //keypos = 0;
    memset(md_name, 0, sizeof(md_name));
    strcpy(md_name, "[fm3in1]");

	memset(his_dir, 0, sizeof(his_dir));

    /** Process command line */
    for(i=0; i< argc; ++i) {
        char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            return 0;
        } else if(strcmp(cmd, "-t") == 0 ||
                  strcmp(cmd, "--test") == 0) {
            lmice_critical_print("Unit test\n");
            test = 1;

        } else if(strcmp(cmd, "-n") == 0 ||
                  strcmp(cmd, "--name") == 0) {
            if(i+1<argc) {
                cmd=argv[i+1];
                memset(md_name, 0, sizeof(md_name));
                strncat(md_name, cmd, sizeof(md_name)-1);
            } else {
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-d") == 0 ||
                  strcmp(cmd, "--device") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                if(strlen(cmd)>63) {
                    lmice_error_print("Adapter device name is too long(>63)\n");
                    return 1;
                }
                memset(devname, 0, sizeof(devname));
                strncat(devname, cmd, sizeof(devname)-1);
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-s") == 0 ||
                  strcmp(cmd, "--silent") == 0) {
            silent = 1;
        } else if(strcmp(cmd, "-u") == 0 ||
                  strcmp(cmd, "--uid") == 0) {
            if(i+1<argc){
                cmd = argv[i+1];
                uid = atoi(cmd);
                ret = seteuid(uid);
                if(ret != 0) {
                    ret = errno;
                    lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
                    return ret;
                }
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-f") == 0 ||
                  strcmp(cmd, "--filter") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                memset(filter, 0, sizeof(filter));
                ret = strlen(cmd);
                if(ret > 128) {
                    lmice_error_print("filter string is too long(>127)\n");
                    return ret;
                }
                strncpy(filter, cmd, ret);
            } else {
                lmice_error_print("Command(%s) require filter string\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-p") == 0 ||
                  strcmp(cmd, "--path") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                strcpy( his_dir, argv[i+1] );
            } else {
                lmice_error_print("Command(%s) require directory path\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-c") == 0 ||
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

                ret = lmspi_cpuset(NULL, cpuset, setcount);
                lmice_critical_print("set CPUset %d return %d\n", setcount, ret);

            } else {
                lmice_error_print("Command(%s) require cpuset param, separated by comma(,)\n", cmd);
            }
            ++i;
        }else if(strcmp(cmd, "-sv") == 0 ||
                 strcmp(cmd, "--svalue") == 0)
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
        else if(strcmp(cmd, "-sp") == 0 ||
                strcmp(cmd, "--sposition") == 0)
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
		else if(strcmp(cmd, "-sf") == 0 ||
                strcmp(cmd, "--sfee") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
               	g_cur_status.m_md.fee_rate = atof(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require fee rate can hold(double)\n", cmd);
                return 1;
            }
        }
		else if(strcmp(cmd, "-sm") == 0 ||
                strcmp(cmd, "--smultiple") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
               	g_cur_status.m_md.m_multiple = atoi(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require multiple can hold(uint)\n", cmd);
                return 1;
            }
        }
		else if(strcmp(cmd, "-st") == 0 ||
                strcmp(cmd, "--spricet") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
               	g_cur_status.m_md.m_price_tick = atof(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require price tick can hold(uint)\n", cmd);
                return 1;
            }
        }
        else if(strcmp(cmd, "-sl") == 0 ||
                strcmp(cmd, "--sloss") == 0)
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
		else if(strcmp(cmd, "-ss") == 0 ||
                strcmp(cmd, "--ssignal") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
                g_signal_multiplier = atof(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require signal multiplier can make(double)\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tf") == 0 ||
                  strcmp(cmd, "--tfront") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(front_address, 0, sizeof(front_address));
                strncpy(front_address, cmd, sizeof(front_address)-1);
            } else {
                lmice_error_print("Trader Command(%s) require front address string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tu") == 0 ||
                  strcmp(cmd, "--tuser") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(user, 0, sizeof(user));
                strncpy(user, cmd, sizeof(user)-1);
                //投资人ID�?用户ID［去除前面的交易商ID�?
                investor = user + strlen(broker);
            } else {
                lmice_error_print("Trader Command(%s) require user id string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tp") == 0 ||
                  strcmp(cmd, "--tpassword") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(password, 0, sizeof(password));
                strncpy(password, cmd, sizeof(password)-1);
            } else {
                lmice_error_print("Trader Command(%s) require password string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tb") == 0 ||
                  strcmp(cmd, "--tbroker") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(broker, 0, sizeof(broker));
                strncpy(broker, cmd, sizeof(broker)-1);
                //投资人ID�?用户ID［去除前面的交易商ID�?
                investor = user + strlen(broker);
            } else {
                lmice_error_print("Trader Command(%s) require broker string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-te") == 0 ||
                  strcmp(cmd, "--texchange") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(exchange_id, 0, sizeof(exchange_id));
                strncpy(exchange_id, cmd, sizeof(exchange_id)-1);
            } else {
                lmice_error_print("Trader Command(%s) require exchange id string\n", cmd);
                return 1;
            }
        }else if(strcmp(cmd, "-ts") == 0 ||
                  strcmp(cmd, "--tthreshold") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                g_flatten_threshold = atoi(cmd);
            } else {
                lmice_error_print("Trader Command(%s) require flatten threshold\n", cmd);
                return 1;
            }
        } 
    } /* end-for: argc*/

    /** Silence mode */
    init_daemon(silent);


    /** Strategy */
	printf("===== strategy ins name: %s =====\n", md_name);
    g_ins = new strategy_ins(md_name, &st_conf, g_spi);
    memcpy(trading_instrument,
           g_ins->get_forecaster()->get_trading_instrument().c_str(),
           g_ins->get_forecaster()->get_trading_instrument().size());
    strcpy(g_cur_st->m_ins_name,g_ins->get_forecaster()->get_trading_instrument().c_str());

    array_ref_ins = g_ins->get_forecaster()->get_subscriptions();
    lmice_info_print("trading symbol:%s\n", trading_instrument);
	P_HS_MD_T p_tmp_md = new HS_MD_T;
	memset(p_tmp_md, 0, sizeof(HS_MD_T));
	g_valid_md[string(trading_instrument)] = p_tmp_md;
    for(i=0; i<array_ref_ins.size(); ++i) {
        lmice_info_print("ref symbol:%s\n", array_ref_ins[i].c_str());
		p_tmp_md = new HS_MD_T;
		memset(p_tmp_md, 0, sizeof(HS_MD_T));
		g_valid_md[array_ref_ins[i]] = p_tmp_md;
    }

	array_feature_name = g_ins->get_forecaster()->get_subsignal_name();
    for(i=0; i<array_feature_name.size(); ++i) {
        lmice_info_print("feature name:%s\n", array_feature_name[i].c_str());
    }

	/** Order */
	memset( &g_order, 0, sizeof(CUstpFtdcInputOrderField));
    g_order.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
    g_order.HedgeFlag = USTP_FTDC_CHF_Speculation;
    g_order.TimeCondition = USTP_FTDC_TC_GFD;
    g_order.VolumeCondition = USTP_FTDC_VC_AV;
    g_order.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
	strcpy(g_order.InstrumentID, trading_instrument);

	test = 0;
    /** Unit test */
    if(test) {
        unit_test();
    } else {
		vector<string> pcap_file_list;
		int res = hs_get_file_list( his_dir, pcap_file_list );

		if(res > 0)
		{
			memset(stastic_buffer, 0, HS_LOG_BUFFER_LEN);
			sprintf(stastic_buffer, "instrument, date, profit, trading times, fee, rebate, PNL with rebate\n");
			stastic_pos = strlen(stastic_buffer);
		}
		else
		{
			printf("no file to simulator!\n");
			return 0;
		}
	
		vector<string>::iterator it;
		vector<P_HS_MD_T> pcap_md_list;
		for( it=pcap_file_list.begin(); it!=pcap_file_list.end(); it++ )
		{
			int start = (*it).size() - 32;
			printf("prase file: %s\n", (*it).c_str());
			printf("file name: %s\n", (*it).substr(start).c_str());
			
			string res_file_name = "./log_result/result_";
			log_file_name = "./log_result/future_log_";
			//log_file_name.append(trading_instrument);
			log_file_name.append("_");
			res_file_name.append((*it).substr(start+8,10));
			log_file_name.append((*it).substr(start+8,10));
			int file_hour = atoi((*it).substr(start+20,2).c_str());
	
			//int file_day = atoi((*it).substr(start+16,2))
			if( file_hour >= 20 )
			{
				res_file_name.append("_21-00.csv");
				log_file_name.append("_21-00.log");
			}
			else if( file_hour <= 8 )
			{
				res_file_name.append("_09-00.csv");
				log_file_name.append("_09-00.log");
			}
			else
			{
				res_file_name.append("_13-30.csv");
				log_file_name.append("_13-30.log");
			}

			string file_date = res_file_name.substr( res_file_name.size()-20, 16 );
			sprintf( stastic_buffer+stastic_pos, "%s, %s ,", trading_instrument, file_date.c_str() );
			stastic_pos = strlen( stastic_buffer );
			
			memset( log_buffer, 0, HS_LOG_BUFFER_LEN );
			sprintf( log_buffer, "instrument,time,direction,operation,price,volume,money,fee\n" );
			log_pos = strlen(log_buffer);

			memset(signal_log_buffer, 0, sizeof(signal_log_buffer));
			signal_log_line = 0;
			signal_log_pos = 0;
			
			hs_load_file( (*it), pcap_md_list );
			double PNL = calc_result(pcap_md_list);
			pcap_md_list.clear();
			hs_wirte_log( log_file_name, 0 );
			hs_write_result(res_file_name);
			hs_reset_global();
			double rebate = session_fee * rebate_rate;
			sprintf( stastic_buffer+stastic_pos, "%f, %d, %f, %f, %f\n", PNL, session_trading_times, session_fee, rebate, PNL+rebate);
			stastic_pos = strlen( stastic_buffer );
			session_trading_times = 0;
			session_fee = 0;
		}
    } 

	printf("====== stastics ======\n%s\n", stastic_buffer);
	printf("==start write==");
	hs_write_stastic();

    /** Exit and maintain resource */
    lmice_warning_print("Exit %s\n", md_name);
	
    delete g_spi;

    return 0;
}


int init_daemon(int silent) {
    if(!silent)
        return 0;

    if( daemon(0, 1) != 0) {
        return -2;
    }
    return 0;
}

void print_usage(void) {
    printf("his_sim -- histroy simulator app --\n\n"
           "\t-h, --help\t\tshow this message\n"
           "\t-n, --name\t\tset strategy name\n"
           "\t-s, --silent\t\trun in silent mode[backend]\n"
           "\t-p, --path\t\tpath of dump packet directory\n"
           "\t-c, --cpuset\t\tSet cpuset for this app(, separated)\n"

          "\t-sp, --sposition\tmax position can hold\n"
          "\t-sl, --sloss\t\tmax loss can make\n"
          "\t-sf, --sfee\t\tfee rate\n"
		  "\t-sm, --smultiple\tmultiple\n"
		  "\t-st, --spricet\t\tprice tick\n"
		  "\t-ss, --ssignal\t\tsignal multipler\n\n"

          "\t-ts, --threshold\tset extra flatten threshold"
           "\n"
           );
}

void unit_test(void) 
{

}

int hs_get_file_list( string path, vector<string> &pcap_file_list )
{

	DIR *dir;
	struct dirent *ptr;
	int count = 0;

	if ((dir=opendir(path.c_str())) == NULL)
    {
    	perror("Open dir error...");
    	exit(1);
    }

	while ((ptr=readdir(dir)) != NULL)
    {
    	//printf("==== get dir %s , type: %d ====\n", ptr->d_name,ptr->d_type);
   		if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
   		{
			continue;
		}
		else if(ptr->d_type == 8) //file
		{
			string t_path = path;
			t_path.append("/");
			t_path.append(ptr->d_name);
			pcap_file_list.push_back(t_path);
			count++;
		}
		else if( strncmp(ptr->d_name,"package", 7) == 0 )
		{
                        string t_path = path;
                        t_path.append("/");
                        t_path.append(ptr->d_name);
                        pcap_file_list.push_back(t_path);
                        count++;

		}
		else if(ptr->d_type == 4) //dir
		{
			string path1 = path;
			path1.append("/");
			path1.append(ptr->d_name);
			hs_get_file_list( path1 , pcap_file_list );
		}
		else
		{

		}
	}
	return count;
	
}

int hs_recv_msg( char *buf, vector<P_HS_MD_T> &pcap_md_list )
{
	int len = 0;
	guava_udp_head* ptr_head = (guava_udp_head*)buf;

	P_HS_MD_T p_md = new HS_MD_T;
	memset( p_md, 0, sizeof(HS_MD_T) );
	memcpy( &(p_md->hs_head), ptr_head, sizeof(guava_udp_head) );
	len += sizeof(guava_udp_head);

	if (ptr_head->m_quote_flag != QUOTE_FLAG_SUMMARY)
	{
		guava_udp_normal* ptr_data = (guava_udp_normal*)(buf+ sizeof(guava_udp_head));
		memcpy( &(p_md->hs_body), ptr_data, sizeof(guava_udp_normal) );
		len += sizeof(guava_udp_normal);
	}
	else
	{
		guava_udp_summary* ptr_data = (guava_udp_summary*)(buf + sizeof(guava_udp_head));
		memcpy( &(p_md->hs_body), ptr_data, sizeof(guava_udp_summary) );
		len += sizeof(guava_udp_summary);
	}

	pcap_md_list.push_back( p_md );

	return len;
}

#define PCAP_HEAD_LEN 24
#define PCAP_PKT_GAP 16
#define HS_MSG_LENGTH 42 //MAC+IP+UDP
#define HS_MSG_GAP 58//(PCAP_PKT_GAP+HS_MSG_LENGTH)

int hs_load_file( string filename, vector<P_HS_MD_T> &pcap_md_list )
{
	FILE * pf;
	int buf_len = 0;
	unsigned int count = 0;
	//string file_path(his_dir);
	//file_path.append("/");
	//file_path.append(filename);

	char *file_buf = new char[100*1024*1024];

	if( (pf = fopen( filename.c_str(), "rb" )) == NULL )
	{
		printf("fopen %s error !", filename.c_str());
	}
	
	buf_len = fread( file_buf, 1, 100*1024*1024, pf );

	printf("file length: %d Byte\n", buf_len);

	if( buf_len > (PCAP_HEAD_LEN + HS_MSG_GAP) )
	{
		char *data = file_buf + PCAP_HEAD_LEN + HS_MSG_GAP;
		buf_len -= ( PCAP_HEAD_LEN + HS_MSG_GAP );
		while(buf_len > 0)
		{
			int len = 0;
			len = hs_recv_msg( data, pcap_md_list );
			count++;
			buf_len -=  ( len + HS_MSG_GAP );
			data += ( len + HS_MSG_GAP );
		}
		//printf( "count: %d - vector count: %d\n", count, pcap_md_list.size() );
		//vector<P_HS_MD_T>::iterator it = pcap_md_list.end() - 1;
		//P_HS_MD_T tmp = (*it);
		//printf( "last vector: symbol - %s, time - %s %d\n\n", tmp->hs_head.m_symbol, tmp->hs_head.m_update_time, tmp->hs_head.m_millisecond );
	}	

	delete file_buf;
	fclose( pf );
	return count;

}

int hs_wirte_log( string filename, int check_write )
{
	static FILE * pf = NULL;
	if( check_write )
	{
		if( signal_log_line < 100 && signal_log_pos < 60000 )
		{
			return 0;
		}
	}

	if( NULL == pf )
	{
		if( (pf = fopen( filename.c_str(), "w+" )) == NULL )
		{
			printf("fopen %s error !", filename.c_str());
		}
	}
	//printf("===== write[%d] pos:%d - line:%d =====\n", check_write, signal_log_pos, signal_log_line);
	int count = fwrite(signal_log_buffer, 1, strlen(signal_log_buffer), pf);
	signal_log_line = 0;
	signal_log_pos = 0;

	if( !check_write )
	{
		fclose(pf);
		pf = NULL;
	}
	return count;	
}

int hs_write_result( string filename )
{
	FILE * pf;

	if( (pf = fopen( filename.c_str(), "w+" )) == NULL )
	{
		printf("fopen %s error !", filename.c_str());
	}
	int count = fwrite(log_buffer, 1, strlen(log_buffer), pf);
	fclose(pf);
	return count;
}

int hs_write_stastic()
{
	FILE * pf;
	if( (pf = fopen( "./log_result/strategy_stastic.csv", "w+" )) == NULL )
	{
		printf("fopen %s error !", "./log_result/strategy_stastic.csv");
	}

	int count = fwrite(stastic_buffer, 1, strlen(stastic_buffer), pf);
	fclose(pf);
	return count;
}


void hs_reset_global()
{
	g_ins->get_forecaster()->reset();
	g_cur_st->m_pos.m_buy_pos = 0;
	g_cur_st->m_pos.m_sell_pos = 0;
	g_order.Volume = 0;
	g_order.LimitPrice = 0;
	g_order.Direction = 0;
	g_order.OffsetFlag = 0;
}

double calc_result( vector<P_HS_MD_T> pcap_md_list )
{
	vector<P_HS_MD_T>::iterator it;
	double PNL = 0;
	for( it=pcap_md_list.begin(); it!=pcap_md_list.end(); it++ )
	{
		P_HS_MD_T ptr_md_data = (*it);
		int ret_flag = 1;
		//printf("== %s ==\n", ptr_md_data->hs_head.m_symbol);
		if(strcmp(ptr_md_data->hs_head.m_symbol, trading_instrument) == 0)
		{
			ret_flag = 0;
		} 
		else 
		{
			int i;
			for(i=0; i< array_ref_ins.size(); ++i) 
			{
				if(strcmp(array_ref_ins[i].c_str(), ptr_md_data->hs_head.m_symbol ) == 0) 
				{
					ret_flag = 0;
				}
			}
		}

		if( ret_flag )
		{
			delete ptr_md_data;
			ptr_md_data = NULL;
			continue;
		}
/*
		printf("-|||=== md_func get instrument: %s, flag:%d\n", ptr_md_data->hs_head.m_symbol, ptr_md_data->hs_head.m_quote_flag);
		printf("-|||=== time: %s.%d\n", ptr_md_data->hs_head.m_update_time, ptr_md_data->hs_head.m_millisecond);
		printf("||||||=== bid: %f - volume:%d\n",ptr_md_data->hs_body.hs_md_nor.m_bid_px, ptr_md_data->hs_body.hs_md_nor.m_bid_share);
		printf("||||||=== ask: %f - volume:%d\n",ptr_md_data->hs_body.hs_md_nor.m_ask_px, ptr_md_data->hs_body.hs_md_nor.m_ask_share);
		printf("||||||=== last: %f - volume:%d\n",ptr_md_data->hs_body.hs_md_nor.m_last_px, ptr_md_data->hs_body.hs_md_nor.m_last_share);
		printf("||||||=== value:%f - pos: %d\n", ptr_md_data->hs_body.hs_md_nor.m_total_value, ptr_md_data->hs_body.hs_md_nor.m_total_pos);	
*/
		P_HS_MD_T ptr_valid_data = g_valid_md[string(ptr_md_data->hs_head.m_symbol)];
		memcpy( &ptr_valid_data->hs_head, &ptr_md_data->hs_head, sizeof(struct guava_udp_head));
		switch( ptr_valid_data->hs_head.m_quote_flag)
		{
			case 3:  // 3 with time sale, with lev1
			{
				memcpy( &ptr_valid_data->hs_body, &ptr_md_data->hs_body, sizeof(HS_MD_BODY_U));
				break;
			}
			case 2:   // 2 no time sale��with lev1
			{
				ptr_valid_data->hs_body.hs_md_nor.m_bid_px= ptr_md_data->hs_body.hs_md_nor.m_bid_px;
				ptr_valid_data->hs_body.hs_md_nor.m_bid_share = ptr_md_data->hs_body.hs_md_nor.m_bid_share;
				ptr_valid_data->hs_body.hs_md_nor.m_ask_px = ptr_md_data->hs_body.hs_md_nor.m_ask_px;
				ptr_valid_data->hs_body.hs_md_nor.m_ask_share = ptr_md_data->hs_body.hs_md_nor.m_ask_share;
				break;
			}
			case 1:   //1 // 1 with time sale, no lev1
			{
				ptr_valid_data->hs_body.hs_md_nor.m_last_px = ptr_md_data->hs_body.hs_md_nor.m_last_px;
				ptr_valid_data->hs_body.hs_md_nor.m_last_share = ptr_md_data->hs_body.hs_md_nor.m_last_share;
				ptr_valid_data->hs_body.hs_md_nor.m_total_pos = ptr_md_data->hs_body.hs_md_nor.m_total_pos;
				ptr_valid_data->hs_body.hs_md_nor.m_total_value = ptr_md_data->hs_body.hs_md_nor.m_total_value;
				break;
			}
			case 0:   // 0 no time sale, no lev1
			{
				break;
			}
			case 4:  // summary message
			default: // unkonw
			{
				delete ptr_md_data;
				ptr_md_data = NULL;
				continue;
			}

		}
		PNL += md_func( ptr_valid_data->hs_head.m_symbol, (void *)ptr_valid_data, sizeof(HS_MD_T));
		hs_wirte_log( log_file_name, 1 );
		//printf("=total PNL=%f=\n", PNL);

		delete ptr_md_data;
		ptr_md_data = NULL;
	}
	//printf("==return PNL=%f==\n", PNL);
	PNL += md_func( trading_instrument, g_valid_md[string(trading_instrument)], 0);
	return PNL;
}


/* netmd worker thread */
int netmd_pcap_thread(const char *devname, const char* packet_filter) {
    pcap_t *adhandle;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_int netmask =0xffffff;
    /* setup the package filter  */
    /* const char packet_filter[] = "udp and port 30100"; */
    struct bpf_program fcode;

    /* Open the adapter */
    if ( (adhandle= pcap_open_live(devname, // name of the device
                                   65536, // portion of the packet to capture.
                                   // 65536 grants that the whole packet will be captured on all the MACs.
                                   1, // promiscuous mode
                                   1000, // read timeout
                                   errbuf // error buffer
                                   ) ) == NULL)
    {
        lmice_error_print("\nUnable to open the adapter[%s], as %s\n", devname, errbuf);
        return -1;
    } else {
        lmice_info_print("pcap_open_live done\n");
    }

    /*set buffer size 1MB, use default */
    pcap_set_buffer_size(adhandle, 1024*1024);

    /*compile the filter*/
    if(pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) <0 ){
        lmice_error_print("\nUnable to compile the packet filter. Check the syntax.\n");
        return -1;
    }
    /*set the filter*/
    if(pcap_setfilter(adhandle, &fcode)<0){
        lmice_error_print("\nError setting the filter.\n");
        return -1;
    }

    pcapHandle = adhandle;

    /* start the capture */
    lmice_critical_print("pcap startting...\n");
    pcap_loop(adhandle, -1, netmd_pcap_callback, (u_char*)0);
    lmice_critical_print("pcap stopped.\n");
}

void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet) {
    const char* data = (const char*)(packet+42);

    (void)pkthdr;
    (void)arg;
    // callq  4e990 <_ZN12CFTDCPackage14PreparePackageEjhh>

    g_pkg_time.tv_sec = pkthdr->ts.tv_sec;
    g_pkg_time.tv_usec = pkthdr->ts.tv_usec;


    /** pub data */
    netmd_pub_data( data + position, data, pkthdr->len - 42);


}

void netmd_pcap_stop(int sig) {
    if(pcapHandle) {
        pcap_breakloop(pcapHandle);
        pcapHandle = NULL;
    }
}

//static int key_compare(const void* key, const void* obj) {
//    const uint64_t* hval = (const uint64_t*)key;
//    const uint64_t* val = (const uint64_t*)obj;

//    if(*hval == *val)
//        return 0;
//    else if(*hval < *val)
//        return -1;
//    else
//        return 1;
//}

//forceinline int key_find_or_create(const char* symbol) {
//    uint64_t hval;
//    uint64_t *key = NULL;
//    hval = eal_hash64_fnv1a(symbol, 32);

//    if(keypos == 0) {
//        keylist[keypos] = hval;
//        ++keypos;
//        return 1;
//    }

//    key = (uint64_t*)bsearch(&hval, keylist, keypos, 8, key_compare);
//    if(key == NULL) {
//        /* create new element */
//        if(keypos < MAX_KEY_LENGTH) {
//            keylist[keypos] = hval;
//            ++keypos;
//            qsort(keylist, keypos, 8, key_compare);
//            return 1;
//        } else {
//            lmice_warning_print("Add key[%s] failed as list is full\n", symbol);
//            return -1;
//        }
//    }

//    return 0;

//}

forceinline void netmd_pub_data(const char* sym, const void* addr, int len) {
    size_t i;

    if(g_spi->status() != FMTRADER_LOGIN) {
        return;
    }
    get_system_time(&g_begin_time);

    if(strcmp(sym, trading_instrument) == 0){
       // lmice_critical_print("md_func %s\n", sym);

        md_func(sym, addr, len);
 //       lmice_critical_print("pcap time:%ld\n", g_begin_time - g_pkg_time.tv_sec*10000000L - g_pkg_time.tv_usec*10L);
    } else {
        for(i=0; i< array_ref_ins.size(); ++i) {
            if(strcmp(array_ref_ins[i].c_str(), sym ) == 0) {

                md_func(sym, addr, len);
   //             lmice_critical_print("pcap time:%ld\n", g_begin_time - g_pkg_time.tv_sec*10000000L - g_pkg_time.tv_usec*10L);
                //lmice_critical_print("md_func %s\n", sym);
                break;
            }
        }
    }

}

volatile int g_guava_quit_flag = 0;
void guava_md_stop(int sig) {
    g_guava_quit_flag = 1;
}

static inline void guava_md(int mc_port, const char *mc_group, const char *mc_bindip) {
    int m_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(-1 == m_sock)
    {
        return;
    }

    //socket可以重新使用一个本地地址
    int flag=1;
    if(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag)) != 0)
    {
        return;
    }

    int options = fcntl(m_sock, F_GETFL);
    if(options < 0)
    {
        return;
    }
    options = options | O_NONBLOCK;
    int i_ret = fcntl(m_sock, F_SETFL, options);
    if(i_ret < 0)
    {
        return;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(mc_port);	//multicast port
    if (bind(m_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        return;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(mc_group);	//multicast group ip
    mreq.imr_interface.s_addr = inet_addr(mc_bindip);

    if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
    {
        return;
    }

    int receive_buf_size  = 65536;
    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&receive_buf_size, sizeof(receive_buf_size)) != 0)
    {
        return;
    }

    char line[65536];
    struct sockaddr_in muticast_addr;
    socklen_t len = sizeof(sockaddr_in);
    for(;;) {
        int n_rcved = -1;
        n_rcved = recvfrom(m_sock, line, receive_buf_size, 0, (struct sockaddr*)&muticast_addr, &len);

        //检测线程退出信�?
        if (g_guava_quit_flag == 1)
        {
            //此时已关闭完所有的客户�?
            return;
        }

        if ( n_rcved > 0)
        {
            netmd_pub_data(line+position, line, n_rcved);
        }
        else
        {
            continue;
        }
    

    }

}
