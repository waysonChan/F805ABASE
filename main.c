#include "config.h"
#include "r2h_connect.h"
#include "ap_connect.h"
#include "r2h_frame.h"
#include "command_manager.h"
#include "parameter.h"
#include "rf_ctrl.h"
#include "report_tag.h"
#include "gpio.h"
#include "utility.h"
#include <unistd.h>

#include <signal.h>
#include <string.h>
#include <errno.h>

int log_to_stderr = 0;


int main(int argc, char *argv[])  
{
	if (argc == 2 && !strcmp(argv[1], "-d")) {
		log_to_stderr = 1;
	}
	
	system_param_t *S = sys_param_new();
	r2h_connect_t *C = r2h_connect_new(S);
	ap_connect_t *A = ap_connect_new(S);
	heartbeat_timer_int(S);
	command_init();
	if (r2h_connect_init(C, S) < 0)
		log_msg("r2h_connect_init error");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		log_quit("signal error");

	if (S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC) {
		auto_read_tag(C, S, A);
	}


	while (1) {
		int i, ret, maxfd = 0;
		int temp_connect_type;
		fd_set readset, writeset;
		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		maxfd = A->fd;  
		FD_SET(A->fd, &readset);

		maxfd = MAX(maxfd, A->tag_report.filter_timer);
		FD_SET(A->tag_report.filter_timer, &readset);

		maxfd = MAX(maxfd, S->work_status_timer);
		FD_SET(S->work_status_timer, &readset);

		maxfd = MAX(maxfd, S->heartbeat_timer);
		FD_SET(S->heartbeat_timer, &readset);
	
		if (S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS) {
			maxfd = MAX(maxfd, C->gprs_priv.gprs_timer);
			FD_SET(C->gprs_priv.gprs_timer, &readset);
		}

		for (i = 0; i < GPO_NUMBER; i++) {
			if (S->gpo[i].pulse_timer > maxfd) {
				maxfd = S->gpo[i].pulse_timer;
			}
			FD_SET(S->gpo[i].pulse_timer, &readset);
		}

		/* 一直监听 */
		maxfd = MAX(maxfd, C->listener);
		FD_SET(C->listener, &readset);
		
		for (i = 0; i < R2H_TOTAL; i++) {
			if (((i == R2H_TCP) && (!C->accepted))
				|| ((i == R2H_GPRS) && (!C->gprs_priv.connected))
				|| (C->r2h[i].fd < 0))
				continue;
			maxfd = MAX(maxfd, C->r2h[i].fd);
			FD_SET(C->r2h[i].fd, &readset);
		}

		if (C->gprs_priv.connect_in_progress) {
			maxfd = MAX(maxfd, C->r2h[R2H_GPRS].fd);
			FD_SET(C->r2h[R2H_GPRS].fd, &writeset);
		}


		FD_SET(S->gpio_dece.fd,&readset);

		int err = select(maxfd+1, &readset, &writeset, NULL, NULL);
		if (err < 0) {
			log_ret("select error");
		} else if (err == 0) {
			continue;
		}

		if (C->gprs_priv.connect_in_progress && FD_ISSET(C->r2h[R2H_GPRS].fd, &writeset)) {
			r2h_gprs_conn_check(C);
		}

		if (FD_ISSET(A->fd, &readset)) {
			if (ap_conn_recv(A) == sizeof(RFID_PACKET_COMMON))
				process_cmd_packets(C, S, A);
		}

		if (FD_ISSET(A->tag_report.filter_timer, &readset)) {
			report_tag_send_timer(C, S, A);
		}

		if (FD_ISSET(S->work_status_timer, &readset)) {
			work_status_timer_trigger(C, S);
		}

		if ((S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS)
			&& FD_ISSET(C->gprs_priv.gprs_timer, &readset)) {
			r2h_gprs_timer_trigger(C);
		}

		if (FD_ISSET(S->gpio_dece.fd, &readset)) {
			trigger_to_read_tag(C, S, A);
		}

		if (FD_ISSET(S->heartbeat_timer, &readset)) {
			heartbeat_timer_trigger(C, S );
		}

		for (i = 0; i < GPO_NUMBER; i++) {
			if (FD_ISSET(S->gpo[i].pulse_timer, &readset)) {
				gpo_pulse_timer_trigger(i, S->gpo[i].pulse_timer);
			}
		}

		if (FD_ISSET(C->listener, &readset)) {
			if (C->accepted) {
				r2h_connect_close_client(C);	/* 关闭之前的连接 */
			}
			r2h_tcp_accept(C);
		}
		
		for (i = 0; i < R2H_TOTAL; i++) {
			if (C->r2h[i].fd != -1 && FD_ISSET(C->r2h[i].fd, &readset)) {
				//log_msg("i = %d, fd = %d", i, C->r2h[i].fd);
				ret = r2h_connect_check_in(C, i);
				temp_connect_type = C->conn_type;
				if(S->pre_cfg.upload_mode == UPLOAD_MODE_WIFI && C->conn_type == R2H_WIFI){
					C->flag = true;
					C->recv.rlen = ret;
					while(C->recv.rlen){
						if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
							command_execute(C, S, A);
						}else{
							C->conn_type = temp_connect_type;
						}
					}
					C->flag = false;
					C->count = 0;
				}else{
					C->flag = false;
					if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
						command_execute(C, S, A);
					}else{
						C->conn_type = temp_connect_type;
					}
				}
			}
		}
	}

	return 0;
}
