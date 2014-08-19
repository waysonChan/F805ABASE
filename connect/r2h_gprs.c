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
static ssize_t r2h_gprs_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
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

static int _addr_init(struct sockaddr_in *paddr, cfg_gprs_t *cfg_gprs)
{
	memset(paddr, 0, sizeof(struct sockaddr_in));
	paddr->sin_family = AF_INET;
	paddr->sin_port = htons(cfg_gprs->tcp_port);

	int err = inet_pton(AF_INET, cfg_gprs->ip, &paddr->sin_addr);
	if (err <= 0) {
		log_msg("inet_pton error");
		return -1;
	}

	return 0;
}

int r2h_gprs_close(r2h_connect_t *C)
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
			C->gprs_priv.in_progress = true;
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
	r2h_gprs_close(C);
	return -1;
}

static int r2h_gprs_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	int err = -1;
	gprs_priv_t *gprs_priv = &C->gprs_priv;

	if (gprs_priv->connected) {
		err = _gprs_send(C, buf, nbytes);
		if (err < 0)
			r2h_gprs_close(C);
	}

	return err;
}

static int r2h_gprs_timer_init(gprs_priv_t *gprs_priv)
{
	gprs_priv->gprs_timer = timerfd_create(CLOCK_REALTIME, 0);
	if (gprs_priv->gprs_timer < 0) {
		log_ret("timerfd_create error");
		return -1;
	}

	struct itimerspec its = {
		.it_interval.tv_sec = 3,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = 1,
		.it_value.tv_nsec = 0,
	};

	if (timerfd_settime(gprs_priv->gprs_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime error");
		close(gprs_priv->gprs_timer);
		return -1;
	}

	return 0;
}

int r2h_gprs_timer_trigger(r2h_connect_t *C)
{
	uint64_t num_exp;
	if (read(C->gprs_priv.gprs_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("work_status_timer_trigger read()");
		return -1;
	}

	if (!C->gprs_priv.connected && C->conn_type == R2H_NONE) {
		_gprs_connect_try(C);
	}

	return 0;
}

int r2h_gprs_init(r2h_connect_t *C, system_param_t *S)
{
	int err = _addr_init(&C->gprs_priv.server_addr, &S->cfg_gprs);
	if (err < 0) {
		return -1;
	}

	C->r2h[R2H_GPRS].open = NULL;
	C->r2h[R2H_GPRS].close_client = r2h_gprs_close;
	C->r2h[R2H_GPRS].recv = r2h_gprs_recv;
	C->r2h[R2H_GPRS].send = r2h_gprs_send;

	C->gprs_priv.in_progress = false;

	_gprs_connect_try(C);
	r2h_gprs_timer_init(&C->gprs_priv);
	return 0;
}
