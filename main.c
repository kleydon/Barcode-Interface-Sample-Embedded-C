//Bar Code Reader -> Bluetooth
//
//OVERVIEW
//This application is for relaying barcodes from a keychain barcode reader to a mobile phone via bluetooth.
//
//COMPONENTS, TECHNICAL DETAILS
//The application runs on a PIC16F688 processor. The application was written/compiled using the MPLAB IDE with SourceBoost's 
//C compiler, with optimization "on" for both compiling and linking. The barcode reader is Symbol's CS-1504. The bluetooth 
//module that communicates with the phone is an EB101, from A7 Engineering. The mobile phone's bluetooth server is written 
//in python for an S60v3 mobile phone. 
//
//FUNCTIONAL DESCRIPTION
//When the bar code reader's button is pressed, the application waits for a time, then wakes the bar code 
//reader and aks if there are any barcodes. If there are, the application gets and verifies the first one, 
//erases all stored barcodes in barcode reader flash, wakes the bluetooth module, and attempts to connect 
//to a mobile phone over bluetooth. If connection is successful, the bar code is sent to the phone.
//The application then checks to see if the barcode reader has obtained any new barcodes in the meantime,
//and if so, relays the first and deletes all barcodes from barcode reader flash. The application ends up in 
//a sleep state, where power use is minimized. LED1 provides some feedback; it blinks twice quickly after a barcode
//has successfully been read from the barcode reader, then once when the system has successfully established a 
//bluetooth connection with a mobile phone.
//To check whether or not the system is connecting with a mobile phone properly, tap the barcode reader's button
//quickly, and an "are you awake?" signal is sent over bluetooth to the mobile phone. (The phone's corresponding 
//application has been designed to make an "I am awake!" noise.) 
//
//The application has three special modes, activated by holding the dedicated circuit's Button1 and reset button 
//down, then releasing the reset button:
//1)  GET BLUETOOTH ADDRESS MODE: If, upon releasing the reset button, button1 is held down until the dedicated circuit's 
//LED1 blinks once and is then released, the application enters "obtaining mobile phone bluetooth address" mode. LED1 
//lights up until the A7 Bluetooth module is connected to by a mobile phone, using pass-key "0000". (For the system to 
//work properly, the mobile phone should be paired with the A7 Bluetooth module, and the bluetooth module should be 
//"authorized" to connect automatically, without having to have a passkey entered every time.) Upon successfully
//obtaining and storing phone's bluetooth address, LED1 will blink 4 times; if the system instead times out, LED1
//will blink 7 times quickly.
//
//2)  RESET SYSTEM DEFAULTS MODE: If, upon releasing the reset button, button1 is held down until the dedicated circuit's 
//LED1 blinks twice and is then released, the application enters "resetting system defaults mode". This mode
//automatically resets the bluetooth module and barcode reader to factory defaults and then makes a few necessary
//customizations (The bluetooth module is named "BarCodeKey", the bluetooth module's encryption of serial data is turned off,
//the barcode reader's "connect to host" and "disconnect from host" beeps are turned off, and the barcode reader's ability
//to manually toggle sound on and off is rendered inaccessible). Upon successful completion, the mode blinks 3 times slowly; 
//upon unsuccessful completion, it blinks 6 times quickly.
//
//3)  BLUETOOTH CONSOLE MODE: If, upon releasing the reset button, Button1 is held down until the dedicated circuit's LED1 
//blinks three times, the application enters Bluetooth Console Mode. In this mode, bluetooth module commands can be 
//entered from a connected PC's terminal window and carried out by pressing return. (A7 EB101 BLuetooth commands are outlined 
//in the EB101 protocol document.) Upon entry into the mode, a "PC:" command-line prompt should appear in the window; if it
//doesn't press return a few times. Bluetooth Console Mode can be exited by typing "out<CR>", or pressing the reset button. 
//For bluetooth console mode, the PC rs232 serial terminal window settings should be 9600bps, 1 stop bit, odd-parity, 
//no flow control. While the bluetooth module is set up for *no* parity, the PC is talking to it via a microprocessor that 
//gets configured for odd parity while talking on its wired (i.e. bar code reader or PC) channel. Bluetooth console mode 
//requires that two of the circuit board's four three-way jumpers (UC-BCR-PC) connect microprocessor (UP) signals 
//to (PC) signals. The signals are: RX and TX, from the microprocessor's perspective. (The HOST_IN and DATA_READY jumpers
//should *not* be connected from UC to PC for this mode). This mode furthermore requires that RX and TX signals get swapped, 
//since in PC-UC communications, the PC is the host. In other permutations of the three-way connection, no swap is necessary
//since the UP is either the rs232 host, or it is not connected. 
//
//In order to verify that the barcode reader is functioning properly, all 4 three-way jumpers should connect between PC and 
//BCR. The PC-based shareware application "HDS1504.exe" "Host Driver" can be used to connect to the barcode reader and verify its 
//functionality.


//THINGS TO FIX...
//* Sometimes you scan a barcode and the application starts up, but the application does not get a "data is ready" signal.
//Perhaps the delay between waking up the barcode reader and reading the data ready line is too short/long?
//* Sometimes when you press the barcode reader's button when the system is awake and doing something, the system stays
//awake, its state when this happens is unclear.
//* MAX_BT_SEND_INTER_CHAR_RESPONSE_DELAY 1000 - too long?


#include "PIC16F688.h"
#include <system.h>


//Configuration Register Bits
#pragma DATA _CONFIG, _WDT_OFF & _MCLRE_ON & _BOD_OFF  & _INTRC_OSC_NOCLKOUT & _PWRTE_ON & _FCMEN_OFF & _IESO_OFF 


//Limits
//+++++++++++++++++++++++++++++++++++++++++++++
#define MAX_U16 65535
//---------------------------------------------


//State Machine States
//+++++++++++++++++++++++++++++++++++++++++++++
typedef enum {
	STATE_INITIAL=0,
	STATE_PROGRAMMING_DEFAULTS,
	STATE_BT_CONSOLE,
	STATE_GET_BLUETOOTH_TO_ADDRESS,
	STATE_ASLEEP_SECONDARY_POWER_OFF,
	STATE_GETTING_BARCODE_FROM_READER,
	STATE_SENDING_BARCODE_OVER_BLUETOOTH,
	STATE_SENDING_RUAWAKE_OVER_BLUETOOTH,
} STATE_T; 
//--------------------------------------------


//Flavors of "Done"
//+++++++++++++++++++++++++++++++++++++++++++++
typedef enum {
	ISNT_DONE=0,
	DONE_TIMED_OUT,
	DONE_SUCCESS,
	DONE_FAILURE,
} DONE_T; 
//--------------------------------------------


//Flavors of Barcode Validity
//+++++++++++++++++++++++++++++++++++++++++++++
typedef enum {
	BC_VALID=1,
	BC_NOT_VALID_LENGTH_ZERO,
	BC_NOT_VALID_LENGTH_TOO_LONG,
	BC_NOT_VALID_TYPE,
	BC_NOT_VALID_CHARACTERS,
} BC_VALIDITY_T; 
//--------------------------------------------


//Buffers and String Constants
//+++++++++++++++++++++++++++++++++++++++++++++
#define RX_BUFF_LENGTH 40
unsigned char rx_buff[RX_BUFF_LENGTH];

#define BARCODE_BUFF_LENGTH 20
unsigned char barcode_buff[BARCODE_BUFF_LENGTH];

#define MAX_STR_BUFF_LENGTH 25
unsigned char str_buff[MAX_STR_BUFF_LENGTH];

#define BT_ADDRESS_LENGTH 17
//Bluetooth address bytes are held at the base of EEPROM memory

const unsigned char dashes_s[] = "\n\r-\n\r";
const unsigned char ack_s[] = "ACK\r>"; //Most of the time, we're only looking for the first three chars
//--------------------------------------------


//Delay/Timebase Stuff
//+++++++++++++++++++++++++++++++++++++++++++++
//NOTE: Delays clipped to between min and max. In timer wraps ~ ms
#define MINIMUM_DELAY (10) 
#define MAXIMUM_DELAY (40000) 

typedef struct {
	unsigned short start_tick;
	unsigned short end_tick;
	unsigned char will_wrap; 
} interval;
unsigned short timer0_isr_count=0; 

#define SERIAL_SELECT_DELAY (30)
#define SECONDARY_POWER_DELAY (30)

#define BCR_BUTTON_RELEASE_TO_BCR_WAKEUP_DELAY 3000  //Could be tuned?
#define BCR_WAKEUP_TO_INTERROGATE_DELAY 230  //Could be tuned?
#define INTERROGATE_TO_DR_READY_DELAY 230 //Could be tuned?
#define MAX_BCR_RESPONSE_DELAY 2000

#define MAX_BT_INTER_CHAR_RESPONSE_DELAY 400
#define MAX_BCR_INTER_CHAR_RESPONSE_DELAY 400
#define MAX_BT_SEND_INTER_CHAR_RESPONSE_DELAY 1500

#define NUM_BT_CMD_TRIES 4
#define NUM_BCR_CMD_TRIES 4
#define NUM_BT_SEND_TRIES 6
//--------------------------------------------


//Command/Response Info
//+++++++++++++++++++++++++++++++++++++++++++++
#define BCR_INTERROGATE_CMD_LENGTH	5
const unsigned char bcr_interrogate_cmd[] = {0x01, 0x02, 0x00, 0x9F, 0xDE};
#define BCR_INTERROGATE_RESPONSE_LENGTH 23
#define BCR_INTERROGATE_RESPONSE_START_LENGTH 2
const unsigned char bcr_interrogate_response_start[] = {0x06,0x02};

#define BCR_UPLOAD_CMD_LENGTH 5
const unsigned char bcr_upload_cmd[] = {0x07, 0x02, 0x00, 0x9E, 0x3E};
#define BCR_UPLOAD_RESPONSE_START_LENGTH 2
#define BCR_UPLOAD_RESPONSE_MINIMUM_LENGTH 14
const unsigned char * bcr_upload_response_start = bcr_interrogate_response_start;

#define BCR_CLEAR_BARCODES_CMD_LENGTH 5
const unsigned char bcr_clear_barcodes_cmd[] = {0x02, 0x02, 0x00, 0x9F, 0x2E};
#define BCR_CLEAR_BARCODES_RESPONSE_LENGTH 5
const unsigned char bcr_clear_barcodes_response[] = {0x06, 0x02, 0x00, 0x5E, 0x6F};

#define BCR_POWER_DOWN_CMD_LENGTH 5
const unsigned char bcr_power_down_cmd[] = {0x05, 0x02, 0x00, 0x5E, 0x9F};
#define BCR_POWER_DOWN_RESPONSE_LENGTH 5
const unsigned char * bcr_power_down_response = bcr_clear_barcodes_response; 

#define BCR_RESTORE_DEFAULTS_CMD_LENGTH 7
const unsigned char bcr_restore_defaults_cmd[] = {0x04, 0x02, 0x01, 0x01, 0x00, 0xD7, 0x7B};
#define BCR_RESTORE_DEFAULTS_RESPONSE_LENGTH 8
const unsigned char bcr_restore_defaults_response[] = {0x06, 0x02, 0x02, 0x01, 0x01, 0x00, 0xAA, 0xD7};

#define BCR_CUSTOMIZE_DEFAULTS_CMD_LENGTH 14
const unsigned char bcr_customize_defaults_cmd[] = {0x03, 0x02, 0x02, 0x0A, 0x00, 0x02, 0x0B, 0x00, 0x02, 0x55, 0x00, 0x00, 0x38, 0x78};
#define BCR_CUSTOMIZE_DEFAULTS_RESPONSE_LENGTH 14
const unsigned char bcr_customize_defaults_response[] = {0x06, 0x02, 0x02, 0x0A, 0x01, 0x02, 0x0B, 0x01, 0x02, 0x55, 0x01, 0x00, 0xA8, 0x89};

#define MAX_BARCODE_LENGTH 20
#define FIRST_BARCODE_START_I 12
#define FIRST_BARCODE_TYPE_I 11
#define FIRST_BARCODE_STRLEN_I 10
//--------------------------------------------



//This flag is set when the system knows the barcode reader needs to be queried for any bar codes. 
//(The flag is set by an interrupt generated by the barcode reader button.) The flag is cleared 
//after the barcode reader is queried.
unsigned char query_bcr_f=0; //Flag set by the BCR Button Interrupt. Cleared after BCR has been queried




//Functions
//+++++++++++++++++++++++++++++++++++++++++++++
void TurnOnWDT(void) {
	wdtcon |= 0x01; //Set SWDTEN (Bit0) to 1
}
void TurnOffWDT(void) {
	wdtcon &= 0xFE; //Clear SWDTEN (Bit0)
}
void InitWDT(void) {
	TurnOffWDT();
	//Set prescaler to 0110 (1:8192) ~40 sec //40 SEC NEEDED TO PROGRAM BT ADR...
	wdtcon |= 0x10;
	wdtcon &= 0xF1;
}

void enable_EEPROM_writes(void) {
	eecon1 |= 0x04; //WREN
}
void disable_EEPROM_writes(void) {
	eecon1 &= 0xFB; //WREN
}
unsigned char read_EEPROM_byte(unsigned char pos) {
	eeadr = pos; //Write address into memory
	eecon1 &= 0x7F; //EEPGD - 0 accesses data memory
	eecon1 |= 0x01; //RD - Initiates a read
	while ( (eecon1&0x01) != 0 ){}; //Still reading
	return eedata;
}
void write_EEPROM_byte(unsigned char b, unsigned char pos) {

	intcon &= 0x7F; //Set GIE to 0 to disable interrupts globally

	eecon1 &= 0x7F; //EEPGD - 0 accesses data memory
	eeadr = pos; //Write address into register
	eedata = b; //Wite data into register
	eecon2=0x55; //Special Sequence, Step 1
	eecon2=0xAA; //Special Sequence, Step 2
	eecon1 |= 0x02; //Special Sequence, Step 3 - WR (Bit 1) - Initiates a write

	intcon |= 0x80; //Set GIE to 1 to enable interrupts globally

	while ( (eecon1&0x02) != 0 ){}; //Still writing
}
void BT_Address2Buff(const unsigned char * buff) {
	unsigned char i;
	for (i=0; i<BT_ADDRESS_LENGTH; i++) {
		buff[i] = read_EEPROM_byte(i);
	}
	buff[i] = '\0'; //for good measure
} 


void InitSysClk(void) {
	//Run from internal oscillar, set speed
	osccon |= 0x40; //Set IRCF to 100=1MHz
	osccon &= 0xCF;
	osccon &= 0xF7; //Clear OSTS to 0
	osccon &= 0xFE; //SCS set to 0 //Commented out to control with CONFIG FOSC<2:0>
} 
void InitTimer0(void) {
	timer0_isr_count=0;
	InitSysClk();
	option_reg &= 0xDF; //Clear T0CS to 0 so timer increments on instruction clock
	intcon &= 0xF8; //Clear all interrupt flags
	intcon |= 0x20; //Set TOIE to 1 to enable timer0 interrupt on overflow
	intcon |= 0x80; //Set GIE to 1 to enable interrupts globally
}
void EnableBcrButtonInterrupt(void) {
	intcon &= 0xFD; //Clear INTF interrupt flag
	intcon |= 0x10; //Set INTE (bit 4) to 1 to enable external RA2 interrupt
	intcon |= 0x80; //Set GIE to 1 to enable interrupts globally
}
void DisableBcrButtonInterrupt(void) {
	intcon &= 0xEF; //Clear INTE (bit 4) to 0 to disable external RA2 interrupt
}
void interrupt(void) {
	//NOTE: registers may not be preserved in interrupts by the SourceBoost c compiler; 
	//take care not to accidentally use them here.

	static unsigned char int_src; //should only be used here
	intcon &= 0x7F; //Temporarily disable all interrupts

	//Handle Timer0 Overflow
	int_src = intcon & 0x04;
	if(int_src) { //T0IF	
		timer0_isr_count++;
		intcon &= 0xFB; //Clear T0IF interrupt flag, ready for next
	}
	//Handle External Interrupt (RA2)
	int_src = intcon & 0x02; //INTF
	if (int_src) {
		query_bcr_f = 1;
		intcon &= 0xFD; //Clear INTF interrupt flag, ready for next
	}
	intcon |= 0x80; //Re-enable all interrupts
}

interval GetInterval(unsigned short wait) {
	interval res;
	res.start_tick = timer0_isr_count;
	res.end_tick = res.start_tick+wait;
	//If end_t val is wrong due to timer wrap...
	if (res.start_tick > res.end_tick) {
		res.end_tick = wait - (MAX_U16-res.start_tick);
		res.will_wrap = 1;
	} else {
		res.will_wrap = 0;
	}
	return res;
}

void ms_delay(unsigned short x) { //Presumes 1MHz clock and default timer-prescaler. Err on slow side...
	//Clip values (a safety...)
	if (x<MINIMUM_DELAY) { 
		x = MINIMUM_DELAY;
		return;
	} else if (x>MAXIMUM_DELAY) {
		x=MAXIMUM_DELAY;
	}
	interval di = GetInterval(x);
	if (di.will_wrap) {
		//Wait for timer to wrap around...
		while(timer0_isr_count>di.end_tick) {
			clear_wdt();
		} 
	}
	//Wait for the end of the delay
	while (timer0_isr_count < di.end_tick) {
		clear_wdt();
	}
}


void TurnLEDon(void) {
	portc |= 0x01 ;  //Set Port C Pin 1 to 1
}
void TurnLEDoff(void) {
	portc &= 0xFE ;  //Set Port C Pin 1 to 0

}
void BlinkLED(unsigned char num_reps, unsigned short on_time,  unsigned short off_time) {
	unsigned char i;
	for (i=0;i<num_reps;i++) {
		TurnLEDon();
		ms_delay(on_time);
		TurnLEDoff();
		ms_delay(off_time);	
	}
}
void InitLED(void) {
	TurnLEDoff();
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisc &= 0xFE; //Set LED to output; (TRISC0 = 0)
}


void ConfigPin2ForBTcontrol(void) {
	//NOTE: Requires secondary power, and 2:1 select line to be 1
	porta |= 0x20; //Set Pin 2 to 1
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisa &= 0xDF; //Configure Pin 2 as output; (TRISA5 to 0)
}
void ConfigPin2ForButton(void) {
	//NOTE: Requires secondary power, and 2:1 select line to be 0
	porta &= 0xDF; //Clear Pin 2 to 0
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisa |= 0x20; //Set Pin 2 as input; (TRISA5 to 1)
}
static unsigned char should_be_in_bt_command_mode_when_powered_and_bt_selected=0;
void EnterBTCommandMode(void) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 1
	porta &= 0xDF; //Clear RA5 (Pin 2) to 0
	ms_delay(20);
	should_be_in_bt_command_mode_when_powered_and_bt_selected = 1;
}
unsigned char InBTCommandMode(void) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 1
	return (porta&0x20==0); //RA5 (Pin 2) is 0
}
void ExitBTCommandMode(void) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 1
	porta |= 0x20; //Set RA5 (Pin 2) to 1
	ms_delay(20);
	should_be_in_bt_command_mode_when_powered_and_bt_selected = 0;
}
unsigned char ButtonIsDown(void) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 0
	return ((porta & 0x20)==0);
}
unsigned char ButtonHeldDown(unsigned short duration) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 0
	if (ButtonIsDown()) {
		interval hi = GetInterval(duration); //hold interval
		while (1) {
			if (hi.will_wrap) {
				if (timer0_isr_count<hi.end_tick) { //wrap has happened
					hi.will_wrap = 0; //no longer anticipating wrap
				}
			}	
			else if (timer0_isr_count>=hi.end_tick) {
				return 1;
			}
		
			if (!ButtonIsDown()) {
				return 0;
			}
			clear_wdt();
		}
	}
	//Otherwise...
	return 0;
}
void bl_test(void) {
	//NOTE: Assumes secondary power, and 2:1 select line to be 0
	while(1) {
		if (ButtonIsDown()) {
			TurnLEDon();
			while(ButtonIsDown());
		} 
		else {
			TurnLEDoff();
		}
		clear_wdt();
	}
}


unsigned char BCRButtonIsDown(void) {
	return ((porta & 0x04)!=0); //opposite from other button
}

void InitBCRButton(void) {
	porta &= 0xFB; //Clear A2
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisa |= 0x04; //Set bar code reader button pin as an input; (TRISA2 to 1)
}


void TurnSecondaryPowerOn(void) {
	if ( (portc&0x04)==0 ) {
		portc |= 0x04 ;  //Set C2 (Pin 8) to 1
		ms_delay(SECONDARY_POWER_DELAY);
	}
}
void TurnSecondaryPowerOff(void) {
	if ((portc&0x04)!=0) {
		portc &= 0xFB ;  //Set C2 (Pin 8) to 0
		ms_delay(SECONDARY_POWER_DELAY);
	}
}
void InitSecondaryPower(void) {
	TurnSecondaryPowerOff(); //Clear C2
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisc &= 0xFB; //Set C2, (Pin 8) to output
}


void ConfigSerialForBlueTooth(void) {
	//No parity bit 
	txsta = 0x24; 
	rcsta = 0x90; 
}

void ConfigSerialForWired(void) {
	//Includes parity bit
	txsta = 0x64; 
	rcsta = 0xD0; 
}

void InitSerial(void) {

	//osctune = 0x0F; //max frequency
	osctune = 0x00; //middle frequency
	//osctune = 0x10; //minimum frequency

	//TX Pin - output
	trisc &= 0xEF; //TRISC4 = 0;

	//RX Pin - input
	trisc |= 0x20; //TRISC5 = 1;

	//RX & TX Settings 
	//8bit, Asynchronous mode, High speed, enable RX/TX 
	//Use odd parity bit if communicating with wired bar code reader or PC, 
	//but no parity bit if talking to bluetooth module
 	ConfigSerialForWired();

	//Run from internal oscillar, set speed
	InitSysClk();

	// Baudcontrol - disable auto mode, 8bit baudrate, set BRG16 (bit 3) to 1
	baudctl = 0x00;
	baudctl |= 0x08; //Set BRG16 (bit 3) to 1

	// Set Baudrade - 9600 (from datasheet baudrade table)
	spbrg = 25;
}


void SerialSelectWired(void) {
	ms_delay(SERIAL_SELECT_DELAY); 
	portc &= 0xF7 ;  //Set C3 (Pin 7) to 0
	ConfigSerialForWired();	
	ConfigPin2ForButton();
	ms_delay(SERIAL_SELECT_DELAY); 
}

void SerialSelectBlueTooth(void) {
	ms_delay(SERIAL_SELECT_DELAY); 
	portc |= 0x08 ;  //Set C3 (Pin 7) to 1
	ConfigSerialForBlueTooth();
	ConfigPin2ForBTcontrol();
	if (!InBTCommandMode() && should_be_in_bt_command_mode_when_powered_and_bt_selected==1) {
		EnterBTCommandMode();
	}
	ms_delay(SERIAL_SELECT_DELAY);
}

unsigned char BlueToothSerialSelected(void) {
	return ((portc & 0x08)!=0);
}

void InitSerialSelect(void) {
	SerialSelectWired();
	cmcon0 = 0x07; //All comparators off, pins free for digital i/o
	ansel &= 0x7F; //Set ANS7 to digital i/o	
	trisc &= 0xF7; //Set C3, (Pin 7) to 0 for output
}


void SetHIto(unsigned char val) { 	//Setting HI to 1 upsets the data flow to PC for some reason...
	if (val) { //Clear
		porta &= 0xEF; //Clear A4
	}
	else { //Set
		porta |= 0x10; //Set A4
	}
}	
unsigned char GetHI(void) {
	return ((porta & 0x10)==0);
}
void Init_HI_DR(void) {
	//HI
	SetHIto(0); //Set corresponding port bit
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisa &= 0xEF; //Set "host_in" (pin 3) as an output; (TRISA4 to 0)

	//DR
	portc &= 0xFD ;  //Set C1 (Pin 9) to 0
	cmcon0 = 0x07; //Ensure comparator pins set for digital I/O 
	ansel = 0x00; //Ensure analog pins set for digital I/O
	trisc |= 0x02; //Set "data read" (pin 9) to input; (TRISC1 = 1)
}
unsigned char GetDR(void) {
	return ((portc & 0x02)==0);
}


unsigned char set_tx_parity_bit(unsigned char byte) {
	//With odd parity, the parity bit is selected so that the number of 1-bits 
	//in a byte, including the parity bit, is odd. 	
	unsigned char num_ones;
	num_ones = 0;
	while (byte!=0) {
		if (byte&0x01) {
			num_ones++;
		}
		byte >>= 1;
	}
	
	//If the number of ones in the byte-to-be-sent is already odd...
	if (num_ones&0x01) {  //clear the parity bit to 0 
		txsta &= 0xFE; 
		return 0;
	}
	//Else the number of ones in the byte-to-be-sent is even..
	txsta |= 0x01; //set the parity bit to 1
	return 1;
}	

void  WriteChar(unsigned char byte) {
	// wait until register is empty 
	while(!(pir1 & 0x2)) { //TXIF
		clear_wdt();
	}
	if(!BlueToothSerialSelected()) {
		//Set the parity bit
		set_tx_parity_bit(byte);
	}
	// transmite byte	
	txreg = byte;
}

unsigned char ReadChar(void) {
	//If there are any over-run errors, clear them
	if (rcsta & 0x02) { //OERR (Bit 1)
		rcsta &= 0xEF ; //Clear CREN to 0 (Bit 4)
		rcsta |= 0x10 ; //Set CREN to 1 (Bit 4)
	}
	//Waiting to receive a character...
	while(!(pir1 & 0x20)) { //RXIF
		clear_wdt();
	}
	//Received a character!
	return rcreg;	
}

void FlushRxHwBuffer(void) {
	unsigned char temp;

	//While there are characters in the buffer...
	while(pir1 & 0x20) { //RXIF
		unsigned char temp = rcreg; //empty a character from the buffer
		clear_wdt();
	}

	//If there are any over-run errors, clear them
	if (rcsta & 0x02) { //OERR (Bit 1)
		rcsta &= 0xEF ; //Clear CREN to 0 (Bit 4)
		rcsta |= 0x10 ; //Set CREN to 1 (Bit 4)
	}
}

void EraseBuffer(const unsigned char * buff, unsigned char len) {
	unsigned char i = 0;
	for (i=0;i<len;i++) {
		buff[i] = 0;
	}
}

//Nibble (0-15) to ascii char
unsigned char ntoa(unsigned char n) {
	if (n>=0 && n<10) { //0..9
		n+=48;
		return n;
	}
	else if (n>9 && n<16) { //A..F
		n+=55;
		return n;
	}
 	return 0xFF;  //Error val
}

//Byte to ascii string (2 chars + eol)
void b2str_buff(unsigned char b) {
	if (b<=0x0F) { //One nibble...
		*(str_buff+1) = '\0';
		*(str_buff) = ntoa(b);
	}
	else { //Two nibbles
		*(str_buff+2) = '\0';
		*(str_buff+1) = ntoa((b&0x0F));
		*(str_buff) = ntoa((b>>4));
	}
}

//Numeric Ascii Char -> Decimal Digit
unsigned char a2d(unsigned char a) {
	if (a>47 && a<58) {
		return (a-48);
	}
	else { //Error val
		return 0xFF;
	}	
}

unsigned char buff_equal(const unsigned char * test_buff, const unsigned char * test_against_buff, unsigned char len) {
	unsigned char i;
	for(i=0;i<len; i++) {
		if(test_buff[i]!=test_against_buff[i]) {
			return 0;
		}
	}
	return 1;
}

unsigned char str_equal(const unsigned char * str1, const unsigned char * str2, unsigned char len) {
	unsigned char i;
	for(i=0;i<len; i++) {
		if(str1[i]!=str2[i]) {
			return 0;
		}
		if (str1[i]=='\0' || str2[i]=='\0') {
			return 1;
		}
	}
	return 1;
}

unsigned short strlen(const unsigned char * str) {
	unsigned char i;
	for(i=0; i<MAX_STR_BUFF_LENGTH; i++) {
		if (str[i]=='\0') {
			break;
		}
	}
	return i;
}

void copy_buffer_from_to(const unsigned char * from_buff, const unsigned char * to_buff, unsigned char len) {
	unsigned char i; 
	for (i=0;i<len;i++) {
		to_buff[i]=from_buff[i];
	}
}

void WriteStr(const unsigned char * buff) {
	unsigned char i;
	clear_wdt();
	for (i=0; (*buff!='\0' && i<MAX_STR_BUFF_LENGTH); i++) {
		WriteChar(*buff);
		buff++;
	}
}

void WriteBuff(unsigned char * buff, unsigned char len) {
	unsigned char i;
	clear_wdt();
	for (i=0;i<len; i++) {
		WriteChar(*(buff+i));
	}
}

void PrintBufferBytes(const unsigned char * buff, unsigned char len) {
	WriteStr(dashes_s);
	unsigned char i;
	for (i=0; i<len; i++) {
		WriteStr(" 0x");
		if (buff[i]<16) { //leading zero
			WriteStr("0");
		}
		b2str_buff(buff[i]);
		WriteStr(str_buff);
	}
	WriteStr(dashes_s);
}


unsigned char ListenForResponse(const unsigned char *check, unsigned char check_len, 
								unsigned char expected_response_len,
								unsigned short timeout) {

	//Keep writing any received values to receive buffer until timeout.
	unsigned char i = 0;
	unsigned char done_type = ISNT_DONE;
	interval li = GetInterval(timeout); //listen interval
	while (done_type==ISNT_DONE) {
		
		clear_wdt();
	
		//If there are any over-run errors, clear them
		if (rcsta & 0x02) { //OERR (Bit 1)
			rcsta &= 0xEF ; //Clear CREN to 0 (Bit 4)
			rcsta |= 0x10 ; //Set CREN to 1 (Bit 4)
		}

		// Wait to receive a character
		while(!(pir1 & 0x20) && done_type==ISNT_DONE) { //RXIF
			clear_wdt();
			if (li.will_wrap) {
				if (timer0_isr_count<li.end_tick) { //wrap has happened
					li.will_wrap = 0; //no longer anticipating wrap
				}
			}	
			else if (timer0_isr_count>=li.end_tick) {
				done_type = DONE_TIMED_OUT;
			}
		}

		//Receive character. If there's space left in the software buffer, place it there;
		//if not (or we don't want to actually save the received character) throw it away.
		if (done_type==ISNT_DONE) {
			if (i<RX_BUFF_LENGTH) {
				rx_buff[i] = rcreg;
				i++;
			} else {
				unsigned char temp = rcreg;
			}
			//Reset the intercharacter delay
			li = GetInterval(timeout); 

			//If we are comparing against an expected finished response...
			if (expected_response_len!=0) {

				//If the response is the expected length...
				if (i==expected_response_len) {
					
					//If we're NOT comparing content of response...
					if (check==NULL) {
						done_type = DONE_SUCCESS;
					} 
					//If we ARE checking the contents of a finished response...
					else if (buff_equal(rx_buff, check, check_len)) {
						done_type = DONE_SUCCESS;
					}
					else {
						done_type = DONE_FAILURE;
					}
				}
			}
		}
	}

	//If it has timed out...
	if (done_type==DONE_TIMED_OUT) {

		//If we didn't know how long the message would be...
		if (expected_response_len==0) {

 			//If we knew how the message should begin, and it began properly, pronounce it a success.
			if ( check!=NULL && check_len!=0 && buff_equal(rx_buff, check, check_len) ) {
				done_type = DONE_SUCCESS;
			}

			//If we didn't know how the message should begin and no characters were received, pronounce
			//it a failure.
			else if (i==0) {
				done_type = DONE_FAILURE;
			}
		}
	}

	return done_type;
}


//Send()
//
//When check!=NULL, check_len>0 and expected_response_len!=0, send_len bytes from the send array are sent, and
//the function waits for a reply. When the reply length equals expected_response_len, check_len bytes of the reply 
//in rx_buff are compared against check_len bytes of check. If they are equal, the function returns 1; if not (or if the reply length never
//got to the size of expected_response_len) the function retries. If after all tries the function has not returned 1, 
//it returns 0.

//When check==NULL, check_len==0 and expected_response_len!=0, send_len bytes from the send array are sent, and
//the function waits for a reply. Once the reply length equals expected_response_len, the function 
//returns 1; if the retry_timeout is reached first, the function retries. If, after all tries the function has not 
//returned 1, it returns 0.

//When check!=NULL, check_len>0 and expected_response_len==0, send_len bytes from the send array are sent, and
//the function waits for a reply. When the retry timeout times out, check_len bytes of any reply in rx_buff are 
//compared against check_len bytes of check. If they are equal, the function returns 1; if not, it retries. 
//If, after all retries, the function has not returned 1, it returns 0.

//When check==NULL, check_len==0 and expected_response_len==0, send_len bytes from the send array are sent,
//and the function does not wait for a reply. It ignores the num_tries and retry_timeout arguments and returns 1.
//
unsigned char Send(	const unsigned char * send, const unsigned char send_len,
 				   	const unsigned char * check, const unsigned char check_len, const unsigned char expected_response_len,
 					const unsigned char num_tries, const unsigned short retry_timeout ) {

	unsigned char i, res;
	i = 0;
	res = ISNT_DONE;

	for (i=0; i<num_tries; i++) {
		clear_wdt();

		EraseBuffer(rx_buff, RX_BUFF_LENGTH);
		FlushRxHwBuffer();

		WriteBuff(send, send_len);
		if (expected_response_len==0 && check==NULL) {
			return 1;
		}
		res = ListenForResponse(check, check_len, 
								expected_response_len,
								retry_timeout);
		if (res==DONE_SUCCESS) {
			return 1;
		}
	}

	return 0;
}


unsigned char ValidBarCodeJustReceived(void) {

	//Check length...
	unsigned char bc_length = rx_buff[FIRST_BARCODE_STRLEN_I]-5;
	if (bc_length==251) { //0-5 is 251
		return 	BC_NOT_VALID_LENGTH_ZERO;	
	}
	else if ((FIRST_BARCODE_START_I+bc_length)>=RX_BUFF_LENGTH) {
		return BC_NOT_VALID_LENGTH_TOO_LONG;
	}	

	//Check type...
	unsigned char type = rx_buff[FIRST_BARCODE_TYPE_I];
	if (type==0 || type>14) {
		return BC_NOT_VALID_TYPE;
	}

	//Check content...
	unsigned char i;	
	for (i=FIRST_BARCODE_START_I; (i<(bc_length+FIRST_BARCODE_START_I) && i<RX_BUFF_LENGTH); i++) {
		if ( 	(	( 
					   	(rx_buff[i]==45) || //'-'
					   	(rx_buff[i]==46) || //'.'
						((rx_buff[i]>=65) && (rx_buff[i]<=90))	|| //A-Z
						((rx_buff[i]>=97) && (rx_buff[i]<=122)) || //a-z
						((rx_buff[i]>=48) && (rx_buff[i]<=57))  //1-9
					) 
 				) == 0	) { 
			return BC_NOT_VALID_CHARACTERS;
		}
	}	
	return BC_VALID;
}


unsigned char BT_AddressIsValid(const unsigned char * buff) {

	unsigned char i; 
	for (i=0; i<BT_ADDRESS_LENGTH; i++) {
		//Testing for colons...
		if (i==2 || i==5 || i==8 || i==11 || i==14) {
			if (buff[i]!=58) { //58 is ':'
				return 0;
			}
		}
		//Testing for 0-9/A-F/a-f
		else {
			if ( (buff[i]<48) || ((buff[i]>57)&&(buff[i]<65)) || ((buff[i]>70)&&(buff[i]<97)) || (buff[i]>102) ) { 
				return 0;
			}
		}
	}

	return 1;
}


void PrintBarCodeInRx(void) {

	SerialSelectWired();
	
	WriteStr(dashes_s);
	WriteStr("BC:\r");

	unsigned char v = ValidBarCodeJustReceived();

	switch (v) {	
		case BC_NOT_VALID_LENGTH_ZERO: {
			WriteStr("None in mem.\r"); 
		}
		case BC_NOT_VALID_LENGTH_TOO_LONG: {
			WriteStr("Too long.\r");
		}		
		case BC_NOT_VALID_TYPE: {
			WriteStr("Type unknown.\r");
		}		
		case BC_NOT_VALID_CHARACTERS: {
			WriteStr("Non # chars\r");
		}
		case BC_VALID: {

			unsigned char bc_length = rx_buff[FIRST_BARCODE_STRLEN_I]-5;

			//Print Bar Code's Digits
			unsigned char i;
			for (i=FIRST_BARCODE_START_I; 
					(i<(FIRST_BARCODE_START_I+bc_length) && i<BARCODE_BUFF_LENGTH); i++) {
				WriteChar(' ');
				WriteChar(barcode_buff[i]);
			}
		}
		default: {
			WriteStr("BC valid-check ret unkown val\r");
		}
	}
	WriteStr(dashes_s);	

	return;
}


void InitializeEverything(void) {
	InitLED();
	InitTimer0(); //Do first; all delays depend on timer0

	query_bcr_f = 0;
	InitBCRButton();
	EnableBcrButtonInterrupt();

	InitSecondaryPower();

	Init_HI_DR();
	//NOTE: Setting HI to 0 is best here for barcode reader coms.
	//It impacts serial coms with the PC however. When the switches are thrown
	//for communication with a PC, leave the HI switch disconnected from the PC serial line.
	//(the problem may just be too much capacitance for the driver to handle, or something
	//about the way the PC interperets the line level
	SetHIto(0); 

	InitSerial();
 	InitSerialSelect(); //Sets up Pin2 for button (rather than blue tooth control)

 	InitWDT();
}


void BT_ConsoleLoop(void) {

	unsigned char i,j;

	//Ensure that bluetooth module is in command mode
	SerialSelectBlueTooth();
	EnterBTCommandMode();
	
	while(1) {	

		clear_wdt();

		SerialSelectWired();

		//Terminal->MicroController
		i=0;
		WriteStr("\n\r\n\r");
		//WriteStr("\rPC->MC:");
		WriteStr("PC:");
		while (1) {
			clear_wdt();
			rx_buff[i] = ReadChar();
			//Echo
			//++++
			if (rx_buff[i]==13) { //13='\r'
				WriteChar('\n');
			}
			WriteChar(rx_buff[i]);
			//----
			if (rx_buff[i]=='\r') {
				WriteStr("\n\r");
				break;
			}
			i++;
			if (i>=RX_BUFF_LENGTH) {
				i=RX_BUFF_LENGTH-1;
				break;
			}
		}
		if (str_equal(rx_buff, "out\r", 4)) {
			return;
		}

		SerialSelectBlueTooth();

		//Microcontroller->Bluetooth
		//WriteStr("\rMC->BT:");
		if ( str_equal(rx_buff, "+++\r", 4) ) {
			EraseBuffer(rx_buff, RX_BUFF_LENGTH);
			FlushRxHwBuffer();
			EnterBTCommandMode();
		}
		else if ( str_equal(rx_buff, "ret\r", 4) ) {
			EraseBuffer(rx_buff, RX_BUFF_LENGTH);
			FlushRxHwBuffer();
			ExitBTCommandMode();
		}
		else {
			for(j=0;j<i;j++){
				WriteChar(rx_buff[j]);
			}
			EraseBuffer(rx_buff, RX_BUFF_LENGTH);
			FlushRxHwBuffer();
			WriteChar('\r'); //Sends the command to bluetooth module
		}

		//Bluetooth->MicroController
		i=ListenForResponse(NULL, 0, 0, 2000);

		SerialSelectWired();

		//Microcontroller->Terminal
		if (i!=DONE_FAILURE) {
			//WriteStr("MC->PC:");
			j=0;
			while(j<RX_BUFF_LENGTH && rx_buff[j]!=NULL){
				if (rx_buff[j]==13) { //13='\r'
					WriteChar('\n');
				}
				WriteChar(rx_buff[j]);
				j++;
			}	
		}
		else {
			WriteStr("<No response>");
		}
	}
}


unsigned char ProgramDefaults() {

	unsigned char res, x;
 	res = x = 0;

	//Set Bluetooth Module Defaults
	//+++++++
	SerialSelectBlueTooth();

	//Verify we are in command mode 
	//for the bluetooth module...
	EnterBTCommandMode();
 	x = Send("\r", 1, ">", 1, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	//Expected length is 0 because we could receive a >NACK as well as a >
 	res	+= x;

	Send("rst factory\r", 12, NULL, 0, 0, 1, 2000);

	//Verify we are once again in command mode after the reset 
	ExitBTCommandMode();
	EnterBTCommandMode();
	x = Send("\r", 1, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	if (x==0) {
 		x = Send("\r", 1, ">", 1, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	}
	//Expected length is 0 because we could receive a >NACK as well as a >
 	res	+= x;

 	x = Send("set name BarCodeKey\r", 20, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;

 	x = Send("set encrypt off\r", 16, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;

 	x = Send("set txpower 10\r", 15, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;

 	x = Send("ret\r", 4, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	//Only compare against the first three characters of the ack response, because it could be we're not returning
	//to an existing connection. We're probably not!
 	res	+= x;
	ExitBTCommandMode();

/*
if (x==1) {
TurnLEDon();
}
SerialSelectWired();
PrintBufferBytes(rx_buff, 30);
while(1){clear_wdt();};
*/
	//------


//res=0;

	SerialSelectWired();


	//Set Bar Code Reader Defaults
	//+++++++
	ms_delay(BCR_BUTTON_RELEASE_TO_BCR_WAKEUP_DELAY);
	
	SetHIto(1); //Wakes barcode reader

	ms_delay(BCR_WAKEUP_TO_INTERROGATE_DELAY);


	//Establish connection by sending an "interrogate" command
 	x = Send(bcr_interrogate_cmd, BCR_INTERROGATE_CMD_LENGTH, 
			 bcr_interrogate_response_start, BCR_INTERROGATE_RESPONSE_START_LENGTH, BCR_INTERROGATE_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;


	ms_delay(INTERROGATE_TO_DR_READY_DELAY);


	//Restore defaults
 	x = Send(bcr_restore_defaults_cmd, BCR_RESTORE_DEFAULTS_CMD_LENGTH, 
			 bcr_restore_defaults_response, BCR_RESTORE_DEFAULTS_RESPONSE_LENGTH, BCR_RESTORE_DEFAULTS_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;


	//Customize defaults
	//************
 	x = Send(bcr_customize_defaults_cmd, BCR_CUSTOMIZE_DEFAULTS_CMD_LENGTH, 
			 bcr_customize_defaults_response, BCR_CUSTOMIZE_DEFAULTS_RESPONSE_LENGTH, BCR_CUSTOMIZE_DEFAULTS_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
 	res	+= x;


	//Power Down
 	x = Send(bcr_power_down_cmd, BCR_POWER_DOWN_CMD_LENGTH, 
			 bcr_power_down_response, BCR_POWER_DOWN_RESPONSE_LENGTH, BCR_POWER_DOWN_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
	//res += x; Doesn't equal 1; not sure why. Seems to power down fine.

	SetHIto(0); //Prepare for next barcode reader wake-up
	//------

//BlinkLED(res, 500, 500);
//ms_delay(2000);

	//Results!
	if (res==9) { //All commands properly received
		BlinkLED(3, 1000, 1000);		
		return 1;
	} 
	else { //One or more commands not properly received
		BlinkLED(6, 100, 100);
		return 0;
	}
}



unsigned char GetAnyBarCodes(void){

	//Assumes secondary power supply is on
	SerialSelectWired();

	unsigned char result=0; 

	ms_delay(BCR_BUTTON_RELEASE_TO_BCR_WAKEUP_DELAY);
	
	SetHIto(1); //Wakes barcode reader

	ms_delay(BCR_WAKEUP_TO_INTERROGATE_DELAY);

	//Establish connection by sending an "interrogate" command
 	Send(bcr_interrogate_cmd, BCR_INTERROGATE_CMD_LENGTH, 
			 bcr_interrogate_response_start, BCR_INTERROGATE_RESPONSE_START_LENGTH, BCR_INTERROGATE_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);

	ms_delay(INTERROGATE_TO_DR_READY_DELAY);

	//if there is data ready...
	if (GetDR()) {

		//Send a signal
		BlinkLED(2,100,100);

		//Upload barcode(s)
 		result = Send(bcr_upload_cmd, BCR_UPLOAD_CMD_LENGTH, 
			bcr_upload_response_start, BCR_UPLOAD_RESPONSE_START_LENGTH, 0, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);

		//If we've received a valid barcode...
		if ( result && (ValidBarCodeJustReceived()==BC_VALID) ) {
			unsigned char bc_length = rx_buff[FIRST_BARCODE_STRLEN_I]-5;
			//Clip in case of bad value. Leave room for eol char
			if ( bc_length>((RX_BUFF_LENGTH-1)-FIRST_BARCODE_START_I) ) {
				bc_length=(RX_BUFF_LENGTH-1)-FIRST_BARCODE_START_I;
			}
			//Copy...
			copy_buffer_from_to( (rx_buff+FIRST_BARCODE_START_I), barcode_buff, bc_length);
			//Add an eol char.
			barcode_buff[bc_length]='\0';
		}

		//Clear barcode(s) in barcode reader
 		Send(bcr_clear_barcodes_cmd, BCR_CLEAR_BARCODES_CMD_LENGTH, 
			 bcr_clear_barcodes_response, BCR_CLEAR_BARCODES_RESPONSE_LENGTH, BCR_CLEAR_BARCODES_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
	}

	//Power Down
 	Send(bcr_power_down_cmd, BCR_POWER_DOWN_CMD_LENGTH, 
			 bcr_power_down_response, BCR_POWER_DOWN_RESPONSE_LENGTH, BCR_POWER_DOWN_RESPONSE_LENGTH, 
			 NUM_BCR_CMD_TRIES, MAX_BCR_INTER_CHAR_RESPONSE_DELAY);
	
	SetHIto(0); //Prepare for next barcode reader wake-up
	
	return result;
}


unsigned char ConnectToRemoteBT() {

	//Assumes we're on the Bluetooth channel and keeps us theres
	BT_Address2Buff(str_buff+4);
	if (!BT_AddressIsValid(str_buff+4)) {
		return 0;
	}

	unsigned char i,res;

	//Enter BT Command Mode; Verify we're in it
	EnterBTCommandMode();
 	res=Send("\r", 1, ">", 1, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	//expected response length isn't given because we could receive ">" or ">NACK"
	//and either can mean we are in communication

	if (res) {
		//Construct the connect command...
		str_buff[0]='c';
		str_buff[1]='o';
		str_buff[2]='n';
		str_buff[3]=' ';
		//4-20 Are the address
		str_buff[21]='\r';
		str_buff[22]='\0';

		res=0;
		for (i=0;i<4;i++) {
			Send(str_buff, 22, NULL, 0, 10, 1, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
			if (buff_equal(rx_buff, ack_s, 3)) {
				//If there was no connection error, or the connection error was due to an existing connection
				//i.e.. "ACK\r>" or "ACK\r>Err 3"
				if ( (rx_buff[9]==0x00) || (rx_buff[9]==0x33) ) { //0x33='3'
					res=1;
					break;
				}
			}
		}
	}

	//Returning
	//The expected length is unknown; either we expect nothing
	//or an error because there wasn't a prior connection.
	//NOTE: WriteStr doesn't clear RxBuff
	WriteStr("ret\r");
	ListenForResponse(NULL, 0, 0, MAX_BT_INTER_CHAR_RESPONSE_DELAY); 
	//***RET MAY BE UN-NECESSARY...

	ExitBTCommandMode();

	if (res) {
		BlinkLED(1,50,50);
		return 1;
	}
	return 0;
}


unsigned char DisconnectFromRemoteBT() {

	//Assumes we're on the Bluetooth channel and keeps us there

	unsigned char res;

	//Enter BT Command Mode; Verify we're in it
	EnterBTCommandMode();
 	res=Send("\r", 1, ">", 1, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
	//expected response length isn't given because we could receive ">" or ">NACK"

	if (res) {
		//Disconnect command
		res=Send("dis\r", 4, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY); 
		if (res) {
		}
	}

	//Returning
	//The expected length is unknown; either we expect nothing
	//or an error because there wasn't a prior connection.
	//NOTE: WriteStr doesn't clear RxBuff
	WriteStr("ret\r");
	ListenForResponse(NULL, 0, 0, MAX_BT_INTER_CHAR_RESPONSE_DELAY); 
	//***RET MAY BE UN-NECESSARY..

	ExitBTCommandMode();

	if (res) {
		return 1;
	}
	return 0;
}



// main function
void main(void) {

	//NOTE: It is each state's responsibility to turn secondary power off/on for its own use.
	//If secondary power needs to be off AND on WITHIN a given state, the functions called within
	//the state should be responsible for returning power to its original value...
		
	unsigned char prev_state, current_state;
	current_state = STATE_INITIAL;

	while (1) {

		if (current_state==STATE_INITIAL) {
			clear_wdt();

			InitializeEverything();
			TurnOnWDT();

			//Default next state is sleep state
			prev_state = STATE_INITIAL;
			current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;

			//If there is button input, however, we may be entering a
			//special mode...
			TurnSecondaryPowerOn(); //Needed for button to be readable
			if (ButtonHeldDown(4000)) {
 				BlinkLED(1, 1000, 1000);
				current_state = STATE_GET_BLUETOOTH_TO_ADDRESS;
				if (ButtonIsDown()) {
					if (ButtonHeldDown(4000)) {
	 					BlinkLED(2, 500, 500);
						current_state = STATE_PROGRAMMING_DEFAULTS;
						if (ButtonIsDown()) {
							if (ButtonHeldDown(4000)) {
	 							BlinkLED(3, 333, 333);
								current_state = STATE_BT_CONSOLE;
							}
						}
					}
				} 
			}
			TurnSecondaryPowerOff(); //Was needed for buttons
		}


		else if (current_state==STATE_PROGRAMMING_DEFAULTS) { 
			clear_wdt();

			TurnSecondaryPowerOn();

			ProgramDefaults();

			prev_state = current_state;
			current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
		}


		else if (current_state==STATE_BT_CONSOLE) {
			clear_wdt();

			TurnSecondaryPowerOn();
 
			BT_ConsoleLoop();

			prev_state = current_state;
			current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
		}


		else if (current_state==STATE_GET_BLUETOOTH_TO_ADDRESS) {
			clear_wdt();
			unsigned char i;
			i = 0;

			TurnSecondaryPowerOn();
	
			SerialSelectBlueTooth(); 

			//Enter BT Command Mode; Verify we're in it
			EnterBTCommandMode();
 			i=Send("\r", 1, ">", 1, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);
			//expected response length isn't given because we could receive ">" or ">NACK"

			i=Send("del trusted all\r", 16, ack_s, 3, 0, NUM_BT_CMD_TRIES, MAX_BT_INTER_CHAR_RESPONSE_DELAY);

			if (i) {
				//Wait for trusted device
				TurnLEDon();
				interval pi = GetInterval(40000); //programming interval


				while (1) {
					
					clear_wdt(); 
	
					//If timer needs to wrap...
					if (pi.will_wrap) {
						if (timer0_isr_count<pi.end_tick) { //wrap has happened
							pi.will_wrap = 0; //no longer anticipating wrap
						}
					}
					//If timer has timed out...
					else if (timer0_isr_count>=pi.end_tick) {
						BlinkLED(7, 500, 500);
						break;
					}

					i=Send("lst trusted\r", 12, ack_s, 3, 0, 1, MAX_BT_INTER_CHAR_RESPONSE_DELAY);

					if ((i==1) && (rx_buff[4]!=62)) { //62='>'
						if (BT_AddressIsValid(rx_buff+4)) {
							enable_EEPROM_writes();
							for (i=0; i<BT_ADDRESS_LENGTH; i++) {
								write_EEPROM_byte(rx_buff[i+4], i);
							}
							write_EEPROM_byte('\0', i);
							disable_EEPROM_writes();
							BlinkLED(4, 1000, 1000);
							break;
						}
					}	
				}
			}

			SerialSelectWired();

			prev_state = current_state;
			current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
		}


		else if (current_state==STATE_ASLEEP_SECONDARY_POWER_OFF) {
			clear_wdt();

			TurnSecondaryPowerOff();

			//Configure  Pins for Low Power
			//++++++++++++++++
			//Pin 1 - Vdd. Do nothing
				
			//Pin 2 - Btn/BtBreak. Both connect to weak pullups, so we
			//set this to a high output
			porta |= 0x20; //Set Pin 2 to 1
			trisa &= 0xDF; //Configure Pin 2 as output; (TRISA5 to 0)

			//Pin 3 - "HostInput" (an output from uP). Make sure it is low
			SetHIto(1); //Setting to 1 puts output low

			//Pin 4 - _MCLR. A necessary input
	
			//Pin 5 - Rx 
			//Do nothing for now

			//Pin 6 - Tx. (C4). Configure as a low digital output
			portc &= 0xEF; //Set Pin 6 (C4) low 
			trisc &= 0xEF; //Set Pin 6 (C4) to output; (TRISC4 = 0)

			//Disable UART
			txsta &= 0xDF; //Clear TXEN (bit 5) 
			rcsta &= 0x6F; //Clear CREN (bit4) and SPEN (7)

			//Pin 7 - Select. Already a digital output. Set to 0.
			portc &= 0xF7;  //Set C3 (Pin 7) to 0

			//Pin 8 - Already an output (low). Its turning secondary power off

			//Pin 9 - DR.
			// Do nothing for now

			//Pin 10 - LED output. Set low.
			TurnLEDoff(); 

			//Pin 11 (RA2) - READ_BEGUN. A necessary input

			//Pin 12 - (RA1) ICSP Clk. Set as low digital output
			porta &= 0xFD; //Set A1 to 0
			trisa &= 0xFD; //Configure A1 as output; (TRISA1 to 0)

			//Pin 13 - ICSP Data (RA0). Set as low digital output
			porta &= 0xFE; //Set A0 to 0
			trisa &= 0xFE; //Configure A0 as output; (TRISA0 to 0)

			//Pin 14 - Vss. Do nothing
			//----------------


			//Sleep until BCR button input. Decide what to do. 
			while(1) {
		
				EnableBcrButtonInterrupt();
				TurnOffWDT();

				sleep(); //Sourceboost's PIC sleep function

				DisableBcrButtonInterrupt();
				clear_wdt();
				TurnOnWDT();
		
				unsigned short down_tick, up_tick, down_time;
				ms_delay(10); //debouncing //was 25
				down_tick = timer0_isr_count;
				while(BCRButtonIsDown()){ //Wait until unpressed
					clear_wdt();
				}; 
				up_tick = timer0_isr_count;

				//Calculate down time, taking into account timer wrap
				if (down_tick>up_tick) { //timer wrapped
					down_time = up_tick + (MAX_U16-down_tick);
				} 
				else {
					down_time = up_tick - down_tick;
				}
	
				//Process what the button press means.
				//If time to short...
				if (down_time<20) { //Was 25
					//Don't do anything
				}
				//If its a quick press...
				else if (down_time<235) { //Was 250
					//Go to "RU Awake?" state
					prev_state = current_state;
					current_state = STATE_SENDING_RUAWAKE_OVER_BLUETOOTH;
					break;
				}
				//Its time to see if a barcode got scanned!
				else {
					prev_state = current_state;
					current_state =  STATE_GETTING_BARCODE_FROM_READER;
					break;
				}
			}

			//Reconfigure everything for normal use.
			InitializeEverything(); //Re-enables BCRButton interrupt
		}


		else if (current_state==STATE_GETTING_BARCODE_FROM_READER) {
			clear_wdt();

			query_bcr_f = 0; //Clear the flag set by the bcr button interrupt

			TurnSecondaryPowerOn();

			EraseBuffer(barcode_buff, BARCODE_BUFF_LENGTH);
			if (GetAnyBarCodes()) {
				prev_state = current_state;
				current_state = STATE_SENDING_BARCODE_OVER_BLUETOOTH;
			}
			else {
				prev_state = current_state;
				current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
			}
		}


		else if (current_state==STATE_SENDING_BARCODE_OVER_BLUETOOTH) {
			clear_wdt();

		 	//If we have a valid barcode	
			if (barcode_buff[0]!='\0') {
		
				TurnSecondaryPowerOn();

				SerialSelectBlueTooth();

				unsigned char res=0;

				//Calculate the barcode length, clipping if need be
				unsigned char barcode_len = strlen(barcode_buff);
				if ( barcode_len>(BARCODE_BUFF_LENGTH-2) ) {
					barcode_len = BARCODE_BUFF_LENGTH-2; 
				}
				barcode_buff[barcode_len]='\r';

				if (ConnectToRemoteBT()) {
					res=Send( barcode_buff, barcode_len+1, "$", 1, 0, NUM_BT_SEND_TRIES, MAX_BT_SEND_INTER_CHAR_RESPONSE_DELAY);
					DisconnectFromRemoteBT();
				}

				SerialSelectWired();
			}

			prev_state = current_state;

			//If a new barcode may have been captured while
			//we were sending the current one...
			if (query_bcr_f==1) {
				current_state = STATE_GETTING_BARCODE_FROM_READER;
			}
			//We're done; go to the resting state
			else {
				current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
			}
		} 


		else if (current_state==STATE_SENDING_RUAWAKE_OVER_BLUETOOTH) {
			clear_wdt();
			
			TurnSecondaryPowerOn();	
			SerialSelectBlueTooth();
			
			unsigned char res=0;

			if (ConnectToRemoteBT()) {				
				//Send("*", 1, NULL, 0,0, 1, 0); //A send with no reply expected
				res=Send("*", 1, "$", 1, 0,  NUM_BT_SEND_TRIES, MAX_BT_SEND_INTER_CHAR_RESPONSE_DELAY);
				DisconnectFromRemoteBT();
			}

			SerialSelectWired();

			prev_state = current_state;


			//If a new barcode may have been captured while
			//we were sending the r-u-awake signal...
			if (query_bcr_f==1) {
				current_state = STATE_GETTING_BARCODE_FROM_READER;
			}
			//We're done; go to the resting state
			else {
				current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
			}
		}
		

		else {
			//Should never get here...	
			BlinkLED(20,200,200);	
			prev_state = current_state;
			current_state = STATE_ASLEEP_SECONDARY_POWER_OFF;
		}

	} //state machine while
} //main




