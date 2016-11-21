#ifndef _GPIO_H
#define _GPIO_H

#include "config.h"
#include "parameter.h"
#include "ap_connect.h"
#include "r2h_connect.h"

#define MAX_GPIO_NUM		20
#define MAX_GPIO_NAME_LENGTH	79
#define MAX_GPI_NUM		2

#define GPO_OUTPUT_LOW		0
#define GPO_OUTPUT_HIGH		1

typedef struct _gpio {
	int fd;
	char status; 
	char file_name[MAX_GPIO_NAME_LENGTH];
}gpio_t, *gpio_pt;

/*
 * ע��: 
 *   1. LED6�ı�����ű����͵�GND,�����ϵ��ֻ��һֱΪ��
 *   2. E_GpioIndex�����ŵı����g_atGpio[]�����е�Ԫ��һһ��Ӧ
 *   3. Ϊ�������LED,E_GpioIndexǰ������ű����E_LedIndex�����߶�Ӧ
 */

typedef enum {
	LED1_GREEN,		/* ϵͳ״̬�Ʊ���: LED_ERR */
	LED1_RED,		/* ϵͳ״̬�Ʊ��: LED_ACT */
	LED2_GREEN, 		/* Ϊ��ʱ����4����: LED_NC4 */
	LED2_RED,		/* Ϊ��ʱ����4���: LED_ANT3 */
	LED3_GREEN, 		/* Ϊ��ʱ����3����: LED_NC3 */
	LED3_RED,		/* Ϊ��ʱ����3���: LED_ANT2 */
	LED4_GREEN, 		/* Ϊ��ʱ����2����: LED_NC2 */
	LED4_RED,		/* Ϊ��ʱ����2���: LED_ANT1 */
	LED5_GREEN,		/* Ϊ��ʱ����1����: LED_NC1 */
	LED5_RED,		/* Ϊ��ʱ����1���: LED_ANT0 */
	BEEP,			/* ������ */
	RF_ANTE1,		/* �����л�����1 */
	RF_ANTE2,		/* �����л�����2 */
	IO_OUT1,
	IO_OUT2,
	IO_OUT3,
	IO_OUT4,
	R2000_RESET,
	MAX_GPIO_INDEX
} gpio_index_e;

typedef enum {
	LED_COLOR_NONE,		/* ��ɫ */
	LED_COLOR_RED,		/* ��ɫ */
	LED_COLOR_GREEN,	/* ��ɫ */
	LED_COLOR_MIX		/* ��ɫ */
} led_color_e;

typedef enum {
	BEEP_OFF,		/* ���������� */
	BEEP_ON			/* �������� */
} beep_action_e;

#define GPIO_BOARD_INFO(sts, name) \
	.status = (sts), .file_name = (name)

char get_gpio_status(gpio_index_e gpio_index);
int set_gpio_status(gpio_index_e gpio_index, char val);
int get_active_antenna(void);
int set_active_antenna(system_param_t *S, int ant_index);
int trigger_set_next_antenna (r2h_connect_t *C, system_param_t *S, ap_connect_t *A);
int set_next_active_antenna(system_param_t *S);
int set_antenna_led_status(int ant_index, led_color_e color, int dev_type);
int beep(beep_action_e action);
int get_btns_status(uint8_t *btns_val);
int gpo_pulse_timer_trigger(gpo_index_e gpo_idx, int pulse_timer);
int gpo_pulse_set_timer(gpo_t *gpo);
int gpo_pulse_timer_init(gpo_t *gpo);
int beep_and_blinking(gpo_t *beep);
int gpio_init(void);

#endif	/* _GPIO_H */
