#include "command.h"
#include "command_def.h"
#include "command_manager.h"
#include "errcode.h"
#include "rf_ctrl.h"
#include "gpio.h"
#include "maccmds.h"
#include "hostifregs.h"
#include "rfid_constants.h"

#include <string.h>
#include <unistd.h>

int work_status_timer_trigger(r2h_connect_t *C, system_param_t *S)
{
	uint64_t num_exp;
	if (read(S->work_status_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("work_status_timer_trigger read()");
		return -1;
	}

	uint8_t cmd_id;
	switch (S->work_status) {
	case WS_READ_USER:
		cmd_id = COMMAND_18K6C_MAN_READ_USERBANK;
		break;
	case WS_WRITE_USER:
		cmd_id = COMMAND_18K6C_MAN_WRITE_USERBANK;
		break;
	default:
		log_msg("work_status_timer_trigger: invalid work_status");
		return -1;
	}

	log_msg("---EPC access failed---");
	command_answer(C, cmd_id, ERRCODE_EPC_ACCESSFAIL, NULL, 0);
	S->work_status = WS_STOP;
	return 0;
}


/*---------------------------------------------------------------------
 *	指令集:18K6C
 *--------------------------------------------------------------------*/
static command_set_t cmdset_18k6cman = {
	.set_type = COMMAND_18K6C_MAN_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:标签选择
 *--------------------------------------------------------------------*/
static void ec_18k6c_select_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if ((*cmd_param != RFID_18K6C_MEMORY_BANK_EPC) &&
		(*cmd_param != RFID_18K6C_MEMORY_BANK_TID) &&
		(*cmd_param != RFID_18K6C_MEMORY_BANK_USER)) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}
	tag_param_t *T = &S->tag_param;
	T->select_bank = *cmd_param;
	T->select_offset = (*(cmd_param + 1)<<8) | (*(cmd_param + 2));
	if (T->select_offset > 0x3FFF) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	
	T->select_count = *(cmd_param + 3);
	log_msg("bank = %d; offset = %d; count = %d", T->select_bank, 
		T->select_offset, T->select_count);

	if (T->select_bank == RFID_18K6C_MEMORY_BANK_EPC) {
		T->select_offset += 32;
	}

	int nbyte = (T->select_count % 8) == 0 ? T->select_count/8 : T->select_count/8+1;
	memcpy(T->select_mask, cmd_param + 4, nbyte);

	r2000_tag_select(T, A);

out:
	command_answer(C, COMMAND_18K6C_MAN_SELECT_TAG, err, NULL, 0);
}

static command_t cmd_18k6c_select_tag = {
	.cmd_id = COMMAND_18K6C_MAN_SELECT_TAG,
	.execute = ec_18k6c_select_tag,
};

/*---------------------------------------------------------------------
 *	指令:读 EPC 码
 *--------------------------------------------------------------------*/
static void ec_18k6c_read_epc(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	int ant_index = *(cmd_param);

	/* i802s 不能设置为轮询模式 */
	if (((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT1)
		|| ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4)) {
		if (ant_index == 0) 
			ant_index = 1;
	}
	
	if (C->recv.frame.param_len < 4) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		//err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}
	
	if (ant_index >= 1 && ant_index <= 4) {
		if ((!S->ant_array[ant_index-1].enable) || 
			(set_active_antenna(S, ant_index) < 0)) {
			log_msg("antenna invalid");
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}
		r2000_set_ant_rfpower(S, A);

		if (write_mac_register(A, HST_CMD, CMD_18K6CINV) < 0) {
			log_msg("read epc error");
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}

		S->work_status = WS_READ_EPC_FIXED;
	} else if (ant_index == 0) {
		set_next_active_antenna(S);
		r2000_set_ant_rfpower(S, A);

		if (write_mac_register(A, HST_CMD, CMD_18K6CINV) < 0) {
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}

		S->work_status = WS_READ_EPC_INTURN;
	} else {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	//return;
	
out:
	command_answer(C, COMMAND_18K6C_MAN_READ_EPC, err, NULL, 0);
}

static command_t cmd_18k6c_read_epc = {
	.cmd_id = COMMAND_18K6C_MAN_READ_EPC,
	.execute = ec_18k6c_read_epc,
};

/*---------------------------------------------------------------------
 *	指令:读 TID 码
 *--------------------------------------------------------------------*/
static void ec_18k6c_read_tid(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{	
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	int ant_index = *(cmd_param);

	/* i802s 不能设置为轮询模式 */
	if (((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT1)
		|| ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4)) {
		if (ant_index == 0) 
			ant_index = 1;
	}

	if (C->recv.frame.param_len < 4) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		//err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_TID;
	T->access_offset = 0;
	T->access_wordnum = 4;
	bzero(T->access_pwd, sizeof(T->access_pwd));

	if (ant_index >= 1 && ant_index <= 4) {
		if ((!S->ant_array[ant_index-1].enable) || 
			(set_active_antenna(S, ant_index) < 0)) {
			log_msg("antenna invalid");
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}
		r2000_set_ant_rfpower(S, A);

		if (r2000_tag_read(T, A) < 0) {
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}

		S->work_status = WS_READ_TID_FIXED;
	} else if (ant_index == 0) {
		set_next_active_antenna(S);
		r2000_set_ant_rfpower(S, A);
		
		if (r2000_tag_read(T, A) < 0) {
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}

		S->work_status = WS_READ_TID_INTURN;		
	} else {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	//return;

out:
	command_answer(C, COMMAND_18K6C_MAN_READ_TID, err, NULL, 0);	
}

static command_t cmd_18k6c_read_tid = {
	.cmd_id = COMMAND_18K6C_MAN_READ_TID,
	.execute = ec_18k6c_read_tid,
};

/*---------------------------------------------------------------------
 *	指令:读 EPC + TID 码
 *--------------------------------------------------------------------*/
static void ec_18k6c_read_epc_tid(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	int ant_index = *(cmd_param);

	/* i802s 不能设置为轮询模式 */
	if (((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT1)
		|| ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4)) {
		if (ant_index == 0) 
			ant_index = 1;
	}

	if (C->recv.frame.param_len < 4) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		//err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_TID;
	T->access_offset = 0;
	T->access_wordnum = 4;
	bzero(T->access_pwd, sizeof(T->access_pwd));

	if (ant_index >= 1 && ant_index <= 4) {
		if ((!S->ant_array[ant_index-1].enable) || 
			(set_active_antenna(S, ant_index) < 0)) {
			log_msg("antenna invalid");
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}
		r2000_set_ant_rfpower(S, A);

		if (r2000_tag_read(T, A) < 0) {
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}
		
		S->work_status = WS_READ_EPC_TID_FIXED;
	} else if (ant_index == 0) {
		set_next_active_antenna(S);
		r2000_set_ant_rfpower(S, A);
		
		if (r2000_tag_read(T, A) < 0) {
			err = ERRCODE_EPC_UNKNOWERR;
			goto out;
		}

		S->work_status = WS_READ_EPC_TID_INTURN;		
	} else {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	//return;
	
out:
	command_answer(C, COMMAND_18K6C_MAN_READ_EPC_TID, err, NULL, 0);
}

static command_t cmd_18k6c_read_epc_tid = {
	.cmd_id = COMMAND_18K6C_MAN_READ_EPC_TID,
	.execute = ec_18k6c_read_epc_tid,
};

/*---------------------------------------------------------------------
 *	指令:读用户数据区
 *--------------------------------------------------------------------*/
#define MAX_READ_USER_BANK_LEN	126
static void ec_18k6c_read_userbank(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len < 5) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_USER;
	T->access_offset = (*(cmd_param+1) << 8) | (*(cmd_param+2));
	T->access_wordnum = *(cmd_param+3);
	if (T->access_wordnum == 0 || T->access_wordnum > MAX_READ_USER_BANK_LEN) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	bzero(T->access_pwd, sizeof(T->access_pwd));

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	/* 防work_status永久陷入读用户区状态 */
	work_status_timer_set(S, 800);
	S->work_status = WS_READ_USER;

	if (r2000_tag_read(T, A) < 0) {
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	return;	/* 必要 */

out:
	command_answer(C, COMMAND_18K6C_MAN_READ_USERBANK, err, NULL, 0);
}

static command_t cmd_18k6c_read_userbank = {
	.cmd_id = COMMAND_18K6C_MAN_READ_USERBANK,
	.execute = ec_18k6c_read_userbank,
};

/*---------------------------------------------------------------------
 *	指令:写 EPC 码
 *--------------------------------------------------------------------*/
static void ec_18k6c_write_epc(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len != (*(cmd_param+6)*2 + 8)) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_EPC;
	T->access_offset = PC_START_ADDR;
	T->access_wordnum = *(cmd_param+6) + 1;		/* 字长度+1(PC) */
	T->access_databuf[0] = *(cmd_param+6) << 3;	/* 字长度转化为位长度 */
	T->access_databuf[1] = 0x0;
	memcpy(T->access_pwd, cmd_param+2, 4);
	memcpy(T->access_databuf+2, cmd_param+7, *(cmd_param+6) * 2);

	r2000_tag_write(T, A);
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_WRITE_EPC, err, NULL, 0);
}

static command_t cmd_18k6c_write_epc = {
	.cmd_id = COMMAND_18K6C_MAN_WRITE_EPC,
	.execute = ec_18k6c_write_epc,
};

/*---------------------------------------------------------------------
 *	指令:写用户数据区
 *--------------------------------------------------------------------*/
#define	MAX_WRITE_USER_BANK_LEN		32
static void ec_18k6c_write_userbank(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len != (*(cmd_param+8)*2 + 10)) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_USER;
	T->access_offset = (*(cmd_param+6) << 8) | (*(cmd_param+7));
	T->access_wordnum = *(cmd_param+8);
	if (T->access_wordnum == 0 || T->access_wordnum > MAX_WRITE_USER_BANK_LEN) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	/* 防work_status永久陷入读用户区状态 */
	work_status_timer_set(S, 1500);	/* 读32字大概750，设置为1500是想在失败后再读一次 */
	S->work_status = WS_WRITE_USER;

	memcpy(T->access_pwd, cmd_param+2, 4);
	memcpy(T->access_databuf, cmd_param+9, T->access_wordnum * 2);
	r2000_tag_write(T, A);
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_WRITE_USERBANK, err, NULL, 0);
}

static command_t cmd_18k6c_write_userbank = {
	.cmd_id = COMMAND_18K6C_MAN_WRITE_USERBANK,
	.execute = ec_18k6c_write_userbank,
};

/*---------------------------------------------------------------------
 *	指令:设置访问密码
 *--------------------------------------------------------------------*/
static void ec_18k6c_set_accesspin(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len != 11) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_RESERVED;
	T->access_offset = ACCESS_PIN_ADDR;
	T->access_wordnum = ACCESS_PIN_LEN;
	memcpy(T->access_pwd, cmd_param+2, 4);
	memcpy(T->access_databuf, cmd_param+6, 4);

	r2000_tag_write(T, A);
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_SET_ACCESSPIN, err, NULL, 0);
}

static command_t cmd_18k6c_set_accesspin = {
	.cmd_id = COMMAND_18K6C_MAN_SET_ACCESSPIN,
	.execute = ec_18k6c_set_accesspin,
};

/*---------------------------------------------------------------------
 *	指令:设置销毁密码
 *--------------------------------------------------------------------*/
static void ec_18k6c_set_killpin(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len != 11) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	T->access_bank = RFID_18K6C_MEMORY_BANK_RESERVED;
	T->access_offset = KILL_PIN_ADDR;
	T->access_wordnum = KILL_PIN_LEN;
	memcpy(T->access_pwd, cmd_param+2, 4);
	memcpy(T->access_databuf, cmd_param+6, 4);

	r2000_tag_write(T, A);
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_SET_KILLPIN, err, NULL, 0);
}

static command_t cmd_18k6c_set_killpin = {
	.cmd_id = COMMAND_18K6C_MAN_SET_KILLPIN,
	.execute = ec_18k6c_set_killpin,
};

/*---------------------------------------------------------------------
 *	指令:设置标签锁状态
 *--------------------------------------------------------------------*/
static void ec_18k6c_lock_operate(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if ((*(cmd_param+5) > 3) || (*(cmd_param+6) > 5) 
		|| (C->recv.frame.param_len != 8)) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	T->lock_type = *(cmd_param+5);
	T->lock_bank = *(cmd_param+6);
	T->access_bank = RFID_18K6C_MEMORY_BANK_RESERVED;
	T->access_offset = ACCESS_PIN_ADDR;
	T->access_wordnum = ACCESS_PIN_LEN;
	memcpy(T->access_pwd, cmd_param+1, 4);
	memcpy(T->access_databuf, T->access_pwd, 4);

	if (r2000_tag_lock(C, T, A) < 0) {
		log_msg("r2000_tag_lock error");
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_LOCK_OPERATION, err, NULL, 0);
}

static command_t cmd_18k6c_lock_operate = {
	.cmd_id = COMMAND_18K6C_MAN_LOCK_OPERATION,
	.execute = ec_18k6c_lock_operate,
};

/*---------------------------------------------------------------------
 *	指令:杀死标签
 *--------------------------------------------------------------------*/
static void ec_18k6c_tag_kill(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len < 5 || C->recv.frame.param_len > 18) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	if (r2000_error_check(C, S, A) < 0) {
		err = ERRCODE_SYS_R2000_ERR;
		goto out;
	}

	int ant_index = *(cmd_param);
	if (ant_index < 1 || ant_index > 4) {
		err = ERRCODE_CMD_PARAM;
		goto out;
	} else if ((!S->ant_array[ant_index-1].enable) || 
		(set_active_antenna(S, ant_index) < 0)) {
		log_msg("antenna invalid");
		err = ERRCODE_EPC_UNKNOWERR;
		goto out;
	}
	r2000_set_ant_rfpower(S, A);

	tag_param_t *T = &S->tag_param;
	memcpy(T->kill_pwd, cmd_param+1, 4);

	if (r2000_tag_kill(T, A) < 0) {
		log_msg("r2000_tag_kill error");
		err = ERRCODE_OPT_UNKNOWERR;
		goto out;
	}
	return;

out:
	command_answer(C, COMMAND_18K6C_MAN_KILL_TAG, err, NULL, 0);
}

static command_t cmd_18k6c_tagkill = {
	.cmd_id = COMMAND_18K6C_MAN_KILL_TAG,
	.execute = ec_18k6c_tag_kill,
};

int epc_18k6c_init(void)
{
	int err = command_set_register(&cmdset_18k6cman);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_select_tag);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_read_epc);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_read_tid);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_write_epc);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_read_userbank);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_write_userbank);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_set_accesspin);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_set_killpin);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_lock_operate);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_tagkill);
	err |= command_register(&cmdset_18k6cman, &cmd_18k6c_read_epc_tid);
	return err;
}
