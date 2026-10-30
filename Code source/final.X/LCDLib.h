#include	<string.h>
#include	<delays.h>
#include	"LCDdefs.h"

#define ROM const far rom char *

#define		LCD										// to remove conflict with Hyperterminal routines

#define		E		PORTDbits.RD6
#define		RW		PORTDbits.RD5
#define		RS		PORTDbits.RD4

void InitLCD(void);
void SendCmd(char);
void SendChar(char);
void SendNibble(char);
void Wait4LCD(void);
void DispRamStr(char, char *);
void DispRomStr(char, ROM);
void DispVarStr(char *, char, unsigned char);
void DispBlanks(char, unsigned char);
void Bin2Asc(unsigned char, char *);
void CompDig(unsigned char *, unsigned char, char *);
void Bin2AscE(unsigned int, char *);
void CompDigE(unsigned int *, unsigned int, char *);
void Asc2Bin(char *, unsigned int *);
void Asc2BinE(char *, unsigned int *);
void PBCD2Asc(unsigned char, unsigned char *);

void InitLCD(void)									// initialize LCD in 4-bit mode, <RD3:RD0> = data
{
	char LCDstr[] = {0x33, 0x32, 0x28, 0x0C, 0x01, 0x06, '\0'};
	unsigned char i = 0;


        TRISD &=0b10001111;
	ANSELD=0x00;

	while (LCDstr[i] != '\0')
		SendCmd(LCDstr[i++]);
}

void SendCmd(char cmd)								// send a command to the LCD
{
	RS = 0; RW = 0; E = 0;         					// set LCD in command mode
    SendNibble(cmd >> 4);   						// send upper nibble of ASCII data
    SendNibble(cmd);      							// send lower nibble of ASCII data
	Wait4LCD();
}

void SendChar(char ch)								// send a character to the LCD
{
	RS = 1; RW = 0; E = 0;         					// set LCD in data mode
    SendNibble(ch >> 4);     						// send upper nibble of ASCII data
    SendNibble(ch); 	     						// send lower nibble of ASCII data
	Wait4LCD();
}

void SendNibble(char ch)
{
    TRISD &= 0xF0;									// RC3..RC0 are output pins (LCD bus)
    PORTD = (PORTD & 0xF0) | (ch & 0x0F); 			// send data to LCD W/O modifying upper nibble of PORTD
    E = 1;               							// toggle E: +ve edge latches control state (RS/R_W)
    Delay10TCYx(2);									// 20 cycles delay
    E = 0;		 									// -ve edge latches data/command
}

void Wait4LCD(void)									// Wait for the LCD busy flag to clear.
{
	unsigned char LoNibble, HiNibble, status;

	TRISD |= 0x0F;									// switch data port to input
	RW = 1; RS = 0; E = 0;   	      				// read busy flag
   	do {
		E = 1;										// provide logic high on E
	  	Nop();										// 1 cycle delay
		HiNibble = PORTD << 4;						// read the high nibble
		E = 0;										// provide falling edge
		E = 1;										// provide logic high on E
	  	Nop();										// 1 cycle delay
		LoNibble = (PORTD & 0x0F);					// read the low nibble
		E = 0;										// provide falling edge
		status = HiNibble | LoNibble;				// combine to form busy status and DD address
    } while (status & 0x80);						// test busy flag
}

// display null-terminated string (in RAM) at StartPos
void DispRamStr(char StartPos, char *Str)
{
	unsigned char i = 0;

	SendCmd(StartPos);
	while (*Str != '\0')
		SendChar(*Str++);
}

// display null-terminated string (in ROM) at StartPos
void DispRomStr(char StartPos, ROM string)
{
	SendCmd(StartPos);
	while (*string != '\0')
		SendChar(*string++);
}

// Display ASCII variable of specified length at StartPos.
void DispVarStr(char *Str, char StartPos, unsigned char NumOfChars)
{
	unsigned char i;

	SendCmd(StartPos);
	for (i = 0; i < NumOfChars; i++)
		SendChar(*Str++);
}

// Display a number of blank characters at StartPos.
void DispBlanks(char StartPos, unsigned char NumOfChars)
{
	unsigned char i;

	SendCmd(StartPos);
	for (i = 0; i < NumOfChars; i++)
		SendChar(' ');					// send a blank character
}

// Convert 8-bit N to ASCII characters and store in array a (starting with MSD)
void Bin2Asc(unsigned char N, char *a)
{
	CompDig(&N, 100, &a[0]);			// a[0] = 100s
	CompDig(&N, 10, &a[1]);				// a[1] = 10s
	a[2] = N + '0';						// a[2] = units
}

// Used by Bin2Asc
void CompDig(unsigned char *N, unsigned char subtractor, char *digit)
{
	*digit = '0';						// base value of ASCII characters
	while (*N >= subtractor)
	{
		*N = *N - subtractor;
		*digit = *digit + 1;
	}
}

// Convert 16-bit N to ASCII characters and store in array a (starting with MSD)
void Bin2AscE(unsigned int N, char *a)
{
	CompDigE(&N, 10000, &a[0]);			// a[0] =  10 Ks
	CompDigE(&N, 1000, &a[1]);			// a[1] =  1000s
	CompDigE(&N, 100, &a[2]);			// a[2] =  100s
	CompDigE(&N, 10, &a[3]);			// a[3] =  10s
	a[4] = N + '0';						// a[4] =  units
}

// Used by Bin2AscE
void CompDigE(unsigned int *N, unsigned int subtractor, char *digit)
{
	*digit = '0';						// base value of ASCII characters
	while (*N >= subtractor)
	{
		*N = *N - subtractor;
		*digit = *digit + 1;
	}
}

// Convert 3-digit ASCII array to binary
void Asc2Bin(char *a, unsigned int *reg)  					// Convert the 3-byte ASCII arrary a to binary.
{
	*reg = (int) (a[0] & 0x0F)*100 + (a[1] & 0x0F)*10 + (a[2] & 0x0F);
}

// Convert 4-digit ASCII array to binary
void Asc2BinE(char *a, unsigned int *reg)  					// Convert the 4-byte ASCII arrary a to binary.
{
	*reg = (int) (a[0] & 0x0F)*1000 + (a[1] & 0x0F)*100 + (a[2] & 0x0F)*10 + (a[3] & 0x0F);
}

// packed BCD to ASCII
void PBCD2Asc(unsigned char SrcReg, unsigned char *DstArr)
{
	*DstArr++ = (SrcReg >> 4) | '0';						// convert upper nibble to ASCII
	*DstArr-- = (SrcReg & 0x0F) | '0';						// convert lower nibble to ASCII
}
