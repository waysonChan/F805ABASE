#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "fw_upgrade.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

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

/*---------------------------------------------------------------------
 *	指令集:R2000操作指令集
 *--------------------------------------------------------------------*/
static command_set_t cmdset_r2000_specific = {
	.set_type = COMMAND_R2000_SPECIFIC_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:COMMAND_R2000_LOG_ENABLE
 *--------------------------------------------------------------------*/
static void ec_r2000_log_enable(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (C->recv.frame.param_len != 2) {	/* 指令字的长度包含在内 */
		log_msg("%s: invalid param_len", __FUNCTION__);
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	if (*cmd_param != 0x0 && *cmd_param != 0x1) {
		log_msg("%s: invalid *cmd_param", __FUNCTION__);
		err = ERRCODE_CMD_PARAM;
		goto out;
	}
	
	A->r2000_error_log = *cmd_param;

out:
	command_answer(C, COMMAND_R2000_LOG_ENABLE, err, NULL, 0);
}

static command_t cmd_r2000_log_enable = {
	.cmd_id = COMMAND_R2000_LOG_ENABLE,
	.execute = ec_r2000_log_enable,
};

/*---------------------------------------------------------------------
 *	指令:COMMAND_R2000_ERROR_REPORT
 *--------------------------------------------------------------------*/

/* 此指令为读写器主动上报指令，上位机不予回复 */

/*---------------------------------------------------------------------
 *	指令:COMMAND_R2000_FW_UPDATE
 *--------------------------------------------------------------------*/
static void ec_r2000_fw_update(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;

	if (S->work_status != WS_STOP) {
		log_msg("reader busy");
		err = ERRCODE_OPT_READERBUSY;
		goto out;
	}

	S->work_status = WS_UPGRADE;

	/* 文件类型 */
	uint8_t file_type = cmd_param[0];
	if (file_type != FILE_TYPE_R2000 && file_type != FILE_TYPE_LINUX_FILE) {
		log_msg("%s: invalid file type", __FUNCTION__);
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	/* 文件长度 */
	uint32_t file_len = (cmd_param[1] << 24) | (cmd_param[2] << 16) 
		| (cmd_param[3] << 8) | (cmd_param[4]);
	log_msg("file_len = %d", file_len);
	if (file_len > MAX_FILE_LEN) {
		log_msg("%s: invalid file_len", __FUNCTION__);
		err = ERRCODE_CMD_PARAM;
		goto out;		
	}

	/* 文件校验值 */
	uint32_t crc32 = (cmd_param[5] << 24) | (cmd_param[6] << 16) 
		| (cmd_param[7] << 8) | (cmd_param[8]);
	log_msg("crc32 = %08X", crc32);

	/* 文件名长度 */
	uint8_t file_name_len = cmd_param[9];
	log_msg("file_name_len = %d, param_len = %d", file_name_len, C->recv.frame.param_len);
	if (file_name_len > MAX_FILE_NAME_LEN
		|| (file_name_len != C->recv.frame.param_len-11)) {	/* 13表示除文件名之外的长度 */
		log_msg("%s: invalid file_name_len", __FUNCTION__);
		err = ERRCODE_CMD_PARAM;
		goto out;
	}

	/* 文件名 */
#define MAX_ABSOLUTE_FILE_NAME_LEN	(MAX_FILE_NAME_LEN+20)
	char tmp_name[MAX_FILE_NAME_LEN] = {0};
	char file_name[MAX_ABSOLUTE_FILE_NAME_LEN] = {0};		/* 文件名加上目录 */
	memcpy(tmp_name, cmd_param+10, file_name_len);
	snprintf(file_name, MAX_ABSOLUTE_FILE_NAME_LEN, "/f806/upgrade/%s", tmp_name);

	/* 检查文件长度 */
	struct stat sb;
	if (lstat(file_name, &sb) < 0) {
		log_ret("lstat error");
		err = ERRCODE_OPT_UNFINISHICMD;
		goto out;
	} else {
		if (sb.st_size != file_len) {
			log_msg("file_len do NOT match");
			err = ERRCODE_OPT_UNFINISHICMD;
			goto out;
		}
	}

	/* 检查CRC32 */
	uint8_t *file_buf = malloc(file_len);
	if (!file_buf) {
		log_msg("malloc error");
		err = ERRCODE_OPT_UNFINISHICMD;
		goto out;		
	}

	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		log_ret("open error");
		err = ERRCODE_OPT_UNFINISHICMD;
		goto out1;		
	}

	ssize_t nread = read(fd, file_buf, file_len);
	if (nread != file_len) {
		log_ret("read error");
		err = ERRCODE_OPT_UNFINISHICMD;
		goto out2;
	}

	uint32_t crc_ret = 0;
	crc_ret = fast_crc32(crc_ret, file_buf, file_len);
	if (crc_ret != crc32) {
		log_msg("crc_ret != crc32");
		err = ERRCODE_OPT_UNFINISHICMD;
		goto out2;
	}

	log_msg("----- updating -----");

	/* 执行文件更新 */
	switch (file_type) {
	case FILE_TYPE_R2000:
		if (upgrade_f860(A, file_name) < 0) {
			err = ERRCODE_OPT_UNFINISHICMD;
			goto out2;
		}
		break;
	case FILE_TYPE_LINUX_FILE:
		if (upgrade_linux_file(tmp_name) < 0) {
			err = ERRCODE_OPT_UNFINISHICMD;
			goto out2;
		}		
		break;
	default:
		err = ERRCODE_CMD_PARAM;
		goto out2;
	}

	log_msg("----- update successfully-----");
out2:
	close(fd);
out1:
	free(file_buf);
out:
	command_answer(C, COMMAND_R2000_FW_UPDATE, err, NULL, 0);
	S->work_status = WS_STOP;
}

static command_t cmd_r2000_fw_update = {
	.cmd_id = COMMAND_R2000_FW_UPDATE,
	.execute = ec_r2000_fw_update,
};

int r2000_specific_init(void)
{
	int err = command_set_register(&cmdset_r2000_specific);
	err |= command_register(&cmdset_r2000_specific, &cmd_r2000_log_enable);
	err |= command_register(&cmdset_r2000_specific, &cmd_r2000_fw_update);
	return err;
}
