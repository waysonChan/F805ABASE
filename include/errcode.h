#ifndef _ERRCODE_H
#define _ERRCODE_H

#define CMD_EXE_SUCCESS			0x00

/*---------------------------------------------------------------------
 *	读写器系统错误代码
 *--------------------------------------------------------------------*/
#define ERRCODE_SYS_CODECREV		0X10
#define ERRCODE_SYS_BSBHWREV		0X11
#define ERRCODE_SYS_RFBHWREV		0X12
#define ERRCODE_SYS_PARAMETER		0X13
#define ERRCODE_SYS_R2000_ERR		0X14	/* R2000 not found  */
#define ERRCODE_SYS_RFBDETECT		0X15
#define ERRCODE_SYS_ETNETDETECT		0X16
#define ERRCODE_SYS_RTCLK		0X17
#define ERRCODE_SYS_MEMORY		0X18
#define ERRCODE_SYS_UNKNOWERR		0X1F

/*---------------------------------------------------------------------
 *	读写器操作错误代码
 *--------------------------------------------------------------------*/
#define ERRCODE_OPT_PASSWORD		0X20
#define ERRCODE_OPT_ANTENNA		0X21
#define ERRCODE_OPT_TESTMODE		0X22
#define ERRCODE_OPT_WRITEFLASH		0X23
#define ERRCODE_OPT_UNBREAKOPTION	0X24
#define ERRCODE_OPT_READERRUN		0x25
#define ERRCODE_OPT_LIMITUSER		0X26
#define ERRCODE_OPT_UNFINISHICMD	0X27
#define ERRCODE_OPT_VERIFY		0X28
#define ERRCODE_OPT_UNKNOWERR		0X2F

/*---------------------------------------------------------------------
 *	指令传输/接收/处理错误
 *--------------------------------------------------------------------*/
#define ERRCODE_CMD_NOEND		0X60
#define ERRCODE_CMD_CRC			0X61
#define ERRCODE_CMD_ERRTYPE		0X62
#define ERRCODE_CMD_PROTOCOL		0X63
#define ERRCODE_CMD_PARAM		0X64
#define ERRCODE_CMD_FRAME		0X65
#define ERRCODE_CMD_UNSUPPORT		0X66
#define ERRCODE_OPT_READERBUSY		0X67
#define ERRCODE_OPT_COMMPLETE		0X68
#define ERRCODE_CMD_UNKNOWERR		0X6F

/*---------------------------------------------------------------------
 *	EPC C1G2标签操作错误
 *--------------------------------------------------------------------*/
#define ERRCODE_EPC_NOANSWER		0XB0
#define ERRCODE_EPC_NOBANK		0XB1
#define ERRCODE_EPC_RTNADDRFLOW		0XB2
#define ERRCODE_EPC_RTNBANKLOCK		0XB3
#define ERRCODE_EPC_ACCESSPIN		0XB4
#define ERRCODE_EPC_KILLPIN		0XB5
#define ERRCODE_EPC_TAGBREAK		0XB6
#define ERRCODE_EPC_OPTUNINITTAG	0XB7
#define ERRCODE_EPC_UNINITTAG		0XB8
#define ERRCODE_EPC_RTNOTHER		0XB9
#define ERRCODE_EPC_RTNINSPOW		0XBA
#define ERRCODE_EPC_ACCESSFAIL		0XBB
#define ERRCODE_EPC_RTNNOSPC		0XBC
#define ERRCODE_EPC_WRITEVERIFY		0XBD
#define ERRCODE_EPC_UNKNOWERR		0XBF

#endif	/* _ERRCODE_H */
