#include "r2h_connect.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t r2h_tcp_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = recv(C->r2h[R2H_TCP].fd, buf+n_read, nbytes-n_read, 0);
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

static ssize_t r2h_tcp_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = send(C->r2h[R2H_TCP].fd, buf+n_written, nbytes-n_written, 0);
		if (ret < 0) {
			if (errno == EAGAIN)
				return n_written;
			return -1;
		}
		assert(ret != 0);
		
		n_written += ret;
	}

	return n_written;
}

#define MAX_TCP_CONNECT		1
#define R2H_TCP_PORT		7086
static int r2h_tcp_open(r2h_connect_t *C, int tcp_port)
{
	C->listener = socket(AF_INET, SOCK_STREAM, 0);
	if (C->listener < 0) {
		log_ret("r2h_tcp_open: socket error");
		return C->listener;
	}

	int err = fcntl(C->listener, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_tcp_open: fcntl error");
		return err;
	}

	int optval;
	err = setsockopt(C->listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (err < 0) {
		log_ret("r2h_tcp_open: setsockopt error");
		return err;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_port = htons(R2H_TCP_PORT);
	sin.sin_port = htons(tcp_port);

	err = bind(C->listener, (struct sockaddr *)&sin, sizeof(sin));
	if (err < 0) {
		log_ret("r2h_tcp_open: bind error");
		return err;
	}

	err = listen(C->listener, MAX_TCP_CONNECT);
	if (err < 0) {
		log_ret("r2h_tcp_open: listen error");
		return err;
	}

	return 0;
}

static int r2h_tcp_close_client(r2h_connect_t *C)
{
	close(C->r2h[R2H_TCP].fd);
	C->r2h[R2H_TCP].fd = -1;
	return 0;
}

int r2h_tcp_accept(r2h_connect_t *C)
{
	struct sockaddr_in peer;
	socklen_t sock_len = sizeof(peer);

	C->r2h[R2H_TCP].fd = accept(C->listener, (struct sockaddr *)&peer, &sock_len);
	if (C->r2h[R2H_TCP].fd < 0) {
		log_ret("accept error");
		return C->r2h[R2H_TCP].fd;
	}

	int err = fcntl(C->r2h[R2H_TCP].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("LLRP_Conn_accept: fcntl error");
		return err;
	}

	C->accepted = true;
	return 0;
}

int r2h_tcp_init(r2h_connect_t *C, int tcp_port)
{
	C->r2h[R2H_TCP].open = r2h_tcp_open;
	C->r2h[R2H_TCP].close_client = r2h_tcp_close_client;
	C->r2h[R2H_TCP].recv = r2h_tcp_recv;
	C->r2h[R2H_TCP].send = r2h_tcp_send;

	return r2h_tcp_open(C, tcp_port);
}
