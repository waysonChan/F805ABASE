#include "config.h"
#include "param_ether.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define EHTER_NET_INTERFACE	"eth0"

/*---------------------------------------------------------------------
 *	获取/设置 IP 地址
 *--------------------------------------------------------------------*/
int ether_get_ip_addr(uint8_t *ip)
{
	int sock, err = 0;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	
	err = ioctl(sock, SIOCGIFADDR, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

	memcpy(ip, &((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr, 4);

out:
	close(sock);
	return err;
}

int ether_set_ip_addr(uint32_t ip_addr)
{
	char ip[20];
	int sock, err = 0;
	uint32_t local_ip_addr;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == ip_addr) {
		log_msg("%s: parameter invalid, ip_addr = %d.", __FUNCTION__, ip_addr);
		return 0;
	}

	local_ip_addr = ntohl(ip_addr);
	sprintf(ip, "%d.%d.%d.%d", (local_ip_addr & 0xff000000) >> 24, 
					(local_ip_addr & 0xff0000) >> 16, 
					(local_ip_addr & 0xff00) >> 8, 
					(local_ip_addr & 0xff));
	log_msg("%s: new ip = %s", __FUNCTION__, ip);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}
	/*
	 * 初始化 struct sockaddr_in
	 */
	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(ip, &sa_in.sin_addr);/* 将输入字符串转成网络地址 */
	/*
	 * 初始化 struct ifreq
	 */
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));
	/*
	 * 设置 IP 地址
	 */
	err = ioctl(sock, SIOCSIFADDR, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

out:
	close(sock);
	return err;
}

/*---------------------------------------------------------------------
 *	获取/设置子网掩码
 *--------------------------------------------------------------------*/
int ether_get_net_mask(uint8_t *mask)
{
	int sock, err = 0;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	
	err = ioctl(sock, SIOCGIFNETMASK, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

	memcpy(mask, &((struct sockaddr_in*)&(ifr.ifr_netmask))->sin_addr, 4);

out:
	close(sock);
	return err;
}

int ether_set_net_mask(uint32_t net_mask)
{
	int sock, err = 0;
	char nm[20];
	uint32_t local_net_mask;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == net_mask) {
		log_msg("%s: parameter invalid, net_mask = %d.", __FUNCTION__, net_mask);
		return 0;
	}

	local_net_mask = ntohl(net_mask);
	sprintf(nm, "%d.%d.%d.%d", (local_net_mask & 0xff000000) >> 24, 
					(local_net_mask & 0xff0000) >> 16, 
					(local_net_mask & 0xff00) >> 8, 
					(local_net_mask & 0xff));
	log_msg("%s: new netmask = %s", __FUNCTION__, nm);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}
	/*
	 * 初始化 struct sockaddr_in
	 */
	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(nm, &sa_in.sin_addr);	/* 将输入字符串转成网络地址 */
	/*
	 * 初始化 struct ifreq
	 */
	memset(&ifr, 0, sizeof(struct ifreq));	
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));
	/*
	 * 设置子网掩码
	 */
	err = ioctl(sock, SIOCSIFNETMASK, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

out:
	close(sock);
	return err;
}

/*---------------------------------------------------------------------
 *	获取/设置网关
 *--------------------------------------------------------------------*/
int ether_get_gate_way(uint8_t *gate)
{
	char devname[64];
	unsigned long d, m, g = 0;
	int flgs, ref, use, metric, mtu, win, ir, err = 0;
	FILE *fp = fopen("/proc/net/route", "r");

	/* 跳过第一行 */
	err = fscanf(fp, "%*[^\n]\n");
	if (err < 0) {
		goto out;
	}

	for (;;) {
		int ret = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
					devname, &d, &g, &flgs, &ref, &use, &metric, &m,
					&mtu, &win, &ir);
		if (ret != 11) {
			err = -1;
			goto out;
		}
		if (flgs == 0x3) {
			memcpy(gate, &g, 4);
			break;
		}
	}

out:
	fclose(fp);
	return err;
}

int ether_set_gate_way(uint32_t gate_way)
{
	int sock, err = 0;
	struct rtentry rt;

	if (0 == gate_way) {
		log_msg("%s: parameter invalid, gate_way = %d.", __FUNCTION__, gate_way);
		return 0;
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}
	/*
	 * 删除现有的默认网关
	 */
	memset(&rt, 0, sizeof(struct rtentry));
	rt.rt_dst.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;
	
	rt.rt_genmask.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

	rt.rt_flags = RTF_UP;

	err = ioctl(sock, SIOCDELRT, &rt);
	if (0 != err && ESRCH != errno) {
		log_msg("%s: ioctl() ERR1, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}
	/*
	 * 设置默认网关
	 */
	memset(&rt, 0, sizeof(struct rtentry));

	rt.rt_dst.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;

	rt.rt_gateway.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr = gate_way;

	rt.rt_genmask.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

	rt.rt_flags = RTF_UP | RTF_GATEWAY;

	err = ioctl(sock, SIOCADDRT, &rt);
	if (err < 0 && ESRCH != errno) {
		log_msg("%s: ioctl() ERR2, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

out:
	close(sock);
	return err;
}

int ether_set_ip_config(const uint8_t *ptr)
{
	/* ip */
	int ip_addr;
	memcpy(&ip_addr, ptr, 4);
	if (ether_set_ip_addr(ip_addr)) {
		log_msg("ether_set_ip_addr error");
		return -1;
	}

	/* mask */
	int net_mask;
	memcpy(&net_mask, ptr+4, 4);
	if (ether_set_net_mask(net_mask)) {
		log_msg("ether_set_net_mask error");
		return -1;
	}

	/* gateway */
	int gate_way;
	memcpy(&gate_way, ptr+8, 4);
	if (ether_set_gate_way(gate_way)) {
		log_msg("ether_set_gate_way error");
		return -1;
	}

	return 0;
}

/*---------------------------------------------------------------------
 *	获取/设置 MAC 地址
 *--------------------------------------------------------------------*/
int ether_get_mac_addr(uint8_t *pmac)
{
	int i, sock, err = 0;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	err = ioctl(sock, SIOCGIFHWADDR, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

	for (i = 0; i < 6; i++) {
		pmac[i] = (uint8_t)ifr.ifr_hwaddr.sa_data[i];
	}

out:
	close(sock);
	return err;
}

int ether_set_mac_addr(const uint8_t *mac_addr)
{
	int sock, err = 0;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_msg("%s: socket() ERR, <%s>!", __FUNCTION__, strerror(errno));
		return -1;
	}
	/*
	 * 1.关闭接口
	 */
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	err = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

	if (ifr.ifr_flags & IFF_UP) {
		ifr.ifr_flags &= ~IFF_UP;
		err = ioctl(sock, SIOCSIFFLAGS, &ifr);
		if(err < 0) {
			log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
			goto out;
		}
	}
	/*
	 * 2.设置 MAC 地址
	 */
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(&ifr.ifr_hwaddr.sa_data, mac_addr, 6);

	err = ioctl(sock, SIOCSIFHWADDR, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}
	/*
	 * 3.开启接口
	 */
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, EHTER_NET_INTERFACE, IFNAMSIZ);
	err = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (err < 0) {
		log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
		goto out;
	}

	if (!(ifr.ifr_flags & IFF_UP)) {
		ifr.ifr_flags |= IFF_UP;
		err = ioctl(sock, SIOCSIFFLAGS, &ifr);
		if(err < 0) {
			log_msg("%s: ioctl() ERR, <%s>!", __FUNCTION__, strerror(errno));
			goto out;
		}
	}

out:
	close(sock);
	return err;
}
