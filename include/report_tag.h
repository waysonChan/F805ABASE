#ifndef _REPORT_TAG_H
#define _REPORT_TAG_H

#include "config.h"
#include "parameter.h"
#include "r2h_connect.h"
#include "ap_connect.h"

int report_tag_send(r2h_connect_t *C, system_param_t *S, ap_connect_t *A, tag_t *ptag);
int report_tag_send_timer(r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int report_tag_set_timer(ap_connect_t *A, uint32_t ms);
int report_tag_init(tag_report_t *tag_report);
int report_tag_reset(tag_report_t *tag_report);

int tag_storage_write(tag_t *ptag);
int tag_storage_read(tag_t *ptag);
int tag_storage_delete(void);
int tag_storage_init(void);

#endif /* _REPORT_TAG_H */
