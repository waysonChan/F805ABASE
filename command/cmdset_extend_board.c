#include "config.h"
#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "parameter.h"
#include "connect.h"

#include <strings.h>
#include <string.h>
#include <unistd.h>

/*---------------------------------------------------------------------
 *	指令集:扩展板相关操作
 *--------------------------------------------------------------------*/
static command_set_t cmdset_extend_board = {
	.set_type = COMMAND_EXTEND_BOARD_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:WIFI 进入透传或复位
 *--------------------------------------------------------------------*/

#define WIFI_TRANS_PARAM_LEN	2

static void ec_wifi_raw_transmit(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;

	if (C->recv.frame.param_len != WIFI_TRANS_PARAM_LEN
		|| C->conn_type != R2H_RS232) {
		log_msg("param_len error or conn_type != R2H_RS232");
		err = ERRCODE_CMD_FRAME;
	} else {
		log_msg("wifi_transparent_mode = true");
		C->wifi_transparent_mode = true;
	}

	command_answer(C, COMMAND_WIFI_TRANS_RESET, err, NULL, 0);
}

static command_t cmd_wifi_raw_transmit = {
	.cmd_id = COMMAND_WIFI_TRANS_RESET,
	.execute = ec_wifi_raw_transmit,
};

/*
 * 注册指令集和属于此指令集的所有指令
 */
int extend_board_init(void)
{
	int err = command_set_register(&cmdset_extend_board);
	err |= command_register(&cmdset_extend_board, &cmd_wifi_raw_transmit);
	return err;
}
