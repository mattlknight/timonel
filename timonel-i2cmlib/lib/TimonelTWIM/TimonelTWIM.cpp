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
	//struct status status_;
  	byte ack_rx[V_CMD_LENGTH] = { 0 };  					/* Data received from I2C slave */
	TWICmdXmit(GETTMNLV, ACKTMNLV, ack_rx, V_CMD_LENGTH);
	if ((ack_rx[CMD_ACK_POS] == ACKTMNLV) && (ack_rx[V_SIGNATURE] == T_SIGNATURE)) {
		status_.signature = ack_rx[V_SIGNATURE];
		status_.version_major = ack_rx[V_MAJOR];
		status_.version_minor = ack_rx[V_MINOR];
		status_.features_code = ack_rx[V_FEATURES];
		status_.bootloader_start = (ack_rx[V_BOOT_ADDR_MSB] << 8) + ack_rx[V_BOOT_ADDR_LSB];
		status_.application_start = (ack_rx[V_APPL_ADDR_LSB] << 8) + ack_rx[V_APPL_ADDR_MSB];
		status_.trampoline_addr = (~(((ack_rx[V_APPL_ADDR_MSB] << 8) | ack_rx[V_APPL_ADDR_LSB]) & 0xFFF));
		status_.trampoline_addr++;
		status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
	}
  	else {
	    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, ack_rx[0]);
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
void Timonel::InitTiny(void) {
	Wire.beginTransmission(addr_);
	Wire.write(INITTINY);
	Wire.endTransmission();
	Wire.requestFrom(addr_, (byte)1);
	//byte block_rx_size = 0;
}

// Function TwoStepInit
void Timonel::TwoStepInit(word time) {
	delay(time);
	InitTiny();												/* Two-step Tiny85 initialization: STEP 1 */
	QueryStatus(); 											/* Two-step Tiny85 initialization: STEP 2 */
}

// Function WritePageBuff
byte Timonel::WritePageBuff(byte data_array[]) {
	const byte tx_size = TXDATASIZE + 2;
	byte cmd_tx[tx_size] = { 0 };
	byte comm_errors = 0;									/* I2C communication error counter */
	byte checksum = 0;
	cmd_tx[0] = WRITPAGE;
	for (int b = 1; b < tx_size - 1; b++) {
		cmd_tx[b] = data_array[b - 1];
		checksum += (byte)data_array[b - 1];
	}
	cmd_tx[tx_size - 1] = checksum;
	byte ack_rx[2] = { 0 };   // Data received from slave
	TWICmdXmit(cmd_tx, tx_size, ACKWTPAG, ack_rx, 2);
	if (ack_rx[0] == ACKWTPAG) {
		if (ack_rx[1] == checksum) {
		}
		else {
			USE_SERIAL.printf_P("[WritePageBuff] Data parsed with {{{ERROR}}} <<< Checksum = 0x%X\r\n", ack_rx[1]);
			if (comm_errors++ > 0) {						/* Checksum error detected ... */
				USE_SERIAL.printf_P("\n\r[WritePageBuff] Checksum Errors, Aborting ...\r\n");
				exit(comm_errors);
			}
		}
	}
	else {
		USE_SERIAL.printf_P("[WritePageBuff] Error parsing %d command! <<< %d\r\n", cmd_tx[0], ack_rx[0]);
		if (comm_errors++ > 0) {							/* Opcode error detected ... */
			USE_SERIAL.printf_P("\n\r[WritePageBuff] Opcode Reply Errors, Aborting ...\n\r");
			exit(comm_errors);
		}
	}
	return(comm_errors);
}

// Upload a user application to an ATTiny85 running Timonel
byte Timonel::UploadApplication(const byte payload[], int payload_size, int start_address) {
	byte packet = 0;										/* Byte counter to be sent in a single I2C data packet */
	byte padding = 0;										/* Amount of padding bytes to match the page size */
	byte page_end = 0;										/* Byte counter to detect the end of flash mem page */
	byte page_count = 1;
	byte wrt_errors = 0;
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
			data_packet[packet] = 0xff;						/* If there are no more data, complete the page with padding (0xff) */
		}
		if (packet++ == (TXDATASIZE - 1)) {					/* When a data packet is completed to be sent ... */
			for (int b = 0; b < TXDATASIZE; b++) {
				USE_SERIAL.printf_P(".");
			}
			wrt_errors += WritePageBuff(data_packet);		/* Send data to T85 through I2C */
			packet = 0;
			delay(10);										/* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
		}
		if (wrt_errors > 0) {
			//DeleteFlash();
			TwoStepInit(2000);
#if ESP8266
			ESP.restart();
#else
			resetFunc();
#endif /* ESP8266 */
			return(wrt_errors);
		}
		if (page_end++ == (FLASHPGSIZE - 1)) {				/* When a page end is detected ... */

			USE_SERIAL.printf_P("%d", page_count++);
			//DumpPageBuff(FLASHPGSIZE, TXDATASIZE, TXDATASIZE);
			delay(100);										/* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

			if (i < (payload_size - 1)) {
				page_end = 0;
			}
		}
	}
	if (wrt_errors == 0) {
		USE_SERIAL.printf_P("\n\n\r[UploadFirmware] Application was successfully transferred to T85, please select 'run app' command to start it ...\n\r");
	}
	else {
		USE_SERIAL.printf_P("\n\n\r[UploadFirmware] Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d\n\r", wrt_errors);
		//DeleteFlash();
		TwoStepInit(2000);
#if ESP8266
		ESP.restart();
#else
		resetFunc();
#endif /* ESP8266 */
	}
	return(wrt_errors);
}

// Ask Timonel to stop executing and run the user application
byte Timonel::RunApplication(void) {
	USE_SERIAL.printf_P("\n\r[RunApplication] Exit bootloader & run application >>> %d\r\n", EXITTMNL);
	return(TWICmdXmit(EXITTMNL, ACKEXITT));
}

// Instruct Timonel to delete the user application
byte Timonel::DeleteApplication(void) {
	USE_SERIAL.printf_P("\n\r[DeleteFirmware] Delete Flash Memory >>> %d\r\n", DELFLASH);
	return(TWICmdXmit(DELFLASH, ACKDELFL));
}

// Function DumpFlashMem
byte Timonel::DumpFlashMem(word flash_size, byte rx_data_size, byte values_per_line) {
	const byte tx_size = 5;
	byte cmd_tx[tx_size] = { READFLSH, 0, 0, 0, 0 };
	//byte tx_size = 5;
	byte checksum_errors = 0;
	int v = 1;
	cmd_tx[3] = rx_data_size;

	//byte transmitData[1] = { 0 };
	USE_SERIAL.printf_P("\n\n\r[DumpFlashMem] - Dumping Flash Memory ...\n\n\r");
	USE_SERIAL.printf_P("Addr %X :    ", 0);

	for (word address = 0; address < flash_size; address += rx_data_size) {
		//byte rx_data_size = 0;							/* Requested T85 buffer data size */
		//byte dataIX = 0;									/* Requested T85 buffer data start position */
		cmd_tx[1] = ((address & 0xFF00) >> 8);				/* Flash page address high byte */
		cmd_tx[2] = (address & 0xFF);						/* Flash page address low byte */
		cmd_tx[4] = (byte)(cmd_tx[0] + cmd_tx[1] + cmd_tx[2] + cmd_tx[3]); /* READFLSH Checksum */

		byte ack_rx[rx_data_size + 2];

		byte errors = TWICmdXmit(cmd_tx, tx_size, ACKRDFSH, ack_rx, rx_data_size + 2);

		// for (int i = 0; i < tx_size; i++) {
		// 	transmitData[i] = cmd_tx[i];
		// 	Wire.beginTransmission(addr_);
		// 	Wire.write(transmitData[i]);
		// 	Wire.endTransmission();
		// }

		// Receive acknowledgement
		// byte block_rx_size = Wire.requestFrom(addr_, (byte)(rx_data_size + 2));
		// byte ack_rx[rx_data_size + 2];   // Data received from slave
		// for (int i = 0; i < block_rx_size; i++) {
		// 	ack_rx[i] = Wire.read();
		// }

		//if (ack_rx[0] == ACKRDFSH) {
		if (errors == 0) {
			//USE_SERIAL.print("ESP8266 - Command ");
			//USE_SERIAL.print(cmd_tx[0]);
			//USE_SERIAL.print(" parsed OK <<< ");
			//USE_SERIAL.println(ack_rx[0]);
			byte checksum = 0;
			for (byte i = 1; i < (rx_data_size + 1); i++) {
				if (ack_rx[i] < 16) {
					//USE_SERIAL.print("0x0");
					USE_SERIAL.print("0");
				}
				//else {
				//	USE_SERIAL.print("0x");
				//}
				USE_SERIAL.printf_P("%X", ack_rx[i]);		/* Byte values */
				//checksum += (ack_rx[i]);
				if (v == values_per_line) {
					USE_SERIAL.printf_P("\n\r");
					if ((address + rx_data_size) < flash_size) {
						USE_SERIAL.printf_P("Addr %X", address + rx_data_size);
						if ((address + rx_data_size) < 0x1000) {
							if ((address + rx_data_size) < 0x100) {
								USE_SERIAL.printf_P(":   ");
							}
							else {
								USE_SERIAL.printf_P(":  ");
							}
						}
						else {
							USE_SERIAL.printf_P(": ");
						}
					}
					v = 0;
				}
				else {
					USE_SERIAL.printf_P(" ");
				}
				v++;
				//USE_SERIAL.printf_P(" |\n\r");
				checksum += (byte)ack_rx[i];
			}
			//if (checksum + 1 == ack_rx[rx_data_size + 1]) {
			if (checksum == ack_rx[rx_data_size + 1]) {
				//USE_SERIAL.printf_P("   >>> Checksum OK! <<<   ");
				//USE_SERIAL.printf_P("%d\r\n", checksum);
			}
			else {
				USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   %d\n\r", checksum);
				//USE_SERIAL.printf_P("%d\n\r", checksum + 1);
				//USE_SERIAL.printf_P(" <-- calculated, received --> %d\n\r", ack_rx[rx_data_size + 1]);

				if (checksum_errors++ == MAXCKSUMERRORS) {
					USE_SERIAL.printf_P("[DumpFlashMem] - Too many Checksum ERRORS, aborting! \n\r");
					delay(1000);
					exit(1);
				}
			}
		}
		else {
			USE_SERIAL.printf_P("[DumpFlashMem] - DumpFlashMem Error parsing %d command <<< %d\n\r", cmd_tx[0], ack_rx[0]);
			return(1);
		}
		delay(100);
	}
	return(0);
}

// Function TWI command transmit (Overload A: transmit command single byte)
byte Timonel::TWICmdXmit(byte twi_command, byte twi_acknowledge, byte reply_array[], byte reply_size) {
	const byte sc_length = 1;
	byte twi_cmd_array[sc_length] = { twi_command };
	return(TWICmdXmit(twi_cmd_array, sc_length, twi_acknowledge, reply_array, reply_size));
}

// Function TWI command transmit (Overload B: transmit command multi byte)
byte Timonel::TWICmdXmit(byte twi_cmd_array[], byte cmd_size, byte twi_acknowledge, byte reply_array[], byte reply_size) {
	for (int i = 0; i < cmd_size; i++) {
		Wire.beginTransmission(addr_);
		Wire.write(twi_cmd_array[i]);
		Wire.endTransmission();
	}
	// Receive reply
	if (reply_size == 0) {
		Wire.requestFrom(addr_, ++reply_size);
		byte reply = Wire.read();
		if (reply == twi_acknowledge) {						/* I2C reply from slave */
			USE_SERIAL.printf_P("[TWICmdXmit] Command %d parsed OK <<< %d\n\n\r", twi_cmd_array[0], reply);
			return(0);
		}
		else {
			USE_SERIAL.printf_P("[TWICmdXmit] Error parsing %d command <<< %d\n\n\r", twi_cmd_array[0], reply);
			return(1);
		}
	}
	else {
		byte reply_length = Wire.requestFrom(addr_, reply_size);
  		for (int i = 0; i < reply_size; i++) {
	    	reply_array[i] = Wire.read();
  		}
	 	if ((reply_array[0] == twi_acknowledge) && (reply_length == reply_size)) {
			//USE_SERIAL.printf_P("[TWICmdXmit] Command %d parsed OK <<< %d\n\n\r", twi_cmd_array[0], reply_array[0]);
			return(0);
		}
		else {
			//USE_SERIAL.printf_P("[TWICmdXmit] Error parsing %d command <<< %d\n\n\r", twi_cmd_array[0], reply_array[0]);
			return(1);
		}		  
	}
}