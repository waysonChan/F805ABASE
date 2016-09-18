#ifndef _REPORT_TAG_H
#define _REPORT_TAG_H

#include "config.h"
#include "parameter.h"
#include "r2h_connect.h"
#include "ap_connect.h"
#include "list.h"



void tag_report_list_del(tag_report_t *tag_report);
int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag);
int gprs_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int report_tag_send_timer(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int report_tag_set_timer(ap_connect_t *A, uint32_t ms);
int report_tag_init(tag_report_t *tag_report);
int report_tag_reset(tag_report_t *tag_report);

int tag_storage_write(tag_t *ptag);
int tag_storage_fflush(void);
int tag_storage_read(tag_t *ptag);
int tag_storage_delete(bool all);
uint16_t tag_storage_get_cnt(void);
int tag_storage_init(void);
int heartbeat_timer_int(system_param_t *S);
int heartbeat_timer_trigger(r2h_connect_t *C,system_param_t *S);
int wifi_tag_send_header(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int report_triggerstatus(r2h_connect_t *C, system_param_t *S );
void recv_frame_identify(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int report_tag_confirm(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);



int triger_status_init(void);
int triger_status_write(char *buf);
int triger_status_delete(bool all);
int triggerstatus_timer_init(system_param_t *S);
int triggerstatus_timer_trigger(r2h_connect_t *C, system_param_t *S );
void send_triggerstatus(r2h_connect_t *C, system_param_t *S, const void *buf, size_t sz);
int triger_status_read(char *buf);
uint16_t triger_status_get_cnt(void);
int delay_timer_init(system_param_t *S);
int delay_timer_trigger(r2h_connect_t *C, system_param_t *S,ap_connect_t *A );
int delay_timer_set(system_param_t *S, int s);





#endif /* _REPORT_TAG_H */
