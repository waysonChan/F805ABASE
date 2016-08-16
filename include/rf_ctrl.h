#ifndef _RF_CTRL_H
#define _RF_CTRL_H

#include "config.h"
#include "r2h_connect.h"
#include "ap_connect.h"
#include "parameter.h"

#define BYTES_PER_LEN_UNIT		4

typedef enum {
	R2000_CANCEL,
	R2000_SOFTRESET,
	R2000_ABORT,
	R2000_PAUSE,
	R2000_RESUME,
	R2000_GET_SN,
	R2000_RESET_TO_BOOTLOADER
} ctrl_cmd_t;

int r2000_control_command(ap_connect_t *A, ctrl_cmd_t ctrl_cmd);
int read_mac_register(ap_connect_t *A, uint16_t reg_addr, uint32_t *reg_data);
int write_mac_register(ap_connect_t *A, uint16_t reg_addr, uint32_t reg_data);
int r2000_error_check(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int process_cmd_packets(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int auto_read_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int r2000_set_freq(system_param_t *S, ap_connect_t *A);
int r2000_set_operate_mode(ap_connect_t *A, int mode);
int r2000_set_ant_rfpower(system_param_t *S, ap_connect_t *A);
int r2000_get_ant_rfpower(ap_connect_t *A, uint32_t *rfpower);
int r2000_tag_read(tag_param_t *T, ap_connect_t *A);
int r2000_tag_write(tag_param_t *T, ap_connect_t *A);
int r2000_tag_lock(r2h_connect_t *C, tag_param_t *T, ap_connect_t *A);
int r2000_tag_kill(tag_param_t *T, ap_connect_t *A);
int r2000_tag_select(select_param_t *param, ap_connect_t *A);
int r2000_tag_deselect(ap_connect_t *A);
int r2000_check_freq_std(system_param_t *S, ap_connect_t *A);
int r2000_freq_config(system_param_t *S, ap_connect_t *A);

int trigger_to_read_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);


#endif	/* _RF_CTRL_H */
