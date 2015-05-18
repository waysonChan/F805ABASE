#include "r2h_frame.h"
#include "utility.h"

#include <assert.h>

static inline void _r2h_frame_init(r2h_connect_t *C)
{
	r2h_frame_t *F = &C->recv.frame;
	F->frame_state = FRAME_GET_CONTROL_FIELD;
	F->param_len = 0;
	F->remain_len = 0;
	F->param_index = 0;
	F->crc_val = 0;
}

static inline void _r2h_frame_reset(r2h_connect_t *C)
{
	C->recv.frame.frame_state = FRAME_READY_STATE;
}

static inline uint8_t _r2h_frame_parse_byte(r2h_connect_t *C, uint8_t byte)
{
	r2h_frame_t *F = &C->recv.frame;
	switch (F->frame_state) {
	case FRAME_READY_STATE:
		return FRAME_UNCOMPLETE;
	case FRAME_GET_CONTROL_FIELD:
		F->ctrl_field = byte;
		F->frame_state = FRAME_GET_BUSADDR;
		break;
	case FRAME_GET_BUSADDR:
		F->bus_addr = byte;
		F->frame_state = FRAME_GET_LENGTH;
		break;
	case FRAME_GET_LENGTH:
		if (byte < 1) {
			_r2h_frame_reset(C);
			return FRAME_PAMERROR;
		}
		F->param_len = byte;
		F->remain_len = byte - 1;/* 注意:减掉<指令类型>的1字节长度得到参数实际长度 */
		F->frame_state = FRAME_GET_COMMAND;
		break;
	case FRAME_GET_COMMAND:
		F->cmd_id = byte;
		F->param_index = 0;
		if (0 == F->remain_len) {
			F->frame_state = FRAME_GET_CRCHIGH;
		} else {
			F->frame_state = FRAME_GET_PARAMET;
		}
		break;
	case FRAME_GET_PARAMET:
		F->param_buf[F->param_index++] = byte;
		if (--F->remain_len == 0) {
			F->frame_state = FRAME_GET_CRCHIGH;
		}
		break;
	case FRAME_GET_CRCHIGH:
		F->frame_state = FRAME_GET_CRCLOW;
		break;
	case FRAME_GET_CRCLOW:
		F->frame_state = FRAME_RCV_FINISH;
		break;
	default:
		_r2h_frame_reset(C);
		return FRAME_UNKNOWERROR;
	}

	F->crc_val = crc_16_byte(byte, F->crc_val);
	if (FRAME_RCV_FINISH == F->frame_state) {
		_r2h_frame_reset(C);
		if (0x0 == F->crc_val)
			return FRAME_COMPLETE;
		else
			return FRAME_CRCERROR;
	}

	return FRAME_UNCOMPLETE;
}

int r2h_frame_parse(r2h_connect_t *C, int result)
{
	uint8_t *ptr;
	r2h_frame_t *F = &C->recv.frame;

	if (result > 0) {
		C->recv.rlen = result;
	} else if (result == 0 && 
		(C->conn_type == R2H_TCP || C->conn_type == R2H_USB || C->conn_type == R2H_GPRS)) {
		/* EOF */
		log_msg("recv EOF, close client. <conn_type = %d>", C->conn_type);
		r2h_connect_close_client(C);
		goto out;
	} else if (result == -1) {
		log_msg("recv error, close client.");
		r2h_connect_close_client(C);
		goto out;
	} else {
		/* result == -R2H_RS485 */
		goto out;
	}

	for (ptr=C->recv.rbuf; C->recv.rlen>0; C->recv.rlen--, ptr++) {
		switch (*ptr) {
		case FRAME_HEADER:
			_r2h_frame_init(C);
			F->last_byte = 0x0;	/* 注意:帧头 0x55 不校验 */
			continue;
		case SUBSTITUTE_DATA_56:
			if (SUBSTITUTE_DATA_56 == F->last_byte) {
				*ptr = FRAME_HEADER;
			} else {
				F->last_byte = SUBSTITUTE_DATA_56;
				continue;	/* 注意:如果上一字节不是0x56则把此0x56留到下次再做处理 */
			}
			break;
		case SUBSTITUTE_DATA_57:
			if (SUBSTITUTE_DATA_56 == F->last_byte) {
				*ptr = SUBSTITUTE_DATA_56;
			}
			break;
		default:
			if (SUBSTITUTE_DATA_56 == F->last_byte) {
				_r2h_frame_reset(C);
				return FRAME_PROTOCOLERROR;	/* 按照协议不可能出现这种情况 */
			}
		}

		F->last_byte = 0;
		switch (_r2h_frame_parse_byte(C, *ptr)) {
		case FRAME_COMPLETE:
			return FRAME_COMPLETE;
		case FRAME_UNCOMPLETE:
			break;
		case FRAME_CRCERROR:
		case FRAME_PAMERROR:
		case FRAME_UNKNOWERROR:
		default:
			log_msg("r2h_frame_parse error");
			goto out;
		}
	}

out:
	C->recv.rlen = 0;
	C->recv.frame.frame_state = FRAME_READY_STATE;
	return FRAME_UNCOMPLETE;
}
