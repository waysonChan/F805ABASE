#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "parameter.h"

#include <string.h>

/*---------------------------------------------------------------------
 *	指令集:系统控制
 *--------------------------------------------------------------------*/
static command_set_t cmdset_sys_control = {
	.set_type = COMMAND_SYS_CONTROL_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:系统信息配置指令
 *--------------------------------------------------------------------*/
static void ec_sys_info_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const char *cmd_param = (const char *)C->recv.frame.param_buf;
	uint16_t param_len = C->recv.frame.param_len - 2;	/* 1字节指令字 + 1字节配置类型 */
	
	switch (*cmd_param) {
	case CFG_READER_NAME:
		sp_set_reader_name(S, cmd_param+1, param_len);
		err = CMD_EXE_SUCCESS;
		break;
	case CFG_READER_TYPE:
		if (READER_TYPE_LEN != param_len) {
			err = ERRCODE_CMD_FRAME;
		} else {
			sp_set_reader_type(S, cmd_param+1, READER_TYPE_LEN);
			err = CMD_EXE_SUCCESS;
		}

		break;
	case CFG_READER_SN:
		if (PRODUCT_SN_LEN != param_len) {
			err = ERRCODE_CMD_FRAME;
		} else {
			sp_set_reader_sn(S, cmd_param+1, PRODUCT_SN_LEN);
			err = CMD_EXE_SUCCESS;
		}
		
		break;
	case CFG_PASS_WORD:
		if (PASSWORD_LEN != param_len) {
			err = ERRCODE_CMD_FRAME;
		} else {
			sp_set_password(S, cmd_param+1, PASSWORD_LEN);
			err = CMD_EXE_SUCCESS;
		}
		
		break;
	default:			
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	command_answer(C, COMMAND_SYS_CONTROL_INFO_CFG, err, NULL, 0);
}

static command_t cmd_sys_info_config = {
	.cmd_id = COMMAND_SYS_CONTROL_INFO_CFG,
	.execute = ec_sys_info_config,
};

/*---------------------------------------------------------------------
 *	指令:系统信息查询指令
 *--------------------------------------------------------------------*/
static void ec_sys_info_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	int i;
	char *tmp;
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case CFG_READER_NAME:
		tmp = S->sysinfo.reader_name;
		for (i = 0; i < READER_NAME_LEN; i++) {
			if (0 == tmp[i])
				break;
		}
		
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, tmp, i);
		return;
	case CFG_READER_TYPE:
		tmp = S->sysinfo.reader_type;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, READER_TYPE_LEN);
		return;
	case CFG_READER_SN:
		tmp = S->sysinfo.reader_sn;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, PRODUCT_SN_LEN);
		return;
	case CFG_MCU_SWREV:
		tmp = S->sysinfo.mcu_swrev;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, READER_MCU_SWREV_LEN);
		return;
	case CFG_FPGA_SWREV:
		tmp = S->sysinfo.fpga_swrev;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, READER_FPGA_SWREV_LEN);
		return;
	case CFG_BSB_HWREV:			
		tmp = S->sysinfo.bsb_hwrev;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, READER_BSB_HWREV_LEN);
		return;
	case CFG_RFB_HWREV:
		tmp = S->sysinfo.rfb_hwrev;
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, CMD_EXE_SUCCESS, 
			tmp, READER_RFB_SWREV_LEN);
		return;
	case CFG_PASS_WORD:			
	default:			
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	if (err != CMD_EXE_SUCCESS) {
		command_answer(C, COMMAND_SYS_CONTROL_INFO_QUER, err, 0, 0);
	}
}

static command_t cmd_sys_info_query = {
	.cmd_id = COMMAND_SYS_CONTROL_INFO_QUER,
	.execute = ec_sys_info_query,
};

int sys_control_init(void)
{
	int err = command_set_register(&cmdset_sys_control);
	err |= command_register(&cmdset_sys_control, &cmd_sys_info_config);
	err |= command_register(&cmdset_sys_control, &cmd_sys_info_query);
	return err;
}
