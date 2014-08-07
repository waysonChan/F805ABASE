#ifndef _RFID_PACKETS_H
#define _RFID_PACKETS_H

#include "config.h"

enum {
	RFID_18K6C_QUERYREP   = 0x00,
	RFID_18K6C_ACK        = 0x01,
	RFID_18K6C_QUERY      = 0x08,
	RFID_18K6C_QUERYADJ   = 0x09,
	RFID_18K6C_SELECT     = 0x0A,
	RFID_18K6C_NAK        = 0xC0,
	RFID_18K6C_REQRN      = 0xC1,
	RFID_18K6C_READ       = 0xC2,
	RFID_18K6C_WRITE      = 0xC3,
	RFID_18K6C_KILL       = 0xC4,
	RFID_18K6C_LOCK       = 0xC5,
	RFID_18K6C_ACCESS     = 0xC6,
	RFID_18K6C_BLOCKWRITE = 0xC7,
	RFID_18K6C_BLOCKERASE = 0xC8,
	RFID_18K6C_QT         = 0xE0	/* MSB of 18k6c command code only */
};

typedef enum {
	RFID_PACKET_CLASS_COMMON,
	RFID_PACKET_CLASS_DIAGNOSTICS,
	RFID_PACKET_CLASS_STATUS,
	RFID_PACKET_CLASS_RESERVED,
	RFID_PACKET_CLASS_DEBUG,
	RFID_PACKET_CLASS_LAST
} RFID_PACKET_CLASS;

/*
 * A packet type number is comprised of an 8-bit class and a 12-bit packet
 * number.  The layout of the packet type number is 0xCNNN where C is the
 * class and N is the number within the class.
 */

/*
 * The macros help in creating and breaking apart packet types from/into their
 * their component values.
 */
#define RFID_PACKET_CLASS_MASK      0x000F
#define RFID_PACKET_CLASS_SHIFT     12
#define RFID_PACKET_NUMBER_MASK     0x0FFF

/*
 * Creates the base packet type number for a class.  The base packet type
 * number is incremented to obtain packet type numbers for subsequent packets.
 */
#define RFID_PACKET_CLASS_BASE(c)   ((uint16_t) ((c) << RFID_PACKET_CLASS_SHIFT))

/* Extracts the RFID class or number from a packet type (t) */
#define EXTRACT_RFID_PACKET_CLASS(t)                                           \
    (((t) >> RFID_PACKET_CLASS_SHIFT) & RFID_PACKET_CLASS_MASK)
#define EXTRACT_RFID_PACKET_NUMBER(t)                                          \
    ((t) & RFID_PACKET_NUMBER_MASK)

/*
 * The 16-bit packet types that will be found in the common packet header pkt_type field.
 * When adding a new packet type to a class, simply append it to end of the
 * appropriate type's enumeration list.
 * NOTE: These packet type constants are in the endian format for the system
 * upon which the compile is being performed.  Before comparing them against
 * the packet type field from the packet, ensure that, if necessary, the
 * packet type field is converted from little endian (i.e., MAC format) to
 * the endian format for the system running the application.
 */
enum {
	/* common packets */
	RFID_PACKET_TYPE_COMMAND_BEGIN = 
	RFID_PACKET_CLASS_BASE(RFID_PACKET_CLASS_COMMON),	/* 0x0000 */
	RFID_PACKET_TYPE_COMMAND_END,				/* 0x0001 */
	RFID_PACKET_TYPE_ANTENNA_CYCLE_BEGIN,			/* 0x0002 */
	RFID_PACKET_TYPE_ANTENNA_BEGIN,				/* 0x0003 */
	RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_BEGIN,		/* 0x0004 */
	RFID_PACKET_TYPE_18K6C_INVENTORY,			/* 0x0005 */
	RFID_PACKET_TYPE_18K6C_TAG_ACCESS,			/* 0x0006 */
	RFID_PACKET_TYPE_ANTENNA_CYCLE_END,			/* 0x0007 */
	RFID_PACKET_TYPE_ANTENNA_END,				/* 0x0008 */
	RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_END,		/* 0x0009 */
	RFID_PACKET_TYPE_INVENTORY_CYCLE_BEGIN,			/* 0x000A */
	RFID_PACKET_TYPE_INVENTORY_CYCLE_END,			/* 0x000B */
	RFID_PACKET_TYPE_CARRIER_INFO,				/* 0x000C */
	RFID_PACKET_TYPE_RES5,					/* 0x000D */
	RFID_PACKET_TYPE_COMMAND_ACTIVE,			/* 0x000E */
    
	/* diagnostics packets */
	RFID_PACKET_TYPE_RES0 = 
	RFID_PACKET_CLASS_BASE(RFID_PACKET_CLASS_DIAGNOSTICS),	/* 0x1000 */
	RFID_PACKET_TYPE_RES1,					/* 0x1001 */
	RFID_PACKET_TYPE_RES2,					/* 0x1002 */
	RFID_PACKET_TYPE_RES3,					/* 0x1003 */
	RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_BEGIN_DIAGS,	/* 0x1004 */
	RFID_PACKET_TYPE_18K6C_INVENTORY_ROUND_END_DIAGS,	/* 0x1005 */
	RFID_PACKET_TYPE_18K6C_INVENTORY_DIAGS,			/* 0x1006 */
	RFID_PACKET_TYPE_RES4,					/* 0x1007 */
	RFID_PACKET_TYPE_INVENTORY_CYCLE_END_DIAGS,		/* 0x1008 */
	RFID_PACKET_TYPE_SJC_SCANRESULT,			/* 0x1009 */
	RFID_PACKET_TYPE_TX_RANDOM_DATA_STATUS,			/* 0x100A */
	RFID_PACKET_TYPE_CSM_PROTSCHED_SM_STATUS,		/* 0x100B */
	RFID_PACKET_TYPE_CSM_PROTSCHED_LBT_STATUS,		/* 0x100C */

	/* status packets */
	RFID_PACKET_TYPE_NONCRITICAL_FAULT =
	RFID_PACKET_CLASS_BASE(RFID_PACKET_CLASS_STATUS),	/* 0x2000 */

	/* reserved/command response packets */
	RFID_PACKET_TYPE_ENGTESTPAT_ZZS = 
	RFID_PACKET_CLASS_BASE(RFID_PACKET_CLASS_RESERVED),	/* 0x3000 */
	RFID_PACKET_TYPE_ENGTESTPAT_FFS,			/* 0x3001 */
	RFID_PACKET_TYPE_ENGTESTPAT_W1S,			/* 0x3002 */
	RFID_PACKET_TYPE_ENGTESTPAT_W0S,			/* 0x3003 */
	RFID_PACKET_TYPE_ENGTESTPAT_BND,			/* 0x3004 */
	RFID_PACKET_TYPE_MBP_READ_REG,				/* 0x3005 */
	RFID_PACKET_TYPE_GPIO_READ,				/* 0x3006 */
	RFID_PACKET_TYPE_OEMCFG_READ,				/* 0x3007 */
	RFID_PACKET_TYPE_ENG_RSSI,				/* 0x3008 */
	RFID_PACKET_TYPE_ENG_INVSTATS,				/* 0x3009 */
	RFID_PACKET_TYPE_ENG_BERTSTRESULT,			/* 0x300A */
	RFID_PACKET_TYPE_NVMEMUPDCFG,				/* 0x300B */
	RFID_PACKET_TYPE_LPROF_READ_REG,			/* 0x300C */
	RFID_PACKET_TYPE_RES6,					/* 0x300D */
	RFID_PACKET_TYPE_ENG_XY,				/* 0x300E */

	/* debug packets */
	RFID_PACKET_TYPE_DEBUG =
	RFID_PACKET_CLASS_BASE(RFID_PACKET_CLASS_DEBUG),	/* 0x3000 */
};

typedef struct hostpkt_cmn {
	uint8_t pkt_ver;
	uint8_t flags;
	uint16_t pkt_type;
	uint16_t pkt_len;
	uint16_t res0;
} RFID_PACKET_COMMON;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t command;
	uint32_t ms_ctr;
} RFID_PACKET_COMMAND_BEGIN;

/* 
 * Macros to make it easier to extract the bit fields out of the command begin 
 * flags field (i.e., cmn.flags)
 */
#define RFID_COMMAND_IN_CONTINUOS_MODE(f)        ((f) & 0x01)
#define RFID_COMMAND_NOT_IN_CONTINUOUS_MODE(f)   \
	(!(RFID_COMMAND_IN_CONTINUOUS_MODE(f)))

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint32_t status;
} RFID_PACKET_COMMAND_END;

/* Command end flags field i.e., cmn.flags) bit definitions */
#define HOSTIF_TX_SATURATE_FLAG       0x01

typedef struct {
	RFID_PACKET_COMMON cmn;
} RFID_PACKET_ANTENNA_CYCLE_BEGIN;

typedef struct {
	RFID_PACKET_COMMON cmn;
} RFID_PACKET_ANTENNA_CYCLE_END;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t antenna;
} RFID_PACKET_ANTENNA_BEGIN;

typedef struct {
	RFID_PACKET_COMMON cmn;
} RFID_PACKET_ANTENNA_END;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
} RFID_PACKET_INVENTORY_CYCLE_BEGIN;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
} RFID_PACKET_INVENTORY_CYCLE_END;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t querys;
	uint32_t rn16rcv;
	uint32_t rn16to;
	uint32_t epcto;
	uint32_t good_reads;
	uint32_t crc_failures;
} RFID_PACKET_INVENTORY_CYCLE_END_DIAGS;

typedef struct {
	RFID_PACKET_COMMON cmn;
} RFID_PACKET_18K6C_INVENTORY_ROUND_BEGIN;

typedef struct {
	RFID_PACKET_COMMON cmn;
} RFID_PACKET_18K6C_INVENTORY_ROUND_END;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint32_t sing_params;
} RFID_PACKET_18K6C_INVENTORY_ROUND_BEGIN_DIAGS;

/* 
 * Macros to make it easier to extract the bit fields out of the singulation
 * parameters field (i.e., sing_params).  NOTE - since the packet format
 * specifies that 16-bit fields are transmitted in little endian, ensure that
 * the field is byte swapped, if necessary, for the host system before
 * applying any of the macros.
 */
#define RFID_SINGULATION_PARMS_CURRENT_Q(sing)		(((sing) & 0x000F)
#define RFID_SINGULATION_PARMS_CURRENT_SLOT(sing)	(((sing) >> 4) & 0x1ffff)
#define RFID_SINGULATION_PARMS_INVENTORY_A(sing)	(!(((sing) >> 21) & 0x0001))
#define RFID_SINGULATION_PARMS_INVENTORY_B(sing)	(((sing) >> 21)& 0x0001)
#define RFID_SINGULATION_PARMS_CURRENT_RETRY(sing)	(((sing) >> 22) & 0xff)

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint32_t querys;
	uint32_t rn16rcv;
	uint32_t rn16to;
	uint32_t epcto;
	uint32_t good_reads;
	uint32_t crc_failures;
} RFID_PACKET_18K6C_INVENTORY_ROUND_END_DIAGS;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint8_t nb_rssi;
	uint8_t wb_rssi;
	uint16_t ana_ctrl1;
	uint16_t rssi;
	uint16_t res0;
	uint32_t inv_data[1];
} RFID_PACKET_18K6C_INVENTORY;

/*
 * Macros to make it easier to extract the bit fields out of the 18k6c
 * inventory flags field (i.e., cmn.flags)
 */
#define RFID_18K6C_INVENTORY_CRC_IS_INVALID(f)   ((f) & 0x01)
#define RFID_18K6C_INVENTORY_CRC_IS_VALID(f)	\
    !(RFID_18K6C_INVENTORY_CRC_IS_INVALID(f))
#define RFID_18K6C_INVENTORY_PADDING_BYTES(f)    (((f) >> 6) & 0x03)

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t prot_parms;
} RFID_PACKET_18K6C_INVENTORY_DIAGS;

/*
 * Macros to make it easier to extract the bit fields out of the 18k6c
 * inventory diagnostics protocol parameters (i.e., prot_parms) field.
 * NOTE - since the packet format specifies that 32-bit fields are transmitted in
 * little endian, ensure that the field is byte swapped, if necessary, for the
 * host system before applying any of the macros.
 */
#define RFID_18K6C_PROTOCOL_PARMS_Q(parms)       ((parms) & 0x0000000F)
#define RFID_18K6C_PROTOCOL_PARMS_C(parms)       (((parms) & 0x00000070) >> 4)
#define RFID_18K6C_PROTOCOL_PARMS_TARI(parms)    (((parms) & 0x0000FF00) >> 8)

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint8_t command;
	uint8_t tag_error_code;
	uint16_t prot_error_code;
	uint16_t write_word_count;
	uint16_t res0;
	uint32_t data[1];
} RFID_PACKET_18K6C_TAG_ACCESS;

/* Errors detected by protocol code during 18k6 caccess */
#define PROT_ACCESS_ERROR_FAIL                          0xFFFF /* -1 */
#define PROT_ACCESS_ERROR_NONE                          0x0000
#define PROT_ACCESS_ERROR_HANDLE_MISMATCH               0x0001
#define PROT_ACCESS_ERROR_BAD_CRC                       0x0002
#define PROT_ACCESS_ERROR_NO_REPLY                      0x0003
#define PROT_ACCESS_ERROR_INVALID_PASSWORD              0x0004
#define PROT_ACCESS_ERROR_ZERO_KILL_PASSWORD            0x0005
#define PROT_ACCESS_ERROR_TAG_LOST                      0x0006
#define PROT_ACCESS_ERROR_CMD_FORMAT_ERROR              0x0007
#define PROT_ACCESS_ERROR_READ_COUNT_INVALID            0x0008
#define PROT_ACCESS_ERROR_RETRY_COUNT_EXCEEDED          0x0009

/* 18k6c backscatter error codes */
#define BACKSCATTER_OTHER_ERROR                         0x00
#define BACKSCATTER_MEM_OVERRUN                         0x03
#define BACKSCATTER_MEM_LOCKED                          0x04
#define BACKSCATTER_INSUFFICIENT_POWER                  0x0B
#define BACKSCATTER_NONSPECIFIC_ERROR                   0x0F

/*
 * Macros to make it easier to extract the bit fields out of the 18k6c
 * tag access flags (i.e., cmn.flags) field
 */
#define RFID_18K6C_TAG_ACCESS_MAC_ERROR(f)             ((f) & 0x01)
#define RFID_18K6C_TAG_ACCESS_BACKSCATTER_ERROR(f)     ((f) & 0x02)
#define RFID_18K6C_TAG_ACCESS_ANY_ERROR(f)		\
	(RFID_18K6C_TAG_ACCESS_MAC_ERROR(f) || 	\
	RFID_18K6C_TAG_ACCESS_BACKSCATTER_ERROR(f))

#define RFID_18K6C_TAG_ACCESS_PADDING_BYTES(f)		(((f) >> 6) & 0x3)

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint16_t fault_type;
	uint16_t fault_subtype;
	uint32_t context;
} RFID_PACKET_NONCRITICAL_FAULT;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint32_t plldivmult;
	uint16_t chan;
	uint16_t cw_flags;
} RFID_PACKET_CARRIER_INFO;

#define CWFLAGS_CWON  0x0001
#define CWFLAGS_CWOFF 0x0000
#define CARRIER_INFO_IS_CWON(cw_flags)  (cw_flags & CWFLAGS_CWON)
#define CARRIER_INFO_IS_CWOFF(cw_flags) ((cw_flags & CWFLAGS_CWON) == 0 ? 1: 0)
#define CARRIER_INFO_SET_CWON(cw_flags) (cw_flags |= CWFLAGS_CWON)
#define CARRIER_INFO_SET_CWOFF(cw_flags) (cw_flags &= ~CWFLAGS_CWON)

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
} RFID_PACKET_COMMAND_ACTIVE;

/*
 * Host interface timeout value (in seconds) for sending the command
 * active packet, in the abscence of other host interface tx data.
 */
#define CMD_ACTIVE_TIMEOUT 3  

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint32_t elapsedTimeMs;
} RFID_PACKET_TX_RANDOM_DATA_STATUS;

typedef struct {
	RFID_PACKET_COMMON cmn;
	uint32_t ms_ctr;
	uint16_t counter;
	uint16_t id;
	uint8_t debug_data[sizeof(uint32_t)];
} RFID_PACKET_DEBUG;

#endif /* _RFID_PACKETS_H */
