#include "r2h_connect.h"
#include "connect.h"

#include <stdlib.h>
#include <string.h>

r2h_connect_t *r2h_connect_new(system_param_t *S)
{
	r2h_connect_t *C = malloc(sizeof(r2h_connect_t));
	if (NULL == C) {
		log_quit("malloc error");
	}

	memset(C, 0, sizeof(r2h_connect_t));
	C->accepted = false;
	C->recv.rlen = 0;
	C->send.wlen = 0;
	C->conn_type = R2H_NONE;
	C->wifi_transparent_mode = false;
	C->status_cnt = 6;
	C->time = 0;
	C->triger_confirm_flag = false;
	C->wifi_connect = false;
	C->tcp_send_symbol = SEND_TYPE_RAM;
	C->set_delay_timer_flag = 0;
	C->set_start_timer_cnt = 0;
	C->set_delay_timer_cnt = 0;
	S->action_status.status_1 = 0;
	S->action_status.status_2 = 0;
	S->action_status.status_3 = 0;
	S->action_status.status_4 = 0;
	S->action_status.status_5 = 0;
	S->action_status.action_flag = false;
	S->action_status.first_in = 0;
	S->action_status.report_status = 0xff;

	return C;
}

int r2h_connect_close_client(r2h_connect_t *C)
{
	C->accepted = false;
	C->recv.rlen = 0;
	C->send.wlen = 0;

	if (C->conn_type != R2H_NONE) {
		C->r2h[C->conn_type].close_client(C);
		C->conn_type = R2H_NONE;
	}

	return 0;
}

ssize_t r2h_connect_recv(r2h_connect_t *C, size_t nbytes)
{
	if (C->conn_type != R2H_NONE) {
		return C->r2h[C->conn_type].recv(C, C->recv.rbuf, nbytes);
	}

	return -1;
}

ssize_t r2h_connect_send(r2h_connect_t *C, size_t nbytes)
{
	if (C->conn_type != R2H_NONE) {
		return C->r2h[C->conn_type].send(C, C->send.wbuf, nbytes);
	}
	return -1;
}

int r2h_connect_check_in(r2h_connect_t *C, int conn_type)
{
	/* RS485只发不收 */
	if (conn_type == R2H_RS485) {
		rs232_flush(C->r2h[R2H_RS485].fd);
		return -R2H_RS485;
	}

	C->conn_type = conn_type;
	return C->r2h[conn_type].recv(C, C->recv.rbuf, R2H_BUF_SIZE);
}

int r2h_connect_init(r2h_connect_t *C, system_param_t *S)
{
	int err;
	err = r2h_tcp_init(C, S->eth0.tcp_port);
	err |= r2h_udp_init(C, S->eth0.udp_port);
	err |= r2h_rs232_init(C, S, baud_table[S->rs232.baud_rate]);
	err |= r2h_rs485_init(C, baud_table[S->rs485.baud_rate]);
	err |= r2h_usb_init(C, R2H_USB_BAUD_RATE);
	err |= r2h_wifi_init(C, S, 115200);
	err |= wiegand_init(C, S->pre_cfg.wg_pulse_width);
	err |= r2h_gprs_init(C, S);
	
	return err;
}
