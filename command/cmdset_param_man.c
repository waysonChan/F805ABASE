#include "command.h"
#include "command_manager.h"
#include "command_def.h"
#include "parameter.h"
#include "errcode.h"
#include "rf_ctrl.h"
#include "gpio.h"
#include "report_tag.h"
#include "cfg_file.h"
#include "utility.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

/*---------------------------------------------------------------------
 *	指令集:参数表操作
 *--------------------------------------------------------------------*/
static command_set_t cmdset_param_man = {
	.set_type = COMMAND_PARAMETER_MAN_BASE,
	.cmd_head  = NULL,
	.next = NULL,
};

#define PARATABLE_RESTORE	0x0
#define PARATABLE_IMPORT	0x1
#define PARATABLE_EXPORT	0x2

#define DSC_IP_LEN		4
#define DSC_TCP_PORT_LEN	2
/*---------------------------------------------------------------------
 *	指令 0x10:参数表操作指令
 *--------------------------------------------------------------------*/
static void ec_param_table_man(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t addr = *(cmd_param+1);
	uint8_t len = *(cmd_param+2);

	log_msg("type = %d, addr = %d, len = %d", *cmd_param, addr, len);

	switch (*cmd_param) {
	case PARATABLE_RESTORE:
		log_msg("coping");
		system("cp /f806/f806.cfg.bk /f806/f806.cfg");
		break;
	case PARATABLE_IMPORT: {
#if 0
		if (len != 1) {
			err = ERRCODE_CMD_ERRTYPE;
			goto out;
		}
#endif
		uint8_t im_val = *(cmd_param+3);
		switch (addr) {
		case 0:		/* work mode */
			if (im_val == WORK_MODE_COMMAND 
				|| im_val == WORK_MODE_AUTOMATIC
				|| im_val == WORK_MODE_TRIGGER) {
				S->pre_cfg.work_mode = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 1:		/* device type */
			if ((im_val >= 0x00 && im_val <= 0x02)
				|| (im_val >= 0x10 && im_val <= 0x12)
				|| (im_val >= 0x20 && im_val <= 0x22)
				) {
				S->pre_cfg.dev_type = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 122:	/* dsc apn */
			if (len > DSC_APN_LEN) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			
			memcpy(S->data_center.apn, cmd_param+3, len);
			cfg_set_data_center(&S->data_center);
			set_gprs_apn(S->data_center.apn);
			break;
		case 123:	/* dsc username */
			if (len > DSC_USERNAME_LEN) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			
			memcpy(S->data_center.username, cmd_param+3, len);
			cfg_set_data_center(&S->data_center);
			break;
		case 124:	/* dsc passwd */
			if (len > DSC_PASSWD_LEN) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			
			memcpy(S->data_center.passwd, cmd_param+3, len);
			cfg_set_data_center(&S->data_center);
			break;
		case 159:	/* serial port speed */
			S->rs232.baud_rate = im_val;
			cfg_set_rs232(&S->rs232, CFG_BAUD_RATE);
			break;
		case 160:	/* weigand start */
			S->pre_cfg.wg_start = im_val;
			cfg_set_pre_cfg(&S->pre_cfg);
			break;
		case 161:	/* weigand length */
			if (im_val == WG_LEN_26
				|| im_val == WG_LEN_34) {
				S->pre_cfg.wg_len = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 162:	/* tid length */
			if (im_val == TID_LEN_4
				|| im_val == TID_LEN_6
				|| im_val == TID_LEN_8) {
				S->pre_cfg.tid_len = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 220:	/* pulse width */
			if (im_val < 1 || im_val > 20) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			} else {
				S->pre_cfg.wg_pulse_width = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			}
			break;
		case 221:	/* pulse periods */
			if (im_val < 10 || im_val > 200) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			} else {
				S->pre_cfg.wg_pulse_periods = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			}
			break;
		case 228:   /* extended_table */
			if (len < 0 || len > 10) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			memcpy(S->extended_table, &im_val, len);
			cfg_set_extended_table(S->extended_table,len);
			break;
		case 239:	/* operation type */
			if (im_val == OPERATE_READ_EPC
				|| im_val == OPERATE_READ_TID
				|| im_val == OPERATE_READ_USER) {
				S->pre_cfg.oper_mode = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 240:	/* antenna index */
			 if (im_val == ANT_IDX_POLL) {
					 S->pre_cfg.ant_idx = im_val;
					 cfg_set_pre_cfg(&S->pre_cfg);
			 } else if(im_val == ANT_IDX_1
					 || im_val == ANT_IDX_2
					 || im_val == ANT_IDX_3
					 || im_val == ANT_IDX_4){
						 if(S->ant_array[im_val-1].enable){
							 S->pre_cfg.ant_idx = im_val;
							 cfg_set_pre_cfg(&S->pre_cfg);
						 } else {
							 err = ERRCODE_CMD_ERRTYPE;
						 }
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 241:	/* communication port */
			if (im_val == UPLOAD_MODE_NONE
				|| im_val == UPLOAD_MODE_RS232
				|| im_val == UPLOAD_MODE_RS485
				|| im_val == UPLOAD_MODE_WIEGAND
				|| im_val == UPLOAD_MODE_WIFI
				|| im_val == UPLOAD_MODE_GPRS
				|| im_val == UPLOAD_MODE_TCP
				|| im_val == UPLOAD_MODE_UDP) {
				S->pre_cfg.upload_mode = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 249:	/* flash enable */
			if (im_val == NAND_FLASH_ENBABLE
				|| im_val == NAND_FLASH_DISABLE) {
				S->pre_cfg.flash_enable = im_val;
				cfg_set_pre_cfg(&S->pre_cfg);
			} else {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}
			break;
		case 250: 	/* dsc ip */
			if (len != DSC_IP_LEN) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}

			sp_set_dsc_ip(S, cmd_param+3);
			break;
		case 254:	/* dsc tcp port */
			if (len != DSC_TCP_PORT_LEN) {
				err = ERRCODE_CMD_ERRTYPE;
				goto out;
			}

			S->data_center.tcp_port = (*(cmd_param+3) << 8) + *(cmd_param+4);
			cfg_set_data_center(&S->data_center);
			break;
		default:
			err = ERRCODE_CMD_ERRTYPE;
			goto out;
		}

		break;
	}
	case PARATABLE_EXPORT: {
#if 0
		if (len != 1) {
			err = ERRCODE_CMD_ERRTYPE;
			goto out;
		}
#endif
		uint8_t ex_val;
		switch (addr) {
		case 0:		/* work mode */
			ex_val = S->pre_cfg.work_mode;
			break;
		case 1:		/* device type */
			ex_val = S->pre_cfg.dev_type;
			break;
		case 122:	/* dsc apn */
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				S->data_center.apn, DSC_APN_LEN);
			return;
		case 123:	/* dsc username */
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				S->data_center.username, DSC_USERNAME_LEN);
			return;
		case 124:	/* dsc passwd */
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				S->data_center.passwd, DSC_PASSWD_LEN);
			return;
		case 159:	/* serial port speed */
			ex_val = S->rs232.baud_rate;
			break;
		case 160:	/* weigand start */
			ex_val = S->pre_cfg.wg_start;
			break;
		case 161:	/* weigand length */
			ex_val = S->pre_cfg.wg_len;
			break;
		case 162:	/* tid length */
			ex_val = S->pre_cfg.tid_len;
			break;
		case 220:	/* pulse width */
			ex_val = S->pre_cfg.wg_pulse_width;
			break;
		case 221:	/* pulse periods */
			ex_val = S->pre_cfg.wg_pulse_periods;
			break;
		case 228:  {/* get_extended_table */
			uint8_t get_extended_table[10];
			cfg_get_extended_table(get_extended_table,10);
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				get_extended_table, 10);
			return;
			}
		case 239:	/* operation type */
			ex_val = S->pre_cfg.oper_mode;
			break;
		case 240:	/* antenna index */
			ex_val = S->pre_cfg.ant_idx;			
			break;
		case 241:	/* communication port */
			ex_val = S->pre_cfg.upload_mode;
			break;
		case 249:	/* flash enable */
			ex_val = S->pre_cfg.flash_enable;
			break;
		case 250: {	/* dsc ip */
			uint8_t dsc_ip[DSC_IP_LEN] = {0};
			sp_get_dsc_ip(S, dsc_ip);
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				dsc_ip, sizeof(dsc_ip));
			return;
		}
		case 254: {	/* dsc tcp port */
			uint16_t tcp_port = swap16(S->data_center.tcp_port);
			command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, CMD_EXE_SUCCESS, 
				&tcp_port, DSC_TCP_PORT_LEN);
			return;
		}
		default:
			err = ERRCODE_CMD_ERRTYPE;
			goto out;
		}

		command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, err, &ex_val, 1);
		return;
	}
	default:
		err = ERRCODE_CMD_ERRTYPE;
	}

out:
	if (err != CMD_EXE_SUCCESS) {
		log_msg("ec_param_table_man: invalid parameter.");
	}

	command_answer(C, COMMAND_PARAMETER_MAN_PARATABLE, err, NULL, 0);
}

static command_t cmd_param_table_man = {
	.cmd_id = COMMAND_PARAMETER_MAN_PARATABLE,
	.execute = ec_param_table_man,
};

/*---------------------------------------------------------------------
 *	指令 0x11:天线端口参数配置指令
 * 
 * 注意:cmd_param 在帧处理时已经跳过指令字
 * 00 00 00 01
 * 00 00 00 0a
 * 00 00 00 14
 *--------------------------------------------------------------------*/
static void ec_ante_param_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	uint8_t param_len = C->recv.frame.param_len;
	if (param_len != 0x0d) {
		log_msg("param_len != 0x0d!");
		return;
	}

	/* 注意: 此处仅更新参数表, 实际的天线配置操作不在这里进行 */
	int i;
	for (i = 0; i < ANTENNA_NUM; i++) {
		S->ant_array[i].enable = cmd_param[i];
		if (cmd_param[i]) {
			set_antenna_led_status(i+1, LED_COLOR_GREEN, S->pre_cfg.dev_type);
		} else {
			set_antenna_led_status(i+1, LED_COLOR_NONE, S->pre_cfg.dev_type);
		}
		S->ant_array[i].rfpower = cmd_param[i+4];
		S->ant_array[i].switch_time = cmd_param[i+8];
		cfg_set_ant(&S->ant_array[i], i+1);
	}

	command_answer(C, COMMAND_PARAMETER_MAN_RF_CFG, CMD_EXE_SUCCESS, NULL, 0);
}

static command_t cmd_ante_param_config = {
	.cmd_id = COMMAND_PARAMETER_MAN_RF_CFG,
	.execute = ec_ante_param_config,
};

/*---------------------------------------------------------------------
 *	指令 0x12:天线端口参数查询指令
 *--------------------------------------------------------------------*/
static void ec_ante_param_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	int i;
	uint8_t ante_param[ANTENNA_NUM * sizeof(antenna_t)];

	for (i = 0; i < ANTENNA_NUM; i++) {
		ante_param[i] = S->ant_array[i].enable;
		ante_param[i+4] = S->ant_array[i].rfpower;
		ante_param[i+8] = S->ant_array[i].switch_time;
	}
	
	command_answer(C, COMMAND_PARAMETER_MAN_RF_QUERY, CMD_EXE_SUCCESS, 
		ante_param, ANTENNA_NUM * 3);
}

static command_t cmd_ante_param_query = {
	.cmd_id = COMMAND_PARAMETER_MAN_RF_QUERY,
	.execute = ec_ante_param_query,
};

/*---------------------------------------------------------------------
 *	指令 0x13:载波参数配置指令
 *--------------------------------------------------------------------*/
static void ec_freq_table_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (*(cmd_param+1) > FREQTAB_LEN || *(cmd_param+1) == 0) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	if ((*(cmd_param+1) + 3) != C->recv.frame.param_len) {
		err = ERRCODE_CMD_FRAME;
		goto out;
	}

	int i;
	for (i = 0; i < *(cmd_param+1); i++) {
		if (*(cmd_param+2+i) > FREQSCAN_END) {
			err = ERRCODE_CMD_PARAM;
			goto out;
		}
	}

	memset(S->freq_table, 0xFF, FREQ_MAP_LEN);
	memcpy(S->freq_table, cmd_param+2, *(cmd_param+1));
	S->pre_cfg.hop_freq_enable = 1;
	cfg_set_pre_cfg(&S->pre_cfg);
	cfg_set_freq_table(S->freq_table, FREQ_MAP_LEN);

	/* 先回成功，不然上位机超时 */
	command_answer(C, COMMAND_PARAMETER_MAN_CARRIER_CFG, err, NULL, 0);
	r2000_freq_config(S, A);
	return;

out:
	command_answer(C, COMMAND_PARAMETER_MAN_CARRIER_CFG, err, NULL, 0);
}

static command_t cmd_freq_table_config = {
	.cmd_id = COMMAND_PARAMETER_MAN_CARRIER_CFG,
	.execute = ec_freq_table_config,
};
/*---------------------------------------------------------------------
 *	指令 0x14:载波参数查询指令
 *--------------------------------------------------------------------*/
static void ec_freq_table_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	if (C->recv.frame.param_len != 2) {
		command_answer(C, COMMAND_PARAMETER_MAN_CARRIER_QUERY, ERRCODE_CMD_FRAME, NULL, 0);
		return;
	}

	int i = 0;
	while (S->freq_table[i] != 0xFF && i <= FREQSCAN_END) {
		i++;
	}
	command_answer(C, COMMAND_PARAMETER_MAN_CARRIER_QUERY, CMD_EXE_SUCCESS, S->freq_table, i);	
}

static command_t cmd_freq_table_query = {
	.cmd_id = COMMAND_PARAMETER_MAN_CARRIER_QUERY,
	.execute = ec_freq_table_query,
};
/*---------------------------------------------------------------------
 *	指令 0x15:通信参数配置指令
 *--------------------------------------------------------------------*/
static void ec_comm_param_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case COMM_ADDR:
		if (!sp_set_bus_addr(S, *(cmd_param+1))) {
			err = CMD_EXE_SUCCESS;
		} else {
			err = ERRCODE_CMD_PARAM;
		}
		
		break;
	case COMM_RS232_BAUD:
	case COMM_RS485_BAUD:
		sp_set_baud_rate(S, *(cmd_param+1));
		err = CMD_EXE_SUCCESS;
		break;
	case WIEGAND_LEN:
		S->pre_cfg.wg_len = *(cmd_param + 1);
		cfg_set_pre_cfg(&S->pre_cfg);
		break;
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	command_answer(C, COMMAND_PARAMETER_MAN_COMM_CFG, err, NULL, 0);
}

static command_t cmd_comm_param_config = {
	.cmd_id = COMMAND_PARAMETER_MAN_COMM_CFG,
	.execute = ec_comm_param_config,
};
/*---------------------------------------------------------------------
 *	指令 0x16:通信参数查询指令
 *--------------------------------------------------------------------*/
static void ec_comm_param_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS, val;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	
#if 0
	int i;
	int len = get_cmd_param_len();
	for (i = 0; i < len; i++) {
		log_msg("%02X ", cmd_param[i]);
	}
	log_msg("\n");
#endif

	switch (*cmd_param) {
	case COMM_ADDR:
		val = S->rs232.bus_addr;
		command_answer(C, COMMAND_PARAMETER_MAN_COMM_QUERY, CMD_EXE_SUCCESS, 
					&val, SERIAL_BUSADDR_LEN);
		return;
	case COMM_RS232_BAUD:
	case COMM_RS485_BAUD:
		val = S->rs232.baud_rate;
		command_answer(C, COMMAND_PARAMETER_MAN_COMM_QUERY, CMD_EXE_SUCCESS, 
					&val, UART_BAUDRATE_LEN);
		return;
	case WIEGAND_LEN:
		val = S->pre_cfg.wg_len;
		command_answer(C, COMMAND_PARAMETER_MAN_COMM_QUERY, CMD_EXE_SUCCESS, 
					&val, UART_BAUDRATE_LEN);
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	if (err != ERRCODE_CMD_ERRTYPE) {
		command_answer(C, COMMAND_PARAMETER_MAN_COMM_QUERY, err, NULL, 0);
	}
}

static command_t cmd_comm_param_query = {
	.cmd_id = COMMAND_PARAMETER_MAN_COMM_QUERY,
	.execute = ec_comm_param_query,
};

/*---------------------------------------------------------------------
 *	指令 0x17:网络参数配置指令
 *--------------------------------------------------------------------*/
#define MAC_ADDR_PARAM_LEN		8
static void ec_ether_param_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint16_t port;
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case ETHER_MAC_ADDR:
		if (C->recv.frame.param_len != MAC_ADDR_PARAM_LEN) {
			err = ERRCODE_CMD_FRAME;
			goto out;
		}

		sp_set_mac_addr(S, cmd_param+1);
		err = CMD_EXE_SUCCESS;
		break;
	case ETHER_IP_ADDR:
		if (C->recv.frame.param_len != IP_CONFG_LEN+2) {
			err = ERRCODE_CMD_FRAME;
			goto out;
		}

		/*检查配置参数是否为全零或全FF*/
		if(((*(cmd_param+1)|*(cmd_param+2)|*(cmd_param+3)|*(cmd_param+4))==0x00)
			||((*(cmd_param+1)&*(cmd_param+2)&*(cmd_param+3)&*(cmd_param+4))==0xff)
			||((*(cmd_param+5)|*(cmd_param+6)|*(cmd_param+7)|*(cmd_param+8))==0x00)
			||((*(cmd_param+5)&*(cmd_param+6)&*(cmd_param+7)&*(cmd_param+8))==0xff)
			||((*(cmd_param+9)|*(cmd_param+10)|*(cmd_param+11)|*(cmd_param+12))==0x00)
			||((*(cmd_param+9)&*(cmd_param+10)&*(cmd_param+11)&*(cmd_param+12))==0xff)
			) {
			err = ERRCODE_CMD_FRAME;
			goto out;
		}

		/*检查配置参数是否为非法IP地址（主机为0或255）*/
		if (*(cmd_param+4) == 0x00 || *(cmd_param+4) == 0xFF) {
			err = ERRCODE_CMD_FRAME;
			goto out;
		}

		sp_set_ip_config(S, cmd_param+1);
		err = CMD_EXE_SUCCESS;			
		break;
	case ETHER_SOCKET_TCP_PORT:
		port = *(cmd_param+2) + (*(cmd_param+1) << 8);
		sp_set_tcp_port(S, port);
		err = CMD_EXE_SUCCESS;
		break;
	case ETHER_SOCKET_UDP_PORT:
		port = *(cmd_param+2) + (*(cmd_param+1) << 8);
		sp_set_udp_port(S, port);
		err = CMD_EXE_SUCCESS;
		break;
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

out:
	command_answer(C, COMMAND_PARAMETER_MAN_ETHER_CFG, err, NULL, 0);
}

static command_t cmd_ether_param_config = {
	.cmd_id = COMMAND_PARAMETER_MAN_ETHER_CFG,
	.execute = ec_ether_param_config,
};

/*---------------------------------------------------------------------
 *	指令 0x18:网络参数查询指令
 *--------------------------------------------------------------------*/
static void ec_ether_param_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint16_t sock_port;
	uint8_t mac[6], ip[12];
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	
	switch (*cmd_param) {
	case ETHER_MAC_ADDR:
		sp_get_mac_addr(S, mac);
		command_answer(C, COMMAND_PARAMETER_MAN_ETHER_QUERY, CMD_EXE_SUCCESS, 
			mac, MAC_ADDR_LEN);
		return;
	case ETHER_IP_ADDR:
		sp_get_ip_config(S, ip);
		command_answer(C, COMMAND_PARAMETER_MAN_ETHER_QUERY, CMD_EXE_SUCCESS, 
			ip, IP_CONFIG_LEN);
		return;
	case ETHER_SOCKET_TCP_PORT:
		sock_port = S->eth0.tcp_port;
		sock_port = htons(sock_port);
		command_answer(C, COMMAND_PARAMETER_MAN_ETHER_QUERY, CMD_EXE_SUCCESS, 
			(uint8_t *)&sock_port, SOCKET_NUM_LEN);
		return;
	case ETHER_SOCKET_UDP_PORT:
		sock_port = S->eth0.udp_port;
		sock_port = htons(sock_port);
		command_answer(C, COMMAND_PARAMETER_MAN_ETHER_QUERY, CMD_EXE_SUCCESS, 
			(uint8_t *)&sock_port, SOCKET_NUM_LEN);
		return;
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	if (err != CMD_EXE_SUCCESS) {
		command_answer(C, COMMAND_PARAMETER_MAN_ETHER_QUERY, err, NULL, 0);
	}
}

static command_t cmd_ether_param_query = {
	.cmd_id = COMMAND_PARAMETER_MAN_ETHER_QUERY,
	.execute = ec_ether_param_query,
};

/*---------------------------------------------------------------------
 *	指令 0x19:标签参数配置指令
 *--------------------------------------------------------------------*/
#include <stdio.h>
static void ec_tag_param_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case TAG_FILTER_ENABLE:
		A->tag_report.filter_enable = *(cmd_param+1);
		cfg_set_filter_enable(A->tag_report.filter_enable);
		log_msg("filter_enable = %d", A->tag_report.filter_enable);
		break;
	case TAG_FILTER_TIME:
		A->tag_report.filter_time = *(cmd_param+1);
		report_tag_set_timer(A, A->tag_report.filter_time * 100);
		cfg_set_filter_time(A->tag_report.filter_time);
		break;
	case TAG_Q_VALUE:
		S->tag_param.q_val = *(cmd_param+1);
		break;
	case TAG_SELECT_PARAM:
		if ((*(cmd_param+1) != RFID_18K6C_MEMORY_BANK_EPC) &&
			(*(cmd_param+1) != RFID_18K6C_MEMORY_BANK_TID) &&
			(*(cmd_param+1) != RFID_18K6C_MEMORY_BANK_USER)) {
			err = ERRCODE_CMD_FRAME;
			goto out;
		}

		select_param_t *param = &A->select_param;
		param->bank = *(cmd_param+1);
		param->offset = (*(cmd_param + 2)<<8) | (*(cmd_param + 3));
		if (param->offset > 0x3FFF) {
			err = ERRCODE_CMD_PARAM;
			goto out;
		}
		
		param->count = *(cmd_param + 4);
		log_msg("bank = %d; offset = %d; count = %d", param->bank, 
			param->offset, param->count);
		
		if (param->bank == RFID_18K6C_MEMORY_BANK_EPC) {
			param->offset += 32;
		}
		
		int nbyte = (param->count % 8) == 0 ? param->count/8 : param->count/8+1;
		memcpy(param->mask, cmd_param + 5, nbyte);
		
		r2000_tag_select(param, A);
		write_select_param(param);
		break;
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

out:
	command_answer(C, COMMAND_PARAMETER_MAN_TAG_CFG, err, NULL, 0);
}

static command_t cmd_tag_param_config = {
	.cmd_id = COMMAND_PARAMETER_MAN_TAG_CFG,
	.execute = ec_tag_param_config,
};

/*---------------------------------------------------------------------
 *	指令 0x20:标签参数查询指令
 *--------------------------------------------------------------------*/
static void ec_tag_param_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t val, err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	switch (*cmd_param) {
	case TAG_FILTER_ENABLE:
		val = A->tag_report.filter_enable;
		command_answer(C, COMMAND_PARAMETER_MAN_TAG_QUERY, CMD_EXE_SUCCESS, &val, 1);
		return;
	case TAG_FILTER_TIME:
		val = A->tag_report.filter_time;
		command_answer(C, COMMAND_PARAMETER_MAN_TAG_QUERY, CMD_EXE_SUCCESS, &val, 1);
		return;
	case TAG_Q_VALUE:
		val = S->tag_param.q_val;
		command_answer(C, COMMAND_PARAMETER_MAN_TAG_QUERY, CMD_EXE_SUCCESS, &val, 1);
		return;
	default:
		err = ERRCODE_CMD_ERRTYPE;
		log_msg("%s: Unkown parameter 0X%02X.", __FUNCTION__, *cmd_param);
	}

	if (err != CMD_EXE_SUCCESS) {
		command_answer(C, COMMAND_PARAMETER_MAN_TAG_QUERY, err, NULL, 0);
	}
}

static command_t cmd_tag_param_query = {
	.cmd_id = COMMAND_PARAMETER_MAN_TAG_QUERY,
	.execute = ec_tag_param_query,
};

/*
 * 注册指令集和属于此指令集的所有指令
 */
int param_man_init(void)
{
	int err = command_set_register(&cmdset_param_man);
	err |= command_register(&cmdset_param_man, &cmd_param_table_man);
	err |= command_register(&cmdset_param_man, &cmd_ante_param_config);
	err |= command_register(&cmdset_param_man, &cmd_ante_param_query);
	err |= command_register(&cmdset_param_man, &cmd_freq_table_config);
	err |= command_register(&cmdset_param_man, &cmd_freq_table_query);
	err |= command_register(&cmdset_param_man, &cmd_comm_param_config);
	err |= command_register(&cmdset_param_man, &cmd_comm_param_query);
	err |= command_register(&cmdset_param_man, &cmd_ether_param_config);
	err |= command_register(&cmdset_param_man, &cmd_ether_param_query);
	err |= command_register(&cmdset_param_man, &cmd_tag_param_config);
	err |= command_register(&cmdset_param_man, &cmd_tag_param_query);
	return err;
}
