/*
  libconfig.h
  =================================
  Timonel TWI Master Configuration
  ---------------------------------
  2019-02-07 Gustavo Casanova
  ---------------------------------
*/

#ifndef _TML_TWIMCONFIG_H_
#define _TML_TWIMCONFIG_H_

#define USE_SERIAL Serial

#define CMD_ACK_POS 0       /* Command acknowledge reply position */
#define V_CMD_LENGTH 9      /* Timonel version reply lenght */
#define T_SIGNATURE 84      /* Timonel signature (expected "T") */
#define V_SIGNATURE 1       /* Ver cmd reply: signature position */
#define V_MAJOR 2           /* Ver cmd reply: major number position */
#define V_MINOR 3           /* Ver cmd reply: minor number position */
#define V_FEATURES 4        /* Ver cmd reply: available features code position */
#define V_BOOT_ADDR_MSB 5   /* Ver cmd reply: Timonel start address MSB position */
#define V_BOOT_ADDR_LSB 6   /* Ver cmd reply: Timonel start address LSB position */
#define V_APPL_ADDR_MSB 7   /* Ver cmd reply: Application address MSB position */
#define V_APPL_ADDR_LSB 8   /* Ver cmd reply: Application address LSB position */

#define TXDATASIZE 8  		  /* Data block size for WRITBUFF command transmisions */
#define PAGE_SIZE 64		    /* Tiny85 flash page buffer size */
#define MAXCKSUMERRORS 1    /* Dump flash max count of errors accepted */

#define MCU_TOTAL_MEM 8192  /* Dumpflash: ATTiny flash memory size */
#define RX_DATA_SIZE 8      /* Dumpflash: TWI command reply data size */
#define VALUES_PER_LINE 32  /* Dumpflash: Values per line to display */

#define OK 0                /* No error */
#define ERR_01 0x01         /* Error GetTmlID (0) 1: Error parsing GETTMNLV command */
#define ERR_02 0x02         /* Error GetTmlID (0) 2: Firmware signature unknown */

// Function SetPageAddress definitions
// #define STPGADDR_CMD_SZ 4
// #define STPGADDR_RPL_SZ 2 
// #define WORD_MSB_MASK 0xFF00
// #define WORD_LSB_MASK 0xFF
// End of SetPageAddress definitions

#endif /* _TML_TWIMCONFIG_H_ */