#include "command.h"
#include "command_def.h"
#include "command_manager.h"
#include "errcode.h"

/*---------------------------------------------------------------------
 *	指令集:系统控制
 *--------------------------------------------------------------------*/
static command_set_t cmdset_trans_control = {
	.set_type = COMMAND_TRANSMIT_CONTROL_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:建立连接指令
 *--------------------------------------------------------------------*/
static void ec_trans_ctrl_connect(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;

	log_msg("accept a client");
	if (S->work_status != WS_STOP) {
		stop_read_tag(S, A);
	}

	command_answer(C, COMMAND_TRANSMIT_CONTROL_LINK, err, NULL, 0);
}

static command_t cmd_trans_ctrl_connect = {
	.cmd_id = COMMAND_TRANSMIT_CONTROL_LINK,
	.execute = ec_trans_ctrl_connect,
};

/*---------------------------------------------------------------------
 *	指令:断开连接指令
 *--------------------------------------------------------------------*/
static void ec_trans_ctrl_closeconn(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_answer(C, COMMAND_TRANSMIT_CONTROL_UNLINK, CMD_EXE_SUCCESS, NULL, 0);
	r2h_connect_close_client(C);
}

static command_t cmd_trans_ctrl_closeconn = {
	.cmd_id = COMMAND_TRANSMIT_CONTROL_UNLINK,
	.execute = ec_trans_ctrl_closeconn,
};

/*---------------------------------------------------------------------
 *	指令:心跳检测指令
 *--------------------------------------------------------------------*/
static void ec_trans_ctrl_heartbeat(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_answer(C, COMMAND_TRANSMIT_CONTROL_HEARTBEAT, CMD_EXE_SUCCESS, NULL, 0);
}

static command_t cmd_trans_ctrl_heartbeat = {
	.cmd_id = COMMAND_TRANSMIT_CONTROL_HEARTBEAT,
	.execute = ec_trans_ctrl_heartbeat,
};


/*---------------------------------------------------------------------
 *	指令:I/O 触发状态检测指令
 *--------------------------------------------------------------------*/
static void ec_trans_ctrl_triggerstatus(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_answer(C, COMMAND_TRANSMIT_CONTROL_TRIGGERSTATUS, CMD_EXE_SUCCESS, NULL, 0);
}

static command_t cmd_trans_ctrl_triggerstatus = {
	.cmd_id = COMMAND_TRANSMIT_CONTROL_TRIGGERSTATUS,
	.execute = ec_trans_ctrl_triggerstatus,
};


int trans_control_init(void)
{
	int err = command_set_register(&cmdset_trans_control);
	err |= command_register(&cmdset_trans_control, &cmd_trans_ctrl_connect);
	err |= command_register(&cmdset_trans_control, &cmd_trans_ctrl_heartbeat);
	err |= command_register(&cmdset_trans_control, &cmd_trans_ctrl_closeconn);
	err |= command_register(&cmdset_trans_control, &cmd_trans_ctrl_triggerstatus);
	return err;
}
