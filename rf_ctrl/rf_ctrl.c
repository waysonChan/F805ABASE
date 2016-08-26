#include "rf_ctrl.h"
#include "connect.h"
#include "rfid_packets.h"
#include "hostpkts.h"
#include "errcode.h"
#include "command.h"
#include "command_def.h"
#include "macregs.h"
#include "maccmds.h"
#include "rfid_constants.h"
#include "utility.h"
#include "gpio.h"
#include "report_tag.h"
#include "command_manager.h"
#include <sys/timerfd.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

//static uint8_t error_check[] = {0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
/* Control Commands */
static uint8_t cmd_cancel[] = {0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmd_abort[] = {0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmd_pause[] = {0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmd_resume[] = {0x40, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmd_get_sn[] = {0xC0, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmd_reset_bl[] = {0x40, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
 * 此函数用于在R2000复位后,恢复复位前设置的功率
 */
static void _set_ant_power(ap_connect_t *A)
{
	write_mac_register(A, HST_ANT_DESC_SEL, 0x0);
	write_mac_register(A, HST_ANT_DESC_CFG, 0x1);
	write_mac_register(A, HST_ANT_DESC_PORTDEF, 0x0);
	write_mac_register(A, HST_ANT_DESC_DWELL, 2000);
	write_mac_register(A, HST_ANT_DESC_RFPOWER, RFPOWER_F806_TO_R2000(A->cur_ant_power));
	log_msg("rfpower = %d", A->cur_ant_power);
}

#ifdef R2000_SOFT_RESET
static uint8_t cmd_reset[] = {0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static int _r2000_soft_reset(ap_connect_t *A)
{
	int ret;
	uint8_t buf[256] = {0};
	ret = rs232_write(A->fd, cmd_reset, sizeof(cmd_reset));
	if (ret != sizeof(cmd_reset))
		return -1;

	if (rs232_read(A->fd, buf, sizeof(buf)) < 0)
		return -1;

	return buf[6] == 0xBF ? 0 : -1;	/* P21 */
}
#else
static int _r2000_hard_reset(ap_connect_t *A)
{
	//set_gpio_status(BEEP, GPO_OUTPUT_LOW);
	set_gpio_status(R2000_RESET, 0);
	usleep(50000);
	set_gpio_status(R2000_RESET, 1);
	usleep(300000);
	rs232_flush(A->fd);
	//set_gpio_status(BEEP, GPO_OUTPUT_HIGH);
	
	return 0;
}
#endif

static int _r2000_abort(ap_connect_t *A)
{
	int ret;
	uint8_t buf[256] = {0};
	ret = rs232_write(A->fd, cmd_abort, sizeof(cmd_abort));
	if (ret != sizeof(cmd_abort))
		return -1;

	if (rs232_read(A->fd, buf, sizeof(buf)) < 0)
		return -1;

	return buf[6] == 0xBF ? 0 : -1;	/* P20 */
}

static int _r2000_reset_bl(ap_connect_t *A)
{
	int ret;
	uint8_t buf[256] = {0};
	ret = rs232_write(A->fd, cmd_reset_bl, sizeof(cmd_reset_bl));
	if (ret != sizeof(cmd_reset_bl))
		return -1;

	if (rs232_read(A->fd, buf, sizeof(buf)) < 0)
		return -1;

	return buf[6] == 0xBF ? 0 : -1;	/* P22 */
}

static int _r2000_get_sn(ap_connect_t *A)
{
	int ret;
	uint8_t buf[256] = {0};
	ret = rs232_write(A->fd, cmd_get_sn, sizeof(cmd_get_sn));
	if (ret != sizeof(cmd_get_sn))
		return -1;

	if (rs232_read(A->fd, buf, sizeof(buf)) < 0)
		return -1;

	return buf[1] == 0x03 ? 0 : -1;
}

int r2000_control_command(ap_connect_t *A, ctrl_cmd_t ctrl_cmd)
{
	int err;
	
	switch (ctrl_cmd) {
	case R2000_CANCEL:	/* NOT response */
		err = rs232_write(A->fd, cmd_cancel, sizeof(cmd_cancel));
		break;
	case R2000_SOFTRESET:	/* response/P21 */
#ifdef R2000_SOFT_RESET
		err = _r2000_soft_reset(A);		
		usleep(500000);
#else
		err = _r2000_hard_reset(A);
#endif
		_set_ant_power(A);
		if (A->select_param.count) {
			r2000_tag_select(&A->select_param, A);
		}
		break;
	case R2000_ABORT:	/* response/P21 */
		err = _r2000_abort(A);
		break;
	case R2000_PAUSE:	/* NOT response */
		err = rs232_write(A->fd, cmd_pause, sizeof(cmd_pause));
		break;
	case R2000_RESUME:	/* NOT response */
		err = rs232_write(A->fd, cmd_resume, sizeof(cmd_resume));
		break;
	case R2000_GET_SN:	/* response/P22 */
		err = _r2000_get_sn(A);
		break;
	case R2000_RESET_TO_BOOTLOADER:	/* response/P22 */
		err = _r2000_reset_bl(A);
		break;
	default:
		log_msg("invalid control command");
		return -1;
	}

	return err;
}

int read_mac_register(ap_connect_t *A, uint16_t reg_addr, uint32_t *reg_data)
{
	int ret;
	struct host_reg_resp response;
	struct host_reg_req request;

	request.access_flg = HOST_REG_REQ_ACCESS_READ;
	request.reg_addr = reg_addr;
	request.reg_data = 0;

	ret = rs232_write(A->fd, (uint8_t *)&request, sizeof(request));
	if (ret != sizeof(request))
		return -1;
#if 0
	if (rs232_read(A->fd, (uint8_t *)&response, sizeof(response)) < 0)
		return -1;
#else
	ret = rs232_read(A->fd, (uint8_t *)&response, sizeof(response));
	if (ret != sizeof(response)) {
		/* 读寄存器出错,复位 */
		//log_msg("read_mac_register: rs232_read error, >>>>> reseting R2000 <<<<<");
		//r2000_control_command(A, R2000_SOFTRESET);	
		log_msg("read_mac_register: rs232_read error");
		return -1;
	}
#endif
	*reg_data = response.reg_data;
	return 0;
}

int write_mac_register(ap_connect_t *A, uint16_t reg_addr, uint32_t reg_data)
{
	struct host_reg_req request;
	request.access_flg = HOST_REG_REQ_ACCESS_WRITE;
	request.reg_addr = reg_addr;
	request.reg_data = reg_data;

	return rs232_write(A->fd, (uint8_t *)&request, sizeof(request));
}

static void _r2000_error_report(r2h_connect_t *C, uint8_t ant_index, uint32_t mac_err, bool log_enable)
{
	if (!log_enable)
		return;

	uint8_t wbuf[3] = {0};
	wbuf[0] = ant_index;
	wbuf[1] = mac_err >> 8;
	wbuf[2] = mac_err & 0xFF;

	command_answer(C, COMMAND_R2000_ERROR_REPORT, CMD_EXE_SUCCESS, wbuf, sizeof(wbuf));
}

#define MAC_ERR_MODULE_NOT_FOUND	0x0801
int r2000_error_check(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint32_t mac_err = 0;
	if (read_mac_register(A, MAC_ERROR, &mac_err) < 0) {
		log_msg("%s: read_mac_register error, >>>>> reseting R2000 <<<<<", __FUNCTION__);
		r2000_control_command(A, R2000_SOFTRESET);

		if (r2000_control_command(A, R2000_GET_SN) < 0) {
			log_msg("R2000 not found");
			_r2000_error_report(C, S->cur_ant, MAC_ERR_MODULE_NOT_FOUND, A->r2000_error_log);
			return -1;
		}
	}

	if (mac_err) {
		log_msg(">>>>> mac_err = 0x%08X <<<<<", mac_err);
		_r2000_error_report(C, S->cur_ant, mac_err, A->r2000_error_log);
		
		if (mac_err != 0x0309 && mac_err != 0x0316) {	/* 没接天线也报 0x0309 故不能复位 */
			log_msg(">>>>> reseting R2000 <<<<<");
			r2000_control_command(A, R2000_SOFTRESET);

			if (r2000_control_command(A, R2000_GET_SN) < 0) {
				log_msg("R2000 not found");
				_r2000_error_report(C, S->cur_ant, MAC_ERR_MODULE_NOT_FOUND, A->r2000_error_log);
				return -1;
			}
		}
	}

	return 0;
}

static tag_t epc_tid;
static tag_t tag_user;
static int read_user_done = 0;
static int wlk_done = 0;

static int tag_user_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	if (!read_user_done) {
		log_msg("tag_user_send: no tag");
		return -1;
	}

	if (C->conn_type != R2H_NONE) {
		uint8_t cmd_id = COMMAND_18K6C_MAN_READ_USERBANK;
		command_answer(C, cmd_id, CMD_EXE_SUCCESS, &tag_user, tag_user.tag_len);
		read_user_done = 0;
	} else {
		report_tag_send(C, S, A, &tag_user);
	}

	return 0;
}


int process_cmd_packets(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t buf[1024];
	int pkt_len;
	RFID_PACKET_COMMON *pcmn = &A->recv.cmn;
	RFID_PACKET_COMMAND_BEGIN cmd_begin;
	RFID_PACKET_COMMAND_END cmd_end;
	RFID_PACKET_18K6C_INVENTORY inv;
	RFID_PACKET_18K6C_TAG_ACCESS access;

	switch (pcmn->pkt_type) {
	case RFID_PACKET_TYPE_COMMAND_BEGIN:
		pkt_len = sizeof(cmd_begin) - sizeof(*pcmn);
		rs232_read(A->fd, (uint8_t *)&cmd_begin+sizeof(*pcmn), pkt_len);
		log_msg("COMMAND BEGIN");
		break;
	case RFID_PACKET_TYPE_COMMAND_END:
		pkt_len = sizeof(cmd_end) - sizeof(*pcmn);
		rs232_read(A->fd, (uint8_t *)&cmd_end+sizeof(*pcmn), pkt_len);
		log_msg("COMMAND END");
		r2000_error_check(C, S, A);

		switch (S->work_status) {
		case WS_READ_EPC_FIXED:
			r2000_set_ant_rfpower(S, A);
			write_mac_register(A, HST_CMD, CMD_18K6CINV);
			break;
		case WS_READ_EPC_INTURN:
			set_next_active_antenna(S);
			r2000_set_ant_rfpower(S, A);
			write_mac_register(A, HST_CMD, CMD_18K6CINV);
			break;
		case WS_READ_TID_INTURN:
		case WS_READ_EPC_TID_INTURN:
			set_next_active_antenna(S);
			/* no break */
		case WS_READ_TID_FIXED:
		case WS_READ_EPC_TID_FIXED:
			r2000_set_ant_rfpower(S, A);
			r2000_tag_read(&S->tag_param, A);
			break;
		case WS_READ_USER:
			r2000_set_ant_rfpower(S, A);
			if (C->conn_type != R2H_NONE) {
				work_status_timer_set(S, 0);
				S->work_status = WS_STOP;
				tag_user_send(C, S, A);
			} else {
				tag_user_send(C, S, A);
				r2000_tag_read(&S->tag_param, A);
			}
			break;
		case WS_WRITE_USER:
			work_status_timer_set(S, 0);
			S->work_status = WS_STOP;
			if (wlk_done) {
				wlk_done = 0;
				command_answer(C, C->recv.frame.cmd_id, CMD_EXE_SUCCESS, NULL, 0);
			}
			break;
		case WS_STOP:
			set_antenna_led_status(S->cur_ant, LED_COLOR_GREEN, S->pre_cfg.dev_type);
			break;
		default:
			log_msg("process_cmd_packets: invalid work mode");
			return -1;
		}
		
		break;
	case RFID_PACKET_TYPE_ANTENNA_CYCLE_BEGIN:
		pkt_len = sizeof(RFID_PACKET_TYPE_ANTENNA_CYCLE_BEGIN);
		log_msg("not implemented");
		break;
	case RFID_PACKET_TYPE_ANTENNA_BEGIN:
		pkt_len = sizeof(RFID_PACKET_TYPE_ANTENNA_BEGIN);
		log_msg("not implemented");
		break;
	case RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_BEGIN:
		pkt_len = sizeof(RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_BEGIN);
		log_msg("not implemented");
		break;
	case RFID_PACKET_TYPE_18K6C_INVENTORY:
		beep_and_blinking(&S->gpo[GPO_IDX_BEEP]);
		pkt_len = sizeof(inv) - sizeof(inv.inv_data) - sizeof(*pcmn);
		rs232_read(A->fd, (uint8_t *)&inv+sizeof(*pcmn), pkt_len);
		if (pcmn->pkt_len <= 3) {
			log_msg("invalid pcmn->pkt_len");
			return -1;
		}

		/*
		 * 注意:
		 *	1. pcmn->pkt_len的单位是双字,即4字节(乘4的原因)
		 *	2. pcmn->pkt_len不包括RFID_PACKET_COMMON自身的长度(8字节)
		 *	3. -3 的原因: Bytes8到Bytes19为inventory的相关参数
		 *	4. 从Bytes20开始为Inv_Data: 依次为PC -> EPC -> CRC16
		 */
		int data_len = (pcmn->pkt_len - 3) * 4;
		if (data_len > sizeof(buf)) {
			log_msg("data_len(%d) invalid", data_len);
			return -1;
		}
		
		if (rs232_read(A->fd, buf, data_len) != data_len) {
			log_msg("nread != data_len(%d)", data_len);
			return -1;
		}

		/* 根据PC值确定epc的长度,完整信息参考手册页P99/F.3 */
		int epc_length = (buf[0] >> 3) * 2;
		if (0x1D0F != crc_ccitt_buf(buf, epc_length + 4)) { 	/* 加上2字节PC和2字节CRC */
			log_msg("CRC check error");
			return -1;
		}

		if ((S->work_status == WS_READ_EPC_FIXED) ||
			(S->work_status == WS_READ_EPC_INTURN)) {
			tag_t tag;
			tag.ant_index = S->cur_ant;
			tag.tag_len = epc_length;
			memcpy(tag.data, buf+2, epc_length);		/* 跳过头2字节的PC */
			report_tag_send(C, S, A, &tag);
		} else if ((S->work_status == WS_READ_EPC_TID_FIXED) ||
			(S->work_status == WS_READ_EPC_TID_INTURN)) {
			memcpy(epc_tid.data+2, buf+2, epc_length);	/* 跳过头2字节的PC */
			epc_tid.data[0] = (epc_length) & 0xff;
			epc_tid.tag_len = epc_length;
		}
#if 0
		int i;
		for (i = 0; i < data_len - 4; i++)
			printf("%02X ", buf[2+i]);
		printf("\n");
#endif
		break;
	case RFID_PACKET_TYPE_18K6C_TAG_ACCESS:
		/* 注意:minihost的这里处理有误,何睿的程序是对的 */
		pkt_len = sizeof(access) - sizeof(access.data) - sizeof(*pcmn);
		if (rs232_read(A->fd, (uint8_t *)&access+sizeof(*pcmn), pkt_len) != pkt_len) {
			log_msg("invalid pkt_len");
			return -1;
		}

		if (RFID_18K6C_TAG_ACCESS_BACKSCATTER_ERROR(pcmn->flags)) {
			log_msg("Tag backscattered error 0x%.8x on access", 
				access.tag_error_code);
			return -1;
		} else if (RFID_18K6C_TAG_ACCESS_MAC_ERROR(pcmn->flags)) {
			log_msg("Protocol access error 0x%.8x on access", 
				access.prot_error_code);
			return -1;
		}

		switch (access.command) {
		case RFID_18K6C_READ: {
			int data_len = pcmn->pkt_len * BYTES_PER_LEN_UNIT + 
				sizeof(RFID_PACKET_COMMON) - 
				sizeof(RFID_PACKET_18K6C_TAG_ACCESS) +
				sizeof(access.data) - 
				RFID_18K6C_TAG_ACCESS_PADDING_BYTES(pcmn->flags);
			if (data_len) {
#if 1
				int fixed_data_len = (data_len % 4) ? data_len+2 : data_len;
				if (rs232_read(A->fd, buf, fixed_data_len) != fixed_data_len) {
					log_msg("invalid data_len");
					return -1;
				}
#else
				if (rs232_read(A->fd, buf, data_len) != data_len) {
					log_msg("invalid data_len");
					return -1;
				}
#endif				
				if ((S->work_status == WS_READ_TID_FIXED) || 
					(S->work_status == WS_READ_TID_INTURN)) {
					tag_t tag;
					tag.ant_index = S->cur_ant;
					tag.tag_len = data_len;
					memcpy(tag.data, buf, data_len);
					report_tag_send(C, S, A, &tag);
				} else if ((S->work_status == WS_READ_EPC_TID_FIXED) || 
					(S->work_status == WS_READ_EPC_TID_INTURN)) {
					epc_tid.ant_index = S->cur_ant;
					memcpy(epc_tid.data+2+epc_tid.tag_len, buf, data_len);
					epc_tid.data[1] = data_len & 0xff;
					epc_tid.tag_len += data_len;
					epc_tid.tag_len += 2;	/* 加上2字节的tag_len */
					report_tag_send(C, S, A, &epc_tid);
				} else if (S->work_status == WS_READ_USER) {
#if 0
					tag_t tag;
					tag.ant_index = S->cur_ant;
					tag.tag_len = data_len;
					memcpy(tag.data, buf, data_len);
					report_tag_send(C, S, A, &tag);
#else
					tag_user.ant_index = S->cur_ant;
					tag_user.tag_len = data_len;
					memcpy(tag_user.data, buf, data_len);
					read_user_done = 1;
					log_msg("read done");
#endif
				}
#if 0
				int i;
				for (i = 0; i < data_len; i++) {
					if (i % 10 == 0) {
						printf("\n");
					}
					printf("%02X ", buf[i]);
				}
				printf("\n");
#endif
			}
			break;
		}
		case RFID_18K6C_WRITE:
			log_msg("RFID_18K6C_WRITE");
			if (S->work_status == WS_WRITE_USER) {
				wlk_done = 1;
			} else {
				command_answer(C, C->recv.frame.cmd_id, CMD_EXE_SUCCESS, NULL, 0);
			}
			break;
		case RFID_18K6C_LOCK:
			log_msg("RFID_18K6C_LOCK");
			command_answer(C, C->recv.frame.cmd_id, CMD_EXE_SUCCESS, NULL, 0);
			break;
		case RFID_18K6C_KILL:
			log_msg("RFID_18K6C_KILL");
			command_answer(C, C->recv.frame.cmd_id, CMD_EXE_SUCCESS, NULL, 0);
			break;
		default:
			log_msg("invalid access.command");
			return -1;
		}
		
		break;
	case RFID_PACKET_TYPE_ANTENNA_CYCLE_END:
		log_msg("RFID_PACKET_TYPE_ANTENNA_CYCLE_END not implemented");
		break;
	case RFID_PACKET_TYPE_ANTENNA_END:
		log_msg("RFID_PACKET_TYPE_ANTENNA_END not implemented");
		break;
	case RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_END:
		log_msg("RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_END not implemented");
		break;
	case RFID_PACKET_TYPE_INVENTORY_CYCLE_BEGIN:
		log_msg("RFID_PACKET_TYPE_INVENTORY_CYCLE_BEGIN not implemented");
		break;
	case RFID_PACKET_TYPE_INVENTORY_CYCLE_END:
		log_msg("RFID_PACKET_TYPE_INVENTORY_CYCLE_END not implemented");
		break;
	case RFID_PACKET_TYPE_CARRIER_INFO:
		log_msg("RFID_PACKET_TYPE_CARRIER_INFO not implemented");
		break;
	case RFID_PACKET_TYPE_COMMAND_ACTIVE:
		log_msg("RFID_PACKET_TYPE_COMMAND_ACTIVE not implemented");
		break;
	default:
		log_msg("unknown packets type: pkt_type = %d", pcmn->pkt_type);
		return -1;
	}
	
	return 0;
}

int auto_read_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	switch (S->work_status) {
	case WS_READ_EPC_FIXED:
		set_active_antenna(S, S->cur_ant);
		r2000_set_ant_rfpower(S, A);
		write_mac_register(A, HST_CMD, CMD_18K6CINV);
		break;
	case WS_READ_EPC_INTURN:
		set_next_active_antenna(S);
		r2000_set_ant_rfpower(S, A);
		write_mac_register(A, HST_CMD, CMD_18K6CINV);
		break;
	case WS_READ_TID_FIXED: {
		tag_param_t *T = &S->tag_param;
		T->access_bank = RFID_18K6C_MEMORY_BANK_TID;
		T->access_offset = 0;
		T->access_wordnum = 4;
		bzero(T->access_pwd, sizeof(T->access_pwd));
		
		set_active_antenna(S, S->cur_ant);
		r2000_set_ant_rfpower(S, A);
		r2000_tag_read(T, A);
		break;
	}
	case WS_READ_TID_INTURN: {
		tag_param_t *T = &S->tag_param;
		T->access_bank = RFID_18K6C_MEMORY_BANK_TID;
		T->access_offset = 0;
		T->access_wordnum = 4;
		bzero(T->access_pwd, sizeof(T->access_pwd));

		set_next_active_antenna(S);
		r2000_set_ant_rfpower(S, A);
		r2000_tag_read(T, A);
		break;
	}
	case WS_READ_USER: {	/* 为山东淄博定制 */
		tag_param_t *T = &S->tag_param;
		T->access_bank = RFID_18K6C_MEMORY_BANK_USER;
		T->access_offset = 0;
		T->access_wordnum = 14;
		bzero(T->access_pwd, sizeof(T->access_pwd));

		set_active_antenna(S, S->cur_ant);
		r2000_set_ant_rfpower(S, A);
		r2000_tag_read(T, A);
		break;
	}
	default:
		log_msg("invalid work status");
		return -1;
	}
	
	return 0;
}

int r2000_set_freq(system_param_t *S, ap_connect_t *A)
{
	return 0;
}

int r2000_set_operate_mode(ap_connect_t *A, int mode)
{
	if (mode != RFID_RADIO_OPERATION_MODE_CONTINUOUS &&
		mode != RFID_RADIO_OPERATION_MODE_NONCONTINUOUS) {
		log_msg("invalid mode");
		return -1;
	}

	uint32_t reg_data = 
		HST_ANT_CYCLES_CYCLES(RFID_RADIO_OPERATION_MODE_CONTINUOUS == mode ?
		HST_ANT_CYCLES_CYCLES_INFINITE : 1) | HST_ANT_CYCLES_RFU1(0);
	return write_mac_register(A, HST_ANT_CYCLES, reg_data);
}

int r2000_set_ant_rfpower(system_param_t *S, ap_connect_t *A)
{
	//write_mac_register(A, HST_CMD, CMD_CWOFF);	/* 关闭载波 */
	write_mac_register(A, HST_ANT_DESC_SEL, 0x0);
	write_mac_register(A, HST_ANT_DESC_CFG, 0x1);
	write_mac_register(A, HST_ANT_DESC_PORTDEF, 0x0);

	/* 天线驻留时间 */
	if ((S->work_status == WS_READ_EPC_INTURN) ||
		(S->work_status == WS_READ_TID_INTURN) ||
		(S->work_status == WS_READ_EPC_TID_INTURN)) {
		write_mac_register(A, HST_ANT_DESC_DWELL, 
			S->ant_array[S->cur_ant-1].switch_time * 100);
	} else {
		write_mac_register(A, HST_ANT_DESC_DWELL, 2000);
	}

	/* 天线功率 */
	if ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4) {
		A->cur_ant_power = S->ant_array[0].rfpower;
	} else {
		A->cur_ant_power = S->ant_array[S->cur_ant-1].rfpower;
	}

	write_mac_register(A, HST_ANT_DESC_RFPOWER, RFPOWER_F806_TO_R2000(A->cur_ant_power));
	log_msg("rfpower = %d", A->cur_ant_power);
	return 0;
}

int r2000_get_ant_rfpower(ap_connect_t *A, uint32_t *rfpower)
{
	return read_mac_register(A, HST_ANT_DESC_RFPOWER, rfpower);
}

int r2000_tag_read(tag_param_t *T, ap_connect_t *A)
{
	//r2000_error_check(A);
	uint32_t pwd = (T->access_pwd[0] << 24) |
		(T->access_pwd[1] << 16) |
		(T->access_pwd[2] << 8) |
		(T->access_pwd[3]);
	if ((write_mac_register(A, HST_TAGACC_BANK, T->access_bank) < 0) || 
		(write_mac_register(A, HST_TAGACC_PTR, T->access_offset) < 0) ||
		(write_mac_register(A, HST_TAGACC_CNT, T->access_wordnum) < 0) ||
		(write_mac_register(A, HST_TAGACC_ACCPWD, pwd) < 0) ||
		(write_mac_register(A, HST_CMD, CMD_18K6CREAD) < 0)) {
		log_msg("r2000_tag_read error");
		return -1;
	}
#if 0
	log_msg("18K6C_READ: bank = %d, access_offset = %d, access_wordnum = %d", 
		T->access_bank, T->access_offset, T->access_wordnum);
#endif
	return 0;
}

/*
 * 功能: 写EPC,用户数据区,设置access密码,设置kill密码
 * 要求: 在调用前已将S->tag_param设置好
 */
int r2000_tag_write(tag_param_t *T, ap_connect_t *A)
{
#if 0
	log_msg("18K6C_WRITE: bank = %d, access_offset = %d, access_wordnum = %d", 
		T->access_bank, T->access_offset, T->access_wordnum);
#endif
	//r2000_error_check(A);
#if 0
	uint32_t reg_data;
	if (read_mac_register(A, HST_TAGACC_DESC_CFG, &reg_data) < 0)
		return -1;

	HST_TAGACC_DESC_CFG_SET_VERIFY_ENABLED(reg_data);
	write_mac_register(A, HST_TAGACC_DESC_CFG, reg_data);
#else
	write_mac_register(A, HST_TAGACC_DESC_CFG, 6);
#endif

	write_mac_register(A, HST_TAGACC_BANK, T->access_bank);
	write_mac_register(A, HST_TAGACC_PTR, T->access_offset);
	write_mac_register(A, HST_TAGACC_CNT, 
		HST_TAGACC_CNT_LEN(T->access_wordnum) | HST_TAGACC_CNT_RFU1(0));

	uint32_t count1 = T->access_wordnum;
	uint32_t reg_bank, count2 = 0;	/* 写入寄存器的偏移地址，不断累加 */
	for (reg_bank = 0; count1; ++reg_bank) {
		write_mac_register(A, HST_TAGWRDAT_SEL, 
			HST_TAGWRDAT_SEL_SELECTOR(reg_bank) | HST_TAGWRDAT_SEL_RFU1(0));
		uint16_t reg_addr = HST_TAGWRDAT_0;
		uint32_t i = 0, j = 0, offset = 0;
		while (count1 > 0 && offset < 16) {
			uint32_t tmp = (T->access_databuf[j+reg_bank*32] << 8) |
				(T->access_databuf[j+1+reg_bank*32]);
			uint32_t reg_val = HST_TAGWRDAT_N_DATA(tmp) | HST_TAGWRDAT_N_OFF(count2);
			write_mac_register(A, reg_addr, reg_val);
			i++;
			j+=2;
			reg_addr++;
			offset++;
			count2++;
			count1--;
		}
	}

	uint32_t pwd = (T->access_pwd[0] << 24) |
		(T->access_pwd[1] << 16) |
		(T->access_pwd[2] << 8) |
		(T->access_pwd[3]);
	write_mac_register(A, HST_TAGACC_ACCPWD, pwd);
	write_mac_register(A, HST_CMD, CMD_18K6CWRITE);

	return 0;
}

int r2000_tag_lock(r2h_connect_t *C, tag_param_t *T, ap_connect_t *A)
{
	uint32_t mask, action;
	
	switch (T->lock_bank) {
	case LOCKSET_ALL_BANK:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x2aa;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x2aa;
			action = 0x2aa;			
		}
		break;
	case LOCKSET_TID_BANK:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x8;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x8;
			action = 0x8; 		
		}
		break;
	case LOCKSET_EPC_BANK:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x20;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x20;
			action = 0x20; 		
		}
		break;
	case LOCKSET_USER_BANK:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x2;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x2;
			action = 0x2; 		
		}
		break;
	case LOCKSET_ACCESS_PIN:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x80;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x80;
			action = 0x80; 		
		}
		break;
	case LOCKSET_KILL_PIN:
		if (T->lock_type) {
			/* 解锁 */
			mask = 0x200;
			action = 0x0;
		} else {
			/* 锁 */			
			mask = 0x200;
			action = 0x200; 		
		}
		break;
	default:
		command_answer(C, COMMAND_18K6C_MAN_LOCK_OPERATION, ERRCODE_CMD_PARAM, NULL, 0);
		return -1;
	}

	uint32_t pwd = (T->access_pwd[0] << 24) |
		(T->access_pwd[1] << 16) |
		(T->access_pwd[2] << 8) |
		(T->access_pwd[3]);
	write_mac_register(A, HST_TAGACC_ACCPWD, pwd);

	uint32_t lock_cfg = action | (mask << 10);
	write_mac_register(A, HST_TAGACC_LOCKCFG, lock_cfg);

	/* Issue the lock command */
	write_mac_register(A, HST_CMD, CMD_18K6CLOCK);
	
	return 0;
}

int r2000_tag_kill(tag_param_t *T, ap_connect_t *A)
{
	uint32_t pwd = (T->access_pwd[0] << 24) |
		(T->access_pwd[1] << 16) |
		(T->access_pwd[2] << 8) |
		(T->access_pwd[3]);
	write_mac_register(A, HST_TAGACC_ACCPWD, pwd);

	pwd = (T->kill_pwd[0] << 24) |
		(T->kill_pwd[1] << 16) |
		(T->kill_pwd[2] << 8) |
		(T->kill_pwd[3]);
	write_mac_register(A, HST_TAGACC_KILLPWD, pwd);
	write_mac_register(A, HST_CMD, CMD_18K6CKILL);

	return 0;
}

int r2000_tag_select(select_param_t *param, ap_connect_t *A)
{
	/* 标签分组 */
	write_mac_register(A, HST_QUERY_CFG, 0x180);
	write_mac_register(A, HST_INV_CFG, 0x4001);

	/* 标签选择 */
	write_mac_register(A, HST_TAGMSK_DESC_CFG, 0x9);
	write_mac_register(A, HST_TAGMSK_BANK, param->bank);
	write_mac_register(A, HST_TAGMSK_PTR, param->offset);

	int i, count = (param->count % 32) == 0 ? param->count : param->count+32;
	for (i = 0; i < count/32; i++) {
		uint32_t reg_val = param->mask[i*4] |
			(param->mask[i*4+1]<<8) |
			(param->mask[i*4+2]<<16) |
			(param->mask[i*4+3]<<24);
		write_mac_register(A, HST_TAGMSK_0_3+i, reg_val);
	}
	write_mac_register(A, HST_TAGMSK_LEN, param->count);

	return 0;
}

int r2000_tag_deselect(ap_connect_t *A)
{
	write_mac_register(A, HST_QUERY_CFG, 0xC0);
	write_mac_register(A, HST_INV_CFG, 0x1);
	write_mac_register(A, HST_TAGMSK_DESC_CFG, HST_TAGMSK_DESC_CFG_DISABLED);
	return 0;
}

uint32_t r2000_oem_read(ap_connect_t *A, uint16_t addr)
{
	uint8_t buf[256] = {0};
	write_mac_register(A, HST_OEM_ADDR, addr);
	write_mac_register(A, HST_CMD, CMD_RDOEM);
	rs232_read(A->fd, buf, 48);
	
	return buf[28] + (buf[29] << 8) + (buf[30] << 16) + (buf[31] << 24);	
}

#define FREQ_STD_GB	1
#define FREQ_STD_FCC	2
#define FREQ_STD_CE	3

#define FREQ_DIV_GB	0x30
#define FREQ_DIV_FCC	0x18
#define FREQ_DIV_CE	0x3C

int r2000_check_freq_std(system_param_t *S, ap_connect_t *A)
{
	uint32_t plldivmult;
	if (read_mac_register(A, HST_RFTC_FRQCH_DESC_PLLDIVMULT, &plldivmult) < 0) {		
		log_msg("%s: read_mac_register error", __FUNCTION__);
		return -1;
	}

	uint8_t div = (plldivmult & 0x00FF0000) >> 16;
	log_msg("div = 0x%02X", div);
	if (div == FREQ_DIV_GB) {
		/* GB */
		S->freq_std = FREQ_STD_GB;		
	} else if (div == FREQ_DIV_FCC) {
		/* FCC */
		S->freq_std = FREQ_STD_FCC;	
	} else if (div == FREQ_DIV_CE) {
		/* CE */
		S->freq_std = FREQ_STD_CE;
	} else {
		log_msg("unkown frequency standard, set freq_std to GB");
		S->freq_std = FREQ_STD_GB;
	}

	return 0;
}

int r2000_freq_config(system_param_t *S, ap_connect_t *A)
{
	int i;
	uint8_t buf[256] = {0};

	/* 至少得配置一个频点 */
	if (S->freq_table[0] == 0xFF) {
		return -1;
	}

	write_mac_register(A, HST_CMNDIAGS, 0x210);

	/* OEMCFGADDR_HW_OPTIONS0 - 0x9F */
	write_mac_register(A, HST_OEM_ADDR, 0x9F);
	write_mac_register(A, HST_OEM_DATA, 0x01111000);
	write_mac_register(A, HST_CMD, CMD_WROEM);
	rs232_read(A->fd, buf, 32);

	/* OEMCFGADDR_AUTO_LOW_POWER_CMD_0 - 0x03DB */
	write_mac_register(A, HST_OEM_ADDR, 0x03DB);
	write_mac_register(A, HST_OEM_DATA, 0x0);
	write_mac_register(A, HST_CMD, CMD_WROEM);
	rs232_read(A->fd, buf, 32);

	/* OEMCFGADDR_AUTO_LOW_POWER_CMD_1 - 0x03DC */
	write_mac_register(A, HST_OEM_ADDR, 0x03DC);
	write_mac_register(A, HST_OEM_DATA, 0x0);
	write_mac_register(A, HST_CMD, CMD_WROEM);
	rs232_read(A->fd, buf, 32);

	rs232_flush(A->fd);

	/* 关闭1  通道之外的其他通道 */
	for (i = 0; i < 50; i++) {
		/* OEMCFGADDR_FREQCFG_CHAN_INFO - 0xBC */
		write_mac_register(A, HST_OEM_ADDR, 0xBC + i*3);
		write_mac_register(A, HST_OEM_DATA, 0x1);	/* 关闭 */
		write_mac_register(A, HST_CMD, CMD_WROEM);
		rs232_read(A->fd, buf, 32);
	}

	uint32_t plldivmult_start = 0;
	uint32_t plldacctl = 0;
	switch (S->freq_std) {
	case FREQ_STD_GB:
		plldivmult_start = 0x00301CC5;
		plldacctl = 0x14070100;
		break;
	case FREQ_STD_FCC:
		plldivmult_start = 0x00180E1B;
		plldacctl = 0x14070100;
		break;
	case FREQ_STD_CE:
		plldivmult_start = 0x003C21CA;
		plldacctl = 0x14070200;
		break;
	default:
		log_msg("unkown frequency standard");
		return -1;
	}

	/* 按照跳频表设置 RFID 模块的频率 */
	for(i = 0; i < 50 && S->freq_table[i] != 0xFF; i++) {
		/* OEMCFGADDR_FREQCFG_CHAN_INFO - 0xBC */
		write_mac_register(A, HST_OEM_ADDR, 0xBC + i*3);
		write_mac_register(A, HST_OEM_DATA, 0x3);	/* 打开 */
		write_mac_register(A, HST_CMD, CMD_WROEM);
		rs232_read(A->fd, buf, 32);
		log_msg("0x%02X = %08X", 0xBC + i*3, r2000_oem_read(A, 0xBC + i*3));

		/* OEMCFGADDR_FREQCFG_PLLDIVMULT - 0xBD */
		uint32_t plldivmult = plldivmult_start + S->freq_table[i]*2;
		write_mac_register(A, HST_OEM_ADDR, 0xBD + i*3);
		write_mac_register(A, HST_OEM_DATA, plldivmult);
		write_mac_register(A, HST_CMD, CMD_WROEM);
		rs232_read(A->fd, buf, 32);
		log_msg("0x%02X = %08X", 0xBD + i*3, r2000_oem_read(A, 0xBD + i*3));

		/* OEMCFGADDR_FREQCFG_PLLDACCTL - 0xBE */
		write_mac_register(A, HST_OEM_ADDR, 0xBE + i*3);
		write_mac_register(A, HST_OEM_DATA, plldacctl);
		write_mac_register(A, HST_CMD, CMD_WROEM);
		rs232_read(A->fd, buf, 32);
		log_msg("0x%02X = %08X", 0xBE + i*3, r2000_oem_read(A, 0xBE + i*3));
	}

	r2000_control_command(A, R2000_SOFTRESET);
	return 0;
}


/* 触发读卡功能 */
int trigger_to_read_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	unsigned char key_vals[2];
	if(S->pre_cfg.work_mode != WORK_MODE_TRIGGER){
		return -1;
	}
	if(read(S->gpio_dece.fd, key_vals, sizeof(key_vals)) < 0){
		log_ret("trigger_to_read_tag read()\n");
		return -1;
	}
	S->gpio_dece.gpio1_val = key_vals[0];
	S->gpio_dece.gpio2_val = key_vals[1];
	report_triggerstatus(C,S);//上传状态
	C->status_cnt = 6;
	if((key_vals[0]==1) || (key_vals[1]==1)){//接入设备
		if(S->work_status == WS_STOP)
		{
			switch (S->pre_cfg.oper_mode) {
			case OPERATE_READ_EPC:
				if (S->pre_cfg.ant_idx >= 1 && S->pre_cfg.ant_idx <= 4) {
					S->work_status = WS_READ_EPC_FIXED;
					S->cur_ant = S->pre_cfg.ant_idx;
				} else {
					S->work_status = WS_READ_EPC_INTURN;
				}
				break;
			case OPERATE_READ_TID:
				if (S->pre_cfg.ant_idx >= 1 && S->pre_cfg.ant_idx <= 4) {
					S->work_status = WS_READ_TID_FIXED;
					S->cur_ant = S->pre_cfg.ant_idx;
				} else {
					S->work_status = WS_READ_TID_INTURN;
				}			
				break;
			case OPERATE_READ_USER:
				/* 读用户区不支持轮询模式 */
				if (S->pre_cfg.ant_idx != 0) {
					S->cur_ant = S->pre_cfg.ant_idx;
				} else {
					S->cur_ant = 1;
				}
				S->work_status = WS_READ_USER;
				break;
			default:
				S->work_status = WS_STOP;
				log_msg("invalid operate mode");
			}
			if(r2000_error_check(C, S, A)<0)
				log_msg("start error");
			auto_read_tag(C, S, A);
		}
	}else{//拔出设备
		if(S->work_status != WS_STOP){
			stop_read_tag(S, A);
			S->work_status = WS_STOP;
		}
	}
	return 0;
}





