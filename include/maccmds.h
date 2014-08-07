#ifndef MACCMDS_H
#define MACCMDS_H

enum CMD {
    CMD_NONE                            = 0x00, /* CMD_NONE */
    CMD_NV_MEM_UPDATE                   = 0x01, /* NV Memory Update */
    CMD_WROEM                           = 0x02, /* Write OEM Configuration Area */
    CMD_RDOEM                           = 0x03, /* Read OEM Configuration Area */
    CMD_ENGTEST                         = 0x04, /* Engineering Test - Base command for following sub-commands */
    CMD_MBPRDREG                        = 0x05, /* MAC Bypass Register Read */
    CMD_MBPWRREG                        = 0x06, /* MAC Bypass Register Write */
    CMD_RESERVED_07                     = 0x07, /* CMD_RESERVED_07 - Command not supported */
    CMD_RESERVED_08                     = 0x08, /* CMD_RESERVED_08 - Command not supported */
    CMD_RESERVED_09                     = 0x09, /* CMD_RESERVED_09 - Command not supported */
    CMD_RESERVED_0A                     = 0x0A, /* CMD_RESERVED_0A - Command not supported */
    CMD_RESERVED_0B                     = 0x0B, /* CMD_RESERVED_0B - Command not supported */
    CMD_RDGPIO                          = 0x0C, /* Read GPIO */
    CMD_WRGPIO                          = 0x0D, /* Write GPIO */
    CMD_CFGGPIO                         = 0x0E, /* Configure GPIO */
    CMD_18K6CINV                        = 0x0F, /* ISO 18000-6C Inventory */
    CMD_18K6CREAD                       = 0x10, /* ISO 18000-6C Read */
    CMD_18K6CWRITE                      = 0x11, /* ISO 18000-6C Write */
    CMD_18K6CLOCK                       = 0x12, /* ISO 18000-6C Lock */
    CMD_18K6CKILL                       = 0x13, /* ISO 18000-6C Kill */
    CMD_SETPWRMGMTCFG                   = 0x14, /* Set Power Management Configuration */
    CMD_CLRERR                          = 0x15, /* Clear Error */
    CMD_RESERVED_16                     = 0x16, /* CMD_RESERVED_16 - Command not supported */
    CMD_CWON                            = 0x17, /* Turn Carrie Wave On */
    CMD_CWOFF                           = 0x18, /* Turn Carrier Wave Off */
    CMD_UPDATELINKPROFILE               = 0x19, /* Update Link Profile */
    CMD_RESERVED_1A                     = 0x1A, /* CMD_RESERVED_1A - Command not supported */
    CMD_CALIBRATE_GG                    = 0x1B, /* Calibrate Gross Gains */
    CMD_LPROF_RDXCVRREG                 = 0x1C, /* Link Profile Transceiver Register Read */
    CMD_LPROF_WRXCVRREG                 = 0x1D, /* Link Profile Transceiver Register Write */
    CMD_18K6CBLOCKERASE                 = 0x1E, /* ISO 18000-6C Block Erase */
    CMD_18K6CBLOCKWRITE                 = 0x1F, /* ISO 18000-6C Block Write */
    CMD_POP_SPURWATABLE                 = 0x20, /* Populate Spur Workaround Table */
    CMD_UPDATESENSORCOEFF               = 0x21, /* Update Sensor Coefficients */
    CMD_TX_RANDOM_DATA                  = 0x22, /* Transmit Random Data */
    CMD_18K6CQT                         = 0x23, /* ISO 18000-6C QT */
    CMD_CALIBRATE_DC_OFFSET             = 0x24, /* DC Offset Calibration */
    CMD_OEM_STRING_READ                 = 0x25, /* OEM String Read */
    CMD_OEM_STRING_WRITE                = 0x26, /* OEM String Write */
    CMD_SET_FREQUENCY                   = 0x27, /* Set Frequency */
    CMD_FLUSH_OEM                       = 0x28, /* Flush OEM */
    CMD_KEY_GEN                         = 0x29, /* Key Generator */
    CMD_FORMAT_OEM                      = 0x2A, /* Format OEM */
    CMD_CALIBRATE_LBT_RSSI              = 0x2B, /* LBT RSSI Threshold Calibration */
    CMD_CALIBRATE_PA_BIAS               = 0x2C, /* PA Bias Calibration */
    CMD_INJECT_RANDOM_TX                = 0x2D, /* Inject Random Tx Data */
};

#define NUM_CMDS                       46
#define CMD_INVALIDVAL                 0xFFFFFFFF

/* Constants - CMD_MBPRDREG, CMD_MBPWRREG */
#define MBP_FIRST_ADDR      0x0000
#define MBP_LAST_ADDR_R1000 0x0500
#define MBP_LAST_ADDR_R2000 0x0502
#define MBP_LAST_ADDR_R500  0x0502

/* Control Channel Commands */
#define CTLCMD_CANCEL             0x4001
#define CTLCMD_SOFTRESET          0x4002
#define CTLCMD_ABORT              0x4003
#define CTLCMD_PAUSE              0x4004
#define CTLCMD_RESUME             0x4005
#define CTLCMD_GETSERIALNUMBER    0x4006
#define CTLCMD_RESETTOBOOTLOADER  0x4007

#endif /* MACCMDS_H */
