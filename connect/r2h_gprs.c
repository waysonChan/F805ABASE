#include "r2h_connect.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <errno.h>

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t _r2h_gprs_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = recv(C->r2h[R2H_GPRS].fd, buf+n_read, nbytes-n_read, 0);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
		return 0;
	} else if (ret < 0) {
		if (errno == EAGAIN)
			return n_read;
		return -1;
	}

	assert(n_read != 0);
	return n_read;
}

int gprs_cheek_connection(r2h_connect_t *C)
{
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	return getpeername(C->r2h[R2H_GPRS].fd, &addr, &addrlen);
}

static int _addr_init(struct sockaddr_in *paddr, data_center_t *data_center)
{
	memset(paddr, 0, sizeof(struct sockaddr_in));
	paddr->sin_family = AF_INET;
	paddr->sin_port = htons(data_center->tcp_port);

	log_msg("tcp_port = %d, ip = %s", data_center->tcp_port, data_center->ip);

	int err = inet_pton(AF_INET, data_center->ip, &paddr->sin_addr);
	if (err <= 0) {
		log_msg("inet_pton error");
		return -1;
	}

	return 0;
}

static int _r2h_gprs_close(r2h_connect_t *C)
{
	C->gprs_priv.connected = false;
	close(C->r2h[R2H_GPRS].fd);
	C->r2h[R2H_GPRS].fd = -1;
	return 0;
}

static ssize_t _gprs_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = send(C->r2h[R2H_GPRS].fd, buf+n_written, nbytes-n_written, 0);
		if (ret < 0) {
			if (errno == EAGAIN) {
				return n_written;
			}

			log_ret("send error");
			return -1;
		}
		assert(ret != 0);
		
		n_written += ret;
	}

	return n_written;
}

static int _gprs_connect_try(r2h_connect_t *C)
{
	C->r2h[R2H_GPRS].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (C->r2h[R2H_GPRS].fd < 0) {
		log_ret("socket error");
		return -1;
	}

	if (fcntl(C->r2h[R2H_GPRS].fd, F_SETFL, O_NONBLOCK) < 0) {
		log_ret("tcp_upload_connect_try: fcntl error");
		goto out;
	}

	if (connect(C->r2h[R2H_GPRS].fd, (struct sockaddr *)&C->gprs_priv.server_addr,
		sizeof(struct sockaddr_in)) < 0) {
		if (errno == EINPROGRESS) {
			/* nonblocking and the connection cannot be completed immediately */
			C->gprs_priv.connect_in_progress = true;
			return 0;
		}
		
		log_ret("connect error");
		goto out;
	}

	C->gprs_priv.connected = true;
	C->conn_type = R2H_GPRS;
	log_msg("tcp upload connect successfully");
	return 0;

out:
	_r2h_gprs_close(C);
	return -1;
}

static int _r2h_gprs_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	int err = -1;
	gprs_priv_t *gprs_priv = &C->gprs_priv;

	if (gprs_priv->connected) {
		err = _gprs_send(C, buf, nbytes);
		if (err < 0)
			_r2h_gprs_close(C);
	}

	return err;
}

void r2h_gprs_conn_check(r2h_connect_t *C)
{
	int optval, ret;
	socklen_t optlen = sizeof(optval);
	
	ret = getsockopt(C->r2h[R2H_GPRS].fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (ret < 0 || optval != 0) {
		log_msg("main: gprs upload connect fail!");
		_r2h_gprs_close(C);
		if (optval)
			errno = optval;
	} else{
		C->gprs_priv.connected = true;
		C->conn_type = R2H_GPRS;
		log_msg("main: gprs upload connect successfully");
	}
	
	C->gprs_priv.connect_in_progress = false;
}

static int r2h_gprs_timer_init(gprs_priv_t *gprs_priv)
{
	gprs_priv->gprs_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (gprs_priv->gprs_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	}

	struct itimerspec its = {
		.it_interval.tv_sec = 10,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = 10,
		.it_value.tv_nsec = 0,
	};

	if (timerfd_settime(gprs_priv->gprs_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(gprs_priv->gprs_timer);
		return -1;
	}

	return 0;
}

int r2h_gprs_timer_trigger(r2h_connect_t *C, system_param_t *S)
{
	uint64_t num_exp;
	if (read(C->gprs_priv.gprs_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("r2h_gprs_timer_trigger read()");
		return -1;
	}

	if(S->pre_cfg.work_mode == WORK_MODE_COMMAND)
		return 0;
	/* C->conn_type = R2H_GPRS */
	if (!C->gprs_priv.connected){
		_r2h_gprs_close(C);
		_gprs_connect_try(C);
	}

	return 0;
}

int r2h_gprs_init(r2h_connect_t *C, system_param_t *S)
{
	int err = _addr_init(&C->gprs_priv.server_addr, &S->data_center);
	if (err < 0) {
		return -1;
	}

	C->r2h[R2H_GPRS].open = NULL;
	C->r2h[R2H_GPRS].close_client = _r2h_gprs_close;
	C->r2h[R2H_GPRS].recv = _r2h_gprs_recv;
	C->r2h[R2H_GPRS].send = _r2h_gprs_send;

	C->gprs_priv.connect_in_progress = false;
	C->gprs_priv.gprs_fail_cnt = 0;
	C->gprs_priv.gprs_wait_flag = false;

	/* GPRS是否自动上传是由设备类型决定的 */
	if (S->pre_cfg.dev_type & DEV_TYPE_FLAG_GPRS) {
		_gprs_connect_try(C);
		r2h_gprs_timer_init(&C->gprs_priv);
	} else {
		C->r2h[R2H_GPRS].fd = -1;
	}
	
	return 0;
}
