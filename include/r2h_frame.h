#ifndef _FRAME_H
#define _FRAME_H

#include "r2h_connect.h"

/*---------------------------------------------------------------------
 *	帧解析结果定义
 *--------------------------------------------------------------------*/
#define FRAME_COMPLETE			0
#define FRAME_UNCOMPLETE		1
#define FRAME_CRCERROR			2
#define FRAME_PAMERROR			3
#define FRAME_UNKNOWERROR		4
#define FRAME_PROTOCOLERROR		5

/*---------------------------------------------------------------------
 *	数据传输协议关键字定义
 *--------------------------------------------------------------------*/
#define FRAME_HEADER			0x55
#define SUBSTITUTE_DATA_56		0x56
#define SUBSTITUTE_DATA_57		0x57

/*---------------------------------------------------------------------
 *	指令帧接收状态机的状态宏定义
 *--------------------------------------------------------------------*/
#define FRAME_READY_STATE		0
#define FRAME_GET_CONTROL_FIELD		1
#define FRAME_GET_BUSADDR		2
#define FRAME_GET_LENGTH		3
#define FRAME_GET_COMMAND		4
#define FRAME_GET_PARAMET		5
#define FRAME_GET_CRCHIGH		6
#define FRAME_GET_CRCLOW		7
#define FRAME_RCV_FINISH		8

int r2h_frame_parse(r2h_connect_t *C, int result);

#endif	/* _FRAME_H */
