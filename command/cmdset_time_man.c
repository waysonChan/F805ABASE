#include "config.h"
#include "utility.h"
#include "command_manager.h"
#include "command_def.h"
#include "command.h"
#include "errcode.h"
#include "rf_ctrl.h"
#include "parameter.h"
#include "report_tag.h"

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

/*---------------------------------------------------------------------
 *	指令集:读写器时间操作
 *--------------------------------------------------------------------*/
static command_set_t cmdset_time_man = {
	.set_type = COMMAND_TIME_MAN_BASE,
	.cmd_head = NULL,
	.next = NULL,
};

/*---------------------------------------------------------------------
 *	指令:读写器时间设置
 *--------------------------------------------------------------------*/
struct linux_rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};
#define RTC_SET_TIME    _IOW('p', 0x0a, struct linux_rtc_time) /* Set RTC time    */

static int from_sys_clock(struct tm *t)
{
	int rtc = open("/dev/rtc0",  O_WRONLY);
	if (rtc < 0) {
		log_msg("%s: open ERR, <%s>", __FUNCTION__, strerror(errno));
		return rtc;
	}
	
	int err = ioctl(rtc, RTC_SET_TIME, t);
	if (err < 0) {
		log_msg("%s: ioctl ERR, <%s>", __FUNCTION__, strerror(errno));
	}

	close(rtc);
	return err;
}

static void ec_time_config(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	/*                       秒    分    时   星期   日    月    年
	 * uint8_t aucTmp[] = {0x33, 0x22, 0x11, 0x03, 0x26, 0x11, 0x13};
	 */
	struct tm t;
	uint8_t err = CMD_EXE_SUCCESS;
	const uint8_t *cmd_param = C->recv.frame.param_buf;
	
	t.tm_sec = convert_dec(cmd_param[0]);	/* 秒 */
	t.tm_min = convert_dec(cmd_param[1]);	/* 分 */
	t.tm_hour = convert_dec(cmd_param[2]);	/* 时 */
	t.tm_wday = convert_dec(cmd_param[3]);	/* 星期 */
	t.tm_mday = convert_dec(cmd_param[4]);	/* 日 */
	t.tm_mon = convert_dec(cmd_param[5] - 1);	/* 月 */
	t.tm_year = convert_dec(cmd_param[6]) + 100;	/* 年 */
	t.tm_isdst = 0;

	log_msg("t.tm_year = %d", t.tm_year);

	time_t timet = mktime(&t);
	if (timet < 0) {
		log_msg("%s: mktime() ERR, <%s>!", __FUNCTION__, strerror(errno));
		err = ERRCODE_CMD_UNKNOWERR;		
	}

	if (stime(&timet) < 0) {
		log_msg("%s: stime() ERR, <%s>!", __FUNCTION__, strerror(errno));
		err = ERRCODE_CMD_UNKNOWERR;
	}

	if (from_sys_clock(&t) < 0) {
		err = ERRCODE_CMD_UNKNOWERR;
	}

	command_answer(C, COMMAND_READER_TIME_CONFIG, err, NULL, 0);
}

static command_t cmd_time_config = {
	.cmd_id = COMMAND_READER_TIME_CONFIG,
	.execute = ec_time_config,
};

/*---------------------------------------------------------------------
 *	指令:读写器时间查询
 *--------------------------------------------------------------------*/
static void ec_time_query(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	/*                       秒    分    时   星期   日    月    年
	 * uint8_t aucTmp[] = {0x33, 0x22, 0x11, 0x03, 0x26, 0x11, 0x13};
	 */
	time_t timet;
	struct tm *ptm;
	uint8_t time_buf[7];

	if (time(&timet) < 0) {
		log_msg("%s: time() ERR!", __FUNCTION__);
		command_answer(C, COMMAND_READER_TIME_QUERY, ERRCODE_CMD_UNKNOWERR, NULL, 0);
		return;
	}

	ptm = localtime(&timet);
	time_buf[0] = convert_hex(ptm->tm_sec);		/* 秒 */
	time_buf[1] = convert_hex(ptm->tm_min);		/* 分 */
	time_buf[2] = convert_hex(ptm->tm_hour);	/* 时 */
	time_buf[3] = convert_hex(ptm->tm_wday);	/* 星期 */
	time_buf[4] = convert_hex(ptm->tm_mday);	/* 日 */
	time_buf[5] = convert_hex(ptm->tm_mon + 1);	/* 月 */
	time_buf[6] = convert_hex(ptm->tm_year % 100);	/* 年 */
	
	command_answer(C, COMMAND_READER_TIME_QUERY, CMD_EXE_SUCCESS, 
		time_buf, sizeof(time_buf));
}

static command_t cmd_time_query = {
	.cmd_id = COMMAND_READER_TIME_QUERY,
	.execute = ec_time_query,
};

/*---------------------------------------------------------------------
 *	指令:查询Flash缓存标签数
 *--------------------------------------------------------------------*/
static void ec_get_flash_tag_cnt(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	uint16_t tag_cnt = tag_storage_get_cnt();
	uint8_t cnt[4] = {0, 0, tag_cnt >> 8, tag_cnt & 0xFF};

	command_answer(C, COMMAND_FLASHDATA_COUNT_QUERY, CMD_EXE_SUCCESS, cnt, sizeof(cnt));
}

static command_t cmd_get_flash_tag_cnt = {
	.cmd_id = COMMAND_FLASHDATA_COUNT_QUERY,
	.execute = ec_get_flash_tag_cnt,
};
/*---------------------------------------------------------------------
 *	指令:清除Flash缓存
 *--------------------------------------------------------------------*/
static void ec_clear_flash_tag(r2h_connect_t *C, system_param_t *S, ap_connect_t *A)
{
	tag_storage_delete(true);
	command_answer(C, COMMAND_FLASHDATA_CLEAR, CMD_EXE_SUCCESS, NULL, 0);
}

static command_t cmd_get_flash_tag = {
	.cmd_id = COMMAND_FLASHDATA_CLEAR,
	.execute = ec_clear_flash_tag,
};

/*
 * 注册指令集和属于此指令集的所有指令
 */
int time_man_init(void)
{
	int err = command_set_register(&cmdset_time_man);
	err |= command_register(&cmdset_time_man, &cmd_time_config);
	err |= command_register(&cmdset_time_man, &cmd_time_query);
	err |= command_register(&cmdset_time_man, &cmd_get_flash_tag_cnt);
	err |= command_register(&cmdset_time_man, &cmd_get_flash_tag);
	return err;
}
