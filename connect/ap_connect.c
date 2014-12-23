#include "ap_connect.h"
#include "errorlog.h"
#include "connect.h"
#include "rf_ctrl.h"
#include "hostifregs.h"
#include "report_tag.h"
#include "cfg_file.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define AP_FRAME_HEADER_LEN	sizeof(RFID_PACKET_COMMON)

int ap_conn_recv(ap_connect_t *A)
{
	return rs232_read(A->fd, (uint8_t *)&A->recv.cmn, sizeof(RFID_PACKET_COMMON));
}

ap_connect_t *ap_connect_new(system_param_t *S)
{
	ap_connect_t *A = malloc(sizeof(ap_connect_t));
	if (NULL == A)
		log_quit("malloc error");
	memset(A, 0, sizeof(ap_connect_t));

	if (report_tag_init(&A->tag_report) < 0)
		log_quit("report_tag_init error");

	A->fd = rs232_init("/dev/ttyS6", 115200);
	if (A->fd < 0)
		log_quit("rs232_init error");

	r2000_control_command(A, R2000_SOFTRESET);
	if (r2000_control_command(A, R2000_GET_SN) < 0)
		log_msg("R2000 not found");

	uint32_t mac_ver;
	if (read_mac_register(A, MAC_VER, &mac_ver) < 0) {
		log_msg("read_mac_register error");
	} else {
		log_msg("MAC firmware version: %ld.%ld.%ld", (mac_ver>>24)&0xFF, 
			(mac_ver>>16)&0xFF, (mac_ver>>8)&0xFF);
		snprintf(S->sysinfo.fpga_swrev, READER_FPGA_SWREV_LEN, "%d.%d.%d", 
			(mac_ver>>24)&0xFF, (mac_ver>>16)&0xFF, (mac_ver>>8)&0xFF);
	}

	r2000_check_freq_std(S, A);

	cfg_get_filter_enable(&A->tag_report.filter_enable);
	cfg_get_filter_time(&A->tag_report.filter_time);

	/* 如果为韦根自动上传自过滤时间为250ms */
	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC
		&& S->pre_cfg.upload_mode == UPLOAD_MODE_WIEGAND) {
		A->tag_report.filter_time = 2;
	}
	
	if (A->tag_report.filter_time) {
		report_tag_set_timer(A, A->tag_report.filter_time * 100);
	}

	/* 标签选择参数 */
	select_param_t param;
	if (!read_select_param(&param)) {
		r2000_tag_select(&param, A);
	}

	A->recv.rlen = 0;
	A->send.wlen = 0;
	A->cur_ant_power = 0;
	A->r2000_error_log = false;
	return A;
}
