#include "command_manager.h"
#include "command_def.h"

/*---------------------------------------------------------------------
 *	指令集:系统控制
 *--------------------------------------------------------------------*/
static command_set_t cmdset_set_integration_app = {
	.set_type = COMMAND_INTERGRATION_APPLY_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:系统信息配置指令
 *--------------------------------------------------------------------*/
/* TODO */

int integration_app_init(void)
{
	int err = command_set_register(&cmdset_set_integration_app);
	return err;
}
