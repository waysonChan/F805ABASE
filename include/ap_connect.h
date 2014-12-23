#ifndef _AP_CONNECTION_H
#define _AP_CONNECTION_H

#include "config.h"
#include "rfid_packets.h"
#include "parameter.h"
#include "list.h"

#include <time.h>

/*---------------------------------------------------------------------
 * ��������
 *--------------------------------------------------------------------*/
#define AP_BUF_SIZE		4096
#define MAX_TAG_EPC_LEN		256
#define MAX_TAG_NUM		512

typedef struct {
	uint8_t rbuf[AP_BUF_SIZE];
	size_t rlen;
	size_t bytes_need;
	RFID_PACKET_COMMON cmn;
} ap_recv_t;

typedef struct {
	uint8_t wbuf[AP_BUF_SIZE];
	size_t wlen;	
} ap_send_t;

typedef struct _tag_t tag_t;
struct _tag_t{
	struct list_head list;
	bool has_append_time;		/* ��ֹ���׷��ʱ�� */
	uint8_t ant_index;		/* ���һ�ζ���������ID */
	time_t first_time;		/* ��1�ζ���ʱ�� */
	time_t last_time;		/* ���1�ζ���ʱ�� */
	uint16_t cnt;			/* �����Ĵ��� */
	uint16_t tag_len;		/* ��ǩ���� */
	uint8_t data[MAX_TAG_EPC_LEN];	/* EPC�� */
};

typedef struct {
	uint8_t filter_enable;
	uint8_t filter_time;		/* ��ǩ����ʱ��,��λ100ms */
	uint16_t speed;			/* ��ǩ��ȡ����(��/��) */
	uint32_t tag_cnt;		/* ��ǩ����(�����ظ���ǩ) */
	int filter_timer;		/* ���������� */
	struct itimerspec filter_its;	/* ���˶�ʱ�� */
	struct timeval start_time;	/* ��ʼ����ʱ�� */	
} tag_report_t;

typedef struct {
	int fd;
	bool connected;
	bool r2000_error_log;
	uint8_t cur_ant_power;		/* ����λ��ָ�����ʹ�� */
	ap_recv_t recv;
	ap_send_t send;
	tag_report_t tag_report;
	select_param_t select_param;
} ap_connect_t;

int ap_conn_recv(ap_connect_t *A);
ap_connect_t *ap_connect_new(system_param_t *S);

#endif	/* _AP_CONNECTION_H */
