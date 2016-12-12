#ifndef _COMMAND_DEF_H
#define _COMMAND_DEF_H

/*---------------------------------------------------------------------
 *	上位机指令类型前缀，分类定义
 *--------------------------------------------------------------------*/
#define COMMAND_BASE_MASK				0xf0	/* 指令类型屏蔽掩码 */
#define COMMAND_SYS_CONTROL_BASE			0x00	/* 系统控制指令 */
#define COMMAND_PARAMETER_MAN_BASE			0x10	/* 参数表操作指令 */
#define COMMAND_READER_MAN_BASE				0x20	/* 读写器操作指令 */
#define COMMAND_EXTMODULE_MAN_BASE			0x30	/* 扩展模块指令 */
#define COMMAND_TRANSMIT_CONTROL_BASE			0x50	/* 传输控制指令 */
#define COMMAND_ACTIVE_TAG_MAN_BASE			0x60	/* 有源标签操作指令 */
#define COMMAND_18K6B_MAN_BASE				0x80	/* ISO18K-6B操作指令 */
#define COMMAND_18K6C_MAN_BASE				0x90	/* ISO18K-6C操作指令 */
#define COMMAND_INTERGRATION_APPLY_BASE			0xA0	/* 集成应用指令集 */
#define COMMAND_TIME_MAN_BASE				0xB0	/* 时间操作指令集 */
#define COMMAND_EXTEND_BOARD_BASE			0xC0	/* 扩展板操作指令集 */
#define COMMAND_DATA_CENTER_BASE			0xD0	/* 保留部分 */
#define COMMAND_R2000_SPECIFIC_BASE			0xE0	/* R2000操作指令集 */

/*---------------------------------------------------------------------
 *	系统控制指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_SYS_CONTROL_INFO_CFG			0x00	/* 系统信息配置指令 */
#define COMMAND_SYS_CONTROL_INFO_QUER			0x01	/* 系统信息查询指令 */
#define COMMAND_SYS_CONTROL_WORKMODE_CFG		0x02	/* 工作模式配置指令 */
#define COMMAND_SYS_CONTROL_UPDATE_PROGRAM		0x03	/* 读写器系统程序更新(保留) */
#define COMMAND_SYS_CONTROL_TESTMODE_CFG		0x0A	/* 系统测试控制指令 */
#define COMMAND_SYS_CONTROL_ONNET_EQUIPEMENT_SEARCH 	0x0C	/* 在网设备搜索指令 */
#define COMMAND_SYS_CONTROL_ONNET_EQUIPEMENT_CFG	0x0D	/* 在网设备配置指令 */

/* 系统信息查询/配置指令类型定义 */


/*---------------------------------------------------------------------
 *	系统参数操作指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_PARAMETER_MAN_PARATABLE			0x10	/* 参数表操作指令 */
#define COMMAND_PARAMETER_MAN_RF_CFG			0x11	/* 射频端口参数配置指令 */
#define COMMAND_PARAMETER_MAN_RF_QUERY			0x12	/* 射频端口参数查询指令 */
#define COMMAND_PARAMETER_MAN_CARRIER_CFG		0x13	/* 载波参数配置指令 */
#define COMMAND_PARAMETER_MAN_CARRIER_QUERY		0x14	/* 载波参数查询指令 */
#define COMMAND_PARAMETER_MAN_COMM_CFG			0x15	/* 通讯参数配置指令 */
#define COMMAND_PARAMETER_MAN_COMM_QUERY		0x16	/* 通讯参数查询指令 */
#define COMMAND_PARAMETER_MAN_ETHER_CFG			0x17	/* 网络参数配置指令 */
#define COMMAND_PARAMETER_MAN_ETHER_QUERY		0x18	/* 网络参数查询指令 */
#define COMMAND_PARAMETER_MAN_TAG_CFG			0x19	/* 标签参数配置指令 */
#define COMMAND_PARAMETER_MAN_TAG_QUERY			0x1A	/* 标签参数查询指令 */
#define COMMAND_PARAMETER_MAN_EXTBOARD_CFG		0x1B	/* 扩展板参数配置指令 */
#define COMMAND_PARAMETER_MAN_EXTBOARD_QUERY		0x1C	/* 扩展板参数查询指令 */

/*---------------------------------------------------------------------
 *	读写器操作指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_READER_MAN_RFPWD			0x20	/* 载波操作指令 */
#define COMMAND_READER_MAN_IO_OUTPUT			0x21	/* IO输出操作指令 */
#define COMMAND_READER_MAN_IO_INPUT			0x22	/* IO输入查询指令 */
#define COMMAND_READER_MAN_BEEP_CFG			0x23	/* 蜂鸣器操作指令 */
#define COMMAND_READER_MAN_BEEP_QUERY			0x24	/* 蜂鸣器查询指令 */
#define COMMAND_READER_MAN_PULSE_WIDTH_CFG		0x25	/* 脉冲宽度配置 */
#define COMMAND_READER_MAN_PULSE_WIDTH_QUERY		0x26	/* 脉冲宽度查询 */
#define COMMAND_READER_MAN_REBOOT			0x2A	/* 设备重启 */
#define COMMAND_READER_MAN_APP_EXIT			0x2B	/* 程序退出 */

/*---------------------------------------------------------------------
 *	传输控制指令指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_TRANSMIT_CONTROL_LINK			0x50	/* 请求连接指令 */
#define COMMAND_TRANSMIT_CONTROL_UNLINK			0x51	/* 断开连接指令 */
#define COMMAND_TRANSMIT_CONTROL_FINISH			0x53	/* 指令 */
#define COMMAND_TRANSMIT_CONTROL_HEARTBEAT		0x58	/* 心跳包指令 */
#define COMMAND_TRANSMIT_CONTROL_TRIGGERSTATUS	0x59	/* I/O 触发状态检测指令*/


/*---------------------------------------------------------------------
 *	ISO18000-6C协议标签操作指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_18K6C_MAN_SELECT_TAG			0x90	/* 标签选择指令 */
#define COMMAND_18K6C_MAN_READ_EPC			0x91	/* 读EPC码指令 */
#define COMMAND_18K6C_MAN_READ_TID			0x92	/* 读TID码指令 */
#define COMMAND_18K6C_MAN_WRITE_EPC			0x93	/* 写EPC数据指令 */
#define COMMAND_18K6C_MAN_READ_USERBANK			0x94	/* 读用户数据区指令 */
#define COMMAND_18K6C_MAN_WRITE_USERBANK		0x95	/* 写用户数据区指令 */
#define COMMAND_18K6C_MAN_WRITE_BLOCK			0x96	/* 块写Bank数据指令 */
#define COMMAND_18K6C_MAN_ERASE_BLOCK			0x97	/* 块擦Bank数据指令 */
#define COMMAND_18K6C_MAN_SET_ACCESSPIN			0x98	/* 配置访问密码指令 */
#define COMMAND_18K6C_MAN_SET_KILLPIN			0x99	/* 设置销毁密码指令 */
#define COMMAND_18K6C_MAN_LOCK_OPERATION		0x9A	/* 标签锁状态配置指令 */
#define COMMAND_18K6C_MAN_KILL_TAG			0x9B	/* 标签灭活指令 */
#define COMMAND_18K6C_MAN_CHANGE_EAS			0x9C	/* EAS标志配置指令 */
#define COMMAND_18K6C_MAN_SET_EASALARM			0x9D	/* EAS监控功能设置指令 */
#define COMMAND_18K6C_MAN_INITIALIZE_TAG		0x9E	/* 初始化标签指令 */
#define COMMAND_18K6C_MAN_READ_EPC_TID			0x9f	/* 读18K6C标签TID和EPC码 */

/*---------------------------------------------------------------------
 *	集成应用指令集，子类型定义
 *--------------------------------------------------------------------*/
#define COMMAND_INTERGRATION_APPLY_READ			0xA0	/* 读卡指令 */
#define COMMAND_INTERGRATION_APPLY_6BIDDATA		0xA1	/* 读IS018000-6B标签及数据 */
#define COMMAND_INTERGRATION_APPLY_6CIDDATA		0xA2	/* 读EPCG2标签及数据 */

/*---------------------------------------------------------------------
 *	读写器时间
 *--------------------------------------------------------------------*/
#define COMMAND_READER_TIME_CONFIG			0xB0	/* 读写器时间查询指令 */
#define COMMAND_READER_TIME_QUERY			0xB1	/* 读写器时间配置指令 */
#define COMMAND_FLASHDATA_QUERY				0xB2	/* FLASH 数据查询 */
#define COMMAND_FLASHDATA_COUNT_QUERY			0xB3	/* FLASH 数据标签数查询 */
#define COMMAND_FLASHDATA_CLEAR				0xB4	/* FLASH 数据清空 */

/*---------------------------------------------------------------------
 *	扩展板配置
 *--------------------------------------------------------------------*/
#define COMMAND_GPRS_CONFIG				0xC0	/* GPRS 配置 */
#define COMMAND_GPRS_QUERY				0xC1	/* GPRS 查询 */
#define COMMAND_WIFI_CONFIG				0xC2	/* WIFI 配置 */
#define COMMAND_WIFI_QUERY				0xC3	/* WIFI 查询 */
#define COMMAND_WIFI_TRANS_RESET			0xC4	/* WIFI 进入透传或复位 */

/*---------------------------------------------------------------------
 *	数据中心
 *--------------------------------------------------------------------*/
#define COMMAND_RECV_TAG_CONFIRM					0xD0	/* 接收标签确认指令 */
#define COMMAND_RECV_TAG_CONFIRM_WIRELESS			0xD1  	/* 接收标签确认指令 */
#define COMMAND_RECV_TAG_CONFIRM_TRIGGER_STATUS		0xD2	/* 接收触发状态确认指令 */
#define COMMAND_RECV_CONFIRM_WIFI_CONNECT			0xD3	/* 接收确认WIFI  连接指令 */



/*---------------------------------------------------------------------
 *	R2000 相关指令
 *--------------------------------------------------------------------*/
#define COMMAND_R2000_LOG_ENABLE			0xE0	/* 配置日志功能 */
#define COMMAND_R2000_ERROR_REPORT			0xE1	/* R2000错误上报指令 */
#define COMMAND_R2000_FW_UPDATE				0xE2	/* R2000模块程序升级 */

#endif	/* _COMMAND_DEF_H */
