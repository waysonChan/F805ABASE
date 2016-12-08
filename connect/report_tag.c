#include "report_tag.h"
#include "command.h"
#include "command_def.h"
#include "errcode.h"
#include "utility.h"
#include "command_manager.h"
#include "rf_ctrl.h"

#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <stdlib.h>


LIST_HEAD(tag_report_list);
static void send_wiegand(r2h_connect_t *C, system_param_t *S, tag_t *ptag);


/* 链表尾添加 */
#define MAX_TAG_NUM_IN_RAM	5000
int tag_report_list_add(ap_connect_t *A, tag_t *p)
{

	/* 1.存在性检查 */
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

	/* 2.总量检查 */
	if (A->tag_report.tag_cnt >= MAX_TAG_NUM_IN_RAM) {
		tag_report_list_del(&A->tag_report);
	}

	/* 3.实例化 */
	/* 3.1分配内存 */
	tag_t *new_tag = malloc(sizeof(tag_t));
	if (!new_tag) {
		log_msg("tag_report_list_add: malloc error");
		return -1;
	}

	/* 3.2填充数据 */
	new_tag->first_time = p->first_time;
	new_tag->cnt = 1;
	new_tag->tag_len = p->tag_len;
	new_tag->ant_index = p->ant_index;
	memcpy(new_tag->data, p->data, p->tag_len);

	new_tag->time_cnt = A->tag_report.filter_time;//100ms

	/* 3.3防止多次追加时间 */
	new_tag->has_append_time = false;

	/* 4.添加链表 */
	list_add_tail(&new_tag->list, &tag_report_list);


	/* 5.增加计数 */
	A->tag_report.tag_cnt++;

	return 1;
}

/* 链表头删除 */
void tag_report_list_del(tag_report_t *tag_report)
{
	/* 1.存在性检查 */
	if (list_empty(&tag_report_list)) {
		//log_msg("tag_report_list_del: list empty");
		return;
	}

	/* 2.删除链表 */
	struct list_head *l = tag_report_list.next;
	list_del(l);

	/* 3.删除实例 */
	tag_t *p = list_entry(l, tag_t, list);		
	free(p);

	/* 4.减少计数 */
	tag_report->tag_cnt--;
}

void TimeTickTagFilterList(tag_report_t *tag_report)
{
	int i=0;
    if(tag_report->tag_cnt > 0)                                //链表中节点不为空
    {
    	i = tag_report->tag_cnt;
		struct list_head *l = tag_report_list.next;
        while (!list_empty(&tag_report_list) && (i--))             //从链表起始节点开始遍历到尾部
        {
        	tag_t *tmp = list_entry(l, tag_t, list);
            tmp->time_cnt -=  1; //超时时间处理,各个节点的超时时间值减去定时器的步进值
			
            if(tmp->time_cnt <= 0)//超时时间到了,要删除该节点
            {
            	//删除链表
            	tag_report_list_del(tag_report);
				l = tag_report_list.next;	
            } else {
				l = l->next;
			}
        }
    }
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
		log_msg("_get_cmd_id: invalid work_status <%d>.", work_status);
		return -1;
	}

	return 0;
}



int work_command_send_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag){
	uint8_t cmd_id;
	int ret = 0;
	
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("_finally_tag_send: invalid cmd_id");
		return -1;
	}

	/* 已建立连接 */
	if (C->conn_type != R2H_NONE) {
		if (C->conn_type == R2H_WIFI || C->conn_type == R2H_GPRS) {
			_append_tag_time(ptag);
		}
		C->tmp_send_len = ptag->tag_len;
		memcpy(C->tmp_send_data,ptag->data,ptag->tag_len);
		ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
	} else {
		ret = -1;
	}
	return ret;
}


int work_auto_send_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag){
	uint8_t cmd_id;
	int ret = 0;
	
	if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
		log_msg("_finally_tag_send: invalid cmd_id");
		return -1;
	}

	/* 已建立连接 */
	if (C->conn_type != R2H_NONE) {
		_append_tag_time(ptag);
		C->tmp_send_len = ptag->tag_len;
		memcpy(C->tmp_send_data,ptag->data,ptag->tag_len);
		ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
		return ret;
	}

	switch (S->pre_cfg.upload_mode) {
	case UPLOAD_MODE_WIEGAND:
		send_wiegand(C,S,ptag);// 韦根一次只发一个tag 
		return 0;
	case UPLOAD_MODE_RS232:
		//_append_tag_time(ptag);
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
	case UPLOAD_MODE_UDP:
		_append_tag_time(ptag);
		C->conn_type = R2H_UDP;
		break;
	case UPLOAD_MODE_TCP:
		if (C->accepted == false){
			C->conn_type = R2H_NONE;
			return -1;
		} else {
			_append_tag_time(ptag);
			C->conn_type = R2H_TCP;
		}
		break;
	default:
		log_msg("_finally_tag_send: invalid upload_mode");
		return -1;
	}
	C->tmp_send_len = ptag->tag_len;
	memcpy(C->tmp_send_data,ptag->data,ptag->tag_len);

	ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
	C->conn_type = R2H_NONE;	/* 必需 */
	
	return ret;
}


int work_trigger_send_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag){
	uint8_t cmd_id;
	int ret = 0;

	if(C->conn_type == R2H_NONE){
		cmd_id = S->pre_cfg.oper_mode;
		switch (S->pre_cfg.upload_mode) {
		case UPLOAD_MODE_WIEGAND:
			send_wiegand(C,S,ptag);// 韦根一次只发一个tag 
			return 0;
		case UPLOAD_MODE_RS232:
			_append_tag_time(ptag);
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
		case UPLOAD_MODE_UDP:
			_append_tag_time(ptag);
			C->conn_type = R2H_UDP;
			break;		
		case UPLOAD_MODE_TCP:
			if (C->accepted == false){
				C->conn_type = R2H_NONE;
				return -1;
			} else {
				_append_tag_time(ptag);
				C->conn_type = R2H_TCP;
			}
			break;
		default:
			C->conn_type = R2H_NONE;
			break;
		}
		C->tmp_send_len = ptag->tag_len;
		memcpy(C->tmp_send_data,ptag->data,ptag->tag_len);
		ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);
		C->conn_type = R2H_NONE;	/* 必需 */
	} else{
		if (_get_cmd_id(S->work_status, &cmd_id) < 0) {
			log_msg("_finally_tag_send: invalid cmd_id");
			if((C->wifi_priv.wifi_send_type == SEND_TYPE_NAND||
				C->gprs_priv.gprs_send_type == SEND_TYPE_NAND ||
				C->tcp_send_symbol == SEND_TYPE_NAND)
				&& C->conn_type != R2H_NONE)
				cmd_id = S->pre_cfg.oper_mode;
			else
				return -1;
		}
		
		_append_tag_time(ptag);
		if(S->gpio_dece.gpio1_val == 1
			|| S->gpio_dece.gpio2_val == 1
			|| C->set_delay_timer_flag == 1
			|| C->wifi_priv.wifi_send_type == SEND_TYPE_NAND
			|| C->gprs_priv.gprs_send_type == SEND_TYPE_NAND
			|| C->tcp_send_symbol == SEND_TYPE_NAND){
			C->tmp_send_len = ptag->tag_len;
			memcpy(C->tmp_send_data,ptag->data,ptag->tag_len);
			ret = command_answer(C, cmd_id, CMD_EXE_SUCCESS, ptag, ptag->tag_len);	
		}

	}

	return ret;
}


static int _finally_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag) {
	int ret = 0;

	switch(S->pre_cfg.work_mode) {
	case WORK_MODE_COMMAND:
		ret = work_command_send_tag(C,S,A,ptag);
		break;
	case WORK_MODE_AUTOMATIC:
		ret = work_auto_send_tag(C,S,A,ptag);
		break;
	case WORK_MODE_TRIGGER:
		ret = work_trigger_send_tag(C,S,A,ptag);
		break;
	default:
		C->conn_type = R2H_NONE;
		ret = -1;
		break;
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
	if (A->tag_report.filter_enable){
		if(tag_report_list_add(A, ptag) == 1){
			ptag->has_append_time = false;
			return _finally_tag_send(C, S, A, ptag);
		}else{
			return 0;
		}
	}

	/* 4. 为WIFI和GPRS加上时间 */
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
	return 0;
}

#define MAX_SEND_FAIL_TIMES	5

static int tag_send_ram (total_priv_t *total_priv, ap_connect_t *A){
	tag_report_t *tag_report = &A->tag_report;
	if (total_priv->wait_flag && (total_priv->fail_cnt++ >= MAX_SEND_FAIL_TIMES)) {
		/* 将链表中tag写入文件 */
		total_priv->fail_cnt = 0;
		total_priv->wait_flag = false;
		return _tag_storage_write_all(tag_report);
	} else {
		/* 发送链表头的tag */
		total_priv->wait_flag = true;
		total_priv->send_type = SEND_TYPE_RAM;
		return 0;
	}
}


int cmp_tag_list(tag_t *p){
		/* 1.存在性检查 */
		struct list_head *l;
		for (l = tag_report_list.next; l != &tag_report_list; l = l->next) {
			tag_t *tmp = list_entry(l, tag_t, list);
			if (p->tag_len == tmp->tag_len
				&& !memcmp(p->data, tmp->data, p->tag_len)) {
				return 0;
			}
		}
		return -1;
}

int report_tag_confirm(r2h_connect_t *C, system_param_t *S, ap_connect_t *A){
	int ret = -1, flag = 0;
	total_priv_t *total_priv;
	if(S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS)
		total_priv = (total_priv_t *)&C->gprs_priv;
	else if(S->pre_cfg.upload_mode == UPLOAD_MODE_WIFI)
		total_priv = (total_priv_t *)&C->wifi_priv;
	else 
		return -1;


	//是否是当前标签
	if(strncmp((const char *)C->recv.frame.param_buf+2,
		(const char *)C->tmp_send_data,C->recv.frame.param_len - 3) == 0){
		flag = 1;
	}else{
		flag = 0;
	}
	

	//卡号不相同
	if(flag == 0){
		tag_t tag;
		tag.ant_index = S->cur_ant;
		tag.has_append_time = true;//不再添加时间
		tag.tag_len = C->tmp_send_len;
		memcpy(tag.data, C->tmp_send_data, C->tmp_send_len);
		ret = cmp_tag_list(&tag);

		tag_send_ram(total_priv,A);
		
		//链表有标签
		if(ret == 0){
			total_priv->fail_cnt = 0;
			total_priv->wait_flag = false;
			total_priv->send_type = SEND_TYPE_RAM;
			ret = _finally_tag_send(C,S,A,&tag);
		}else if(total_priv->fail_cnt <= MAX_SEND_FAIL_TIMES){
			ret = _finally_tag_send(C,S,A,&tag);
		}

	}else{
		total_priv->fail_cnt = 0;
		total_priv->wait_flag = false;
		log_msg("total_priv->send_type = %d",total_priv->send_type);
		if(total_priv->send_type == SEND_TYPE_NAND ){//确认后删除
			ret = tag_storage_delete(false);
			total_priv->send_type = SEND_TYPE_RAM;
		}
	}


	return ret;
}

int gprs_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_report_t *tag_report = &A->tag_report;
	int ret = 0;
	if (!list_empty(&tag_report_list)) {
		ret = _tag_storage_write_all(tag_report);
	} 
	return ret;
}


int wifi_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_report_t *tag_report = &A->tag_report;
	int ret = 0;

	/* 1.处理链表tag */
	if (!list_empty(&tag_report_list)) {
		ret = _tag_storage_write_all(tag_report);
	} 
	return ret;
}


int TCP_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_report_t *tag_report = &A->tag_report;
	int ret = 0;

	/* 1.处理链表tag */
	if (!list_empty(&tag_report_list)) {
		if (C->accepted == false) {
			ret = _tag_storage_write_all(tag_report);
		}
	} else {
		/* 2.处理文件tag */
		tag_t tag;
		if (tag_storage_read(&tag) < 0 || C->accepted == false) {
			ret = -1;
		} else {
			tag.has_append_time = true;
			C->tcp_send_symbol = SEND_TYPE_NAND;
			ret = _finally_tag_send(C, S, A, &tag);
			if(ret > -1) {
				ret = tag_storage_delete(false);
				C->tcp_send_symbol = SEND_TYPE_RAM;
			}
		}
	}
	return ret;
}


int empty_list_to_send_flash(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	int ret = 0;
	if(list_empty(&tag_report_list)){
		tag_t tag;
		if (tag_storage_read(&tag) < 0) {
			return ret;
		} else {
			if(S->pre_cfg.dev_type == DEV_TYPE_FLAG_GPRS){
				C->gprs_priv.gprs_send_type = SEND_TYPE_NAND;
			}else if(S->pre_cfg.dev_type == DEV_TYPE_FLAG_WIFI){
				C->wifi_priv.wifi_send_type = SEND_TYPE_NAND;
			}
			log_msg("empty_list_to_send_flash : %d",tag_storage_get_cnt());
			tag.has_append_time = true;//不再添加时间
			_finally_tag_send(C, S, A, &tag);
		}
		return 1;
	}
	return ret;
}



static void flash_send_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{	
	if(S->pre_cfg.work_mode == WORK_MODE_COMMAND)
		goto out;

	if(S->pre_cfg.flash_enable == NAND_FLASH_ENBABLE){
		switch(S->pre_cfg.upload_mode){
		case UPLOAD_MODE_GPRS:
			if(C->gprs_priv.connected){
				empty_list_to_send_flash(C, S, A);
			}else if(C->conn_type == R2H_NONE){
				gprs_tag_send_header(C, S, A);
			}
			break;
		case UPLOAD_MODE_WIFI:
			if(C->conn_type == R2H_WIFI && S->work_status == WS_STOP)
				break;
			else{
				if(!empty_list_to_send_flash(C, S, A)){
					if(C->wifi_connect){
						C->wifi_connect = false;
						goto out;
					}else{
						wifi_tag_send_header(C, S, A);
					}
				}
			}
			break;
		case UPLOAD_MODE_TCP:
			TCP_tag_send_header(C, S, A);
			break;
		default:
			break;
		}
	}
out:
	return;
}

static void send_wiegand(r2h_connect_t *C, system_param_t *S, tag_t *ptag)
{
	if (C->conn_type == R2H_NONE 
		&& S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
		uint8_t *ptr = ptag->data;
		if (S->pre_cfg.wg_start + S->pre_cfg.wg_len > ptag->tag_len) {
			log_msg("invalid tag_len");
		} else if (S->pre_cfg.wg_len == WG_LEN_34) {
			wiegand_send(C, ptr+S->pre_cfg.wg_start, 4);
		} else {
			wiegand_send(C, ptr+S->pre_cfg.wg_start, 3);
		}
		usleep(1);//防止同时读到多个标签导致韦根输出不能识别。
	}
}

int report_tag_send_timer(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint64_t num_exp;
	if (read(A->tag_report.filter_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("report_tag_timer read()");
		return -1;
	}	
	
	if(A->tag_report.filter_enable == false)
		return 0;
	
	if(++A->tag_report.filter_count >= A->tag_report.filter_time ){//过滤时间
		A->tag_report.filter_count = 0;
		flash_send_tag(C,S,A);
	}
	
	/*删除链表中的卡*/
	TimeTickTagFilterList(&A->tag_report);

	return 0;
}

int report_tag_set_timer(ap_connect_t *A, uint32_t ms)
{
	struct itimerspec its = {
		.it_interval.tv_sec = (ms / 1000),
		.it_interval.tv_nsec = (ms % 1000) * 1000000,
		.it_value.tv_sec     = (ms / 1000),
		.it_value.tv_nsec    = (ms % 1000) * 1000000,
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
	
	bzero(&tag_report->filter_its, sizeof(struct itimerspec));
	if (timerfd_settime(tag_report->filter_timer, 0, &tag_report->filter_its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(tag_report->filter_timer);
		return -1;
	}

	tag_storage_init();
	triger_status_init();
	return 0;
}

int report_tag_reset(tag_report_t *tag_report)
{
	while (tag_report->tag_cnt) {
		tag_report_list_del(tag_report);
	}
	return 0;
}






int heartbeat_timer_set(system_param_t *S, int s)
{
	struct itimerspec its = {
		.it_interval.tv_sec = s,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = s,
		.it_value.tv_nsec = 0,
	};

	if (timerfd_settime(S->heartbeat_timer, 0, &its, NULL) < 0) {
		log_ret("heartbeat timerfd_settime error");
		return -1;
	}

	S->heartbeat_its = its;
	return 0;
}


int heartbeat_timer_int(system_param_t *S)
{
	S->heartbeat_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (S->heartbeat_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	} 
	
	bzero(&S->heartbeat_its, sizeof(struct itimerspec));

	return 0;
}





int heartbeat_timer_trigger(r2h_connect_t *C, system_param_t *S )
{
	uint64_t num_exp;
	int temp_conn_type;
	if (read(S->heartbeat_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("S->heartbeat_timer_trigger read()");
		return -1;
	}
	
	temp_conn_type = C->conn_type;
	switch(S->pre_cfg.upload_mode) {
	case UPLOAD_MODE_WIFI:
		C->conn_type = R2H_WIFI;
		break;
	case UPLOAD_MODE_GPRS:
		C->conn_type = R2H_GPRS;
		break;
	case UPLOAD_MODE_RS232:
		C->conn_type = R2H_RS232;
		break;
	case UPLOAD_MODE_TCP:
		if (C->accepted == true){
			C->conn_type = R2H_TCP;
		} else {
			C->conn_type = R2H_NONE;
		}
		break;
	case UPLOAD_MODE_UDP:
		C->conn_type = R2H_UDP;
		break;
	default:
		C->conn_type = R2H_NONE;
		break;
	}
	
	command_answer(C, COMMAND_TRANSMIT_CONTROL_HEARTBEAT, CMD_EXE_SUCCESS, NULL, 0);
	C->conn_type = temp_conn_type;
	return 0;
}


int triggerstatus_timer_init(system_param_t *S)
{
	S->triggerstatus_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (S->triggerstatus_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	} 
	
	bzero(&S->triggerstatus_its, sizeof(struct itimerspec));
	
	struct itimerspec its = {
		.it_interval.tv_sec = 0,
		.it_interval.tv_nsec = 100000000,
		.it_value.tv_sec = 0,
		.it_value.tv_nsec = 100000000,
	};

	S->triggerstatus_its = its;
	if (timerfd_settime(S->triggerstatus_timer, 0, &S->triggerstatus_its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(S->triggerstatus_timer);
		return -1;
	}

	return 0;
}





int triggerstatus_timer_trigger(r2h_connect_t *C, system_param_t *S )
{
	uint64_t num_exp;
	
	if (read(S->triggerstatus_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("S->triggerstatus_timer_trigger read()");
		return -1;
	}
	/*只在WIFI 和GPRS使用*/
	if(S->pre_cfg.upload_mode == UPLOAD_MODE_WIFI || S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS){

		if(C->time++ == 10){
			C->time = 0;
			if(S->work_status == WS_STOP){
				int status_cnt;
				char buf[10];
				status_cnt = triger_status_get_cnt();
				if(status_cnt){
					C->status_send_from_file = true;
					triger_status_read(buf);
					//log_msg("timer send trigger from flash : %d\n",status_cnt);
					send_triggerstatus(C,S,buf,sizeof(buf));
				}else{
					return -1;
				}
			}
		}
		
		if(C->triger_confirm_flag){
			if(C->status_cnt == 1){
				triger_status_write(C->status);
				C->triger_confirm_flag = false;
				C->status_cnt--;
				return 1;
			}else if(C->status_cnt <= 0){
				return 1;
			}
			C->status_send_from_file = false;
			send_triggerstatus(C,S,C->status,sizeof(C->status));
			C->status_cnt--;
		}
	}

	return 0;
}


void send_triggerstatus(r2h_connect_t *C,system_param_t *S,const void *buf, size_t sz)
{
	int temp_conn_type;
	char *p ;
	p = (char *)buf;
	int i =0;
	for(i=0;i<sz;i++){
		C->status_buf[i] = *p++;
	}
	temp_conn_type = C->conn_type;
	switch(S->pre_cfg.upload_mode) {
	case UPLOAD_MODE_GPRS:
		C->conn_type = R2H_GPRS;
		break;
	case UPLOAD_MODE_WIFI:
		C->conn_type = R2H_WIFI;
		break;
	case UPLOAD_MODE_TCP:
		if (C->accepted == false){
			C->conn_type = R2H_NONE;
			return;
		} else {
			C->conn_type = R2H_TCP;
		}
		break;
	case UPLOAD_MODE_UDP:
		C->conn_type = R2H_UDP;
		break;		
	case UPLOAD_MODE_RS232:
		C->conn_type = R2H_RS232;
		break;
	case UPLOAD_MODE_RS485:
		C->conn_type = R2H_RS485;
		break;		
	default:
		C->conn_type = R2H_NONE;
		break;
	}
	command_answer(C, COMMAND_TRANSMIT_CONTROL_TRIGGERSTATUS, CMD_EXE_SUCCESS, buf, sz);
	C->conn_type = temp_conn_type;

}


int report_triggerstatus(r2h_connect_t *C, system_param_t *S )
{
	tag_t tmp;
	tag_t *p = &tmp;
	memset(&tmp, 0, sizeof(tag_t));
	if (time(&p->first_time) < 0) {
		log_msg("report_triggerstatus time() error!");
		return -1;
	}
	
	C->status[0] = S->gpio_dece.gpio1_val;
	C->status[1] = S->gpio_dece.gpio2_val;	

	p->has_append_time = false;
	_append_tag_time(p);
	memcpy(&C->status[2], &p->data[0], 7);//添加时间戳

	C->status[9] = S->action_status.report_status;//触发顺序

	send_triggerstatus(C,S,C->status,sizeof(C->status));

	return 0;
}

int delay_timer_set(system_param_t *S, int ms)
{
	struct itimerspec its = {
		.it_interval.tv_sec = (ms / 1000),
		.it_interval.tv_nsec = (ms % 1000) * 1000000,
		.it_value.tv_sec = (ms / 1000),
		.it_value.tv_nsec = (ms % 1000) * 1000000,

	};

	if (timerfd_settime(S->delay_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime");
		return -1;
	}

	S->delay_timer_its = its;
	return 0;
}
int delay_timer_init(system_param_t *S)
{
	S->delay_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (S->delay_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	} 
	bzero(&S->delay_timer_its, sizeof(struct itimerspec));
	return 0;
}

int delay_timer_trigger(r2h_connect_t *C, system_param_t *S,ap_connect_t *A )
{
	uint64_t num_exp;
	if (read(S->delay_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("S->delay_timer_trigger read()");
		return -1;
	}
	if(S->work_status == WS_STOP){
		return 0;
	}
	
	if(C->set_delay_timer_flag == 1){
		C->set_delay_timer_cnt++;
	} else {
		C->set_delay_timer_cnt = 0;
	}
	
	return 0;
}


