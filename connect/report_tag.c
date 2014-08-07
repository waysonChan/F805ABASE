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

static int _report_tag_insert(tag_t *ptag, tag_report_t *tag_report)
{
	int i;
	bool exist = false;

	if (tag_report->tag_cnt >= MAX_TAG_NUM) {
		log_msg("tag report buffer overflow");
		return -1;
	}

	tag_t *tag_arr = tag_report->tag_array;
	for (i = 0; i < tag_report->tag_cnt; i++) {
		if (ptag->tag_len == tag_arr[i].tag_len && 
			!memcmp(ptag->data, tag_arr[i].data, ptag->tag_len)) {
			tag_arr[i].cnt++;
			gettimeofday(&tag_arr[i].last_time, NULL);
			exist = true;
			break;
		}
	}

	if (false == exist) {
		uint32_t index = tag_report->tag_cnt;
		gettimeofday(&tag_arr[index].first_time, NULL);
		tag_arr[index].last_time = tag_arr[index].first_time;
		tag_arr[index].cnt = 1;
		memcpy(tag_arr[index].data, ptag->data, ptag->tag_len);
#if 0
		for (i = 0; i < ptag->tag_len; i++)
			printf("%02X ", tag_arr[index].data[i]);
		printf("\n");
#endif
		tag_arr[index].tag_len = ptag->tag_len;
		tag_arr[index].ant_index = ptag->ant_index;
		tag_report->tag_cnt++;
	}

	tag_report->tag_total++;
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
#if 0
	int i;
	for (i = 0; i < ptag->tag_len; i++) {
		printf("%02X ", ptag->data[i]);
	}
	printf("\n");
#endif
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
		return -1;
	}

	return 0;
}

int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag)
{
	uint8_t cmd_id;
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("report_tag_send: invalid cmd_id");
		return -1;
	}
	
	if (((S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) && (C->conn_type == R2H_NONE))
		|| ((S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) && (C->conn_type == R2H_WIFI))
		) {
		switch (S->pre_cfg.upload_mode) {
		case UPLOAD_MODE_NONE:
			break;
		case UPLOAD_MODE_RS232:
			C->conn_type = R2H_RS232;
			command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
			C->conn_type = R2H_NONE;
			break;
		case UPLOAD_MODE_RS485:
			C->conn_type = R2H_RS485;
			command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
			C->conn_type = R2H_NONE;
			break;
		case UPLOAD_MODE_WIFI:
			C->conn_type = R2H_WIFI;
			_add_tag_time(C, ptag);
			command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
			C->conn_type = R2H_NONE;
			break;
		case UPLOAD_MODE_WIEGAND:
			_report_tag_insert(ptag, &A->tag_report);
			break;
		default:
			log_msg("report_tag_send: invalid upload_mode");
			return -1;
		}

		return 0;
	}

	if (A->tag_report.filter_enable) {
		return _report_tag_insert(ptag, &A->tag_report);
	}

	return command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
}

int report_tag_send_timer(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint64_t num_exp;
	if (read(A->tag_report.filter_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("report_tag_timer read()");
		return -1;
	}

	int i;
	tag_report_t *tag_report = &A->tag_report;
	for (i = 0; i < tag_report->tag_cnt; i++) {
		tag_t *ptag = &tag_report->tag_array[i];
		if (S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
			uint8_t *ptr = ptag->data;
			if (S->pre_cfg.wg_start + S->pre_cfg.wg_len > ptag->tag_len) {
				log_msg("invalid tag_len");
				continue;
			}

			if (i != 0) {
				usleep(15000);	/* 发送间隔时间 */
			}
						
			if (S->pre_cfg.wg_len == WG_LEN_34) {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 4);
			} else {
				wiegand_send(C, ptr+S->pre_cfg.wg_start, 3);
#if 0
				int j;
				for (j = 0; j < 3; j++)
					printf("%02X ", ptr[j]);
				printf("\n");
#endif
			}
		} else if (A->tag_report.filter_enable) {
			uint8_t cmd_id;
			if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
				log_msg("report_tag_send: invalid cmd_id");
				return -1;
			}
			
			command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
		}
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
	tag_report->tag_total = 0;
	tag_report->tag_cnt = 0;
	return 0;
}
