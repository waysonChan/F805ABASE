#ifndef _COMMAND_DEF_H
#define _COMMAND_DEF_H

/*---------------------------------------------------------------------
 *	��λ��ָ������ǰ׺�����ඨ��
 *--------------------------------------------------------------------*/
#define COMMAND_BASE_MASK				0xf0	/* ָ�������������� */
#define COMMAND_SYS_CONTROL_BASE			0x00	/* ϵͳ����ָ�� */
#define COMMAND_PARAMETER_MAN_BASE			0x10	/* ���������ָ�� */
#define COMMAND_READER_MAN_BASE				0x20	/* ��д������ָ�� */
#define COMMAND_EXTMODULE_MAN_BASE			0x30	/* ��չģ��ָ�� */
#define COMMAND_TRANSMIT_CONTROL_BASE			0x50	/* �������ָ�� */
#define COMMAND_ACTIVE_TAG_MAN_BASE			0x60	/* ��Դ��ǩ����ָ�� */
#define COMMAND_18K6B_MAN_BASE				0x80	/* ISO18K-6B����ָ�� */
#define COMMAND_18K6C_MAN_BASE				0x90	/* ISO18K-6C����ָ�� */
#define COMMAND_INTERGRATION_APPLY_BASE			0xA0	/* ����Ӧ��ָ� */
#define COMMAND_TIME_MAN_BASE				0xB0	/* ʱ�����ָ� */
#define COMMAND_EXTEND_BOARD_BASE			0xC0	/* ��չ�����ָ� */
#define COMMAND_DATA_CENTER_BASE			0xD0	/* �������� */
#define COMMAND_R2000_SPECIFIC_BASE			0xE0	/* R2000����ָ� */

/*---------------------------------------------------------------------
 *	ϵͳ����ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_SYS_CONTROL_INFO_CFG			0x00	/* ϵͳ��Ϣ����ָ�� */
#define COMMAND_SYS_CONTROL_INFO_QUER			0x01	/* ϵͳ��Ϣ��ѯָ�� */
#define COMMAND_SYS_CONTROL_WORKMODE_CFG		0x02	/* ����ģʽ����ָ�� */
#define COMMAND_SYS_CONTROL_UPDATE_PROGRAM		0x03	/* ��д��ϵͳ�������(����) */
#define COMMAND_SYS_CONTROL_TESTMODE_CFG		0x0A	/* ϵͳ���Կ���ָ�� */
#define COMMAND_SYS_CONTROL_ONNET_EQUIPEMENT_SEARCH 	0x0C	/* �����豸����ָ�� */
#define COMMAND_SYS_CONTROL_ONNET_EQUIPEMENT_CFG	0x0D	/* �����豸����ָ�� */

/* ϵͳ��Ϣ��ѯ/����ָ�����Ͷ��� */


/*---------------------------------------------------------------------
 *	ϵͳ��������ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_PARAMETER_MAN_PARATABLE			0x10	/* ���������ָ�� */
#define COMMAND_PARAMETER_MAN_RF_CFG			0x11	/* ��Ƶ�˿ڲ�������ָ�� */
#define COMMAND_PARAMETER_MAN_RF_QUERY			0x12	/* ��Ƶ�˿ڲ�����ѯָ�� */
#define COMMAND_PARAMETER_MAN_CARRIER_CFG		0x13	/* �ز���������ָ�� */
#define COMMAND_PARAMETER_MAN_CARRIER_QUERY		0x14	/* �ز�������ѯָ�� */
#define COMMAND_PARAMETER_MAN_COMM_CFG			0x15	/* ͨѶ��������ָ�� */
#define COMMAND_PARAMETER_MAN_COMM_QUERY		0x16	/* ͨѶ������ѯָ�� */
#define COMMAND_PARAMETER_MAN_ETHER_CFG			0x17	/* �����������ָ�� */
#define COMMAND_PARAMETER_MAN_ETHER_QUERY		0x18	/* ���������ѯָ�� */
#define COMMAND_PARAMETER_MAN_TAG_CFG			0x19	/* ��ǩ��������ָ�� */
#define COMMAND_PARAMETER_MAN_TAG_QUERY			0x1A	/* ��ǩ������ѯָ�� */
#define COMMAND_PARAMETER_MAN_EXTBOARD_CFG		0x1B	/* ��չ���������ָ�� */
#define COMMAND_PARAMETER_MAN_EXTBOARD_QUERY		0x1C	/* ��չ�������ѯָ�� */

/*---------------------------------------------------------------------
 *	��д������ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_READER_MAN_RFPWD			0x20	/* �ز�����ָ�� */
#define COMMAND_READER_MAN_IO_OUTPUT			0x21	/* IO�������ָ�� */
#define COMMAND_READER_MAN_IO_INPUT			0x22	/* IO�����ѯָ�� */
#define COMMAND_READER_MAN_BEEP_CFG			0x23	/* ����������ָ�� */
#define COMMAND_READER_MAN_BEEP_QUERY			0x24	/* ��������ѯָ�� */
#define COMMAND_READER_MAN_PULSE_WIDTH_CFG		0x25	/* ���������� */
#define COMMAND_READER_MAN_PULSE_WIDTH_QUERY		0x26	/* �����Ȳ�ѯ */
#define COMMAND_READER_MAN_REBOOT			0x2A	/* �豸���� */
#define COMMAND_READER_MAN_APP_EXIT			0x2B	/* �����˳� */

/*---------------------------------------------------------------------
 *	�������ָ��ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_TRANSMIT_CONTROL_LINK			0x50	/* ��������ָ�� */
#define COMMAND_TRANSMIT_CONTROL_UNLINK			0x51	/* �Ͽ�����ָ�� */
#define COMMAND_TRANSMIT_CONTROL_FINISH			0x53	/* ָ�� */
#define COMMAND_TRANSMIT_CONTROL_HEARTBEAT		0x58	/* ������ָ�� */
#define COMMAND_TRANSMIT_CONTROL_TRIGGERSTATUS	0x59	/* I/O ����״̬���ָ��*/


/*---------------------------------------------------------------------
 *	ISO18000-6CЭ���ǩ����ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_18K6C_MAN_SELECT_TAG			0x90	/* ��ǩѡ��ָ�� */
#define COMMAND_18K6C_MAN_READ_EPC			0x91	/* ��EPC��ָ�� */
#define COMMAND_18K6C_MAN_READ_TID			0x92	/* ��TID��ָ�� */
#define COMMAND_18K6C_MAN_WRITE_EPC			0x93	/* дEPC����ָ�� */
#define COMMAND_18K6C_MAN_READ_USERBANK			0x94	/* ���û�������ָ�� */
#define COMMAND_18K6C_MAN_WRITE_USERBANK		0x95	/* д�û�������ָ�� */
#define COMMAND_18K6C_MAN_WRITE_BLOCK			0x96	/* ��дBank����ָ�� */
#define COMMAND_18K6C_MAN_ERASE_BLOCK			0x97	/* ���Bank����ָ�� */
#define COMMAND_18K6C_MAN_SET_ACCESSPIN			0x98	/* ���÷�������ָ�� */
#define COMMAND_18K6C_MAN_SET_KILLPIN			0x99	/* ������������ָ�� */
#define COMMAND_18K6C_MAN_LOCK_OPERATION		0x9A	/* ��ǩ��״̬����ָ�� */
#define COMMAND_18K6C_MAN_KILL_TAG			0x9B	/* ��ǩ���ָ�� */
#define COMMAND_18K6C_MAN_CHANGE_EAS			0x9C	/* EAS��־����ָ�� */
#define COMMAND_18K6C_MAN_SET_EASALARM			0x9D	/* EAS��ع�������ָ�� */
#define COMMAND_18K6C_MAN_INITIALIZE_TAG		0x9E	/* ��ʼ����ǩָ�� */
#define COMMAND_18K6C_MAN_READ_EPC_TID			0x9f	/* ��18K6C��ǩTID��EPC�� */

/*---------------------------------------------------------------------
 *	����Ӧ��ָ��������Ͷ���
 *--------------------------------------------------------------------*/
#define COMMAND_INTERGRATION_APPLY_READ			0xA0	/* ����ָ�� */
#define COMMAND_INTERGRATION_APPLY_6BIDDATA		0xA1	/* ��IS018000-6B��ǩ������ */
#define COMMAND_INTERGRATION_APPLY_6CIDDATA		0xA2	/* ��EPCG2��ǩ������ */

/*---------------------------------------------------------------------
 *	��д��ʱ��
 *--------------------------------------------------------------------*/
#define COMMAND_READER_TIME_CONFIG			0xB0	/* ��д��ʱ���ѯָ�� */
#define COMMAND_READER_TIME_QUERY			0xB1	/* ��д��ʱ������ָ�� */
#define COMMAND_FLASHDATA_QUERY				0xB2	/* FLASH ���ݲ�ѯ */
#define COMMAND_FLASHDATA_COUNT_QUERY			0xB3	/* FLASH ���ݱ�ǩ����ѯ */
#define COMMAND_FLASHDATA_CLEAR				0xB4	/* FLASH ������� */

/*---------------------------------------------------------------------
 *	��չ������
 *--------------------------------------------------------------------*/
#define COMMAND_GPRS_CONFIG				0xC0	/* GPRS ���� */
#define COMMAND_GPRS_QUERY				0xC1	/* GPRS ��ѯ */
#define COMMAND_WIFI_CONFIG				0xC2	/* WIFI ���� */
#define COMMAND_WIFI_QUERY				0xC3	/* WIFI ��ѯ */
#define COMMAND_WIFI_TRANS_RESET			0xC4	/* WIFI ����͸����λ */

/*---------------------------------------------------------------------
 *	��������
 *--------------------------------------------------------------------*/
#define COMMAND_RECV_TAG_CONFIRM					0xD0	/* ���ձ�ǩȷ��ָ�� */
#define COMMAND_RECV_TAG_CONFIRM_WIRELESS			0xD1  	/* ���ձ�ǩȷ��ָ�� */
#define COMMAND_RECV_TAG_CONFIRM_TRIGGER_STATUS		0xD2	/* ���մ���״̬ȷ��ָ�� */
#define COMMAND_RECV_CONFIRM_WIFI_CONNECT			0xD3	/* ����ȷ��WIFI  ����ָ�� */



/*---------------------------------------------------------------------
 *	R2000 ���ָ��
 *--------------------------------------------------------------------*/
#define COMMAND_R2000_LOG_ENABLE			0xE0	/* ������־���� */
#define COMMAND_R2000_ERROR_REPORT			0xE1	/* R2000�����ϱ�ָ�� */
#define COMMAND_R2000_FW_UPDATE				0xE2	/* R2000ģ��������� */

#endif	/* _COMMAND_DEF_H */
