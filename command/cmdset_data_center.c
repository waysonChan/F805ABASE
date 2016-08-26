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
	if (A->tag_report.filter_enable == false
		|| S->pre_cfg.flash_enable == NAND_FLASH_DISABLE)
		return;
	
	gprs_priv_t *gprs_priv = &C->gprs_priv;
	gprs_priv->gprs_fail_cnt = 0;
	gprs_priv->gprs_wait_flag = false;

	if (gprs_priv->gprs_send_type == GPRS_SEND_TYPE_RAM) {
		tag_report_list_del(&A->tag_report);
	} else {
		tag_storage_delete(false);
	}
	if(S->work_status != WS_STOP)
		gprs_tag_send_header(C, S, A);
}




static void ec_recv_tag_confirm_wifi(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	int i = 0;
	bool flag = false;
	
	wifi_priv_t *wifi_priv = &C->wifi_priv;
	if (A->tag_report.filter_enable == false
		|| S->pre_cfg.flash_enable == NAND_FLASH_DISABLE)
		return;
	
	if(C->recv.frame.cmd_id == 0xD1){//过滤心跳包0x58
		for(i = 0;i < C->recv.frame.param_len - 1- 7; i++){
			if(C->recv.frame.param_buf[i] == C->send.wbuf[5+i]){
				continue;//卡号相同
			}else{
				flag = true;
			}
		}
		if(!flag){
			wifi_priv->wifi_fail_cnt = 0;
			wifi_priv->wifi_wait_flag = false;
			if (wifi_priv->wifi_send_type == WIFI_SEND_TYPE_RAM) {
				;
				//tag_report_list_del(&A->tag_report);
			} else {
				tag_storage_delete(false);
			}
			return;
		}
		wifi_tag_send_header(C, S, A);
	}
}


static void ec_recv_tag_confirm_trigger_tstaus(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	int i = 0;
	bool flag = false;
	if (S->pre_cfg.flash_enable == NAND_FLASH_DISABLE)
		return;
	if(C->recv.frame.cmd_id != 0xD2){
		return ;
	}else{
		for(i = 0;i < C->recv.frame.param_len; i++){
			if(C->recv.frame.param_buf[i] == C->send.wbuf[5+i]){
				continue;//相同
			}else{
				log_msg("Not The Same Status");
				flag = true;
				break;
			}
		}
		if(!flag){
			C->status_cnt = 0;
			if(C->status_flag)//如果是从文件里发出来的
				triger_status_delete(false);
		}
	}
	log_msg("ec_recv_tag_confirm_trigger_tstaus");
}





static command_t cmd_recv_tag_confirm = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM,
	.execute = ec_recv_tag_confirm,
};

static command_t cmd_recv_tag_confirm_wifi = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM_WIFI,
	.execute = ec_recv_tag_confirm_wifi,
};

static command_t cmd_recv_tag_confirm_trigger_tstaus = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM_TRIGGER_STATUS,
	.execute = ec_recv_tag_confirm_trigger_tstaus,
};



/*
 * 注册指令集和属于此指令集的所有指令
 */
int data_center_init(void)
{
	int err = command_set_register(&cmdset_data_center);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm_wifi);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm_trigger_tstaus);
	return err;
}
