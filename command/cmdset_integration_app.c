#include "command_manager.h"
#include "command_def.h"

/*---------------------------------------------------------------------
 *	ָ�:ϵͳ����
 *--------------------------------------------------------------------*/
static command_set_t cmdset_set_integration_app = {
	.set_type = COMMAND_INTERGRATION_APPLY_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	ָ��:ϵͳ��Ϣ����ָ��
 *--------------------------------------------------------------------*/
/* TODO */

int integration_app_init(void)
{
	int err = command_set_register(&cmdset_set_integration_app);
	return err;
}
