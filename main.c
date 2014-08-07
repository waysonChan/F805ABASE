#include "config.h"
#include "r2h_connect.h"
#include "ap_connect.h"
#include "r2h_frame.h"
#include "command_manager.h"
#include "parameter.h"
#include "rf_ctrl.h"
#include "report_tag.h"
#include "gpio.h"

#include <signal.h>
#include <string.h>

int log_to_stderr = 0;

int main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp(argv[1], "-d")) {
		log_to_stderr = 1;
	}
	
	system_param_t *S = sys_param_new();
	r2h_connect_t *C = r2h_connect_new(S);
	ap_connect_t *A = ap_connect_new(S);

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
		int r2h_fd = C->r2h[C->conn_type].fd;
		fd_set readset;
		FD_ZERO(&readset);

		maxfd = A->fd;
		FD_SET(A->fd, &readset);

		maxfd = (maxfd >= A->tag_report.filter_timer) ? maxfd : A->tag_report.filter_timer;
		FD_SET(A->tag_report.filter_timer, &readset);

		maxfd = (maxfd >= S->work_status_timer) ? maxfd : S->work_status_timer;
		FD_SET(S->work_status_timer, &readset);

		for (i = 0; i < GPO_NUMBER; i++) {
			if (S->gpo[i].pulse_timer > maxfd) {
				maxfd = S->gpo[i].pulse_timer;
			}
			FD_SET(S->gpo[i].pulse_timer, &readset);
		}

		if (C->connected) {
			maxfd = (maxfd >= r2h_fd) ? maxfd : r2h_fd;
			FD_SET(r2h_fd, &readset);
		} else {
			if (!C->accepted) {
				maxfd = (maxfd >= C->listener) ? maxfd : C->listener;
				FD_SET(C->listener, &readset);
			}
			for (i = 0; i < R2H_TOTAL; i++) {
				if ((i == R2H_TCP) && (!C->accepted))
					continue;
				maxfd = (maxfd >= C->r2h[i].fd) ? maxfd : C->r2h[i].fd;
				FD_SET(C->r2h[i].fd, &readset);
			}
		}

		int err = select(maxfd+1, &readset, NULL, NULL, NULL);
		if (err < 0) {
			log_sys("select error");
		} else if (err == 0) {
			continue;
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

		for (i = 0; i < GPO_NUMBER; i++) {
			if (FD_ISSET(S->gpo[i].pulse_timer, &readset)) {
				gpo_pulse_timer_trigger(i, S->gpo[i].pulse_timer);
			}
		}

		if (C->connected) {
			if (FD_ISSET(r2h_fd, &readset)) {
				ret = r2h_connect_recv(C, R2H_BUF_SIZE);
				if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
					command_execute(C, S, A);
				}
			}
		} else {
			if (!C->accepted && FD_ISSET(C->listener, &readset)) {
				r2h_tcp_accept(C);
				continue;
			}
			for (i = 0; i < R2H_TOTAL; i++) {
				if (FD_ISSET(C->r2h[i].fd, &readset)) {
					ret = r2h_connect_check_in(C, i);
					if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
						command_execute(C, S, A);
					}
				}
			}
		}
	}

	return 0;
}
