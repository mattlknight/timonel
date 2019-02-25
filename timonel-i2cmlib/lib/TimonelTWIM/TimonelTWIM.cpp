/*
  TimonelTWIM.cpp
  ===============
  Library code for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#include "TimonelTWIM.h"

// Class constructor
Timonel::Timonel(byte twi_address, byte sda, byte scl) : addr_(twi_address) {
	if (!((sda == 0) && (scl == 0))) {
		USE_SERIAL.printf_P("\n\r[Constructor] Creating a new I2C connection\n\r");		
		Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
		reusing_twi_connection_ = false;
	}
	else {
		USE_SERIAL.printf_P("\n\r[Constructor] Reusing I2C connection\n\r");
		reusing_twi_connection_ = true;
	}
  	//addr_ = twi_address;
  	TwoStepInit(0);
}

// Function to check the status parameters of the bootloader running on the ATTiny85
byte Timonel::QueryStatus(void) {
  	byte rx_reply[V_CMD_LENGTH] = { 0 };  					/* Status received from I2C slave */
	byte twi_cmd_err = TwiCmdXmit(GETTMNLV, ACKTMNLV, rx_reply, V_CMD_LENGTH);
	if ((twi_cmd_err == 0) && (rx_reply[CMD_ACK_POS] == ACKTMNLV) && (rx_reply[V_SIGNATURE] == T_SIGNATURE)) {
		status_.signature = rx_reply[V_SIGNATURE];
		status_.version_major = rx_reply[V_MAJOR];
		status_.version_minor = rx_reply[V_MINOR];
		status_.features_code = rx_reply[V_FEATURES];
		status_.bootloader_start = (rx_reply[V_BOOT_ADDR_MSB] << 8) + rx_reply[V_BOOT_ADDR_LSB];
		status_.application_start = (rx_reply[V_APPL_ADDR_LSB] << 8) + rx_reply[V_APPL_ADDR_MSB];
		status_.trampoline_addr = (~(((rx_reply[V_APPL_ADDR_MSB] << 8) | rx_reply[V_APPL_ADDR_LSB]) & 0xFFF));
		status_.trampoline_addr++;
		status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
	}
  	else {
	    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, rx_reply[0]);
	    return(ERR_01);
  	}
  	return(OK);
}

// Get the Timonel running status
Timonel::status Timonel::GetStatus(void) {
	QueryStatus();
	return(status_);
}

// Function InitTiny
byte Timonel::InitTiny(void) {
	Wire.beginTransmission(addr_);
	Wire.write(INITTINY);
	Wire.endTransmission();
	Wire.requestFrom(addr_, (byte)1);
	//byte block_rx_size = 0;
	return(0);
}

// Function TwoStepInit
byte Timonel::TwoStepInit(word time) {
	delay(time);
	InitTiny();												/* Two-step Tiny85 initialization: STEP 1 */
	return(QueryStatus()); 											/* Two-step Tiny85 initialization: STEP 2 */
}

// Function WritePageBuff
byte Timonel::WritePageBuff(byte data_array[]) {
	const byte cmd_size = TXDATASIZE + 2;
	const byte reply_size = 2;
	byte twi_cmd[cmd_size] = { 0 };
	byte twi_reply_arr[reply_size] = { 0 };
	byte checksum = 0;
	twi_cmd[0] = WRITPAGE;
	for (int i = 1; i < cmd_size - 1; i++) {
		twi_cmd[i] = data_array[i - 1];
		checksum += (byte)data_array[i - 1];				/* Data checksum accumulator (mod 256) */
	}
	twi_cmd[cmd_size - 1] = checksum;
	byte wrt_errors = TwiCmdXmit(twi_cmd, cmd_size, ACKWTPAG, twi_reply_arr, reply_size);
	if (twi_reply_arr[0] == ACKWTPAG) {
		if (twi_reply_arr[1] != checksum) {
			USE_SERIAL.printf_P("[WritePageBuff] Data parsed with {{{ERROR}}} <<< Checksum = 0x%X\r\n", twi_reply_arr[1]);
			if (wrt_errors++ > 0) {						/* Checksum error detected ... */
				USE_SERIAL.printf_P("\n\r[WritePageBuff] Checksum Errors, Aborting ...\r\n");
				exit(wrt_errors);
			}
		}
	}
	else {
		USE_SERIAL.printf_P("[WritePageBuff] Error parsing %d command! <<< %d\r\n", twi_cmd[0], twi_reply_arr[0]);
		if (wrt_errors++ > 0) {							/* Opcode error detected ... */
			USE_SERIAL.printf_P("\n\r[WritePageBuff] Opcode Reply Errors, Aborting ...\n\r");
			exit(wrt_errors);
		}
	}
	return(wrt_errors);
}

// Upload a user application to an ATTiny85 running Timonel
byte Timonel::UploadApplication(const byte payload[], int payload_size, int start_address) {
	byte packet = 0;										/* Byte amount to be sent in a single I2C data packet */
	byte padding = 0;										/* Amount of padding bytes to match the page size */
	byte page_end = 0;										/* Byte counter to detect the end of flash mem page */
	byte page_count = 1;									/* Current page counter */
	byte upl_errors = 0;									/* Upload error counter */
	byte data_packet[TXDATASIZE] = { 0xFF };
	if ((payload_size % FLASHPGSIZE) != 0) {				/* If the payload to be sent is smaller than flash page size, resize it to match */
		padding = ((((int)(payload_size / FLASHPGSIZE) + 1) * FLASHPGSIZE) - payload_size);
		payload_size += padding;
	}
	USE_SERIAL.printf_P("\n[UploadFirmware] Writing payload to flash, starting at 0x%04X ...\n\n\r", start_address);
	for (int i = 0; i < payload_size; i++) {
		if (i < (payload_size - padding)) {
			data_packet[packet] = payload[i];				/* If there are data to fill the page, use it ... */
		}
		else {
			data_packet[packet] = 0xFF;						/* If there are no more data, complete the page with padding (0xff) */
		}
		if (packet++ == (TXDATASIZE - 1)) {					/* When a data packet is completed to be sent ... */
			for (int b = 0; b < TXDATASIZE; b++) {
				USE_SERIAL.printf_P(".");
			}
			upl_errors += WritePageBuff(data_packet);		/* Send data to T85 through I2C */
			packet = 0;
			delay(10);										/* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
		}
		if (upl_errors > 0) {
			//DeleteFlash();
			TwoStepInit(2000);
#if ESP8266
			ESP.restart();
#else
			resetFunc();
#endif /* ESP8266 */
			return(upl_errors);
		}
		if (page_end++ == (FLASHPGSIZE - 1)) {				/* When a page end is detected ... */

			USE_SERIAL.printf_P("P%d", page_count++);

			SetPageAddress(page_count * FLASHPGSIZE);

			delay(1000);										/* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

			if (i < (payload_size - 1)) {
				page_end = 0;
			}
		}
	}
	if (upl_errors == 0) {
		USE_SERIAL.printf_P("\n\n\r[UploadFirmware] Application was successfully transferred to T85, please select 'run app' command to start it ...\n\r");
	}
	else {
		USE_SERIAL.printf_P("\n\n\r[UploadFirmware] Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d\n\r", upl_errors);
		//DeleteFlash();
		TwoStepInit(2000);
#if ESP8266
		ESP.restart();
#else
		resetFunc();
#endif /* ESP8266 */
	}
	return(upl_errors);
}

// Function SetPageAddress
byte Timonel::SetPageAddress(word page_addr) {
	const byte cmd_size = 4;
	const byte reply_size = 2;
	byte twi_cmd_arr[cmd_size] = { STPGADDR, 0, 0, 0 };
	byte twi_reply_arr[reply_size];
	twi_cmd_arr[1] = ((page_addr & 0xFF00) >> 8);					/* Flash page address MSB */
	twi_cmd_arr[2] = (page_addr & 0xFF);							/* Flash page address LSB */
	twi_cmd_arr[3] = (byte)(twi_cmd_arr[1] + twi_cmd_arr[2]);		/* Checksum */
	USE_SERIAL.printf_P("\n\n\r[SetPageAddress] Setting flash page address on Attiny85 >>> %d (STPGADDR)\n\r", twi_cmd_arr[0]);
	byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, AKPGADDR, twi_reply_arr, reply_size);
	if (twi_cmd_err == 0) {
		//USE_SERIAL.printf_P("[SetPageAddress] Command %d parsed OK <<< %d\n\r", twi_cmd_arr[0], twi_reply_arr[0]);
		if (twi_reply_arr[1] == twi_cmd_arr[3]) {
			//USE_SERIAL.printf_P("[SetPageAddress] Operands %d and %d parsed OK by slave <<< ATtiny85 Flash Page Address Check = %d\n\r", twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
			USE_SERIAL.printf_P("[SetPageAddress] Address %04X made by %02X and %02X parsed OK by slave <<< ATtiny85 Check = %d\n\r", page_addr, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
		}
		else {
			USE_SERIAL.printf_P("[SetPageAddress] Operand %d parsed with {{{ERROR}}} <<< ATtiny85 Flash Page Address Check = %d\r\n", twi_cmd_arr[1], twi_reply_arr[1]);
		}
	}
	else {
		USE_SERIAL.printf_P("[SetPageAddress] Error parsing %d command! <<< %d\n\r", twi_cmd_arr[0], twi_reply_arr[0]);
	}
	return(twi_cmd_err);
}

// Ask Timonel to stop executing and run the user application
byte Timonel::RunApplication(void) {
	USE_SERIAL.printf_P("\n\r[RunApplication] Exit bootloader & run application >>> %d\r\n", EXITTMNL);
	return(TwiCmdXmit(EXITTMNL, ACKEXITT));
}

// Instruct Timonel to delete the user application
byte Timonel::DeleteApplication(void) {
	USE_SERIAL.printf_P("\n\r[DeleteFirmware] Delete Flash Memory >>> %d\r\n", DELFLASH);
	return(TwiCmdXmit(DELFLASH, ACKDELFL));
}

// Function DumpFlashMem
byte Timonel::DumpMemory(word flash_size, byte rx_data_size, byte values_per_line) {
	const byte cmd_size = 5;
	byte twi_cmd_arr[cmd_size] = { READFLSH, 0, 0, 0, 0 };
	byte twi_reply_arr[rx_data_size + 2];
	byte checksum_errors = 0;
	int v = 1;
	twi_cmd_arr[3] = rx_data_size;
	USE_SERIAL.printf_P("\n\n\r[DumpFlashMem] - Dumping Flash Memory ...\n\n\r");
	USE_SERIAL.printf_P("Addr %04X: ", 0);
	for (word address = 0; address < flash_size; address += rx_data_size) {
		twi_cmd_arr[1] = ((address & 0xFF00) >> 8);			/* Flash page address high byte */
		twi_cmd_arr[2] = (address & 0xFF);					/* Flash page address low byte */
		twi_cmd_arr[4] = (byte)(twi_cmd_arr[0] + twi_cmd_arr[1] + twi_cmd_arr[2] + twi_cmd_arr[3]); /* READFLSH Checksum */
		byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, ACKRDFSH, twi_reply_arr, rx_data_size + 2);
		if (twi_cmd_err == 0) {
			byte checksum = 0;
			for (byte i = 1; i < (rx_data_size + 1); i++) {
				USE_SERIAL.printf_P("%02X", twi_reply_arr[i]);							/* Memory values */
				if (v == values_per_line) {
					USE_SERIAL.printf_P("\n\r");
					if ((address + rx_data_size) < flash_size) {
						USE_SERIAL.printf_P("Addr %04X: ", address + rx_data_size);		/* Page address */
					}
					v = 0;
				}
				else {
					USE_SERIAL.printf_P(" ");											/* Space between values */
				}
				v++;
				checksum += (byte)twi_reply_arr[i];
			}
			if (checksum != twi_reply_arr[rx_data_size + 1]) {
				USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   %d\n\r", checksum);
				//USE_SERIAL.printf_P("%d\n\r", checksum + 1);
				//USE_SERIAL.printf_P(" <-- calculated, received --> %d\n\r", rx_reply[rx_data_size + 1]);
				if (checksum_errors++ == MAXCKSUMERRORS) {
					USE_SERIAL.printf_P("[DumpFlashMem] - Too many Checksum ERRORS, aborting! \n\r");
					delay(1000);
					exit(2);
				}
			}
		}
		else {
			USE_SERIAL.printf_P("[DumpFlashMem] - DumpFlashMem Error parsing %d command <<< %d\n\r", twi_cmd_arr[0], twi_reply_arr[0]);
			return(1);
		}
		delay(100);
	}
	return(0);
}

// Function TWI command transmit (Overload A: transmit command single byte)
byte Timonel::TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	const byte cmd_size = 1;
	byte twi_cmd_arr[cmd_size] = { twi_cmd };
	return(TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size));
}

// Function TWI command transmit (Overload B: transmit command multibyte)
byte Timonel::TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	for (int i = 0; i < cmd_size; i++) {
		Wire.beginTransmission(addr_);
		Wire.write(twi_cmd_arr[i]);
		Wire.endTransmission();
	}
	// Receive reply
	if (reply_size == 0) {
		Wire.requestFrom(addr_, ++reply_size);
		byte reply = Wire.read();
		if (reply == twi_reply) {						/* I2C reply from slave */
			USE_SERIAL.printf_P("[TWICmdXmit] Command %d parsed OK <<< %d\n\n\r", twi_cmd_arr[0], reply);
			return(0);
		}
		else {
			USE_SERIAL.printf_P("[TWICmdXmit] Error parsing %d command <<< %d\n\n\r", twi_cmd_arr[0], reply);
			return(1);
		}
	}
	else {
		byte reply_length = Wire.requestFrom(addr_, reply_size);
  		for (int i = 0; i < reply_size; i++) {
	    	twi_reply_arr[i] = Wire.read();
  		}
	 	if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
			//USE_SERIAL.printf_P("[TWICmdXmit] Multibyte command %d parsed OK <<< %d\n\n\r", twi_cmd_arr[0], twi_reply_arr[0]);
			return(0);
		}
		else {
			USE_SERIAL.printf_P("[TWICmdXmit] Error parsing %d multibyte command <<< %d\n\n\r", twi_cmd_arr[0], twi_reply_arr[0]);
			return(2);
		}		  
	}
}
