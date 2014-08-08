#include "cfg_file.h"
#include <string.h>

#define CONFIG_FILE_NAME	"/f806/f806.cfg"

/*---------------------------------------------------------------------
 * sysinfo
 *--------------------------------------------------------------------*/
int cfg_get_sysinfo(sysinfo_pt sysinfo)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;
	const char *reader_name, *reader_type, *reader_sn, *mcu_swrev;
	const char *fpga_swrev, *bsb_hwrev, *rfb_hwrev, *pass_word;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "sysinfo");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_setting_lookup_string(s, "reader_name", &reader_name)
		&& config_setting_lookup_string(s, "reader_type", &reader_type)
		&& config_setting_lookup_string(s, "reader_sn", &reader_sn)
		&& config_setting_lookup_string(s, "mcu_swrev", &mcu_swrev)
		&& config_setting_lookup_string(s, "fpga_swrev", &fpga_swrev)
		&& config_setting_lookup_string(s, "bsb_hwrev", &bsb_hwrev)
		&& config_setting_lookup_string(s, "rfb_hwrev", &rfb_hwrev)
		&& config_setting_lookup_string(s, "pass_word", &pass_word))) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	strncpy(sysinfo->reader_name, reader_name, READER_NAME_LEN);
	strncpy(sysinfo->reader_type, reader_type, READER_TYPE_LEN);
	strncpy(sysinfo->reader_sn, reader_sn, PRODUCT_SN_LEN);
	strncpy(sysinfo->mcu_swrev, mcu_swrev, READER_MCU_SWREV_LEN);
	strncpy(sysinfo->fpga_swrev, fpga_swrev, READER_FPGA_SWREV_LEN);
	strncpy(sysinfo->bsb_hwrev, bsb_hwrev, READER_BSB_HWREV_LEN);
	strncpy(sysinfo->rfb_hwrev, rfb_hwrev, READER_RFB_SWREV_LEN);
	strncpy(sysinfo->pass_word, pass_word, PASSWORD_LEN);

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_sysinfo(sysinfo_pt sysinfo, cfg_sysinfo_e type)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (type) {
	case CFG_READER_NAME:
		s = config_lookup(&cfg, "sysinfo.reader_name");
		if (s) config_setting_set_string(s, sysinfo->reader_name);
		break;
	case CFG_READER_TYPE:
		s = config_lookup(&cfg, "sysinfo.reader_type");
		if (s) config_setting_set_string(s, sysinfo->reader_type);
		break;
	case CFG_READER_SN:
		s = config_lookup(&cfg, "sysinfo.reader_sn");
		if (s) config_setting_set_string(s, sysinfo->reader_sn);
		break;
	case CFG_MCU_SWREV:
		s = config_lookup(&cfg, "sysinfo.mcu_swrev");
		if (s) config_setting_set_string(s, sysinfo->mcu_swrev);
		break;
	case CFG_FPGA_SWREV:
		s = config_lookup(&cfg, "sysinfo.fpga_swrev");
		if (s) config_setting_set_string(s, sysinfo->fpga_swrev);
		break;
	case CFG_BSB_HWREV:
		s = config_lookup(&cfg, "sysinfo.bsb_hwrev");
		if (s) config_setting_set_string(s, sysinfo->bsb_hwrev);
		break;
	case CFG_RFB_HWREV:
		s = config_lookup(&cfg, "sysinfo.rfb_hwrev");
		if (s) config_setting_set_string(s, sysinfo->rfb_hwrev);
		break;
	case CFG_PASS_WORD:
		s = config_lookup(&cfg, "sysinfo.pass_word");
		if (s) config_setting_set_string(s, sysinfo->pass_word);
		break;
	default:
		log_msg("%s: invalid parameter!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * rs232
 *--------------------------------------------------------------------*/
int cfg_get_rs232(rs232_pt rs232)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "rs232");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_setting_lookup_int(s, "bus_addr", &rs232->bus_addr)
		&& config_setting_lookup_int(s, "baud_rate", &rs232->baud_rate)
		&& config_setting_lookup_int(s, "databits", &rs232->databits)
		&& config_setting_lookup_int(s, "stopbit", &rs232->stopbit)
		&& config_setting_lookup_int(s, "parity", &rs232->parity)
		&& config_setting_lookup_int(s, "flow_ctrl", &rs232->flow_ctrl))) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_rs232(rs232_pt rs232, cfg_rs232_e type)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (type) {
	case CFG_BUS_ADDR:
		s = config_lookup(&cfg, "rs232.bus_addr");
		if (s) config_setting_set_int(s, rs232->bus_addr);
		break;
	case CFG_BAUD_RATE:
		s = config_lookup(&cfg, "rs232.baud_rate");
		if (s) config_setting_set_int(s, rs232->baud_rate);
		break;
	case CFG_DATABITS:
		s = config_lookup(&cfg, "rs232.databits");
		if (s) config_setting_set_int(s, rs232->databits);
		break;
	case CFG_STOPBIT:
		s = config_lookup(&cfg, "rs232.stopbit");
		if (s) config_setting_set_int(s, rs232->stopbit);
		break;
	case CFG_PARITY:
		s = config_lookup(&cfg, "rs232.parity");
		if (s) config_setting_set_int(s, rs232->parity);
		break;
	case CFG_FLOW_CTRL:
		s = config_lookup(&cfg, "rs232.flow_ctrl");
		if (s) config_setting_set_int(s, rs232->flow_ctrl);
		break;
	default:
		log_msg("%s: invalid parameter!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * rs485
 *--------------------------------------------------------------------*/
int cfg_get_rs485(rs232_pt rs485)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "rs485");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_setting_lookup_int(s, "bus_addr", &rs485->bus_addr)
		&& config_setting_lookup_int(s, "baud_rate", &rs485->baud_rate)
		&& config_setting_lookup_int(s, "databits", &rs485->databits)
		&& config_setting_lookup_int(s, "stopbit", &rs485->stopbit)
		&& config_setting_lookup_int(s, "parity", &rs485->parity)
		&& config_setting_lookup_int(s, "flow_ctrl", &rs485->flow_ctrl))) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_rs485(rs232_pt rs485, cfg_rs232_e type)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (type) {
	case CFG_BUS_ADDR:
		s = config_lookup(&cfg, "rs485.bus_addr");
		if (s) config_setting_set_int(s, rs485->bus_addr);
		break;
	case CFG_BAUD_RATE:
		s = config_lookup(&cfg, "rs485.baud_rate");
		if (s) config_setting_set_int(s, rs485->baud_rate);
		break;
	case CFG_DATABITS:
		s = config_lookup(&cfg, "rs485.databits");
		if (s) config_setting_set_int(s, rs485->databits);
		break;
	case CFG_STOPBIT:
		s = config_lookup(&cfg, "rs485.stopbit");
		if (s) config_setting_set_int(s, rs485->stopbit);
		break;
	case CFG_PARITY:
		s = config_lookup(&cfg, "rs485.parity");
		if (s) config_setting_set_int(s, rs485->parity);
		break;
	case CFG_FLOW_CTRL:
		s = config_lookup(&cfg, "rs485.flow_ctrl");
		if (s) config_setting_set_int(s, rs485->flow_ctrl);
		break;
	default:
		log_msg("%s: invalid parameter!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * antenna
 *--------------------------------------------------------------------*/
int cfg_get_ant(antenna_t *ant, int ant_index)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (ant_index) {
	case 1:
		s = config_lookup(&cfg, "ant1");
		break;
	case 2:
		s = config_lookup(&cfg, "ant2");
		break;
	case 3:
		s = config_lookup(&cfg, "ant3");
		break;
	case 4:
		s = config_lookup(&cfg, "ant4");
		break;
	default:
		log_msg("invalid ant_index");
		return -1;
	}

	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	int enable, rfpower, switch_time;
	if (!(config_setting_lookup_int(s, "enable", &enable)
		&& config_setting_lookup_int(s, "rfpower", &rfpower)
		&& config_setting_lookup_int(s, "switch_time", &switch_time))) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	ant->enable = (uint8_t)enable;
	ant->rfpower = (uint8_t)rfpower;
	ant->switch_time = (uint8_t)switch_time;

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_ant(antenna_t *ant, int ant_index)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (ant_index) {
	case 1:
		s = config_lookup(&cfg, "ant1.enable");
		if (s) config_setting_set_int(s, ant->enable);
		s = config_lookup(&cfg, "ant1.rfpower");
		if (s) config_setting_set_int(s, ant->rfpower);
		s = config_lookup(&cfg, "ant1.switch_time");
		if (s) config_setting_set_int(s, ant->switch_time);
		break;
	case 2:
		s = config_lookup(&cfg, "ant2.enable");
		if (s) config_setting_set_int(s, ant->enable);
		s = config_lookup(&cfg, "ant2.rfpower");
		if (s) config_setting_set_int(s, ant->rfpower);
		s = config_lookup(&cfg, "ant2.switch_time");
		if (s) config_setting_set_int(s, ant->switch_time);
		break;
	case 3:
		s = config_lookup(&cfg, "ant3.enable");
		if (s) config_setting_set_int(s, ant->enable);
		s = config_lookup(&cfg, "ant3.rfpower");
		if (s) config_setting_set_int(s, ant->rfpower);
		s = config_lookup(&cfg, "ant3.switch_time");
		if (s) config_setting_set_int(s, ant->switch_time);
		break;
	case 4:
		s = config_lookup(&cfg, "ant4.enable");
		if (s) config_setting_set_int(s, ant->enable);
		s = config_lookup(&cfg, "ant4.rfpower");
		if (s) config_setting_set_int(s, ant->rfpower);
		s = config_lookup(&cfg, "ant4.switch_time");
		if (s) config_setting_set_int(s, ant->switch_time);
		break;
	default:
		log_msg("invalid ant_index");
		err = -1;
		goto out;
	}
	
	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * gpo
 *--------------------------------------------------------------------*/
int cfg_get_gpo(gpo_t *gpo, gpo_index_e gpo_index)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (gpo_index) {
	case GPO_IDX_BEEP:
		s = config_lookup(&cfg, "beep");
		break;
	case GPO_IDX_1:
		s = config_lookup(&cfg, "gpo1");
		break;
	case GPO_IDX_2:
		s = config_lookup(&cfg, "gpo2");
		break;
	case GPO_IDX_3:
		s = config_lookup(&cfg, "gpo3");
		break;
	case GPO_IDX_4:
		s = config_lookup(&cfg, "gpo4");
		break;
	case GPO_IDX_BLINK:
		s = config_lookup(&cfg, "blink");
		break;
	default:
		log_msg("invalid ant_index");
		return -1;
	}

	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	int pulse_width;
	if (!config_setting_lookup_int(s, "pulse_width", &pulse_width)) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	gpo->pulse_width = (uint8_t)pulse_width;

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_gpo(gpo_t *gpo, gpo_index_e gpo_index)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (gpo_index) {
	case GPO_IDX_BEEP:
		s = config_lookup(&cfg, "beep.pulse_width");
		if (s) config_setting_set_int(s, gpo->pulse_width);
		break;
	case GPO_IDX_1:
		s = config_lookup(&cfg, "gpo1.pulse_width");
		if (s) config_setting_set_int(s, gpo->pulse_width);
		break;
	case GPO_IDX_2:
		s = config_lookup(&cfg, "gpo2.pulse_width");
		if (s) config_setting_set_int(s, gpo->pulse_width);
		break;
	case GPO_IDX_3:
		s = config_lookup(&cfg, "gpo3.pulse_width");
		if (s) config_setting_set_int(s, gpo->pulse_width);
		break;
	case GPO_IDX_4:
		s = config_lookup(&cfg, "gpo4.pulse_width");
		if (s) config_setting_set_int(s, gpo->pulse_width);
		break;
	default:
		log_msg("invalid ant_index");
		err = -1;
		goto out;
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * pre_cfg
 *--------------------------------------------------------------------*/
int cfg_get_pre_cfg(pre_cfg_t *pre_cfg)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "pre_cfg");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_setting_lookup_int(s, "ant_idx", &pre_cfg->ant_idx)
		&& config_setting_lookup_int(s, "upload_mode", &pre_cfg->upload_mode)
		&& config_setting_lookup_int(s, "gpo_mode", &pre_cfg->gpo_mode)
		&& config_setting_lookup_int(s, "oper_mode", &pre_cfg->oper_mode)
		&& config_setting_lookup_int(s, "work_mode", &pre_cfg->work_mode)
		&& config_setting_lookup_int(s, "dev_type", &pre_cfg->dev_type)
		&& config_setting_lookup_int(s, "wg_start", &pre_cfg->wg_start)
		&& config_setting_lookup_int(s, "wg_len", &pre_cfg->wg_len)
		&& config_setting_lookup_int(s, "wg_pulse_width", &pre_cfg->wg_pulse_width)
		&& config_setting_lookup_int(s, "wg_pulse_periods", &pre_cfg->wg_pulse_periods)
		&& config_setting_lookup_int(s, "tid_len", &pre_cfg->tid_len)
		&& config_setting_lookup_int(s, "hop_freq_enable", &pre_cfg->hop_freq_enable)
		)) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_pre_cfg(pre_cfg_t *pre_cfg)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "pre_cfg.ant_idx");
	if (s) {
		config_setting_set_int(s, pre_cfg->ant_idx);
	}

	s = config_lookup(&cfg, "pre_cfg.upload_mode");
	if (s) {
		config_setting_set_int(s, pre_cfg->upload_mode);
	}

	s = config_lookup(&cfg, "pre_cfg.gpo_mode");
	if (s) {
		config_setting_set_int(s, pre_cfg->gpo_mode);
	}

	s = config_lookup(&cfg, "pre_cfg.oper_mode");
	if (s) {
		config_setting_set_int(s, pre_cfg->oper_mode);
	}

	s = config_lookup(&cfg, "pre_cfg.work_mode");
	if (s) {
		config_setting_set_int(s, pre_cfg->work_mode);
	}

	s = config_lookup(&cfg, "pre_cfg.dev_type");
	if (s) {
		config_setting_set_int(s, pre_cfg->dev_type);
	}

	s = config_lookup(&cfg, "pre_cfg.wg_start");
	if (s) {
		config_setting_set_int(s, pre_cfg->wg_start);
	}

	s = config_lookup(&cfg, "pre_cfg.wg_len");
	if (s) {
		config_setting_set_int(s, pre_cfg->wg_len);
	}

	s = config_lookup(&cfg, "pre_cfg.wg_pulse_width");
	if (s) {
		config_setting_set_int(s, pre_cfg->wg_pulse_width);
	}

	s = config_lookup(&cfg, "pre_cfg.wg_pulse_periods");
	if (s) {
		config_setting_set_int(s, pre_cfg->wg_pulse_periods);
	}

	s = config_lookup(&cfg, "pre_cfg.tid_len");
	if (s) {
		config_setting_set_int(s, pre_cfg->tid_len);
	}

	s = config_lookup(&cfg, "pre_cfg.hop_freq_enable");
	if (s) {
		config_setting_set_int(s, pre_cfg->hop_freq_enable);
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * filter_enable
 *--------------------------------------------------------------------*/
int cfg_get_filter_enable(uint8_t *filter_enable)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "tag_report");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	int tmp;
	if (!(config_setting_lookup_int(s, "filter_enable", &tmp))) {
		log_msg("%s: config_setting_lookup_int() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	*filter_enable = tmp & 0xff;

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_filter_enable(uint8_t filter_enable)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "tag_report.filter_enable");
	if (s) {
		config_setting_set_int(s, filter_enable);
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * filter_time
 *--------------------------------------------------------------------*/
int cfg_get_filter_time(uint8_t *filter_time)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "tag_report");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	int tmp;
	if (!(config_setting_lookup_int(s, "filter_time", &tmp))) {
		log_msg("%s: config_setting_lookup_int() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	*filter_time = tmp & 0xff;

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_filter_time(uint8_t filter_time)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "tag_report.filter_time");
	if (s) {
		config_setting_set_int(s, filter_time);
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * eth0
 *--------------------------------------------------------------------*/
int cfg_get_eth0(eth0_pt eth0)
{
	int err = 0;	
	config_t cfg;
	config_setting_t *s;
	const char *ip, *mask, *gateway, *mac;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "eth0");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_setting_lookup_string(s, "ip", &ip)
		&& config_setting_lookup_string(s, "mask", &mask)
		&& config_setting_lookup_string(s, "gateway", &gateway)
		&& config_setting_lookup_int(s, "tcp_port", &eth0->tcp_port)
		&& config_setting_lookup_int(s, "udp_port", &eth0->udp_port)
		&& config_setting_lookup_int(s, "mac_changed", &eth0->mac_changed)
		&& config_setting_lookup_string(s, "mac", &mac)
		)) {
		log_msg("%s: config_setting_lookup_string() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	strncpy(eth0->ip, ip, ETH_ADDR_LEN);
	strncpy(eth0->mask, mask, ETH_ADDR_LEN);
	strncpy(eth0->gateway, gateway, ETH_ADDR_LEN);
	strncpy(eth0->mac, mac, MAC_ADDR_LEN);

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_eth0(eth0_pt eth0, cfg_eth0_e type)
{
	int err = 0;
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	switch (type) {
	case CFG_IP:
		s = config_lookup(&cfg, "eth0.ip");
		if (s) config_setting_set_string(s, eth0->ip);
		break;
	case CFG_MASK:
		s = config_lookup(&cfg, "eth0.mask");
		if (s) config_setting_set_string(s, eth0->mask);
		break;
	case CFG_GATEWAY:
		s = config_lookup(&cfg, "eth0.gateway");
		if (s) config_setting_set_string(s, eth0->gateway);
		break;
	case CFG_UDP_PORT:
		s = config_lookup(&cfg, "eth0.udp_port");
		if (s) config_setting_set_int(s, eth0->udp_port);
		break;
	case CFG_TCP_PORT:
		s = config_lookup(&cfg, "eth0.tcp_port");
		if (s) config_setting_set_int(s, eth0->tcp_port);
		break;
	case CFG_MAC:
		s = config_lookup(&cfg, "eth0.mac");
		if (s) config_setting_set_string(s, eth0->mac);

		/*
		 * 更新MAC修改标记,以便下次开机后立即配置MAC地址
		 */
		eth0->mac_changed = 1;
		s = config_lookup(&cfg, "eth0.mac_changed");
		if (s) config_setting_set_int(s, eth0->mac_changed);
		break;
	default:
		log_msg("%s: invalid parameter!", __FUNCTION__);
		err = -1;
		goto out;
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}

/*---------------------------------------------------------------------
 * frequency table
 *--------------------------------------------------------------------*/
int cfg_get_freq_table(uint8_t *freq_table, int len)
{
	int i, err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "freq_table");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	for (i = 0; i < len; i++) {
		freq_table[i] = config_setting_get_int_elem(s, i);
	}

out:
	config_destroy(&cfg);
	return err;
}

int cfg_set_freq_table(uint8_t *freq_table, int len)
{
	int i, err = 0;	
	config_t cfg;
	config_setting_t *s;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_NAME)) {
		log_msg("%s:%d - %s", config_error_file(&cfg), 
			config_error_line(&cfg), config_error_text(&cfg));
		err = -1;
		goto out;
	}

	s = config_lookup(&cfg, "freq_table");
	if (NULL == s) {
		log_msg("%s: config_lookup() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

	for (i = 0; i < len; i++) {
		 config_setting_set_int_elem(s, i, freq_table[i]);
	}

	if (!(config_write_file(&cfg, CONFIG_FILE_NAME))) {
		log_msg("%s: config_write_file() ERR!", __FUNCTION__);
		err = -1;
		goto out;
	}

out:
	config_destroy(&cfg);
	return err;
}
