#ifndef _R2H_CONNECTION_H
#define _R2H_CONNECTION_H

#include "config.h"
#include "parameter.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define R2H_BUF_SIZE		4096
#define R2H_PARAM_BUF_SIZE	255

typedef struct {
	uint8_t last_byte;
	uint8_t frame_state;	/* 帧状态机:指示帧接收的状态 */
	uint8_t ctrl_field;
	uint8_t bus_addr;
	uint8_t param_index;
	uint8_t param_len;
	uint8_t remain_len;
	uint8_t cmd_id;
	uint8_t param_buf[R2H_PARAM_BUF_SIZE];
	uint16_t crc_val;
} r2h_frame_t;

typedef struct {
	size_t bytes_need;
	size_t rlen;
	uint8_t rbuf[R2H_BUF_SIZE];
	r2h_frame_t frame;
} r2h_recv_t;

typedef struct {
	uint8_t wbuf[R2H_BUF_SIZE];
	size_t wlen;
} r2h_send_t;


typedef struct r2h_connect r2h_connect_t;
typedef int (*r2h_open)(r2h_connect_t *C, int arg);
typedef int (*r2h_close_client)(r2h_connect_t *C);
typedef ssize_t (*r2h_recv)(r2h_connect_t *C, uint8_t *buf, size_t nbytes);
typedef ssize_t (*r2h_send)(r2h_connect_t *C, uint8_t *buf, size_t nbytes);

typedef struct {
	int fd;
	r2h_open open;
	r2h_close_client close_client;
	r2h_recv recv;
	r2h_send send;
} r2h_t;

#define SEND_TYPE_RAM	1
#define SEND_TYPE_NAND	2

typedef struct {
	bool connected;
	bool connect_in_progress;
	int tcp_port;
	int gprs_timer;	
	struct sockaddr_in server_addr;
	bool gprs_wait_flag;	/* 等待上位机确认接收标签指令 */
	int gprs_fail_cnt;	/* 连续发送标签失败次数 */
	int gprs_send_type;	/* 发送的标签在 RAM 还是 NAND */
} gprs_priv_t;

typedef struct {
	bool connected;
	bool connect_in_progress;
	int tcp_port;
	int wifi_timer;	
	struct sockaddr_in server_addr;
	bool wifi_wait_flag;
	int wifi_fail_cnt;	
	int wifi_send_type;	
} wifi_priv_t;

typedef struct {
	bool connected;
	bool connect_in_progress;
	int tcp_port;
	int timer;	
	struct sockaddr_in server_addr;
	bool wait_flag;
	int fail_cnt;	
	int send_type;	
} total_priv_t;

#define UPLOAD_MODE_NONE	0x00
#define UPLOAD_MODE_RS232	0x01
#define UPLOAD_MODE_RS485	0x02
#define UPLOAD_MODE_WIEGAND	0x03
#define UPLOAD_MODE_WIFI	0x04
#define UPLOAD_MODE_GPRS	0x05
#define UPLOAD_MODE_TCP		0x06
#define UPLOAD_MODE_UDP		0x07


#define R2H_NONE	-1
#define R2H_TCP		0
#define R2H_UDP		1
#define R2H_RS232	2
#define R2H_RS485	3
#define R2H_USB		4
#define R2H_WIFI	5
#define R2H_GPRS	6
#define R2H_TOTAL	7
struct r2h_connect {
	bool wifi_transparent_mode;		/* wifi 透传模式 */
	bool accepted;		/* TCP专用 */
	int listener;		/* TCP专用 */
	bool wifi_connect;
	r2h_recv_t recv;
	r2h_send_t send;
	int conn_type;
	int wg_fd;
	int count;
	int time;
	int tmp_send_len;
	int set_delay_timer_flag;
	uint8_t tmp_send_data[256];	/* EPC码 */
	char status[10];
	int status_cnt;
	bool status_send_from_file;
	bool triger_confirm_flag;
	char status_buf[10];
	bool flag;
	gprs_priv_t gprs_priv;
	wifi_priv_t wifi_priv;
	total_priv_t total_priv;
	struct sockaddr_in udp_client_addr;	/* UDP专用 */
	r2h_t r2h[R2H_TOTAL];
};

int r2h_tcp_accept(r2h_connect_t *C);
int r2h_tcp_init(r2h_connect_t *C, int tcp_port);
int r2h_udp_init(r2h_connect_t *C, int udp_port);
int r2h_rs232_init(r2h_connect_t *C, system_param_t *S, int baud_rate);
ssize_t r2h_rs232_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes);
int r2h_rs485_init(r2h_connect_t *C, int baud_rate);
ssize_t r2h_rs485_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes);
r2h_connect_t *r2h_connect_new(system_param_t *S);
int r2h_connect_close_client(r2h_connect_t *C);
ssize_t r2h_connect_recv(r2h_connect_t *C, size_t nbytes);
ssize_t r2h_connect_send(r2h_connect_t *C, size_t nbytes);
int r2h_connect_check_in(r2h_connect_t *C, int conn_type);
int r2h_connect_init(r2h_connect_t *C, system_param_t *S);

ssize_t wiegand_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes);
int wiegand_init(r2h_connect_t *C, int pulse_width);

#define R2H_USB_BAUD_RATE      115200
int r2h_usb_init(r2h_connect_t *C, int baud_rate);
int r2h_wifi_init(r2h_connect_t *C, system_param_t *S, int baud_rate);

ssize_t r2h_wifi_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes);
ssize_t r2h_wifi_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes);

int gprs_cheek_connection(r2h_connect_t *C);
void r2h_gprs_conn_check(r2h_connect_t *C);
int r2h_gprs_timer_trigger(r2h_connect_t *C);
int r2h_gprs_init(r2h_connect_t *C, system_param_t *S);

#endif	/* _R2H_CONNECTION_H */
