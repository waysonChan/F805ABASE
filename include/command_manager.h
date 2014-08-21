#ifndef _COMMAND_MANAGER_H
#define _COMMAND_MANAGER_H

#include "config.h"
#include "r2h_connect.h"
#include "ap_connect.h"
#include "parameter.h"

#define RFPWD_OFF		0x00	/* 关闭功放 */
#define RFPWD_ON		0x01	/* 打开功放 */

/* 标签参数 */
#define TAG_FILTER_ENABLE	0x02
#define TAG_FILTER_TIME		0x03
#define TAG_Q_VALUE		0x10
#define TAG_SELECT_PARAM	0x11

typedef struct _command {
	uint8_t cmd_id;
	void (*execute)(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
	struct _command *next;
} command_t;

typedef struct _command_set {
	uint8_t set_type;
	command_t *cmd_head;
	struct _command_set *next;
} command_set_t;

int command_set_register(command_set_t *new_set);
int command_register(command_set_t *set, command_t *new_cmd);
int command_execute(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int command_init(void);

int integration_app_init(void);
int param_man_init(void);
int reader_man_init(void);
int sys_control_init(void);
int time_man_init(void);
int trans_control_init(void);
int epc_18k6c_init(void);
int extend_board_init(void);
int data_center_init(void);

uint8_t stop_read_tag(system_param_t *S, ap_connect_t *A);

int work_status_timer_trigger(r2h_connect_t *C, system_param_t *S);

#endif	/* _COMMAND_MANAGER_H */
