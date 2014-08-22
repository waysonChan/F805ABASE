#ifndef _CFG_FILE_H
#define _CFG_FILE_H

#include "config.h"
#include "libconfig.h"

#include <time.h>

#define READER_NAME_LEN			8
#define READER_TYPE_LEN			6
#define PRODUCT_SN_LEN			8
#define READER_MCU_SWREV_LEN		4
#define READER_FPGA_SWREV_LEN		4
#define READER_BSB_HWREV_LEN		4
#define READER_RFB_SWREV_LEN		4
#define PASSWORD_LEN			6

typedef enum {
	CFG_READER_NAME,
	CFG_READER_TYPE,
	CFG_READER_SN,
	CFG_MCU_SWREV,
	CFG_FPGA_SWREV,
	CFG_BSB_HWREV,
	CFG_RFB_HWREV,
	CFG_PASS_WORD
} cfg_sysinfo_e;

typedef struct _sysinfo {
	char reader_name[READER_NAME_LEN+1];
	char reader_type[READER_TYPE_LEN+1];
	char reader_sn[PRODUCT_SN_LEN+1];
	char mcu_swrev[READER_MCU_SWREV_LEN+1];		/* ARM9软件版本 */
	char fpga_swrev[READER_FPGA_SWREV_LEN+1];	/* FPGA软件版本 */
	char bsb_hwrev[READER_BSB_HWREV_LEN+1];		/* 基带板硬件版本 */
	char rfb_hwrev[READER_RFB_SWREV_LEN+1];		/* 射频板硬件版本 */
	char pass_word[PASSWORD_LEN+1];
} sysinfo_t, *sysinfo_pt;

typedef enum {
	CFG_BUS_ADDR,
	CFG_BAUD_RATE,
	CFG_DATABITS,
	CFG_STOPBIT,
	CFG_PARITY,
	CFG_FLOW_CTRL,
} cfg_rs232_e;

typedef struct _rs232 {
	int bus_addr;
	int baud_rate;		/* 159 */
	int databits;
	int stopbit;
	int parity;
	int flow_ctrl;
} rs232_t, *rs232_pt;

#pragma pack(1)
typedef struct {
	uint8_t enable;				/* 读写器天线使能设置 */
	uint8_t rfpower;			/* 读写器各天线端口默认功率 */
	uint8_t switch_time;			/* 读写器天线切换时间 */
} antenna_t;
#pragma pack()

typedef enum {
	CFG_IP,
	CFG_MASK,
	CFG_GATEWAY,
	CFG_MAC,
	CFG_TCP_PORT,
	CFG_UDP_PORT,
} cfg_eth0_e;

#define MAC_ADDR_LEN			18
#define ETH_ADDR_LEN			16
#define IP_CONFG_LEN			12
typedef struct _eth0 {
	char ip[ETH_ADDR_LEN];
	char mask[ETH_ADDR_LEN];
	char gateway[ETH_ADDR_LEN];
	int tcp_port;
	int udp_port;
	int mac_changed;
	char mac[MAC_ADDR_LEN+1];
} eth0_t, *eth0_pt;

typedef struct {
	int tcp_port;
	char ip[ETH_ADDR_LEN];
} data_center_t;

typedef enum {
	GPO_IDX_BEEP,
	GPO_IDX_1,
	GPO_IDX_2,
	GPO_IDX_3,
	GPO_IDX_4,
	GPO_IDX_BLINK,			/* 只读 */
	GPO_NUMBER
} gpo_index_e;

typedef struct {
	int pulse_timer;
	uint8_t pulse_width;
	struct itimerspec pulse_its;	/* 脉冲宽度定时器 */
} gpo_t;

#define ANT_IDX_POLL		0x00
#define ANT_IDX_1		0x01
#define ANT_IDX_2		0x02
#define ANT_IDX_3		0x03
#define ANT_IDX_4		0x04

#define UPLOAD_MODE_NONE	0x00
#define UPLOAD_MODE_RS232	0x01
#define UPLOAD_MODE_RS485	0x02
#define UPLOAD_MODE_WIEGAND	0x03
#define UPLOAD_MODE_WIFI	0x04
#define UPLOAD_MODE_GPRS	0x05

#define NAND_FLASH_ENBABLE	0x01
#define NAND_FLASH_DISABLE	0x02

#define OPERATE_READ_EPC	0x91
#define OPERATE_READ_TID	0x92
#define OPERATE_READ_USER	0x94

#define WG_START_EPC		0x05
#define WG_START_TID		0x10

#define WG_LEN_26		0x03
#define WG_LEN_34		0x04

#define TID_LEN_4		0x04
#define TID_LEN_6		0x06
#define TID_LEN_8		0x08

#define WORK_MODE_COMMAND	0
#define WORK_MODE_AUTOMATIC	1
#define WORK_MODE_TRIGGER	2

/*
 * dev_type:
 * 	bit3-bit0: 表示基础设备类型，默认支持网口，232，485，韦根
 *		0 - F805S
 *		1 - I802-ANT4
 *		2 - I802-ANT1
 *	bit4: WIFI
 *	bit5: GPRS
 */
#define DEV_TYPE_FLAG_WIFI		(0x01 << 4)
#define DEV_TYPE_FLAG_GPRS		(0x01 << 5)
#define DEV_TYPE_BASE_MASK		0x0F
#define DEV_TYPE_BASE_F805S		0x00
#define DEV_TYPE_BASE_I802S_ANT4	0x01
#define DEV_TYPE_BASE_I802S_ANT1	0x02

typedef struct {
	int ant_idx;		/* 240: 天线端口 */
	int upload_mode;	/* 241: 处理模式 */
	int flash_enable;	/* 249: 标签缓存 */
	int gpo_mode;		/* IO输出 */
	int oper_mode;		/* 239: 操作模式 */

	int work_mode;		/* 0 */
	int dev_type;		/* 1 */
	int wg_start;		/* 160 */
	int wg_len;		/* 161 */
	int wg_pulse_width;	/* 220: 脉冲宽度100us */
	int wg_pulse_periods;	/* 221: 脉冲周期1ms */
	int tid_len;		/* 162 */

	int hop_freq_enable;
} pre_cfg_t;

int cfg_get_sysinfo(sysinfo_pt sysinfo);
int cfg_set_sysinfo(sysinfo_pt sysinfo, cfg_sysinfo_e type);
int cfg_get_rs232(rs232_pt rs232);
int cfg_set_rs232(rs232_pt rs232, cfg_rs232_e type);
int cfg_get_rs485(rs232_pt rs232);
int cfg_set_rs485(rs232_pt rs232, cfg_rs232_e type);
int cfg_get_ant(antenna_t *ant, int ant_index);
int cfg_set_ant(antenna_t *ant, int ant_index);
int cfg_get_gpo(gpo_t *gpo, gpo_index_e gpo_index);
int cfg_set_gpo(gpo_t *gpo, gpo_index_e gpo_index);
int cfg_get_pre_cfg(pre_cfg_t *pre_cfg);
int cfg_set_pre_cfg(pre_cfg_t *pre_cfg);
int cfg_get_filter_enable(uint8_t *filter_enable);
int cfg_set_filter_enable(uint8_t filter_enable);
int cfg_get_filter_time(uint8_t *filter_time);
int cfg_set_filter_time(uint8_t filter_time);
int cfg_get_eth0(eth0_pt eth0);
int cfg_set_eth0(eth0_pt eth0, cfg_eth0_e type);
int cfg_get_data_center(data_center_t *data_center);
int cfg_set_data_center(data_center_t *data_center);
int cfg_get_freq_table(uint8_t *freq_table, int len);
int cfg_set_freq_table(uint8_t *freq_table, int len);

#endif	/* _CFG_FILE_H */
