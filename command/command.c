#include "command.h"
#include "command_def.h"
#include "utility.h"
#include "r2h_frame.h"
#include "ap_connect.h"
#include "errcode.h"

#include <string.h>

static void _reader_operate_afc(r2h_connect_t *C, uint8_t cmd_id, 
	uint8_t result, const uint8_t *buf, size_t sz)
{
	C->send.wbuf[0] = FRAME_HEADER;
	C->send.wbuf[1] = C->recv.frame.ctrl_field;
	C->send.wbuf[2] = C->recv.frame.bus_addr;
	C->send.wbuf[3] = sz + 2;	/* 加上 cmd_id 和 result 的长度 */
	C->send.wbuf[4] = cmd_id;
	C->send.wbuf[5] = result;

	if (sz) {
		memcpy(C->send.wbuf+6, buf, sz);
	}
	C->send.wlen = sz + 6;
}

static void _tag_operate_afc(r2h_connect_t *C, uint8_t cmd_id, 
	uint8_t result, const uint8_t *buf, size_t sz)
{
	tag_t *ptag;
	uint8_t ant_index, *tag_data;

	if (buf) {
		ptag = (tag_t *)buf;
		tag_data = ptag->data;
		ant_index = ptag->ant_index;
	}

	if (result != CMD_EXE_SUCCESS) {
		C->send.wbuf[0] = FRAME_HEADER;
		C->send.wbuf[1] = C->recv.frame.ctrl_field;
		C->send.wbuf[2] = C->recv.frame.bus_addr;
		C->send.wbuf[3] = 0x02;		/* NOTE 1 */
		C->send.wbuf[4] = cmd_id;
		C->send.wbuf[5] = result;
		C->send.wlen = 6;
	}

	if (cmd_id == COMMAND_18K6C_MAN_SELECT_TAG) {
		C->send.wbuf[0] = FRAME_HEADER;
		C->send.wbuf[1] = C->recv.frame.ctrl_field;
		C->send.wbuf[2] = C->recv.frame.bus_addr;
		C->send.wbuf[3] = sz + 0x02;	/* NOTE 2 */
		C->send.wbuf[4] = cmd_id;
		C->send.wbuf[5] = result;
		if (sz) {
			memcpy(C->send.wbuf+6, buf, sz);
		}
		C->send.wlen = sz + 6;		
	} else {
		C->send.wbuf[0] = FRAME_HEADER;
		C->send.wbuf[1] = C->recv.frame.ctrl_field;
		C->send.wbuf[2] = C->recv.frame.bus_addr;
		C->send.wbuf[3] = sz + 0x03;	/* NOTE 3 */
		C->send.wbuf[4] = cmd_id;
		C->send.wbuf[5] = result;
		C->send.wbuf[6] = ant_index;
		if (sz) {
			memcpy(C->send.wbuf+7, tag_data, sz);
		}
		C->send.wlen = sz + 7;
	}
}

static void _app_operate_afc(r2h_connect_t *C, uint8_t cmd_id, 
	uint8_t result, const uint8_t *buf, size_t sz)
{
	
}

static inline int _send_answer(r2h_connect_t *C)
{
	uint16_t crc_val = crc_16_buf(C->send.wbuf+1, C->send.wlen-1);	/* 帧头不参加校验 */
	C->send.wbuf[C->send.wlen++] = (uint8_t)(crc_val >> 8);
	C->send.wbuf[C->send.wlen++] = (uint8_t)(crc_val & 0xff);

	size_t total = replace_keyword(C->send.wbuf+1, C->send.wlen-1) + 1;	/* 帧头不能被替换 */
	return r2h_connect_send(C, total);
}

int command_answer(r2h_connect_t *C, uint8_t cmd_id, uint8_t result,
	const void *buf, size_t sz)
{
	switch (cmd_id & COMMAND_BASE_MASK) {
	case COMMAND_SYS_CONTROL_BASE:
	case COMMAND_PARAMETER_MAN_BASE:
	case COMMAND_READER_MAN_BASE:
	case COMMAND_TRANSMIT_CONTROL_BASE:
	case COMMAND_TIME_MAN_BASE:
	case COMMAND_EXTEND_BOARD_BASE:
	case COMMAND_R2000_SPECIFIC_BASE:
		_reader_operate_afc(C, cmd_id, result, buf, sz);
		break;
	case COMMAND_18K6C_MAN_BASE:
		_tag_operate_afc(C, cmd_id, result, buf, sz);
		break;
	case COMMAND_INTERGRATION_APPLY_BASE:
		_app_operate_afc(C, cmd_id, result, buf, sz);
		break;
	default:
		log_msg("%s: Unkown Command.", __FUNCTION__);
	}

	return _send_answer(C);
}
