#ifndef _PARAMETER_H
#define _PARAMETER_H

#include "config.h"
#include "rfid_constants.h"
#include "cfg_file.h"

#include <time.h>

/*---------------------------------------------------------------------
 *	���ڲ�������
 *--------------------------------------------------------------------*/
#define	COMM_ADDR			0x00	/* ���������豸��ַ */
#define	COMM_RS232_BAUD			0x10	/* RS232 ������ */
#define	COMM_RS485_BAUD			0x20	/* RS485 ������ */
#define WIEGAND_LEN			0x30	/* Τ������ */

#define COMM_BAUDRATE_4800		0x00
#define COMM_BAUDRATE_9600		0x01
#define COMM_BAUDRATE_19200		0x02
#define COMM_BAUDRATE_38400		0x03
#define COMM_BAUDRATE_57600		0x04
#define COMM_BAUDRATE_115200		0x05

#define SERIAL_BUSADDR_LEN		1	/* �������ߵ�ַ���� */
#define UART_BAUDRATE_LEN		1	/* �����ʳ��� */

/*---------------------------------------------------------------------
 *	��̫����������
 *--------------------------------------------------------------------*/
#define	ETHER_MAC_ADDR			0x01	/* MAC ��ַ */
#define	ETHER_IP_ADDR			0x02	/* IP ��ַ */
#define	ETHER_SOCKET_TCP_PORT		0x03	/* TCP �˿ں� */
#define	ETHER_SOCKET_UDP_PORT		0x04	/* UDP �˿ں� */

#define IP_CONFIG_LEN			12	/* IP ���ó��� */
#define IP_ADDR_LEN			4	/* IP ��ַ���� */
#define NET_MASK_LEN			4	/* �������볤�� */
#define GATE_WAY_LEN			4	/* ���س��� */
#define SOCKET_NUM_LEN			2	/* �˿ںų��� */

/*---------------------------------------------------------------------
 *	�������ò�ѯ��غ궨��
 *--------------------------------------------------------------------*/
#define ANTENNA_NUM			4	/* �������� */

#define WS_STOP				0	/* ����ֹͣ���� */
#define WS_READ_EPC_FIXED		1	/* �̶����߶�EPC */
#define WS_READ_EPC_INTURN		2	/* ����������EPC */
#define WS_READ_TID_FIXED		3	/* �̶����߶�TID */
#define WS_READ_TID_INTURN		4	/* ����������TID */
#define WS_READ_EPC_TID_FIXED		5	/* �̶����߶�EPC+TID */
#define WS_READ_EPC_TID_INTURN		6	/* ����������EPC+TID */
#define WS_READ_USER			7	/* ���û��� */
#define WS_WRITE_USER			8	/* ���û��� */
#define WS_READ_EPC_FIXED_WEB		9	/* B/S�̶����߶�EPC */
#define WS_UPGRADE			10	/* �̼����� */

/*---------------------------------------------------------------------
 *	��ǩ������ز���
 *--------------------------------------------------------------------*/
#define PC_START_ADDR			0x01
#define KILL_PIN_ADDR			0x00
#define KILL_PIN_LEN			0x02
#define ACCESS_PIN_ADDR			0x02
#define ACCESS_PIN_LEN			0x02
/* ��ǩ������ */
#define LOCKSET_ALL_BANK		0x00
#define LOCKSET_TID_BANK		0x01
#define LOCKSET_EPC_BANK		0x02
#define LOCKSET_USER_BANK		0x03
#define LOCKSET_ACCESS_PIN		0x04
#define LOCKSET_KILL_PIN		0x05

/*---------------------------------------------------------------------
 *	Ƶ�ʲ�����ز���
 *--------------------------------------------------------------------*/
#define FCC_FREQSTAND	1
#ifdef  FCC_FREQSTAND	/* FCC */
#define FREQTAB_LEN			50
#define FREQSCAN_END			49
#define FREQSCAN_START			0
#define WORK_RFPOWER_MAX		26
#define FREQSCAN_STEP			5
#define DFU_JFTABLEN			10
#define FCC_FRE_TABLE_LEN		50
#define FRE_HOPING_TABLE_LEN		10
#else 			/* GB & CE */
#define FREQTAB_LEN			16
#define FREQSCAN_END			15
#define FREQSCAN_START			0
#define WORK_RFPOWER_MAX		26
#define FREQSCAN_STEP			1
#define DFU_JFTABLEN			8
#define FCC_FRE_TABLE_LEN		16
#define FRE_HOPING_TABLE_LEN		8
#endif

#define MAX_FREQ_LEN			FCC_FRE_TABLE_LEN
#define FREQ_MAP_LEN			50

typedef enum {
	TAG_SELECT_NONE,
	TAG_SELECT_ONCE,
	TAG_SELECT_ALWAYS
} tag_select_type_e;

/*
 * ע��:	1.R2000�Ĺ��ʵ�λ��0.1dB, �ӿ�Э������Ĺ��ʵ�λ����20
 *	2.R2000��פ��ʱ�䵥λ��ms, �ӿ�Э�������פ��ʱ�䵥λ��100ms
 */
#define RFPOWER_F806_TO_R2000(x) (((x)+20)*10)
#define RFPOWER_R2000_TO_F806(x) ((x)/10-20)

typedef struct {
	uint16_t bank;
	uint16_t offset;
	uint16_t count;
	uint8_t mask[64];	
} select_param_t;

typedef struct {
	/* inventory ���� */
	uint8_t q_val;

	/* kill ���� */
	uint8_t kill_pwd[4];

	/* access ���� */
	uint8_t access_pwd[4];
	RFID_18K6C_MEMORY_BANK access_bank;	/* ��д������bank */
	uint8_t access_offset;			/* ��д������word��ַ */
	uint8_t access_wordnum;			/* ��д������word�� */
	uint8_t access_databuf[1024];

	/* lock ���� */
	uint8_t lock_type;
	uint8_t lock_bank;
} tag_param_t;

typedef struct {
	/* ϵͳ���� */
	int work_status;			/* ��д������״̬ */
	int work_status_timer;
	struct itimerspec work_status_its;

	int heartbeat_timer;
	struct itimerspec heartbeat_its;

	pre_cfg_t pre_cfg;			/* Ԥ�ù������� */
	sysinfo_t sysinfo;
	rs232_t rs232;
	rs232_t rs485;
	eth0_t eth0;
	data_center_t data_center;

	/* �������� */
	int cur_ant;				/* ��д����ǰ��Ч���� */
	struct timeval last_ant_change_time;
	antenna_t ant_array[ANTENNA_NUM];

	/* 18K6CЭ����� */
	tag_param_t tag_param;

	/* GPO������ */
	gpo_t gpo[GPO_NUMBER];

	/* ��Ƶ�� */
	int freq_std;
	uint8_t freq_table[FREQ_MAP_LEN];

	gpio_desc gpio_dece;
} system_param_t;

extern const uint32_t baud_table[6];
extern const uint32_t freq_table[FCC_FRE_TABLE_LEN];
int sp_set_bus_addr(system_param_t *S, uint8_t bus_addr);
int sp_set_baud_rate(system_param_t *S, uint8_t baud_rate);

void sp_get_mac_addr(system_param_t *S, uint8_t *mac);
int sp_set_mac_addr(system_param_t *S, const uint8_t *mac_addr);

void sp_get_ip_config(system_param_t *S, uint8_t *p);
int sp_set_ip_config(system_param_t *S, const uint8_t *ptr);

void sp_get_dsc_ip(system_param_t *S, uint8_t *ip);
int sp_set_dsc_ip(system_param_t *S, const uint8_t *ip);

int sp_set_tcp_port(system_param_t *S, uint16_t tcp_port);
int sp_set_udp_port(system_param_t *S, uint16_t udp_port);
int sp_set_reader_name(system_param_t *S, const char *name, size_t sz);
int sp_set_reader_type(system_param_t *S, const char *type, size_t sz);
int sp_set_reader_sn(system_param_t *S, const char *reader_sn, size_t sz);
int sp_set_password(system_param_t *S, const char *password, size_t sz);

int read_select_param(select_param_t *param);
int write_select_param(select_param_t *param);

int work_status_timer_int(system_param_t *S);
int work_status_timer_set(system_param_t *S, int ms);

system_param_t *sys_param_new(void);

#endif	/* _PARAMETER_H */
