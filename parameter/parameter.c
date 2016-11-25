#include "cfg_file.h"
#include "parameter.h"
#include "param_ether.h"
#include "config.h"
#include "gpio.h"
#include "report_tag.h"
#include "utility.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>

const uint32_t baud_table[6] = {4800, 9600, 19200, 38400, 57600, 115200};

int sp_set_bus_addr(system_param_t *S, uint8_t bus_addr)
{
	rs232_t rs232 = {.bus_addr = bus_addr};
	if (cfg_set_rs232(&rs232, CFG_BUS_ADDR) < 0)
		return -1;

	S->rs232.bus_addr = bus_addr;
	return 0;
}

int sp_set_baud_rate(system_param_t *S, uint8_t baud_rate)
{
	if (baud_rate >= (sizeof(baud_table)/sizeof(baud_table[0]))) {
		log_msg("%s: invalid parameter.", __FUNCTION__);
		return -1;
	}
	
	rs232_t rs232 = {.baud_rate = baud_rate};
	if (cfg_set_rs232(&rs232, CFG_BAUD_RATE) < 0)
		return -1;

	S->rs232.baud_rate = baud_rate;
	return 0;
}

/*---------------------------------------------------------------------
 * MAC 地址
 *--------------------------------------------------------------------*/
void sp_get_mac_addr(system_param_t *S, uint8_t *mac)
{
	int i;
	uint32_t tmp[6] = {0};
	
	sscanf(S->eth0.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		&tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);
	for (i = 0; i < 6; i++)
		mac[i] = tmp[i] & 0xFF;
}

int sp_set_mac_addr(system_param_t *S, const uint8_t *mac_addr)
{
	memset(S->eth0.mac, 0, MAC_ADDR_LEN);
	snprintf(S->eth0.mac, MAC_ADDR_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
		mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	if (cfg_set_eth0(&S->eth0, CFG_MAC) < 0)
		return -1;

	return 0;
}

/*---------------------------------------------------------------------
 * IP 配置
 *--------------------------------------------------------------------*/
void sp_get_ip_config(system_param_t *S, uint8_t *p)
{
	int i, tmp[4];
	
	sscanf(S->eth0.ip, "%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	for (i = 0; i < 4; i++)
		p[i] = tmp[i] & 0xFF;

	sscanf(S->eth0.mask, "%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	for (i = 0; i < 4; i++)
		p[i+4] = tmp[i] & 0xFF;

	sscanf(S->eth0.gateway, "%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	for (i = 0; i < 4; i++)
		p[i+8] = tmp[i] & 0xFF;
}

int sp_set_ip_config(system_param_t *S, const uint8_t *ptr)
{
	/* ip */
	snprintf(S->eth0.ip, ETH_ADDR_LEN, "%d.%d.%d.%d",
		ptr[0], ptr[1], ptr[2], ptr[3]);
	if (cfg_set_eth0(&S->eth0, CFG_IP) < 0)
		return -1;

	/* mask */
	snprintf(S->eth0.mask, ETH_ADDR_LEN, "%d.%d.%d.%d",
		ptr[4], ptr[5], ptr[6], ptr[7]);
	if (cfg_set_eth0(&S->eth0, CFG_MASK) < 0)
		return -1;

	/* gateway */
	snprintf(S->eth0.gateway, ETH_ADDR_LEN, "%d.%d.%d.%d",
		ptr[8], ptr[9], ptr[10], ptr[11]);
	if (cfg_set_eth0(&S->eth0, CFG_GATEWAY) < 0)
		return -1;

	return 0;
}

/*---------------------------------------------------------------------
 * 数据中心IP 配置
 *--------------------------------------------------------------------*/
void sp_get_dsc_ip(system_param_t *S, uint8_t *ip)
{
	int i, tmp[4];

	sscanf(S->data_center.ip, "%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	for (i = 0; i < 4; i++)
		ip[i] = tmp[i] & 0xFF;	
}

int sp_set_dsc_ip(system_param_t *S, const uint8_t *ip)
{
	snprintf(S->data_center.ip, ETH_ADDR_LEN, "%d.%d.%d.%d",
		ip[0], ip[1], ip[2], ip[3]);
	if (cfg_set_data_center(&S->data_center) < 0)
		return -1;

	return 0;
}

int sp_set_tcp_port(system_param_t *S, uint16_t tcp_port)
{
	eth0_t eth0 = {.tcp_port = tcp_port};
	if (cfg_set_eth0(&eth0, CFG_TCP_PORT) < 0)
		return -1;

	S->eth0.tcp_port = tcp_port;
	return 0;
}

int sp_set_udp_port(system_param_t *S, uint16_t udp_port)
{
	eth0_t eth0 = {.udp_port = udp_port};
	if (cfg_set_eth0(&eth0, CFG_UDP_PORT) < 0)
		return -1;

	S->eth0.udp_port = udp_port;
	return 0;
}

int sp_set_reader_name(system_param_t *S, const char *name, size_t sz)
{
	uint16_t len = sz <= READER_NAME_LEN ? sz : READER_NAME_LEN;

	memset(S->sysinfo.reader_name, 0, READER_NAME_LEN);
	strncpy(S->sysinfo.reader_name, name, len);	
	if (cfg_set_sysinfo(&S->sysinfo, CFG_READER_NAME) < 0)
		return -1;

	return 0;
}

int sp_set_reader_type(system_param_t *S, const char *type, size_t sz)
{
	uint16_t len = sz <= READER_TYPE_LEN ? sz : READER_TYPE_LEN;

	memset(S->sysinfo.reader_type, 0, READER_TYPE_LEN);
	strncpy(S->sysinfo.reader_type, type, len);	
	if (cfg_set_sysinfo(&S->sysinfo, CFG_READER_TYPE) < 0)
		return -1;

	return 0;
}

int sp_set_reader_sn(system_param_t *S, const char *reader_sn, size_t sz)
{
	uint16_t len = sz <= PRODUCT_SN_LEN ? sz : PRODUCT_SN_LEN;

	memset(S->sysinfo.reader_sn, 0, PRODUCT_SN_LEN);
	strncpy(S->sysinfo.reader_sn, reader_sn, len);	
	if (cfg_set_sysinfo(&S->sysinfo, CFG_READER_SN) < 0)
		return -1;

	return 0;
}

/*---------------------------------------------------------------------
 * 读写器访问密码
 *--------------------------------------------------------------------*/
int sp_set_password(system_param_t *S, const char *password, size_t sz)
{
	uint16_t len = sz <= PASSWORD_LEN ? sz : PASSWORD_LEN;

	memset(S->sysinfo.pass_word, 0, PRODUCT_SN_LEN);
	strncpy(S->sysinfo.pass_word, password, len);	
	if (cfg_set_sysinfo(&S->sysinfo, CFG_PASS_WORD) < 0)
		return -1;

	return 0;
}

/*---------------------------------------------------------------------
 * select parameters
 *--------------------------------------------------------------------*/
int read_select_param(select_param_t *param)
{
	FILE *fp = fopen("/f806/select.bin", "r");
	if (!fp) {
		log_msg("fopen error");
		return -1;
	}

	size_t sz = fread(param, sizeof(select_param_t), 1, fp);
	if (sz != 1) {
		log_msg("fread error");
		fclose(fp);
		return -1;
	}
	
	return fclose(fp);
}

int write_select_param(select_param_t *param)
{
	FILE *fp = fopen("/f806/select.bin", "w+");
	if (!fp) {
		log_msg("fopen error");
		return -1;
	}

	size_t sz = fwrite(param, sizeof(select_param_t), 1, fp);
	if (sz != 1) {
		log_msg("fread error");
		fclose(fp);
		return -1;
	}
	
	return fclose(fp);
}

/*
 * 注意:此定时器防止读用户数据区时因收不到 COMMAND_END 而陷入
 * S->work_status 永远置为WS_READ_USER的状态
 */
int work_status_timer_int(system_param_t *S)
{
	S->work_status_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (S->work_status_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	}

	bzero(&S->work_status_its, sizeof(struct itimerspec));
	if (timerfd_settime(S->work_status_timer, 0, &S->work_status_its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(S->work_status_timer);
		return -1;
	}

	return 0;
}

int work_status_timer_set(system_param_t *S, int ms)
{
	struct itimerspec its = {
		.it_interval.tv_sec = 0,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = ms / 1000,
		.it_value.tv_nsec = (ms % 1000) * 1000000,
	};

	if (timerfd_settime(S->work_status_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime");
		return -1;
	}

	S->work_status_its = its;
	return 0;
}


/*---------------------------------------------------------------------
 * 初始化参数表
 *--------------------------------------------------------------------*/
system_param_t *sys_param_new(void)
{
	
	system_param_t *S = malloc(sizeof(system_param_t));
	if (NULL == S) {
		log_quit("malloc error");
	}
	memset(S, 0, sizeof(system_param_t));

	if ((S->gpio_dece.fd = gpio_init()) < 0)
		log_msg("gpio_init error");

	cfg_get_rs232(&S->rs232);
	cfg_get_rs485(&S->rs485);
	cfg_get_sysinfo(&S->sysinfo);
	cfg_get_eth0(&S->eth0);
	cfg_get_pre_cfg(&S->pre_cfg);

	if (S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS) {
		system("touch /f806/gprs-enable");
	} else {
		system("rm -f /f806/gprs-enable");
	}

	char *sw_ver = "1.4.22";
	strncpy(S->sysinfo.mcu_swrev, sw_ver, strlen(sw_ver));
	log_msg("##########APP Current Version:%s##################",S->sysinfo.mcu_swrev);

	/* 配置 eth0 */
	uint8_t mac[6];
	if (S->eth0.mac_changed) {
		sp_get_mac_addr(S, mac);
		ether_set_mac_addr(mac);
	} else {
		ether_get_mac_addr(mac);
		sp_set_mac_addr(S, mac);
	}

	uint8_t eth0[12];
	sp_get_ip_config(S, eth0);
	ether_set_ip_config(eth0, S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS);

	/* data center */
	cfg_get_data_center(&S->data_center);

	/*
	 * 天线端口参数初始化
	 * 注意: S->cur_ant 必须指向实际的天线,否则set_next_active_antenna会报错
	 */
	int ant_index = get_active_antenna();
	if (ant_index < 0) {
		log_msg("%s: get_active_antenna() ERR!", __FUNCTION__);
		return NULL;
	}
	S->cur_ant = ant_index;
	gettimeofday(&S->last_ant_change_time, NULL);

	/* 拓展参数初始化 */
	uint8_t get_extended_table[10];
	int index = 0;
	for(index = 0;index < 10;index++){
		cfg_get_extended_table(get_extended_table,index);
	}
	memcpy(S->extended_table,get_extended_table,10);

	/* 工作状态 */	
	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) {
		switch (S->pre_cfg.oper_mode) {
		case OPERATE_READ_EPC:
			if (S->pre_cfg.ant_idx >= 1 && S->pre_cfg.ant_idx <= 4) {
				S->work_status = WS_READ_EPC_FIXED;
				S->cur_ant = S->pre_cfg.ant_idx;
			} else {
				S->work_status = WS_READ_EPC_INTURN;
			}
			break;
		case OPERATE_READ_TID:
			if (S->pre_cfg.ant_idx >= 1 && S->pre_cfg.ant_idx <= 4) {
				S->work_status = WS_READ_TID_FIXED;
				S->cur_ant = S->pre_cfg.ant_idx;
			} else {
				S->work_status = WS_READ_TID_INTURN;
			}			
			break;
		case OPERATE_READ_USER:
			/* 读用户区不支持轮询模式 */
			if (S->pre_cfg.ant_idx != 0) {
				S->cur_ant = S->pre_cfg.ant_idx;
			} else {
				S->cur_ant = 1;
			}
			S->work_status = WS_READ_USER;
			break;
		default:
			S->work_status = WS_STOP;
			log_msg("invalid operate mode");
		}
	}

	/* 天线指示灯 */
	int i;
	for (i = 0; i < ANTENNA_NUM; i++) {
		if (cfg_get_ant(&S->ant_array[i], i+1) < 0) {
			log_msg("cfg_get_ant error");
			goto out;
		}
		
		if (S->ant_array[i].enable) {
			set_antenna_led_status(i+1, LED_COLOR_GREEN, S->pre_cfg.dev_type);
		} else {
			set_antenna_led_status(i+1, LED_COLOR_NONE, S->pre_cfg.dev_type);
		}	
	}

	/* GPO初始化 */
	for (i = 0; i < GPO_NUMBER; i++) {
		if (cfg_get_gpo(&S->gpo[i], i) < 0) {
			log_msg("cfg_get_gpo error");
			goto out;
		}
	}

	gpo_pulse_timer_init(S->gpo);

	/* 系统状态指示灯 */
	set_gpio_status(LED1_GREEN, 1);
	set_gpio_status(LED1_RED, 1);

	/* 跳频表 */
	cfg_get_freq_table(S->freq_table, FREQ_MAP_LEN);

	work_status_timer_int(S);
	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC 
		|| S->pre_cfg.work_mode == WORK_MODE_TRIGGER){ 
		heartbeat_timer_int(S);
		delay_timer_init(S);
		triggerstatus_timer_init(S);
	}

	/* 设置APN   */
	if(S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS){
		set_gprs_apn(S->data_center.apn);
		set_chap_secrets(S->data_center.username, S->data_center.passwd);
		set_gprs_wave(S->data_center.username);
	}
	
out:
	return S;
}
