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

LIST_HEAD(tag_report_list);

/* ����β��� */
#define MAX_TAG_NUM_IN_RAM	5000
int tag_report_list_add(tag_t *p, tag_report_t *tag_report)
{
	/* 1.�����Լ�� */
	struct list_head *l;
	for (l = tag_report_list.next; l != &tag_report_list; l = l->next) {
		tag_t *tmp = list_entry(l, tag_t, list);
		if (p->tag_len == tmp->tag_len
			&& !memcmp(p->data, tmp->data, p->tag_len)) {
			tmp->cnt++;
			time(&tmp->last_time);
			return 0;
		}
	}

	/* 2.������� */
	if (tag_report->tag_cnt >= MAX_TAG_NUM_IN_RAM) {
		tag_report_list_del(tag_report);
	}

	/* 3.ʵ���� */
	/* 3.1�����ڴ� */
	tag_t *new_tag = malloc(sizeof(tag_t));
	if (!new_tag) {
		log_msg("tag_report_list_add: malloc error");
		return -1;
	}

	/* 3.2������� */
	new_tag->first_time = p->first_time;
	new_tag->cnt = 1;
	new_tag->tag_len = p->tag_len;
	new_tag->ant_index = p->ant_index;
	memcpy(new_tag->data, p->data, p->tag_len);

	/* 3.3��ֹ���׷��ʱ�� */
	new_tag->has_append_time = false;

	/* 4.������� */
	list_add_tail(&new_tag->list, &tag_report_list);

	/* 5.���Ӽ��� */
	tag_report->tag_cnt++;

	return 0;
}

/* ����ͷɾ�� */
void tag_report_list_del(tag_report_t *tag_report)
{
	/* 1.�����Լ�� */
	if (list_empty(&tag_report_list)) {
		log_msg("tag_report_list_del: list empty");
		return;
	}

	/* 2.ɾ������ */
	struct list_head *l = tag_report_list.next;
	list_del(l);

	/* 3.ɾ��ʵ�� */
	tag_t *p = list_entry(l, tag_t, list);		
	free(p);

	/* 4.���ټ��� */
	tag_report->tag_cnt--;
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
	time_buf[0] = convert_hex(ptm->tm_sec);		/* �� */
	time_buf[1] = convert_hex(ptm->tm_min);		/* �� */
	time_buf[2] = convert_hex(ptm->tm_hour);	/* ʱ */
	time_buf[3] = convert_hex(ptm->tm_wday);	/* ���� */
	time_buf[4] = convert_hex(ptm->tm_mday);	/* �� */
	time_buf[5] = convert_hex(ptm->tm_mon + 1);	/* �� */
	time_buf[6] = convert_hex(ptm->tm_year % 100);	/* �� */

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
		log_msg("_get_cmd_id: invalid work_status <%d>.", work_status);
		return -1;
	}

	return 0;
}

static int _finally_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	int ret = 0;
	
	/* 1.��ȡ cmd_id */
	uint8_t cmd_id;
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("_finally_tag_send: invalid cmd_id");
		return -1;
	}

	/* 2.�ѽ������� */
	if (C->conn_type != R2H_NONE) {
		if (C->conn_type == R2H_WIFI || C->conn_type == R2H_GPRS) {
			_append_tag_time(ptag);
		}
		return command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
	}

	/* 3.�Զ�����ģʽ */
	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) {
		switch (S->pre_cfg.upload_mode) {
		case UPLOAD_MODE_WIEGAND:
			return tag_report_list_add(ptag, &A->tag_report);
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
		C->conn_type = R2H_NONE;	/* ���� */
	}

	return ret;
}

/*
 * ע��: 1. �����ӷ�ʽΪ GPRS �� WIFI ʱ�������޷�����ָ��ģʽ���Զ�����ģʽ������������ģʽ���� tag ʱ��
 *       ʼ���ڱ�ǩ���ݺ������ʱ��
 *	2. ����ģʽ��ʱ���ڱ�ǩ����ʱ�ż���
 */
int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	/* 1.��ӱ�ǩʱ�� */
	if (time(&ptag->first_time) < 0) {
		log_msg("report_tag_send: time() error!");
		return -1;
	}

	/* 2.I802Sֻ����1�����߶��� */
	if (((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT1)
		|| ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4)) {
		ptag->ant_index = 1;
	}

	/* 3.(����ģʽ)���� filter_enable */
	if (A->tag_report.filter_enable)
		return tag_report_list_add(ptag, &A->tag_report);

	/* 4. ΪWIFI��GPRS����ʱ�� */
	ptag->has_append_time = false;

	return _finally_tag_send(C, S, A, ptag);
}

static int _tag_storage_write_all(tag_report_t *tag_report)
{
	struct list_head *l;

	for (l = tag_report_list.next; l != &tag_report_list; l = l->next) {
		tag_t *p = list_entry(l, tag_t, list);
		_append_tag_time(p);
		tag_storage_write(p);
	}
	tag_storage_fflush();

	return report_tag_reset(tag_report);
}

#define MAX_SEND_FAIL_TIMES	5
int gprs_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_report_t *tag_report = &A->tag_report;
	gprs_priv_t *gprs_priv = &C->gprs_priv;

	/* 1.��������tag */
	if (!list_empty(&tag_report_list)) {
		struct list_head *l = tag_report_list.next;
		tag_t *p = list_entry(l, tag_t, list);

		if (gprs_priv->gprs_wait_flag && (gprs_priv->gprs_fail_cnt++ >= MAX_SEND_FAIL_TIMES)) {
			/* ��������tagд���ļ� */
			gprs_priv->gprs_fail_cnt = 0;
			gprs_priv->gprs_wait_flag = false;
			return _tag_storage_write_all(tag_report);
		} else {
			/* ��������ͷ��tag */
			gprs_priv->gprs_wait_flag = true;
			gprs_priv->gprs_send_type = GPRS_SEND_TYPE_RAM;
			return _finally_tag_send(C, S, A, p);
		}
	} else {
		/* 2.�����ļ�tag */
		tag_t tag;
		if (tag_storage_read(&tag) < 0) {
			return -1;
		} else {
			tag.has_append_time = true;	/* ���� */
			//log_msg("tag_len = %d", tag.tag_len);
			gprs_priv->gprs_wait_flag = true;
			gprs_priv->gprs_send_type = GPRS_SEND_TYPE_NAND;
			return _finally_tag_send(C, S, A, &tag);
		}
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

	if ((S->pre_cfg.flash_enable == NAND_FLASH_ENBABLE) && 
		((C->conn_type == R2H_GPRS && S->work_status != WS_STOP)
		|| (C->conn_type == R2H_NONE 
		&& S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC
		&& S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS))) {
		return gprs_tag_send_header(C, S, A);
	}

	tag_report_t *tag_report = &A->tag_report;

	while (!list_empty(&tag_report_list)) {
		/* 1.��ȡ����ͷtag */
		struct list_head *l = tag_report_list.next;
		tag_t *p = list_entry(l, tag_t, list);

		/* 2.�ϴ�tag */
		if (C->conn_type == R2H_NONE && S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC
			&& S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
			uint8_t *ptr = p->data;
			if (S->pre_cfg.wg_start + S->pre_cfg.wg_len > p->tag_len) {
				log_msg("invalid tag_len");
				continue;
			}

			//usleep(15000);	/* TODO ���ͼ��ʱ�� */

			if (S->pre_cfg.wg_len == WG_LEN_34) {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 4);
			} else {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 3);
			}

			tag_report_list_del(tag_report);
			break;	/* Τ��һ��ֻ��һ��tag */
		} else if (A->tag_report.filter_enable) {
			_finally_tag_send(C, S, A, p);
		}

		/* 3.ɾ������ͷtag */
		tag_report_list_del(tag_report);
	}

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
	tag_report->tag_cnt = 0;
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
	while (tag_report->tag_cnt) {
		tag_report_list_del(tag_report);
	}
	return 0;
}
