#include "report_tag.h"
#include "command.h"
#include "command_def.h"
#include "errcode.h"
#include "utility.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <stdlib.h>

static tag_t tail_tag_sentinel;
static tag_t head_tag_sentinel = {
	.prev = NULL,
	.next = &tail_tag_sentinel,
};

static tag_t tail_tag_sentinel = {
	.prev = &head_tag_sentinel,
	.next = NULL,
};

static void _list_add_tail(tag_t *p, tag_report_t *tag_report)
{
	tag_t *tail = tag_report->tail_tag;
	p->next = tail;
	p->prev = tail->prev;
	tail->prev->next = p;
	tail->prev = p;
}

static tag_t *_list_new(tag_t *p)
{
	/* 1.分配内存 */
	tag_t *new_tag = malloc(sizeof(tag_t));

	/* 2.填充数据 */
	new_tag->first_time = p->first_time;
	new_tag->cnt = 1;
	new_tag->tag_len = p->tag_len;
	new_tag->ant_index = p->ant_index;
	memcpy(new_tag->data, p->data, p->tag_len);

	/* 3.防止多次追加时间 */
	new_tag->has_append_time = false;

	return new_tag;
}

static void _list_delete(tag_t *p)
{
	p->prev->next = p->next;
	p->next->prev = p->prev;

	/* 释放内存 */
	free(p);
}

void list_delete_header(void)
{
	if (head_tag_sentinel.next == &tail_tag_sentinel) {
		log_msg("list_delete_header: list empty");
		return;
	} else {
		tag_t *p = head_tag_sentinel.next;
		_list_delete(p);
	}
}

static int  _tag_list_insert(tag_t *ptag, tag_report_t *tag_report)
{
	bool exist = false;
	tag_t *p;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;

	for (p = head->next; p != tail; p = p->next) {
		if (ptag->tag_len == p->tag_len
			&& !memcmp(p->data, ptag->data, ptag->tag_len)) {
			p->cnt++;
			time(&p->last_time);
			exist = true;
			break;
		}
	}

	if (false == exist) {
		tag_t *new_tag = _list_new(ptag);
		_list_add_tail(new_tag, tag_report);
	}

	return 0;
}

static int _append_tag_time(tag_t *ptag)
{
	struct tm *ptm;
	uint8_t time_buf[7];

	if (ptag->has_append_time) {
		return 0;
	} else {
		ptag->has_append_time = true;
	}

	ptm = localtime(&ptag->first_time);
	time_buf[0] = convert_hex(ptm->tm_sec);		/* 秒 */
	time_buf[1] = convert_hex(ptm->tm_min);		/* 分 */
	time_buf[2] = convert_hex(ptm->tm_hour);	/* 时 */
	time_buf[3] = convert_hex(ptm->tm_wday);	/* 星期 */
	time_buf[4] = convert_hex(ptm->tm_mday);	/* 日 */
	time_buf[5] = convert_hex(ptm->tm_mon + 1);	/* 月 */
	time_buf[6] = convert_hex(ptm->tm_year % 100);	/* 年 */

	memcpy(ptag->data + ptag->tag_len, time_buf, 7);
	ptag->tag_len += 7;

	return 0;
}

static inline int _get_cmd_id(int work_status, uint8_t *cmd_id)
{
	switch (work_status) {
	case WS_READ_EPC_FIXED:
	case WS_READ_EPC_FIXED_WEB:
	case WS_READ_EPC_INTURN:
		*cmd_id = COMMAND_18K6C_MAN_READ_EPC;
		break;
	case WS_READ_TID_FIXED:
	case WS_READ_TID_INTURN:
		*cmd_id = COMMAND_18K6C_MAN_READ_TID;
		break;
	case WS_READ_EPC_TID_FIXED:
	case WS_READ_EPC_TID_INTURN:
		*cmd_id = COMMAND_18K6C_MAN_READ_EPC_TID;
		break;
	case WS_READ_USER:
		*cmd_id = COMMAND_18K6C_MAN_READ_USERBANK;
		break;
	default:
		log_msg("_get_cmd_id: invalid work_status.");
		return -1;
	}

	return 0;
}

static int _finally_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	int ret = 0;
	
	/* 1.获取 cmd_id */
	uint8_t cmd_id;
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("report_tag_send: invalid cmd_id");
		return -1;
	}

	/* 2.已建立连接 */
	if (C->conn_type != R2H_NONE) {
		if (C->conn_type == R2H_WIFI || C->conn_type == R2H_GPRS) {
			_append_tag_time(ptag);
		}
		return command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
	}

	/* 3.自动工作模式 */
	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) {
		switch (S->pre_cfg.upload_mode) {
		case UPLOAD_MODE_WIEGAND:
			return _tag_list_insert(ptag, &A->tag_report);
		case UPLOAD_MODE_RS232:
			C->conn_type = R2H_RS232;
			break;
		case UPLOAD_MODE_RS485:
			C->conn_type = R2H_RS485;
			break;
		case UPLOAD_MODE_WIFI:
			_append_tag_time(ptag);
			C->conn_type = R2H_WIFI;
			break;
		case UPLOAD_MODE_GPRS:
			_append_tag_time(ptag);
			C->conn_type = R2H_GPRS;
			break;
		default:
			log_msg("_finally_tag_send: invalid upload_mode");
			return -1;
		}

		ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
		C->conn_type = R2H_NONE;	/* 必需 */
	}

	return ret;
}

/*
 * 注意: 1. 当连接方式为 GPRS 或 WIFI 时，由于无法区分指令模式和自动工作模式，故在这两种模式发送 tag 时，
 *       始终在标签数据后加上了时间
 *	2. 过滤模式的时间在标签发送时才加上
 */
int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	/* 1.添加标签时间 */
	if (time(&ptag->first_time) < 0) {
		log_msg("report_tag_send: time() error!");
		return -1;
	}

	/* 2.I802S只能用1号天线读卡 */
	if (((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT1)
		|| ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4)) {
		ptag->ant_index = 1;
	}

	/* 3.(过滤模式)处理 filter_enable */
	if (A->tag_report.filter_enable)
		return _tag_list_insert(ptag, &A->tag_report);

	return _finally_tag_send(C, S, A, ptag);
}

static int _tag_storage_write_all(tag_report_t *tag_report)
{
	tag_t *p;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;

	for (p = head->next; p != tail; ) {
		_append_tag_time(p);
		tag_storage_write(p);
		p = p->next;
		_list_delete(p->prev);
	}

	return tag_storage_fflush();
}

#define MAX_SEND_FAIL_TIMES	3
int gprs_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_t *p;
	tag_report_t *tag_report = &A->tag_report;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;
	gprs_priv_t *gprs_priv = &C->gprs_priv;

	/* 1.处理链表tag */
	if (head->next != tail) {
		p = head->next;
		if (gprs_priv->gprs_wait_flag && (gprs_priv->gprs_fail_cnt++ >= MAX_SEND_FAIL_TIMES)) {
			/* 将链表中tag写入文件 */
			gprs_priv->gprs_fail_cnt = 0;
			gprs_priv->gprs_wait_flag = false;
			return _tag_storage_write_all(tag_report);
		} else {
			/* 发送链表头的tag */
			gprs_priv->gprs_wait_flag = true;
			gprs_priv->gprs_send_type = GPRS_SEND_TYPE_RAM;
			return _finally_tag_send(C, S, A, p);
		}
	} else {		
#if 1
		/* 2.处理文件tag */
		tag_t tag;
		if (tag_storage_read(&tag) < 0) {
			return -1;
		} else {
			gprs_priv->gprs_wait_flag = true;
			gprs_priv->gprs_send_type = GPRS_SEND_TYPE_NAND;
			return _finally_tag_send(C, S, A, &tag);
		}
#endif
	}

	return 0;
}

int report_tag_send_timer(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint64_t num_exp;
	if (read(A->tag_report.filter_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("report_tag_timer read()");
		return -1;
	}

	if (C->conn_type == R2H_GPRS 
		|| (C->conn_type == R2H_NONE 
		&& S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC
		&& S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS)) {
		return gprs_tag_send_header(C, S, A);
	}

	tag_t *p;
	tag_report_t *tag_report = &A->tag_report;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;

	for (p = head->next; p != tail; ) {
		if (C->conn_type == R2H_NONE && S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC
			&& S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
			uint8_t *ptr = p->data;
			if (S->pre_cfg.wg_start + S->pre_cfg.wg_len > p->tag_len) {
				log_msg("invalid tag_len");
				continue;
			}

			if (p != head->next) {
				usleep(15000);	/* 发送间隔时间 */
			}

			if (S->pre_cfg.wg_len == WG_LEN_34) {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 4);
			} else {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 3);
			}
		} else if (A->tag_report.filter_enable) {
			_finally_tag_send(C, S, A, p);
		}

		p = p->next;
		_list_delete(p->prev);
	}

	tag_report->tag_cnt = 0;
	tag_report->tag_total = 0;

	return 0;
}

int report_tag_set_timer(ap_connect_t *A, uint32_t ms)
{
	struct itimerspec its = {
		.it_interval.tv_sec = ms / 1000,
		.it_interval.tv_nsec = (ms % 1000) * 1000000,
		.it_value.tv_sec = 1,
		.it_value.tv_nsec = 0,
	};

	if (timerfd_settime(A->tag_report.filter_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime");
		return -1;
	}
	
	A->tag_report.filter_its = its;
	return 0;
}

int report_tag_init(tag_report_t *tag_report)
{
	tag_report->head_tag = &head_tag_sentinel;
	tag_report->tail_tag = &tail_tag_sentinel;

	tag_report->filter_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (tag_report->filter_timer < 0) {		
		log_ret("timerfd_create error");
		return -1;
	}

	tag_report->filter_its.it_value.tv_sec = 0;
	tag_report->filter_its.it_value.tv_nsec = 0;	
	if (timerfd_settime(tag_report->filter_timer, 0, &tag_report->filter_its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(tag_report->filter_timer);
		return -1;
	}

	tag_storage_init();

	return 0;
}

int report_tag_reset(tag_report_t *tag_report)
{
	tag_t *p;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;

	for (p = head->next; p != tail; ) {
		p = p->next;
		_list_delete(p->prev);		
	}
	
	tag_report->tag_total = 0;
	tag_report->tag_cnt = 0;
	return 0;
}
