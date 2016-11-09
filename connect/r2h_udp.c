#include "r2h_connect.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t r2h_udp_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	while (n_read < nbytes) {
		ret = recvfrom(C->r2h[R2H_UDP].fd, buf+n_read, nbytes-n_read, 0, 
			(struct sockaddr *)&C->udp_client_addr, &addr_len);
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

static ssize_t r2h_udp_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = sendto(C->r2h[R2H_UDP].fd, buf+n_written, nbytes-n_written, 0, 
			(struct sockaddr *)&C->udp_client_addr, sizeof(struct sockaddr_in));
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

#define R2H_UDP_PORT		7088
static int r2h_udp_open(r2h_connect_t *C, int udp_port)
{
	C->r2h[R2H_UDP].fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (C->r2h[R2H_UDP].fd < 0) {
		log_ret("r2h_udp_open: socket error");
		return C->listener;
	}

	int err = fcntl(C->r2h[R2H_UDP].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_udp_open: fcntl error");
		return err;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_port = htons(R2H_UDP_PORT);
	sin.sin_port = htons(udp_port);

	err = bind(C->r2h[R2H_UDP].fd, (struct sockaddr *)&sin, sizeof(sin));
	if (err < 0) {
		log_ret("r2h_udp_open: bind error");
		return err;
	}

	return err;
}

static int r2h_udp_close_client(r2h_connect_t *C)
{
	return 0;
}

int r2h_udp_init(r2h_connect_t *C, int udp_port)
{
	C->r2h[R2H_UDP].open = r2h_udp_open;
	C->r2h[R2H_UDP].close_client = r2h_udp_close_client;
	C->r2h[R2H_UDP].recv = r2h_udp_recv;
	C->r2h[R2H_UDP].send = r2h_udp_send;

	return r2h_udp_open(C, udp_port);
}
