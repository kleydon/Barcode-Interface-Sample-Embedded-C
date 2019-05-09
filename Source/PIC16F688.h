// ***************************************
// BoostC Header file for PIC16F688
// Author(s): David Hobday
//
// Copyright (C) 2003-2008 Pavel Baranov
// Copyright (C) 2003-2008 David Hobday
// All Rights Reserved
// ***************************************

#ifndef _PIC16F688_H_
#define _PIC16F688_H_

////////////////////////////////////////////////////////////////////////////
//
//       Register Definitions
//
////////////////////////////////////////////////////////////////////////////

#define W                     0x0000 
#define F                     0x0001 

/////// Register Files//////////////////////////////////////////////////////
#define INDF                  0x0000 
#define TMR0                  0x0001 
#define PCL                   0x0002 
#define STATUS                0x0003 
#define FSR                   0x0004 
#define PORTA                 0x0005 
#define PORTC                 0x0007 
#define PCLATH                0x000A 
#define INTCON                0x000B 
#define PIR1                  0x000C 
#define TMR1L                 0x000E 
#define TMR1H                 0x000F 
#define T1CON                 0x0010 
#define BAUDCTL               0x0011 
#define SPBRGH                0x0012 
#define SPBRG                 0x0013 
#define RCREG                 0x0014 
#define TXREG                 0x0015 
#define TXSTA                 0x0016 
#define RCSTA                 0x0017 
#define WDTCON                0x0018 
#define CMCON0                0x0019 
#define CMCON1                0x001A 
#define ADRESH                0x001E 
#define ADCON0                0x001F 
#define OPTION_REG            0x0081 
#define TRISA                 0x0085 
#define TRISC                 0x0087 
#define PIE1                  0x008C 
#define PCON                  0x008E 
#define OSCCON                0x008F 
#define OSCTUNE               0x0090 
#define ANSEL                 0x0091 
#define WPU                   0x0095 
#define WPUA                  0x0095 
#define IOC                   0x0096 
#define IOCA                  0x0096 
#define EEDATH                0x0097 
#define EEADRH                0x0098 
#define VRCON                 0x0099 
#define EEDAT                 0x009A 
#define EEDATA                0x009A 
#define EEADR                 0x009B 
#define EECON1                0x009C 
#define EECON2                0x009D 
#define ADRESL                0x009E 
#define ADCON1                0x009F 

/////// STATUS Bits ////////////////////////////////////////////////////////
#define IRP                   0x0007 
#define RP1                   0x0006 
#define RP0                   0x0005 
#define NOT_TO                0x0004 
#define NOT_PD                0x0003 
#define Z                     0x0002 
#define DC                    0x0001 
#define C                     0x0000 

/////// INTCON Bits ////////////////////////////////////////////////////////
#define GIE                   0x0007 
#define PEIE                  0x0006 
#define T0IE                  0x0005 
#define INTE                  0x0004 
#define RAIE                  0x0003 
#define T0IF                  0x0002 
#define INTF                  0x0001 
#define RAIF                  0x0000 

/////// PIR1 Bits //////////////////////////////////////////////////////////
#define EEIF                  0x0007 
#define ADIF                  0x0006 
#define RCIF                  0x0005 
#define C2IF                  0x0004 
#define C1IF                  0x0003 
#define OSFIF                 0x0002 
#define TXIF                  0x0001 
#define T1IF                  0x0000 
#define TMR1IF                0x0000 

/////// T1CON Bits /////////////////////////////////////////////////////////
#define T1GINV                0x0007 
#define TMR1GE                0x0006 
#define T1CKPS1               0x0005 
#define T1CKPS0               0x0004 
#define T1OSCEN               0x0003 
#define NOT_T1SYNC            0x0002 
#define TMR1CS                0x0001 
#define TMR1ON                0x0000 

/////// BAUDCTL Bits ////////////////////////////////////////////////////////
#define ABDOVF                0x0007 
#define RCIDL                 0x0006 
#define SCKP                  0x0004 
#define BRG16                 0x0003 
#define WUE                   0x0001 
#define ABDEN                 0x0000 

/////// TXSTA Bits ////////////////////////////////////////////////////////
#define CSRC                  0x0007 
#define TX9                   0x0006 
#define TXEN                  0x0005 
#define SYNC                  0x0004 
#define SENDB                 0x0003 
#define BRGH                  0x0002 
#define TRMT                  0x0001 
#define TX9D                  0x0000 

/////// RCSTA Bits ////////////////////////////////////////////////////////
#define SPEN                  0x0007 
#define RX9                   0x0006 
#define SREN                  0x0005 
#define CREN                  0x0004 
#define ADDEN                 0x0003 
#define FERR                  0x0002 
#define OERR                  0x0001 
#define RX9D                  0x0000 

/////// WDTCON Bits ////////////////////////////////////////////////////////
#define WDTPS3                0x0004 
#define WDTPS2                0x0003 
#define WDTPS1                0x0002 
#define WDTPS0                0x0001 
#define SWDTEN                0x0000 

/////// COMCON0 Bits ///////////////////////////////////////////////////////
#define C2OUT                 0x0007 
#define C1OUT                 0x0006 
#define C2INV                 0x0005 
#define C1INV                 0x0004 
#define CIS                   0x0003 
#define CM2                   0x0002 
#define CM1                   0x0001 
#define CM0                   0x0000 

/////// COMCON1 Bits ///////////////////////////////////////////////////////
#define T1GSS                 0x0001 
#define C2SYNC                0x0000 

/////// ADCON0 Bits ////////////////////////////////////////////////////////
#define ADFM                  0x0007 
#define VCFG                  0x0006 
#define CHS2                  0x0004 
#define CHS1                  0x0003 
#define CHS0                  0x0002 
#define GO                    0x0001 
#define NOT_DONE              0x0001 
#define GO_DONE               0x0001 
#define ADON                  0x0000 

/////// OPTION Bits ////////////////////////////////////////////////////////
#define NOT_RAPU              0x0007 
#define INTEDG                0x0006 
#define T0CS                  0x0005 
#define T0SE                  0x0004 
#define PSA                   0x0003 
#define PS2                   0x0002 
#define PS1                   0x0001 
#define PS0                   0x0000 

/////// PIE1 Bits //////////////////////////////////////////////////////////
#define EEIE                  0x0007 
#define ADIE                  0x0006 
#define RCIE                  0x0005 
#define C2IE                  0x0004 
#define C1IE                  0x0003 
#define OSFIE                 0x0002 
#define TXIE                  0x0001 
#define T1IE                  0x0000 
#define TMR1IE                0x0000 

/////// PCON Bits //////////////////////////////////////////////////////////
#define ULPWUE                0x0005 
#define SBODEN                0x0004 
#define NOT_POR               0x0001 
#define NOT_BOD               0x0000 

/////// OSCCON Bits ////////////////////////////////////////////////////////
#define IRCF2                 0x0006 
#define IRCF1                 0x0005 
#define IRCF0                 0x0004 
#define OSTS                  0x0003 
#define HTS                   0x0002 
#define LTS                   0x0001 
#define SCS                   0x0000 

/////// OSCTUNE Bits ///////////////////////////////////////////////////////
#define TUN4                  0x0004 
#define TUN3                  0x0003 
#define TUN2                  0x0002 
#define TUN1                  0x0001 
#define TUN0                  0x0000 

/////// ANSEL Bits /////////////////////////////////////////////////////////
#define ANS7                  0x0007 
#define ANS6                  0x0006 
#define ANS5                  0x0005 
#define ANS4                  0x0004 
#define ANS3                  0x0003 
#define ANS2                  0x0002 
#define ANS1                  0x0001 
#define ANS0                  0x0000 

/////// IOC Bits /////////////////////////////////////////////////////////
#define IOC5                  0x0005 
#define IOC4                  0x0004 
#define IOC3                  0x0003 
#define IOC2                  0x0002 
#define IOC1                  0x0001 
#define IOC0                  0x0000 

/////// IOCA Bits /////////////////////////////////////////////////////////
#define IOCA5                 0x0005 
#define IOCA4                 0x0004 
#define IOCA3                 0x0003 
#define IOCA2                 0x0002 
#define IOCA1                 0x0001 
#define IOCA0                 0x0000 

/////// VRCON Bits /////////////////////////////////////////////////////////
#define VREN                  0x0007 
#define VRR                   0x0005 
#define VR3                   0x0003 
#define VR2                   0x0002 
#define VR1                   0x0001 
#define VR0                   0x0000 

/////// EECON1 Bits ////////////////////////////////////////////////////////
#define EEPGD                 0x0007 
#define WRERR                 0x0003 
#define WREN                  0x0002 
#define WR                    0x0001 
#define RD                    0x0000 

/////// ADCON1 Bits ////////////////////////////////////////////////////////
#define ADCS2                 0x0006 
#define ADCS1                 0x0005 
#define ADCS0                 0x0004 

////////////////////////////////////////////////////////////////////////////
//
//       Configuration Bits
//
////////////////////////////////////////////////////////////////////////////

#define _FCMEN_ON             0x3FFF 
#define _FCMEN_OFF            0x37FF 
#define _IESO_ON              0x3FFF 
#define _IESO_OFF             0x3BFF 
#define _BOD_ON               0x3FFF 
#define _BOD_NSLEEP           0x3EFF 
#define _BOD_SBODEN           0x3DFF 
#define _BOD_OFF              0x3CFF 
#define _CPD_ON               0x3F7F 
#define _CPD_OFF              0x3FFF 
#define _CP_ON                0x3FBF 
#define _CP_OFF               0x3FFF 
#define _MCLRE_ON             0x3FFF 
#define _MCLRE_OFF            0x3FDF 
#define _PWRTE_OFF            0x3FFF 
#define _PWRTE_ON             0x3FEF 
#define _WDT_ON               0x3FFF 
#define _WDT_OFF              0x3FF7 
#define _LP_OSC               0x3FF8 
#define _XT_OSC               0x3FF9 
#define _HS_OSC               0x3FFA 
#define _EC_OSC               0x3FFB 
#define _INTRC_OSC_NOCLKOUT   0x3FFC 
#define _INTRC_OSC_CLKOUT     0x3FFD 
#define _EXTRC_OSC_NOCLKOUT   0x3FFE 
#define _EXTRC_OSC_CLKOUT     0x3FFF 
#define _INTOSCIO             0x3FFC 
#define _INTOSC               0x3FFD 
#define _EXTRCIO              0x3FFE 
#define _EXTRC                0x3FFF 

/////////////////////////////////////////////////
// Config Register
/////////////////////////////////////////////////
#define _CONFIG               0x2007

/////////////////////////////////////////////////
// EEPROM Base Address when programing
/////////////////////////////////////////////////
// To initialise EEPROM when a device is programmed
// use #pragma DATA _EEPROM, 12, 34, 56 
#define _EEPROM               0x2100

volatile char indf                   @INDF;
volatile char tmr0                   @TMR0;
volatile char pcl                    @PCL;
volatile char status                 @STATUS;
volatile char fsr                    @FSR;
volatile char porta                  @PORTA;
volatile char portc                  @PORTC;
volatile char pclath                 @PCLATH;
volatile char intcon                 @INTCON;
volatile char pir1                   @PIR1;
volatile char tmr1l                  @TMR1L;
volatile char tmr1h                  @TMR1H;
volatile char t1con                  @T1CON;
volatile char baudctl                @BAUDCTL;
volatile char spbrgh                 @SPBRGH;
volatile char spbrg                  @SPBRG;
volatile char rcreg                  @RCREG;
volatile char txreg                  @TXREG;
volatile char txsta                  @TXSTA;
volatile char rcsta                  @RCSTA;
volatile char wdtcon                 @WDTCON;
volatile char cmcon0                 @CMCON0;
volatile char cmcon1                 @CMCON1;
volatile char adresh                 @ADRESH;
volatile char adcon0                 @ADCON0;
volatile char option_reg             @OPTION_REG;
volatile char trisa                  @TRISA;
volatile char trisc                  @TRISC;
volatile char pie1                   @PIE1;
volatile char pcon                   @PCON;
volatile char osccon                 @OSCCON;
volatile char osctune                @OSCTUNE;
volatile char ansel                  @ANSEL;
volatile char wpu                    @WPU;
volatile char wpua                   @WPUA;
volatile char ioc                    @IOC;
volatile char ioca                   @IOCA;
volatile char eedath                 @EEDATH;
volatile char eeadrh                 @EEADRH;
volatile char vrcon                  @VRCON;
volatile char eedata                 @EEDATA;
volatile char eeadr                  @EEADR;
volatile char eecon1                 @EECON1;
volatile char eecon2                 @EECON2;
volatile char adresl                 @ADRESL;
volatile char adcon1                 @ADCON1;

#endif // _PIC16F688_H_
