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
#include <arpa/inet.h>
#include <unistd.h>


#include <signal.h>
#include <string.h>
#include <errno.h>

int log_to_stderr = 0;

static int set_select_para(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, fd_set *read_set, fd_set *write_set) {
	int i,maxfd;
	fd_set *readset, *writeset;

	readset = read_set;
	writeset = write_set;
	
	maxfd = A->fd;	
	FD_SET(A->fd, readset);

	maxfd = MAX(maxfd, A->tag_report.filter_timer);
	FD_SET(A->tag_report.filter_timer, readset);

	maxfd = MAX(maxfd, S->work_status_timer);
	FD_SET(S->work_status_timer, readset);

	maxfd = MAX(maxfd, S->delay_timer);
	FD_SET(S->delay_timer, readset);

	maxfd = MAX(maxfd, S->gpio_dece.fd);
	FD_SET(S->gpio_dece.fd,readset);
		
	if(S->pre_cfg.upload_mode != UPLOAD_MODE_NONE){
		maxfd = MAX(maxfd, S->heartbeat_timer);
		FD_SET(S->heartbeat_timer, readset);
	}
	
	if(S->pre_cfg.work_mode == WORK_MODE_TRIGGER || S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC){
		maxfd = MAX(maxfd, S->triggerstatus_timer);
		FD_SET(S->triggerstatus_timer, readset);
	}
	
	if (S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS) {
		maxfd = MAX(maxfd, C->gprs_priv.gprs_timer);
		FD_SET(C->gprs_priv.gprs_timer, readset);
	}

	for (i = 0; i < GPO_NUMBER; i++) {
		if (S->gpo[i].pulse_timer > maxfd) {
			maxfd = S->gpo[i].pulse_timer;
		}
		FD_SET(S->gpo[i].pulse_timer, readset);
	}

	/* 一直监听 */
	maxfd = MAX(maxfd, C->listener);
	FD_SET(C->listener, readset);

	for (i = 0; i < R2H_TOTAL; i++) {
		if (((i == R2H_TCP) && (!C->accepted))
			|| ((i == R2H_GPRS) && (!C->gprs_priv.connected))
			|| (C->r2h[i].fd < 0))
			continue;
		maxfd = MAX(maxfd, C->r2h[i].fd);
		FD_SET(C->r2h[i].fd, readset);
	}

	if (C->gprs_priv.connect_in_progress) {
		maxfd = MAX(maxfd, C->r2h[R2H_GPRS].fd);
		FD_SET(C->r2h[R2H_GPRS].fd, writeset);
	}

	return maxfd;
}

static int timer_operation(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, fd_set *read_set, fd_set *write_set) {
	int i;
	fd_set *readset, *writeset;

	readset = read_set;
	writeset = write_set;

	if (FD_ISSET(A->tag_report.filter_timer, readset)) {
		report_tag_send_timer(C, S, A);		
	}

	if (FD_ISSET(S->delay_timer, readset)) {
		delay_timer_trigger(C,S,A);
	}
	
	if (FD_ISSET(S->work_status_timer, readset)) {
		work_status_timer_trigger(C, S);
	}

	if ((S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS)
		&& FD_ISSET(C->gprs_priv.gprs_timer, readset)) {
		r2h_gprs_timer_trigger(C);
	}
	
	if(S->pre_cfg.work_mode == WORK_MODE_TRIGGER || S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC){
		if (FD_ISSET(S->heartbeat_timer, readset)) {
				heartbeat_timer_trigger(C, S );
		}
	}

	if(S->pre_cfg.work_mode == WORK_MODE_TRIGGER || S->pre_cfg.work_mode == WORK_MODE_AUTOMATIC){
		if (FD_ISSET(S->triggerstatus_timer, readset)) {
			triggerstatus_timer_trigger(C, S );
		}
	}

	for (i = 0; i < GPO_NUMBER; i++) {
		if (FD_ISSET(S->gpo[i].pulse_timer, readset)) {
			gpo_pulse_timer_trigger(i, S->gpo[i].pulse_timer);
		}
	}

	return 0; 
}

static int conn_operation(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, fd_set *read_set, fd_set *write_set) {
	int i,cnn,ret;	
	fd_set *readset, *writeset;

	readset = read_set;
	writeset = write_set;
	
	if (C->gprs_priv.connect_in_progress && FD_ISSET(C->r2h[R2H_GPRS].fd, writeset)) {
		r2h_gprs_conn_check(C);
	}

	if (FD_ISSET(C->listener, readset)) {
		if (C->accepted) {
			r2h_connect_close_client(C);	/* 关闭之前的连接 */
		}
		r2h_tcp_accept(C);
	}
	
	cnn = C->conn_type;
	for (i = 0; i < R2H_TOTAL; i++) {
		if (C->r2h[i].fd != -1 && FD_ISSET(C->r2h[i].fd, readset)) {
			ret = r2h_connect_check_in(C, i);
			if((S->pre_cfg.upload_mode == UPLOAD_MODE_WIFI && C->conn_type == R2H_WIFI)
					|| (S->pre_cfg.upload_mode == UPLOAD_MODE_GPRS && C->conn_type == R2H_GPRS)){
				C->flag = true;
				C->recv.rlen = ret;
				while(C->recv.rlen){
					if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
						command_execute(C, S, A);
							C->conn_type = i;
					}else{
							C->conn_type = cnn;
					}
				}
				C->flag = false;
				C->count = 0;
			}else{
				C->flag = false;
				if (r2h_frame_parse(C, ret) == FRAME_COMPLETE) {
					command_execute(C, S, A);
				}else{
						C->conn_type = cnn;
				}
			}
		}
	}
	return 0;
}

static int tag_operation(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, fd_set *read_set, fd_set *write_set) {
	fd_set *readset, *writeset;

	readset = read_set;
	writeset = write_set;
	
	if (FD_ISSET(A->fd, readset)) {
		if (ap_conn_recv(A) == sizeof(RFID_PACKET_COMMON))
			process_cmd_packets(C, S, A);
	}

	
	if(S->pre_cfg.work_mode != WORK_MODE_COMMAND){
		if (FD_ISSET(S->gpio_dece.fd, readset)) {
			if(S->pre_cfg.work_mode == WORK_MODE_TRIGGER)
				trigger_to_read_tag(C, S, A);
			report_triggerstatus(C,S);//上传触发状态
		}
	}else{
		if (FD_ISSET(S->gpio_dece.fd, readset)) {
			unsigned char key_vals[2];
			if(read(S->gpio_dece.fd, key_vals, sizeof(key_vals)) < 0){
				log_ret("trigger_to_read_tag read()\n");
				return -1;
			} 
			S->gpio_dece.gpio1_val = key_vals[0];
			S->gpio_dece.gpio2_val = key_vals[1];
		}
	}	
	return 0;
}

static int work_mode_pre_config (r2h_connect_t *C, system_param_t *S, ap_connect_t *A) {
	struct sockaddr_in *paddr;

	paddr = &C->udp_client_addr;
	switch(S->pre_cfg.work_mode){
	case WORK_MODE_COMMAND:
		break;
	case WORK_MODE_AUTOMATIC:
		auto_read_tag(C, S, A);
	case WORK_MODE_TRIGGER:		
		if(S->pre_cfg.upload_mode == UPLOAD_MODE_UDP) {
			memset(&C->udp_client_addr, 0, sizeof(C->udp_client_addr));	
			C->udp_client_addr.sin_family = AF_INET;
			C->udp_client_addr.sin_port = htons(S->data_center.tcp_port);
			int err = inet_pton(AF_INET, S->data_center.ip, &paddr->sin_addr);
			if (err <= 0) {
				log_msg("inet_pton error");
				return -1;
			}
		}
		break;
	default:
		break;
	}

	return 0;
}	

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

	work_mode_pre_config(C,S,A);

	while (1) {
		int maxfd = 0;
		fd_set readset, writeset;
		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		maxfd = set_select_para(C,S,A,&readset,&writeset);

		int err = select(maxfd+1, &readset, &writeset, NULL, NULL);
		if (err < 0) {
			log_ret("select error");
		} else if (err == 0) {
			continue;
		}
		
		tag_operation(C,S,A,&readset,&writeset);
		
		timer_operation(C,S,A,&readset,&writeset);
		
		conn_operation(C,S,A,&readset,&writeset);
	}
	return 0;
}
