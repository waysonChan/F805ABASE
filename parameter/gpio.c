#include "gpio.h"
#include "utility.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/time.h>

static int btns_fd;
extern int IO_VAL[2];
static gpio_t gpio[] = {
	/* LED1_GREEN */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ERR/brightness")
	},
	/* LED1_RED */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ACT/brightness")
	},
	/* LED2_GREEN */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_NC4/brightness")
	},
	/* LED2_RED */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ANT3/brightness")
	},
	/* LED3_GREEN */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_NC3/brightness")
	},
	/* LED3_RED */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ANT2/brightness")
	},
	/* LED4_GREEN */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_NC2/brightness")
	},
	/* LED4_RED */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ANT1/brightness")
	},
	/* LED5_GREEN */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_NC1/brightness")
	},
	/* LED5_RED */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/LED_ANT0/brightness")
	},
	/* BEEP */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/BEEP/brightness")
	},
	/* RF_ANTE1 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/RF_ANT1/brightness")
	},
	/* RF_ANTE2 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/RF_ANT2/brightness")
	},	
	/* IO_OUT1 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/IO_OUT1/brightness")
	},	
	/* IO_OUT2 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/IO_OUT2/brightness")
	},	
	/* IO_OUT3 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/IO_OUT3/brightness")
	},	
	/* IO_OUT4 */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/IO_OUT4/brightness")
	},	
	/* R2000_RESET */
	{
		GPIO_BOARD_INFO(0, "/sys/devices/platform/leds-gpio/leds/R2000_RESET/brightness")
	},
};

static int beep_status = 0;

char get_gpio_status(gpio_index_e gpio_index)
{
	char c[12] = {0};

	assert(gpio_index >= 0 && gpio_index < MAX_GPIO_INDEX);

	/* 必要: 读操作会改变当前 fd 的偏移量 */
	if (lseek(gpio[gpio_index].fd, 0, SEEK_SET) < 0)
		return -1;

	if (read(gpio[gpio_index].fd, c, sizeof(c)) < 0)
		return -1;

	return c[0] == '0' ? 0 : 1;
}

int set_gpio_status(gpio_index_e gpio_index, char val)
{
	char c;

	assert(gpio_index >= 0 && gpio_index < MAX_GPIO_INDEX);

	c = val ? '1' : '0';
	return write(gpio[gpio_index].fd, &c, 1);
}

int get_active_antenna(void)
{
	char ante1, ante2;

	ante1 = get_gpio_status(RF_ANTE1);
	ante2 = get_gpio_status(RF_ANTE2);

	if (ante1 < 0 || ante2 < 0) {
		return -1;
	} else {
		return 4 - (ante1 << 1) - ante2;
	}
}

int set_active_antenna(system_param_t *S, int ant_index)
{
	int err1, err2;

	if ((S->pre_cfg.dev_type & DEV_TYPE_BASE_MASK) == DEV_TYPE_BASE_I802S_ANT4) {
		if (ant_index == 1) {
			ant_index = 4;
		} else if (ant_index == 4) {
			ant_index = 1;
		}
	}
	switch (ant_index) {
	case 1:
		err1 = set_gpio_status(RF_ANTE1, 1);
		err2 = set_gpio_status(RF_ANTE2, 1);
		break;
	case 2:
		err1 = set_gpio_status(RF_ANTE1, 1);
		err2 = set_gpio_status(RF_ANTE2, 0);
		break;
	case 3:
		err1 = set_gpio_status(RF_ANTE1, 0);
		err2 = set_gpio_status(RF_ANTE2, 1);
		break;
	case 4:
		err1 = set_gpio_status(RF_ANTE1, 0);
		err2 = set_gpio_status(RF_ANTE2, 0);
		break;
	default:
		log_msg("%s: Invalid Antenna Index!", __FUNCTION__);
		return -1;
	}

	if (err1 < 0 || err2 < 0) {
		return -1;
	}
	/*
	 * 更新参数读写器参数
	 */
	S->cur_ant = ant_index;
	set_antenna_led_status(ant_index, LED_COLOR_RED, S->pre_cfg.dev_type);

	return 0;
}

/*
 * 返回0表示所有天线都被设置为禁止读卡
 */
static inline int get_next_enabled_ant(system_param_t *S, int cur_ant_index)
{
	int tmp, next_ante_index = 0, i = 4;

	tmp = (cur_ant_index & 0x3) + 1;

	do {
		if (S->ant_array[tmp-1].enable) {
			next_ante_index = tmp;
			break;
		} else {
			tmp = (tmp & 0x3) + 1;
		}
	} while (--i);

	return next_ante_index;
}

int trigger_set_next_antenna (r2h_connect_t *C, system_param_t *S, ap_connect_t *A) {
	int err = 0;
	int next_ant_index,next_ant,i;
	struct timeval now;
	gettimeofday(&now, NULL);
	int cur_ant = get_active_antenna() - 1;	
	int ms = msec_between(S->last_ant_change_time, now);

	if (ms < 100) {
		err = -1;
		goto out;
	} else {		
		S->last_ant_change_time = now;
		C->ant_trigger.total_timer_cnt++;
		C->ant_trigger.antenna_cnt[cur_ant]++;
	}


	//stop this event  
	if(C->ant_trigger.antenna_cnt[cur_ant] >= C->ant_trigger.total_timer
		|| C->set_delay_timer_cnt > S->extended_table[0]){
		for(i = 3; i >= 0; i--){
			if(S->ant_array[i].enable){
				if(C->ant_trigger.use_time[i] == 0)//continue mode 
					C->ant_trigger.current_able_ant |= 1<<i;
				else
					C->ant_trigger.current_able_ant &=  ~(1<<i);
			}
		}
		C->ant_trigger.total_timer_cnt = 0;
		//we must see which ant shuld be stop
		if(C->set_delay_timer_cnt > S->extended_table[0]){
			C->ant_trigger.current_able_ant = 0;
			C->set_delay_timer_cnt = 0;
			int i;
			for(i = 3; i >= 0; i--){
				if(S->ant_array[i].enable){
					switch(C->ant_trigger.trigger_bind_style[i]){
					case 0:
						break;
					case 1:
						if( S->gpio_dece.gpio1_val ){
							C->ant_trigger.current_able_ant |= 1<<i;
							S->cur_ant = i+1;
						}
						break;
					case 2:
						if( S->gpio_dece.gpio2_val ){
							C->ant_trigger.current_able_ant |= 1<<i;
							S->cur_ant = i+1;
						}
						break;
					case 3:
						if( S->gpio_dece.gpio1_val || S->gpio_dece.gpio2_val ){
							C->ant_trigger.current_able_ant |= 1<<i;
							S->cur_ant = i+1;
						}
						break;//any trigger
					default:
						break;
					}
				}
			}
			C->set_delay_timer_flag = 0;
			if(C->ant_trigger.current_able_ant == 0){
				err = -1;
				goto out;
			}
		}
	}
	
	
	if(C->ant_trigger.use_time[cur_ant] == 0){
		C->ant_trigger.antenna_cnt[cur_ant] = 0;
	} else {
		if( C->ant_trigger.antenna_cnt[cur_ant] >= C->ant_trigger.use_time[cur_ant]){//S->ant_array[cur_ant].switch_time
			C->ant_trigger.current_able_ant &=  ~(1<<cur_ant);
			C->ant_trigger.antenna_cnt[cur_ant] = 0;			
		}
	}
	
	//find next active ant
	next_ant = 1 << cur_ant;
	i = 3;
	do{
		if(next_ant == 0x08){
			next_ant = 1;
		} else {
			next_ant <<= 1;
		}
	}while(!(C->ant_trigger.current_able_ant & next_ant) && i--);

	switch(next_ant){
	case 1:
		next_ant_index = 1;
		break;
	case 2:
		next_ant_index = 2;
		break;
	case 4:
		next_ant_index = 3;
		break;
	case 8:
		next_ant_index = 4;
		break;
	default:
		next_ant_index = 0;//wrong ant
		break;
	}
	
	if(next_ant_index == 0){
		log_msg("no active ant\n");
		err = -1;
		goto out;			
	}
	/* 如果下一个天线等于当前天线则什么也不做 */
	if (next_ant_index == S->cur_ant) {
		set_antenna_led_status(S->cur_ant, LED_COLOR_RED, S->pre_cfg.dev_type);
		err = 0;
		goto out;
	}

	
out:	
	//switch ant
	if(err == 0){
		if(S->ant_array[S->cur_ant-1].enable)
			set_antenna_led_status(S->cur_ant, LED_COLOR_GREEN, S->pre_cfg.dev_type);
		else
			set_antenna_led_status(S->cur_ant, LED_COLOR_NONE, S->pre_cfg.dev_type);
		
		set_active_antenna(S, next_ant_index);	
	}
	return err;
}


int set_next_active_antenna(system_param_t *S)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	int ms = msec_between(S->last_ant_change_time, now);
	if (ms < S->ant_array[S->cur_ant-1].switch_time * 100) {
		return -1;
	} else {
		S->last_ant_change_time = now;
	}
	
	int next_ant_index = get_next_enabled_ant(S, S->cur_ant);
	if (next_ant_index == 0)
		return -1;

	/* 如果下一个天线等于当前天线则什么也不做 */
	if (next_ant_index == S->cur_ant) {
		set_antenna_led_status(S->cur_ant, LED_COLOR_RED, S->pre_cfg.dev_type);
		return 0;
	}

	if(S->ant_array[S->cur_ant-1].enable)
		set_antenna_led_status(S->cur_ant, LED_COLOR_GREEN, S->pre_cfg.dev_type);
	else
		set_antenna_led_status(S->cur_ant, LED_COLOR_NONE, S->pre_cfg.dev_type);
	
	set_active_antenna(S, next_ant_index);
	return 0;
}

/*
 * 根据天线号和LED颜色获取相应的GPIO引脚号
 */
static inline gpio_index_e get_gpio_index(int ant_index, led_color_e color)
{
	int err = -1;
	
	switch (ant_index) {
	case 1:
		err = (color == LED_COLOR_GREEN ? LED5_GREEN : LED5_RED);
		break;
	case 2:
		err = (color == LED_COLOR_GREEN ? LED4_GREEN : LED4_RED);
		break;
	case 3:
		err = (color == LED_COLOR_GREEN ? LED3_GREEN : LED3_RED);
		break;
	case 4:
		err = (color == LED_COLOR_GREEN ? LED2_GREEN : LED2_RED);
		break;
	default:
		log_msg("%s: Invalid Antenna Index!", __FUNCTION__);
		err = -1;
	}

	return err;
}

/*
 * 由于硬件接线的原因，I802S的天线指示灯都接的天线2的指示灯
 */
int set_antenna_led_status(int ant_index, led_color_e color, int dev_type)
{
	int err1 = -1;
	int err2 = -1;

	switch (dev_type & DEV_TYPE_BASE_MASK) {
	case DEV_TYPE_BASE_F805S:
		break;
	case DEV_TYPE_BASE_I802S_ANT4:
		if (ant_index == 4) {
			ant_index = 2;
		} else if (ant_index == 2) {
			ant_index = 4;
		}
		break;
	case DEV_TYPE_BASE_I802S_ANT1:
		if (ant_index == 1) {
			ant_index = 2;
		} else if (ant_index == 2) {
			ant_index = 1;
		}
		break;
	default:
		log_msg("set_antenna_led_status: invalid dev_type");
		return -1;
	}

	gpio_index_e green = get_gpio_index(ant_index, LED_COLOR_GREEN);
	gpio_index_e red = get_gpio_index(ant_index, LED_COLOR_RED);

	switch (color) {
	case LED_COLOR_NONE:
		err1 = set_gpio_status(green, 1);
		err2 = set_gpio_status(red, 1);
		break;
	case LED_COLOR_RED:
		err1 = set_gpio_status(green, 1);
		err2 = set_gpio_status(red, 0);			
		break;
	case LED_COLOR_GREEN:
		err1 = set_gpio_status(green, 0);
		err2 = set_gpio_status(red, 1);			
		break;
	case LED_COLOR_MIX:
		err1 = set_gpio_status(green, 0);
		err2 = set_gpio_status(red, 0);
		break;
	default:
		log_msg("%s: Invalid color!", __FUNCTION__);
		return -1;
	}

	return (err1 < 0 || err2 < 0) ? -1 : 0;
}

int beep(beep_action_e action)
{
	if (BEEP_OFF == action) {
		set_gpio_status(BEEP, 1);
	} else {
		set_gpio_status(BEEP, 0);
	}

	return 0;
}

int get_btns_status(uint8_t *btns_val)
{
/*	if (read(btns_fd, btns_val, MAX_GPI_NUM) < 0) {
		log_ret("read error");
		return -1;
	}
*/
/*
	btns_val[0] = IO_VAL[0];
	btns_val[1] = IO_VAL[1];
	
*/
	return 0;
}

int gpo_pulse_timer_trigger(gpo_index_e gpo_idx, int pulse_timer)
{
	uint64_t num_exp;
	if (read(pulse_timer, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_ret("gpo_pulse_timer_trigger read()");
		return -1;
	}

	switch (gpo_idx) {
	case GPO_IDX_BEEP:
		if (beep_status == 1) {
			beep_status = 2;
			set_gpio_status(BEEP, GPO_OUTPUT_HIGH);
		} else if (beep_status == 3) {
			beep_status = 0;
		}
		break;
	case GPO_IDX_1:
		set_gpio_status(IO_OUT1, GPO_OUTPUT_HIGH);
		break;
	case GPO_IDX_2:
		set_gpio_status(IO_OUT2, GPO_OUTPUT_HIGH);
		break;
	case GPO_IDX_3:
		set_gpio_status(IO_OUT3, GPO_OUTPUT_HIGH);
		break;
	case GPO_IDX_4:
		set_gpio_status(IO_OUT4, GPO_OUTPUT_HIGH);
		break;
	case GPO_IDX_BLINK:
		set_gpio_status(LED1_GREEN, GPO_OUTPUT_HIGH);
		break;
	default:
		log_msg("invalid gpo index");
		return -1;
	}

	return 0;
}

/* 注意:脉冲只触发一次 */
int gpo_pulse_set_timer(gpo_t *gpo)
{
	uint32_t ms = gpo->pulse_width * 100;
	struct itimerspec its = {
		.it_interval.tv_sec = 0,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = ms / 1000,
		.it_value.tv_nsec = (ms % 1000) * 1000000,
	};

	if (timerfd_settime(gpo->pulse_timer, 0, &its, NULL) < 0) {
		log_ret("timerfd_settime");
		return -1;
	}

	gpo->pulse_its = its;
	return 0;
}

int gpo_pulse_timer_init(gpo_t *gpo)
{
	int i;
	for (i = 0; i < GPO_NUMBER; i++) {
		gpo[i].pulse_timer = timerfd_create(CLOCK_REALTIME, 0);
		if (gpo[i].pulse_timer < 0) {		
			log_ret("timerfd_create error");
			return -1;
		}

		bzero(&gpo[i].pulse_its, sizeof(struct itimerspec));
		if (timerfd_settime(gpo[i].pulse_timer, 0, &gpo[i].pulse_its, NULL) < 0) {
			log_ret("timerfd_settime error");
			close(gpo[i].pulse_timer);
			return -1;
		}
	}

	return 0;
}

int beep_and_blinking(gpo_t *gpo)
{
	if (gpo[GPO_IDX_BEEP].pulse_width) {
		if (beep_status == 0) {
			beep_status = 1;
			set_gpio_status(BEEP, GPO_OUTPUT_LOW);
			gpo_pulse_set_timer(&gpo[GPO_IDX_BEEP]);
		} else if (beep_status == 2) {
			beep_status = 3;
			gpo_pulse_set_timer(&gpo[GPO_IDX_BEEP]);
		}
	}

	if (gpo[GPO_IDX_1].pulse_width) {
		set_gpio_status(IO_OUT1, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&gpo[GPO_IDX_1]);
	}

	if (gpo[GPO_IDX_2].pulse_width) {
		set_gpio_status(IO_OUT2, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&gpo[GPO_IDX_2]);
	}

	if (gpo[GPO_IDX_3].pulse_width) {
		set_gpio_status(IO_OUT3, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&gpo[GPO_IDX_3]);
	}

	if (gpo[GPO_IDX_4].pulse_width) {
		set_gpio_status(IO_OUT4, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&gpo[GPO_IDX_4]);
	}

	if (gpo[GPO_IDX_BLINK].pulse_width) {
		set_gpio_status(LED1_GREEN, GPO_OUTPUT_LOW);
		gpo_pulse_set_timer(&gpo[GPO_IDX_BLINK]);
	}

	return 0;
}

int gpio_init(void)
{
	int i;

	for (i = 0; i < MAX_GPIO_INDEX; i++) {
		gpio[i].fd = open(gpio[i].file_name, O_RDWR);
		if (gpio[i].fd < 0) {
			log_msg("%s: open() ERR!", __FUNCTION__);
			return -1;
		}
	}

	btns_fd = open("/dev/buttons", O_RDWR);
	if (btns_fd < 0) {
		log_ret("open error");
		return -1;
	}
	
	return btns_fd;
}
