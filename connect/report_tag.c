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

static void _list_insert(tag_t *p, tag_report_t *tag_report)
{
	tag_t *head = tag_report->head_tag;
	p->next = head->next;
	head->next->prev = p;
	head->next = p;
	p->prev = head;
}

static tag_t *_list_new(tag_t *p)
{
	/* 1.分配内存 */
	tag_t *new_tag = malloc(sizeof(tag_t));

	/* 2.填充数据 */
	time(&new_tag->first_time);
	new_tag->cnt = 1;
	new_tag->tag_len = p->tag_len;
	new_tag->ant_index = p->ant_index;
	memcpy(new_tag->data, p->data, p->tag_len);

	return new_tag;
}

static void _list_delete(tag_t *p)
{
	p->prev->next = p->next;
	p->next->prev = p->prev;

	/* 释放内存 */
	free(p);
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
		_list_insert(new_tag, tag_report);
	}

	return 0;
}

static int _add_tag_time(r2h_connect_t *C, tag_t *ptag)
{
	time_t timet;
	struct tm *ptm;
	uint8_t time_buf[7];

	if (time(&timet) < 0) {
		log_msg("%s: time() ERR!", __FUNCTION__);
		command_answer(C, COMMAND_READER_TIME_QUERY, ERRCODE_CMD_UNKNOWERR, NULL, 0);
		return -1;
	}

	ptm = localtime(&timet);
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

/*
 * 注意: 1. 当连接方式为 GPRS 或 WIFI 时，由于无法区分指令模式和自动工作模式，故在这两种模式发送 tag 时，
 *       始终在标签数据后加上了时间
 *	2. 过滤模式的时间在标签发送时才加上
 */
int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	/* 1.获取 cmd_id */
	uint8_t cmd_id;
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("report_tag_send: invalid cmd_id");
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

	/* 4.已建立连接 */
	if (C->conn_type != R2H_NONE) {
		if (C->conn_type == R2H_WIFI || C->conn_type == R2H_GPRS) {
			_add_tag_time(C, ptag);
		}
		return command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
	}

	/* 5.自动工作模式 */
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
			_add_tag_time(C, ptag);
			C->conn_type = R2H_WIFI;
			break;
		case UPLOAD_MODE_GPRS:
			_add_tag_time(C, ptag);
			C->conn_type = R2H_GPRS;
			break;
		default:
			log_msg("report_tag_send: invalid upload_mode");
			return -1;
		}

		command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
		C->conn_type = R2H_NONE;	/* 必需 */
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

	tag_t *p;
	tag_report_t *tag_report = &A->tag_report;
	tag_t *head = tag_report->head_tag;
	tag_t *tail = tag_report->tail_tag;

	for (p = head->next; p != tail; ) {
		if (S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
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
			uint8_t cmd_id;
			if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
				log_msg("report_tag_send: invalid cmd_id");
				return -1;
			}

			command_answer(C, cmd_id, CMD_EXE_SUCCESS, p, p->tag_len);
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
