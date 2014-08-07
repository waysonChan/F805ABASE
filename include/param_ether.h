#ifndef _PARAMETER_CONFIG_H
#define _PARAMETER_CONFIG_H

int ether_set_ip_addr(uint32_t ip_addr);
int ether_get_ip_addr(uint8_t *ip);

int ether_set_net_mask(uint32_t net_mask);
int ether_get_net_mask(uint8_t *mask);

int ether_set_gate_way(uint32_t gate_way);
int ether_get_gate_way(uint8_t *gate);

int ether_set_ip_config(const uint8_t *ptr);

int ether_get_mac_addr(uint8_t *pmac);
int ether_set_mac_addr(const uint8_t *mac_addr);

#endif	/* _PARAMETER_CONFIG_H */
