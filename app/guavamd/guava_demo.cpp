#include <iostream>
#include <stdio.h>
#include <sys/time.h>

#include "guava_demo.h"
#include "profile.h"
#include "fieldhelper.h"


using std::cout;
using std::cin;
using std::endl;


guava_demo::guava_demo(void)
{
	//
    m_pFileOutput=NULL;
//    m_pFileOutput = fopen("output_guava.csv", "wt");

	m_quit_flag = false;
}


guava_demo::~guava_demo(void)
{
}


void guava_demo::run()
{
	input_param();

	bool ret = init();
	if (!ret)
	{
		string str_temp;

		printf("\n input any char to exit:\n");
		cin >> str_temp;

		return;
	}

	while(!m_quit_flag)
	{
		pause();
	}
	
	close();
}


void guava_demo::input_param()
{
	//string local_ip = "10.1.52.213";
    string local_ip = "192.168.208.16";
//	string str_temp;
//	string str_no = "n";

	/*cout << "local ip: " << local_ip << endl;
	cout << "if use local ip: (y/n) ";*/
	//cin >> str_temp;
	/*if (str_no == str_temp)
	{
		cout << "input new local ip: ";
		cin >> str_temp;
		local_ip = str_temp;
	}*/

	//string cffex_ip = "233.54.12.127";
	string cffex_ip = "233.54.1.100";
	//int cffex_port = 30009;
	int cffex_port = 30100;
	string cffex_local_ip = local_ip;
	//scylla--not necessary
	int cffex_local_port = 23501;

	/*cout << "行情通道组播地址: " << cffex_ip << endl;
	cout << "行情通道组播端口: " << cffex_port << endl;
	cout << "行情通道本机地址: " << cffex_local_ip << endl;
	cout << "行情通道本机端口: " << cffex_local_port << endl;
	cout << "是否使用中金通道缺省配置 (y/n) ";
	cin >> str_temp;
	if (str_no == str_temp)
	{
		cout << "是否使用缺省行情通道组播地址 " << cffex_ip << " (y/n) ";
		cin >> str_temp;
		if (str_no == str_temp)
		{
			cout << "请输入新的行情通道组播地址: ";
			cin >> str_temp;
			cffex_ip = str_temp;
		}

		cout << "是否使用缺省行情通道组播端口 " << cffex_port << " (y/n) ";
		cin >> str_temp;
		if (str_no == str_temp)
		{
			cout << "请输入新的行情通道组播端口: ";
			cin >> str_temp;
			cffex_port = atoi(str_temp.c_str());
		}

		cout << "是否使用缺省行情通道本机地址 " << cffex_local_ip << " (y/n) ";
		cin >> str_temp;
		if (str_no == str_temp)
		{
			cout << "请输入新的行情通道本机地址: ";
			cin >> str_temp;
			cffex_local_ip = str_temp;
		}

		cout << "是否使用缺省行情通道本机端口 " << cffex_local_port << " (y/n) ";
		cin >> str_temp;
		if (str_no == str_temp)
		{
			cout << "请输入新的行情通道本机端口: ";
			cin >> str_temp;
			cffex_local_port = atoi(str_temp.c_str());
		}
	}*/


	multicast_info temp;

	memset(&temp, 0, sizeof(multicast_info));
	
	strcpy(temp.m_remote_ip, cffex_ip.c_str());
	temp.m_remote_port = cffex_port;
	strcpy(temp.m_local_ip, cffex_local_ip.c_str());
	temp.m_local_port = cffex_local_port;

	m_cffex_info = temp;
}


bool guava_demo::init()
{


	return m_guava.init(m_cffex_info, this);
}

void guava_demo::close()
{
	m_guava.close();
}

void guava_demo::pause()
{
	string str_temp;

	printf("\n按任意字符继续(输入q将退出):\n");
	cin >> str_temp;
	if (str_temp == "q")
	{
		m_quit_flag = true;
	}
}

/************************************************************************/
/* function: get the current local timestamp
*  author: lgz
*  create date: 2016.4.29
*/
/************************************************************************/
void gettimestamp(char timestamp[13])
{
	struct  timeval    tv;

	struct  timezone   tz;
	long tv_usec;

	gettimeofday(&tv, &tz);

	tv_usec = tv.tv_usec;

	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	sprintf(timestamp, "%.2d:%.2d:%.2d:%ld",
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv_usec);
}

void guava_demo::on_receive_nomal(guava_udp_head* head, guava_udp_normal* data)
{
	//test rb1610 only
	//cout << "head->m_symbol: " << head->m_symbol << endl;
    //string file="/run/user/1000/lmiced/%s.normal";
    char fname[64]={0};
    sprintf(fname, "/run/user/1000/lmiced/%s.normal", head->m_symbol);
    {
        FILE* fp = fopen(fname, "a");
        fwrite(head, 1, sizeof(struct guava_udp_head), fp);
        fwrite(data, 1, sizeof(struct guava_udp_normal), fp);
        fclose(fp);
    }
//	if(0 == strcmp("rb1610", head->m_symbol))
//	{
//		//string str_head = to_string(head);
//		//string str_body = to_string(data);

//		//cout << "receive nomal head: " << str_head << "  body: " << str_body << endl;

//		//char buf[4096];
//		long sec, usec;
//		//local timestamp
//		GET_SEC_USEC_TIME(sec, usec);
//		//input local time into file:output_guava.csv in format: sec,usec
//		fprintf(m_pFileOutput, "%ld,%ld,", sec, usec);

//		//input local time into screen:output_guava.csv in format: sec,usec
//		fprintf(stdout, "%ld,%ld, ", sec, usec);

//		//cout << "1111111...." << endl;

//		//ex timstamp, instID, curPrice
//		//fprintf(m_pFileOutput,"%s \n",buf);
//		//fprintf(m_pFileOutput,"%s,%s\n",str_head, str_body);
//		char timestamp[13];
//		gettimestamp(timestamp);


//		fprintf(m_pFileOutput,"%s,%s,%d,%s,%.5f,%d,%.5f,%.5f,\n",
//			timestamp, head->m_update_time, head->m_millisecond, head->m_symbol, data->m_last_px, data->m_last_share, data->m_total_value, data->m_total_pos);

//		fprintf(stdout,"%s,%s,%d,%s,%.5f,%ld,%.5f,%.5f,\n",
//			timestamp, head->m_update_time, head->m_millisecond, head->m_symbol, data->m_last_px, data->m_last_share, data->m_total_value, data->m_total_pos);

//		fflush(m_pFileOutput);
//	}

	/*string str_head = to_string(head);
	string str_body = to_string(data);

	cout << "receive nomal head: " << str_head << "  body: " << str_body << endl;*/
	
}

void guava_demo::on_receive_summary(guava_udp_head* head, guava_udp_summary* data)
{
    char fname[64]={0};
    sprintf(fname, "/run/user/1000/lmiced/%s.summary", head->m_symbol);
    {
        FILE* fp = fopen(fname, "a");
        fwrite(head, 1, sizeof(struct guava_udp_head), fp);
        fwrite(data, 1, sizeof(struct guava_udp_summary), fp);
        fclose(fp);
    }
	//test rb1610 only
//	if(0 == strcmp("rb1610", head->m_symbol))
//	{
//		//string str_head = to_string(head);
//		//string str_body = to_string(data);
//		//cout << "receive nomal head: " << str_head << "  body: " << str_body << endl;

//		//char buf[4096];
//		long sec, usec;
//		//local timestamp
//		GET_SEC_USEC_TIME(sec, usec);
//		fprintf(m_pFileOutput, "%ld,%ld, ", sec, usec);

//		//ex timstamp, instID, curPrice
//		//fprintf(m_pFileOutput,"%s \n",buf);
//		//fprintf(m_pFileOutput,"%s,%s\n",str_head, str_body);

//		char timestamp[13];
//		gettimestamp(timestamp);

//		fprintf(m_pFileOutput,"%s,%s,%d,%s,%.5f,%.5f,%.5f,%.5f \n",
//			timestamp, head->m_update_time, head->m_millisecond, head->m_symbol, data->m_open, data->m_high, data->m_low, data->m_today_close);
//	}

	//string str_head = to_string(head);
	//string str_body = to_string(data);

	//cout << "receive nomal head: " << str_head << "  body: " << str_body << endl;
}

string guava_demo::to_string(guava_udp_head* ptr)
{
	char buff[8192];

	memset(buff, 0, sizeof(buff));
	//sprintf(buff, "%u,%d,%d,%d,%d,%s,%s,%d,%d",ptr->m_sequence,(int)(ptr->m_exchange_id),(int)(ptr->m_channel_id), (int)(ptr->m_symbol_type_flag),ptr->m_symbol_code,ptr->m_symbol,ptr->m_update_time, ptr->m_millisecond, (int)(ptr->m_quote_flag));
	sprintf(buff, "%u,%d,%d,%s,%s,%d,%d",ptr->m_sequence,(int)(ptr->m_exchange_id),(int)(ptr->m_channel_id),ptr->m_symbol,ptr->m_update_time, ptr->m_millisecond, (int)(ptr->m_quote_flag));

	string str = buff;
	return str;
}

string guava_demo::to_string(guava_udp_normal* ptr)
{
	char buff[8192];

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%.4f,%d,%.4f,%.4f,%.4f,%d,%.4f,%d"
		,ptr->m_last_px
		,ptr->m_last_share
		,ptr->m_total_value
		,ptr->m_total_pos
		,ptr->m_bid_px
		,ptr->m_bid_share
		,ptr->m_ask_px
		,ptr->m_ask_share
		);

	string str = buff;
	return str;
}

string guava_demo::to_string(guava_udp_summary* ptr)
{
	char buff[8192];

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f"
		,ptr->m_open
		,ptr->m_high
		,ptr->m_low
		,ptr->m_today_close
		,ptr->m_high_limit
		,ptr->m_low_limit
		,ptr->m_today_settle
		,ptr->m_curr_delta
		);

	string str = buff;
	return str;
}




