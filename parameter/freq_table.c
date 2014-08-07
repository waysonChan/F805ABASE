#include "parameter.h"

#ifdef FCC_FREQSTAND
const uint32_t freq_table[FCC_FRE_TABLE_LEN] = {
	/* value		   	   频率	      频点 */
	0xDC65E,		/* 902.75 MHz  1 */
	0xDC852,		/* 903.25 MHz  2 */
	0xDCA46,		/* 903.75 MHz  3 */
	0xDCC3A,		/* 904.25 MHz  4 */
	0xDCE2E,		/* 904.75 MHz  5 */
	0xDD022,		/* 905.25 MHz  6 */
	0xDD216,		/* 905.75 MHz  7 */
	0xDD40A,		/* 906.25 MHz  8 */
	0xDD5FE,		/* 906.75 MHz  9 */
	0xDD7F2,		/* 907.25 MHz  10 */
	0xDD9E6,		/* 907.75 MHz  11 */
	0xDDBDA,		/* 908.25 MHz  12 */
	0xDDDCE,		/* 908.75 MHz  13 */
	0xDDFC2,		/* 909.25 MHz  14 */
	0xDE1B6,		/* 909.75 MHz  15 */
	0xDE3AA,		/* 910.25 MHz  16 */
	0xDE59E,		/* 910.75 MHz  17 */
	0xDE792,		/* 911.25 MHz  18 */
	0xDE986,		/* 911.75 MHz  19 */
	0xDEB7A,		/* 912.25 MHz  20 */
	0xDED6E,		/* 912.75 MHz  21 */
	0xDEF62,		/* 913.25 MHz  22 */
	0xDF156,		/* 913.75 MHz  23 */
	0xDF34A,		/* 914.25 MHz  24 */
	0xDF53E,		/* 914.75 MHz  25 */
	0xDF732,		/* 915.25 MHz  26 */
	0xDF926,		/* 915.75 MHz  27 */
	0xDFB1A,		/* 916.25 MHz  28 */
	0xDFD0E,		/* 916.75 MHz  29 */
	0xDFF02,		/* 917.25 MHz  30 */
	0xE00F6,		/* 917.75 MHz  31 */
	0xE02EA,		/* 918.25 MHz  32 */
	0xE04DE,		/* 918.75 MHz  33 */
	0xE06D2,		/* 919.25 MHz  34 */
	0xE08C6,		/* 919.75 MHz  35 */
	0xE0ABA,		/* 920.25 MHz  36 */
	0xE0CAE,		/* 920.75 MHz  37 */
	0xE0EA2,		/* 921.25 MHz  38 */
	0xE1096,		/* 921.75 MHz  39 */
	0xE128A,		/* 922.25 MHz  40 */
	0xE147E,		/* 922.75 MHz  41 */
	0xE1672,		/* 923.25 MHz  42 */
	0xE1866,		/* 923.75 MHz  43 */
	0xE1A5A,		/* 924.25 MHz  44 */
	0xE1C4E,		/* 924.75 MHz  45 */
	0xE1E42,		/* 925.25 MHz  46 */
	0xE2036,		/* 925.75 MHz  47 */
	0xE222A,		/* 926.25 MHz  48 */
	0xE241E,		/* 926.75 MHz  49 */
	0xE2612 		/* 927.25 MHz  50 */
};
const uint8_t hop_freq_table[FRE_HOPING_TABLE_LEN] = {
	0x1C, 0x20, 0x24, 0x28, 0x1D,
	0x21, 0x25, 0x29, 0x1E, 0x22
};

#elif defined(CE_FREQSTAND)
const uint32_t freq_table[FCC_FRE_TABLE_LEN] = {
	/* value	  	   	   频率	      频点 */
	0xD32E8,		/* 865.000 MHz 1 */
	0xD33B0,		/* 865.200 MHz 2 */
	0xD3478,		/* 865.400 MHz 3 */
	0xD3540,		/* 865.600 MHz 4 */
	0xD3608,		/* 865.800 MHz 5 */
	0xD36D0,		/* 866.000 MHz 6 */
	0xD3798,		/* 866.200 MHz 7 */
	0xD3860,		/* 866.400 MHz 8 */
	0xD3928,		/* 866.600 MHz 9 */
	0xD39F0,		/* 866.800 MHz 10 */
	0xD3AB8,		/* 867.000 MHz 11 */
	0xD3B80,		/* 867.200 MHz 12 */
	0xD3C48,		/* 867.400 MHz 13 */
	0xD3D10,		/* 867.600 MHz 14 */
	0xD3DD8,		/* 867.800 MHz 15 */
	0xD3EA0 		/* 868.000 MHz 16 */
};
const uint8_t hop_freq_table[FRE_HOPING_TABLE_LEN] = {
	0x00, 0x04, 0x01, 0x05,
	0x02, 0x06, 0x03, 0x07
};

#elif defined(CHINA_FREQSTAND)
const uint32_t freq_table[FCC_FRE_TABLE_LEN] = {
	/* value          	   频率        频点 */
	0xE0C31,		/* 920.625 MHz 1 */
	0xE0D2B,		/* 920.875 MHz 2 */
	0xE0E25,		/* 921.125 MHz 3 */
	0xE0F1F,		/* 921.375 MHz 4 */
	0xE1019,		/* 921.625 MHz 5 */
	0xE1113,		/* 921.875 MHz 6 */
	0xE120D,		/* 922.125 MHz 7 */
	0xE1307,		/* 922.375 MHz 8 */
	0xE1401,		/* 922.625 MHz 9 */
	0xE14FB,		/* 922.875 MHz 10 */
	0xE15F5,		/* 923.125 MHz 11 */
	0xE16EF,		/* 923.375 MHz 12 */
	0xE17E9,		/* 923.625 MHz 13 */
	0xE18E3,		/* 923.875 MHz 14 */
	0xE19DD,		/* 924.125 MHz 15 */
	0xE1AD7 		/* 924.375 MHz 16 */
};
const uint8_t hop_freq_table[FRE_HOPING_TABLE_LEN] = {
	0x00, 0x04, 0x01, 0x05,
	0x02, 0x06, 0x03, 0x07
};

#endif
