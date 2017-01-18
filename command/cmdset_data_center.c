#include "config.h"
#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "parameter.h"
#include "connect.h"
#include "report_tag.h"

#include <strings.h>
#include <string.h>
#include <unistd.h>

/*---------------------------------------------------------------------
 *	指令集:数据中心相关操作
 *--------------------------------------------------------------------*/
static command_set_t cmdset_data_center = {
	.set_type = COMMAND_DATA_CENTER_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:接收标签确认,此指令不需要回复
 *--------------------------------------------------------------------*/
static void ec_recv_tag_confirm(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	return;
}




static void ec_recv_tag_confirm_wireless(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	if (A->tag_report.filter_enable == false
		|| S->pre_cfg.flash_enable == NAND_FLASH_DISABLE)
		return;
	C->wifi_connect = true;
	C->gprs_priv.gprs_recv_confirm = true;
	if(C->recv.frame.cmd_id == 0xD1){
		report_tag_confirm(C,S,A);
	}
}


static void ec_recv_tag_confirm_trigger_tstaus(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	if (S->pre_cfg.flash_enable == NAND_FLASH_DISABLE)
		return;
	if(C->recv.frame.cmd_id != 0xD2){
		return ;
	}else{
		if(strncmp((const char *)C->recv.frame.param_buf+1,(const char *)C->status_buf,C->recv.frame.param_len - 2) == 0){
			C->status_cnt = 0;
			C->triger_confirm_flag = false;
			if(C->status_send_from_file){//如果是从文件里发出来的
				C->status_send_from_file = false;
				triger_status_delete(false);
			}
		}
	}
}


static void ec_recv_confirm_wifi_connect(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	if(S->pre_cfg.upload_mode == UPLOAD_MODE_WIFI && C->wifi_connect == false){
		C->wifi_connect = true;
	}
	if(S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS){
		C->gprs_priv.gprs_recv_confirm = true;
	}
	return;
}



static command_t cmd_recv_tag_confirm = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM,
	.execute = ec_recv_tag_confirm,
};

static command_t cmd_recv_tag_confirm_wireless = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM_WIRELESS,
	.execute = ec_recv_tag_confirm_wireless,
};

static command_t cmd_recv_tag_confirm_trigger_tstaus = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM_TRIGGER_STATUS,
	.execute = ec_recv_tag_confirm_trigger_tstaus,
};

static command_t cmd_recv_confirm_wifi_connect = {
	.cmd_id = COMMAND_RECV_CONFIRM_WIFI_CONNECT,
	.execute = ec_recv_confirm_wifi_connect,
};


/*
 * 注册指令集和属于此指令集的所有指令
 */
int data_center_init(void)
{
	int err = command_set_register(&cmdset_data_center);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm_wireless);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm_trigger_tstaus);
	err |= command_register(&cmdset_data_center, &cmd_recv_confirm_wifi_connect);
	return err;
}
