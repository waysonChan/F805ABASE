#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "rf_ctrl.h"
#include "parameter.h"
#include "report_tag.h"
#include "hostifregs.h"
#include "maccmds.h"
#include "gpio.h"

#include <stdlib.h>

/*---------------------------------------------------------------------
 *	指令集:系统控制
 *--------------------------------------------------------------------*/
static command_set_t cmdset_set_reader_man = {
	.set_type = COMMAND_READER_MAN_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

uint8_t stop_read_tag(system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	
	switch (S->work_status) {
	case WS_READ_EPC_FIXED:
	case WS_READ_EPC_FIXED_WEB:
		r2000_control_command(A, R2000_CANCEL);
		/* no break */
	case WS_READ_EPC_INTURN:
	case WS_READ_TID_FIXED:
	case WS_READ_TID_INTURN:
	case WS_READ_EPC_TID_FIXED:
	case WS_READ_EPC_TID_INTURN:
	case WS_READ_USER:
	case WS_WRITE_USER:
		S->work_status = WS_STOP;
		report_tag_reset(&A->tag_report);
		break;
	case WS_STOP:
		log_msg("%s: already stopped.", __FUNCTION__);
		err = CMD_EXE_SUCCESS;
		break;
	default:
		log_msg("%s: unknown work mode.", __FUNCTION__);
		err = ERRCODE_OPT_UNKNOWERR;
	}

	return err;
}

/*---------------------------------------------------------------------
 *	指令:载波操作指令
 *--------------------------------------------------------------------*/
static void ec_reader_man_rfpwd(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	/* 
	 * 注意: 1.这样实现只是为了和F805版本的上位机程序兼容，因为上位机在
	 *         停止读卡时发的是 0x20 指令
	 *       2.另一方面，R2000 模块读卡时会自动控制功放，所以没必要手动
	 *         操作，故本软件不实现"开/关功放"
	 *       3.目前只有"停止"读卡才发送此指令，如果用于其它情况则需要重
	 *         新设计此函数
	 *	4.重新连接也发的此指令 2013-11-29
	 */
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case RFPWD_OFF:
		err = stop_read_tag(S, A);
		break;
	case RFPWD_ON:
		write_mac_register(A, HST_CMD, CMD_CWON);
		break;
	default:
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}

out:
	command_answer(C, COMMAND_READER_MAN_RFPWD, err, NULL, 0);
}

static command_t cmd_reader_man_rfpwd = {
	.cmd_id = COMMAND_READER_MAN_RFPWD,
	.execute = ec_reader_man_rfpwd,
};

/*---------------------------------------------------------------------
 *	指令:IO输出操作指令
 *--------------------------------------------------------------------*/
static void ec_reader_man_io_output(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t port = *(cmd_param);
	uint8_t state = *(cmd_param+1);
	gpio_index_e gpio_index;

	log_msg("port = %d, state = %d", port, state);

	switch (port) {
	case 0:
		gpio_index = 0;
		break;
	case 1:
		gpio_index = IO_OUT1;
		break;
	case 2:
		gpio_index = IO_OUT2;
		break;
	case 3:
		gpio_index = IO_OUT3;
		break;
	case 4:
		gpio_index = IO_OUT4;
		break;
	default:
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	/* 由于硬件原因，输出反转，下同 */
	switch (state) {
	case 0:	/* 低电平 */
		if (gpio_index) {
			set_gpio_status(gpio_index, GPO_OUTPUT_HIGH);
		} else {
			set_gpio_status(IO_OUT1, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT2, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT3, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT4, GPO_OUTPUT_HIGH);
		}
		break;
	case 1:	/* 高电平 */
		if (gpio_index) {
			set_gpio_status(gpio_index, GPO_OUTPUT_LOW);
		} else {
			set_gpio_status(IO_OUT1, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT2, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT3, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT4, GPO_OUTPUT_LOW);
		}
		break;
	case 2:	/* 正脉冲 */
		if (gpio_index) {
			set_gpio_status(gpio_index, GPO_OUTPUT_LOW);
		} else {
			set_gpio_status(IO_OUT1, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT2, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT3, GPO_OUTPUT_LOW);
			set_gpio_status(IO_OUT4, GPO_OUTPUT_LOW);
		}
		break;
	case 3:	/* 负脉冲 */
		if (gpio_index) {
			set_gpio_status(gpio_index, GPO_OUTPUT_HIGH);
		} else {
			set_gpio_status(IO_OUT1, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT2, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT3, GPO_OUTPUT_HIGH);
			set_gpio_status(IO_OUT4, GPO_OUTPUT_HIGH);
		}
		break;
	default:
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}

out:
	command_answer(C, COMMAND_READER_MAN_IO_OUTPUT, err, NULL, 0);	
}

static command_t cmd_reader_man_io_output = {
	.cmd_id = COMMAND_READER_MAN_IO_OUTPUT,
	.execute = ec_reader_man_io_output,
};

/*---------------------------------------------------------------------
 *	指令:IO输入查询指令
 *--------------------------------------------------------------------*/
static void ec_reader_man_io_input(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t state, err = CMD_EXE_SUCCESS;
	uint8_t btns_val[MAX_GPI_NUM];

	if (get_btns_status(btns_val)) {
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}
	btns_val[0] = S->gpio_dece.gpio1_val;
	btns_val[1] = S->gpio_dece.gpio2_val;
	state = btns_val[0] | (btns_val[1] << 1);

out:
	command_answer(C, COMMAND_READER_MAN_IO_INPUT, CMD_EXE_SUCCESS, &state, sizeof(state));
}

static command_t cmd_reader_man_io_input = {
	.cmd_id = COMMAND_READER_MAN_IO_INPUT,
	.execute = ec_reader_man_io_input,
};

/*---------------------------------------------------------------------
 *	指令:蜂鸣器操作指令
 *--------------------------------------------------------------------*/
static void ec_reader_man_beep_cfg(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t mode = *(cmd_param);

	/* BEEP */
	if (mode & 0x1) {
		set_gpio_status(BEEP, GPO_OUTPUT_LOW);	/* 由于硬件原因，输出反转，下同 */
		gpo_pulse_set_timer(&S->gpo[GPO_IDX_BEEP]);
	} else {
		set_gpio_status(BEEP, GPO_OUTPUT_HIGH);
	}

	/* GPO1 */
	if (mode & 0x2) {
		set_gpio_status(IO_OUT1, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&S->gpo[GPO_IDX_1]);
	} else {
		set_gpio_status(IO_OUT1, GPO_OUTPUT_HIGH);
	}

	/* GPO2 */
	if (mode & 0x4) {
		set_gpio_status(IO_OUT2, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&S->gpo[GPO_IDX_2]);
	} else {
		set_gpio_status(IO_OUT2, GPO_OUTPUT_HIGH);
	}

	/* GPO3 */
	if (mode & 0x8) {
		set_gpio_status(IO_OUT3, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&S->gpo[GPO_IDX_3]);
	} else {
		set_gpio_status(IO_OUT3, GPO_OUTPUT_HIGH);
	}

	/* GPO4 */
	if (mode & 0x10) {
		set_gpio_status(IO_OUT4, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&S->gpo[GPO_IDX_4]);
	} else {
		set_gpio_status(IO_OUT4, GPO_OUTPUT_HIGH);
	}

	command_answer(C, COMMAND_READER_MAN_BEEP_CFG, err, NULL, 0);
}

static command_t cmd_reader_man_beep_cfg = {
	.cmd_id = COMMAND_READER_MAN_BEEP_CFG,
	.execute = ec_reader_man_beep_cfg,
};

/*---------------------------------------------------------------------
 *	指令:蜂鸣器查询指令
 *--------------------------------------------------------------------*/
static void ec_reader_man_beep_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t beep = 1 - get_gpio_status(BEEP);	/* 由于硬件原因，输出反转 */
	char gpo1 = 1 - get_gpio_status(IO_OUT1);
	char gpo2 = 1 - get_gpio_status(IO_OUT2);
	char gpo3 = 1 - get_gpio_status(IO_OUT3);
	char gpo4 = 1 - get_gpio_status(IO_OUT4);
	uint8_t state = beep | (gpo1 << 1) | (gpo2 << 2) | (gpo3 << 3) | (gpo4 << 4);
	command_answer(C, COMMAND_READER_MAN_BEEP_QUERY, CMD_EXE_SUCCESS, &state, sizeof(state));
}

static command_t cmd_reader_man_beep_query = {
	.cmd_id = COMMAND_READER_MAN_BEEP_QUERY,
	.execute = ec_reader_man_beep_query,
};

/*---------------------------------------------------------------------
 *	指令:脉冲宽度配置
 *--------------------------------------------------------------------*/
static void ec_reader_man_pulse_width_cfg(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t gpo_index = *(cmd_param);

	if (gpo_index < 0 || gpo_index > 4) {
		log_msg("gpo_index invalid");
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}

	S->gpo[gpo_index].pulse_width = *(cmd_param + 1);
	cfg_set_gpo(&S->gpo[gpo_index], gpo_index);

out:
	command_answer(C, COMMAND_READER_MAN_PULSE_WIDTH_CFG, err, NULL, 0);
}

static command_t cmd_reader_man_pulse_width_cfg = {
	.cmd_id = COMMAND_READER_MAN_PULSE_WIDTH_CFG,
	.execute = ec_reader_man_pulse_width_cfg,
};

/*---------------------------------------------------------------------
 *	指令:脉冲宽度查询
 *--------------------------------------------------------------------*/
static void ec_reader_man_pulse_width_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t gpo_index = *(cmd_param);
	
	if (gpo_index < 0 || gpo_index > 4) {
		log_msg("gpo_index invalid");
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}

out:
	command_answer(C, COMMAND_READER_MAN_PULSE_WIDTH_QUERY, err, 
		&S->gpo[gpo_index].pulse_width, 1);
}

static command_t cmd_reader_man_pulse_width_query = {
	.cmd_id = COMMAND_READER_MAN_PULSE_WIDTH_QUERY,
	.execute = ec_reader_man_pulse_width_query,
};

/*---------------------------------------------------------------------
 *	指令:设备重启
 *--------------------------------------------------------------------*/
static void ec_reader_man_reboot(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_answer(C, COMMAND_READER_MAN_REBOOT, CMD_EXE_SUCCESS, NULL, 0);
	system("reboot");
}

static command_t cmd_reader_man_beep_reboot = {
	.cmd_id = COMMAND_READER_MAN_REBOOT,
	.execute = ec_reader_man_reboot,
};

/*---------------------------------------------------------------------
 *	指令:程序退出
 *--------------------------------------------------------------------*/
static void ec_reader_man_app_exit(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_answer(C, COMMAND_READER_MAN_APP_EXIT, CMD_EXE_SUCCESS, NULL, 0);
	exit(0);
}

static command_t cmd_reader_man_app_exit = {
	.cmd_id = COMMAND_READER_MAN_APP_EXIT,
	.execute = ec_reader_man_app_exit,
};

/*
 * 注册指令集和属于此指令集的所有指令
 */
int reader_man_init(void)
{
	int err = command_set_register(&cmdset_set_reader_man);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_rfpwd);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_io_output);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_io_input);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_beep_cfg);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_beep_query);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_pulse_width_cfg);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_pulse_width_query);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_beep_reboot);
	err |= command_register(&cmdset_set_reader_man, &cmd_reader_man_app_exit);
	return err;
}
