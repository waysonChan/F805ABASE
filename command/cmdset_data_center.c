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
		list_delete_header();
	} else {
		tag_storage_delete();
	}

	gprs_tag_send_header(C, S, A);
}

static command_t cmd_recv_tag_confirm = {
	.cmd_id = COMMAND_RECV_TAG_CONFIRM,
	.execute = ec_recv_tag_confirm,
};

/*
 * 注册指令集和属于此指令集的所有指令
 */
int data_center_init(void)
{
	int err = command_set_register(&cmdset_data_center);
	err |= command_register(&cmdset_data_center, &cmd_recv_tag_confirm);
	return err;
}
