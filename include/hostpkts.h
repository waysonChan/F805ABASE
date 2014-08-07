#ifndef HOSTPKTS_H
#define HOSTPKTS_H

#include "rfid_packets.h"

/* The length of packets in the common packet format are expressed in some    */
/* unit.  In this case, it is 32-bit words.  These macros are used for        */
/* determining the length of particular packets in the expressed units.       */
#define RFID_PACKET_BYTES_PER_UNIT  4   /* 1 Unit = 32 bits (i.e., 4 bytes)   */
#define RFID_PACKET_UNIT_LEN(pkt)   (sizeof(pkt) / RFID_PACKET_BYTES_PER_UNIT)

/* The length, in units, of the common packet header.                         */
#define RFID_PACKET_CMN_LEN         RFID_PACKET_UNIT_LEN(RFID_PACKET_COMMON)

/* When computing the unit length of a packet, the common header unit length  */
/* is subtracted as the unit length is the number of bytes after the common   */
/* header.                                                                    */
#define RFID_PACKET_ADDITIONAL_UNIT_LEN(pkt) \
	((RFID_PACKET_UNIT_LEN(pkt)) - (RFID_PACKET_CMN_LEN))

struct host_reg_req {
	uint16_t access_flg;
	uint16_t reg_addr;
	uint32_t reg_data;
};
#define HOST_REG_REQ_SIZE   sizeof(struct host_reg_req)

/* Constants for the access_flg field */
#define HOST_REG_REQ_ACCESS_TYPE    ((uint16_t) 0x0001)

/* Constants for the register access types */
#define HOST_REG_REQ_ACCESS_READ    ((uint16_t) 0x0000)
#define HOST_REG_REQ_ACCESS_WRITE   ((uint16_t) 0x0001)

struct host_reg_resp {
	uint16_t rfu0;
	uint16_t reg_addr;
	uint32_t reg_data;
};
#define HOST_REG_RESP_SIZE  sizeof(struct host_reg_resp)

typedef struct hostpkt_mbp_read_reg {
	struct hostpkt_cmn cmn;
	uint16_t addr;
	uint16_t data;
} RFID_PACKET_MBP_READ;

typedef struct hostpkt_gpio_read {
	struct hostpkt_cmn cmn;
	uint32_t data;
} RFID_PACKET_GPIO_READ;

typedef struct hostpkt_oemcfg_read {
	struct hostpkt_cmn cmn;
	uint32_t addr;
	uint32_t data;
}RFID_PACKET_OEMCFG_READ;

/*
 * eng_rssi_data
 * RSSI data struct - N of these are included in payload of hostpkt_eng_rssi
 */
#define ENGRSSI_FLAG_EXTFB_VALID      0x00000001
#define ENGRSSI_FLAG_RSSI_VALID       0x00000002
#define ENGRSSI_FLAG_LNA_VALID        0x00000004
#define ENGRSSI_FLAG_IFLNA_I_Q_VALID  0x00000008
#define ENGRSSI_FLAGS_MASK            0x0000000F
#define ENGRSSI_FLAGS_ARG1_SHIFT      16

typedef struct hostpkt_eng_rssi {
	struct hostpkt_cmn cmn;
	unsigned int command_flags;
	unsigned int msctr;
	unsigned int ext_fb;
	unsigned int lna_high_low;
	unsigned int iflna_peakI_peakQ;
	unsigned short nb_rssi;
	unsigned short wb_rssi;
}RFID_PACKET_ENG_RSSI;

typedef struct hostpkt_eng_invstats {
	struct hostpkt_cmn cmn;
	uint32_t queries;
	uint32_t rn16_to;
	uint32_t rn16_rcv;
	uint32_t epc_crcerr;
	uint32_t epc_rcvto;
	uint32_t epc_good;
}RFID_PACKET_ENG_INVSTATS;

typedef struct hostpkt_eng_bertest_result {
	struct hostpkt_cmn cmn;
	uint16_t requested_bit_len;
	uint16_t received_bit_len;
	uint8_t rx_buffer[64]; /* Up to 512 bits of response data */
	uint32_t rfu0;
	uint32_t rfu1;
} RFID_PACKET_ENG_BERTSTRESULT;

typedef struct hostpkt_nvmemupdcfg {
	struct hostpkt_cmn cmn;
	uint32_t max_pkt_len;
	uint16_t nv_upd_ver;
	uint16_t res0;
} RFID_PACKET_NVMEMUPDCFG;

typedef struct hostpkt_engtestpat {
	struct hostpkt_cmn cmn;
	uint32_t data[1];
} RFID_PACKET_ENGTESTPAT_READ;

typedef struct hostpkt_lprof_read_reg {
	struct hostpkt_cmn cmn;
	uint16_t sel;
	uint16_t addr;
	uint16_t data;
	uint16_t rfu;
} RFID_PACKET_LPROF_READ;

typedef struct hostpkt_sjc_scanresult {
	struct hostpkt_cmn cmn;
	uint32_t ms_ctr;
	uint32_t algo_time;
	uint8_t origin_I;
	uint8_t origin_Q;
	uint8_t step_size;
	uint8_t scan_size;
	uint8_t scale_out_I;
	uint8_t scale_out_Q;
	uint16_t rssi_pre_sjc;
	uint16_t rssi_post_sjc;
	uint8_t external_lo;
	uint8_t rfu;
} RFID_PACKET_SJC_SCANRESULT;

typedef struct hostpkt_csm_protsched_sm_status {
	struct hostpkt_cmn cmn;
	uint32_t ms_ctr;
	uint8_t protsched_state;
	uint8_t curr_freq_idx;
	uint8_t curr_ant_idx;
	uint8_t rfu0;
	uint32_t rfu1;
} RFID_PACKET_CSM_PROTSCHED_SM_STATUS;

typedef struct hostpkt_csm_protsched_lbt_status {
	struct hostpkt_cmn cmn;
	uint32_t ms_ctr;
	uint8_t intf_present;
	uint8_t rssi;
	uint8_t curr_freq_idx;
	uint8_t curr_ant_idx;
	uint16_t ana_ctrl;
	uint8_t lbt_chans_offset;
	uint8_t rfu0;
	uint32_t rfu1;
}RFID_PACKET_CSM_PROTSCHED_LBT_STATUS;

typedef struct hostpkt_eng_xy {
	struct hostpkt_cmn cmn;
	unsigned int id;
	unsigned int calculationTimeUs;
	unsigned int x;
	unsigned int y;
	unsigned int res0;
	unsigned int res1;
	unsigned int res2;
	unsigned int res3;
}RFID_PACKET_ENG_XY;

/* The length of the individual packets in the unit of measure that is present*/
/* in the common packet format preamble.                                      */
#define RFID_PACKET_LEN_COMMAND_BEGIN                                         \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_COMMAND_BEGIN)
#define RFID_PACKET_LEN_COMMAND_END                                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_COMMAND_END)
#define RFID_PACKET_LEN_CYCLE_BEGIN                                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_CYCLE_BEGIN)
#define RFID_PACKET_LEN_CYCLE_END                                             \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_CYCLE_END)
#define RFID_PACKET_LEN_CYCLE_BEGIN_DIAGS                                     \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_CYCLE_BEGIN_DIAGS)
#define RFID_PACKET_LEN_CYCLE_END_DIAGS                                       \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_CYCLE_END_DIAGS)
#define RFID_PACKET_LEN_ANTENNA_BEGIN                                         \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_BEGIN)
#define RFID_PACKET_LEN_ANTENNA_END                                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_END)
#define RFID_PACKET_LEN_ANTENNA_BEGIN_DIAGS                                   \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_BEGIN_DIAGS)
#define RFID_PACKET_LEN_ANTENNA_END_DIAGS                                     \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ANTENNA_END_DIAGS)
#define RFID_PACKET_LEN_INVENTORY_CYCLE_BEGIN                                 \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_INVENTORY_CYCLE_BEGIN)
#define RFID_PACKET_LEN_INVENTORY_CYCLE_END                                   \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_INVENTORY_CYCLE_END)
#define RFID_PACKET_LEN_INVENTORY_CYCLE_END_DIAGS                             \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_INVENTORY_CYCLE_END_DIAGS)
#define RFID_PACKET_LEN_INVENTORY_ROUND_BEGIN                                 \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY_ROUND_BEGIN)
#define RFID_PACKET_LEN_INVENTORY_ROUND_END                                   \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY_ROUND_END)
#define RFID_PACKET_LEN_INVENTORY_ROUND_BEGIN_DIAGS                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY_ROUND_BEGIN_DIAGS)
#define RFID_PACKET_LEN_INVENTORY_ROUND_END_DIAGS                             \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY_ROUND_END_DIAGS)
/* Since this is a variable-length packet, have to subtract off 1 unit for the*/
/* single-element array at the end                                            */
#define RFID_PACKET_LEN_MIN_18K6C_INVENTORY                                   \
    (RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY) - 1)
#define RFID_PACKET_LEN_18K6C_INVENTORY_DIAGS                                 \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_INVENTORY_DIAGS)
/* Since this is a variable-length packet, have to subtract off 1 unit for the*/
/* single-element array at the end                                            */
#define RFID_PACKET_LEN_MIN_18K6C_TAG_ACCESS                                  \
    (RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_18K6C_TAG_ACCESS) - 1)
#define RFID_PACKET_LEN_FREQUENCY_HOP_DIAGS                                   \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_FREQUENCY_HOP_DIAGS)
#define RFID_PACKET_LEN_NONCRITICAL_FAULT                                     \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_NONCRITICAL_FAULT)
#define RFID_PACKET_LEN_MBP_READ_REG                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_MBP_READ)
#define RFID_PACKET_LEN_GPIO_READ                                             \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_GPIO_READ)
#define RFID_PACKET_LEN_OEMCFG_READ                                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_OEMCFG_READ)
#define RFID_PACKET_LEN_ENG_RSSI                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ENG_RSSI)
#define RFID_PACKET_LEN_ENG_INVSTATS                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ENG_INVSTATS)
#define RFID_PACKET_LEN_MIN_ENGTESTPAT                                         \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ENGTESTPAT)
#define RFID_PACKET_LEN_ENG_BERTSTRESULT                                      \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ENG_BERTSTRESULT)
#define RFID_PACKET_LEN_CARRIER_INFO                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_CARRIER_INFO)
#define RFID_PACKET_LEN_COMMAND_ACTIVE                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_COMMAND_ACTIVE)
#define RFID_PACKET_LEN_NVMEMUPDCFG                                           \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_NVMEMUPDCFG)
#define RFID_PACKET_LEN_LPROF_READ_REG                                        \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_LPROF_READ)
#define RFID_PACKET_LEN_TX_RANDOM_DATA_STATUS                                      \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_TX_RANDOM_DATA_STATUS)
#define RFID_PACKET_LEN_SJC_SCANRESULT                                        \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_SJC_SCANRESULT)
#define RFID_PACKET_LEN_CSM_PROTSCHED_SM_STATUS                               \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_CSM_PROTSCHED_SM_STATUS)
#define RFID_PACKET_LEN_CSM_PROTSCHED_LBT_STATUS                              \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_CSM_PROTSCHED_LBT_STATUS)
#define RFID_PACKET_LEN_ENG_XY                                          \
    RFID_PACKET_ADDITIONAL_UNIT_LEN(RFID_PACKET_ENG_XY)

/* The version nubmers to put in the common packet headers for different packets. */
#define RFID_PACKET_VER_COMMAND_BEGIN               0x01
#define RFID_PACKET_VER_COMMAND_END                 0x01
#define RFID_PACKET_VER_CYCLE_BEGIN                 0x01
#define RFID_PACKET_VER_CYCLE_END                   0x01
#define RFID_PACKET_VER_CYCLE_BEGIN_DIAGS           0x01
#define RFID_PACKET_VER_CYCLE_END_DIAGS             0x01
#define RFID_PACKET_VER_ANTENNA_BEGIN               0x01
#define RFID_PACKET_VER_ANTENNA_END                 0x01
#define RFID_PACKET_VER_ANTENNA_BEGIN_DIAGS         0x01
#define RFID_PACKET_VER_ANTENNA_END_DIAGS           0x01
#define RFID_PACKET_VER_INVENTORY_CYCLE_BEGIN       0x01
#define RFID_PACKET_VER_INVENTORY_CYCLE_END         0x01
#define RFID_PACKET_VER_INVENTORY_CYCLE_END_DIAGS   0x01
#define RFID_PACKET_VER_INVENTORY_ROUND_BEGIN       0x01
#define RFID_PACKET_VER_INVENTORY_ROUND_END         0x01
#define RFID_PACKET_VER_INVENTORY_ROUND_BEGIN_DIAGS 0x01
#define RFID_PACKET_VER_INVENTORY_ROUND_END_DIAGS   0x01
#define RFID_PACKET_VER_18K6C_INVENTORY             0x01
#define RFID_PACKET_VER_18K6C_INVENTORY_DIAGS       0x01
#define RFID_PACKET_VER_18K6C_TAG_ACCESS            0x01
#define RFID_PACKET_VER_FREQUENCY_HOP_DIAGS         0x01
#define RFID_PACKET_VER_NONCRITICAL_FAULT           0x01
#define RFID_PACKET_VER_MBP_READ_REG                0x01
#define RFID_PACKET_VER_GPIO_READ                   0x01
#define RFID_PACKET_VER_OEMCFG_READ                 0x01
#define RFID_PACKET_VER_ENG_RSSI                    0x01
#define RFID_PACKET_VER_ENG_INVSTATS                0x01
#define RFID_PACKET_VER_ENGTESTPAT                  0x01
#define RFID_PACKET_VER_ENG_BERTSTRESULT            0x01
#define RFID_PACKET_VER_CARRIER_INFO                0x01
#define RFID_PACKET_VER_COMMAND_ACTIVE              0x01
#define RFID_PACKET_VER_NVMEMUPDCFG                 0x01
#define RFID_PACKET_VER_LPROF_READ_REG              0x01
#define RFID_PACKET_VER_CYCCFG_EVT                  0x01
#define RFID_PACKET_VER_DEBUG                       0x01
#define RFID_PACKET_VER_TX_RANDOM_DATA_STATUS       0x01
#define RFID_PACKET_VER_SJC_SCANRESULT              0x01
#define RFID_PACKET_VER_CSM_PROTSCHED_SM_STATUS     0x01
#define RFID_PACKET_VER_CSM_PROTSCHED_LBT_STATUS    0x01
#define RFID_PACKET_VER_ENG_XY                      0x01

/* Macro to get the complete TX byte length of a packet including the common fields */
#define RFID_PACKET_FULL_BYTE_LEN(pktlen) \
	((uint32_t) ((RFID_PACKET_CMN_LEN * RFID_PACKET_BYTES_PER_UNIT) + \
	((pktlen) * RFID_PACKET_BYTES_PER_UNIT)))

/* Macro to convert packet specific length to a byte len */
#define RFID_PACKET_PKT_BYTE_LEN(pktlen) \
	((uint32_t) ((pktlen) * RFID_PACKET_BYTES_PER_UNIT))

#endif	/* HOSTPKTS_H */
