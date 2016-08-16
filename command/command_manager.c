#include "command_manager.h"

static command_set_t *command_set_head = NULL;

int command_set_register(command_set_t *new_set)
{
	command_set_t *tmp;

	if (!command_set_head) {
		command_set_head = new_set;
		new_set->next = NULL;
	} else {
		tmp = command_set_head;
		while (tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = new_set;
		new_set->next = NULL;
	}

	return 0;
}

int command_register(command_set_t *set, command_t *new_cmd)
{
	command_t * tmp;

	if (!set->cmd_head) {
		set->cmd_head = new_cmd;
		new_cmd->next = NULL;
	} else {
		tmp = set->cmd_head;
		while (tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = new_cmd;
		new_cmd->next = NULL;
	}

	return 0;
}

int command_execute(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	command_t *cmd;
	command_set_t *set;
	uint8_t cmd_id = C->recv.frame.cmd_id;

//if(cmd_id != 0xD1)
	log_msg("%s: cmd_id = 0X%02X.", __FUNCTION__, cmd_id);
	for (set = command_set_head; set; set = set->next) {
		for (cmd = set->cmd_head; cmd; cmd = cmd->next) {
			if (cmd->cmd_id == cmd_id) {
				cmd->execute(C, S, A);
				return 0;
			}
		}
	}
	log_msg("%s: Unkown Command.", __FUNCTION__);
	return -1;
}

int command_init(void)
{
	int err = 0;
	err |= integration_app_init();
	err |= param_man_init();
	err |= reader_man_init();
	err |= sys_control_init();
	err |= time_man_init();
	err |= trans_control_init();
	err |= epc_18k6c_init();
	err |= extend_board_init();
	err |= data_center_init();
	err |= r2000_specific_init();
	return err;
}
