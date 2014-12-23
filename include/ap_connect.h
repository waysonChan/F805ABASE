#ifndef _AP_CONNECTION_H
#define _AP_CONNECTION_H

#include "config.h"
#include "rfid_packets.h"
#include "parameter.h"
#include "list.h"

#include <time.h>

/*---------------------------------------------------------------------
 * 常量定义
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
	bool has_append_time;		/* 防止多次追加时间 */
	uint8_t ant_index;		/* 最后一次读到的天线ID */
	time_t first_time;		/* 第1次读卡时间 */
	time_t last_time;		/* 最后1次读卡时间 */
	uint16_t cnt;			/* 读到的次数 */
	uint16_t tag_len;		/* 标签长度 */
	uint8_t data[MAX_TAG_EPC_LEN];	/* EPC码 */
};

typedef struct {
	uint8_t filter_enable;
	uint8_t filter_time;		/* 标签过滤时间,单位100ms */
	uint16_t speed;			/* 标签读取速率(张/秒) */
	uint32_t tag_cnt;		/* 标签数量(不计重复标签) */
	int filter_timer;		/* 过滤描述符 */
	struct itimerspec filter_its;	/* 过滤定时器 */
	struct timeval start_time;	/* 开始读卡时间 */	
} tag_report_t;

typedef struct {
	int fd;
	bool connected;
	bool r2000_error_log;
	uint8_t cur_ant_power;		/* 供复位后恢复功率使用 */
	ap_recv_t recv;
	ap_send_t send;
	tag_report_t tag_report;
	select_param_t select_param;
} ap_connect_t;

int ap_conn_recv(ap_connect_t *A);
ap_connect_t *ap_connect_new(system_param_t *S);

#endif	/* _AP_CONNECTION_H */
