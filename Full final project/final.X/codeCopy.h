#include<p18f45k22.h>
#include<delays.h>
#include"LCDLib.h"

// General Properties
char passCode[4] = {1,2,3,3};
char ButtonsContent[] = {0b10111110, 0b01111101, 0b10111101,               // 0 1 2
                         0b11011101, 0b01111011, 0b10111011,               // 3 4 5
                         0b11011011, 0b01110111, 0b10110111, 0b11010111 }; // 6 7 8 9
char trials = 0x03;
char do_event = 0x00;

// Displaying Properties
char RawKey = 0x00;
char lcdPositions[] = {Ln2Ch0, Ln2Ch1, Ln2Ch2, Ln2Ch3};
char currentPosition = 0;
char digits[4];
char digit;
char msgDisp = 0; // a flag that specify is a message is displayed on the LCD

void Setup(){
    // PORTA
    TRISA = 0xFF;
    ANSELA = 0x00;

    // PORTB
    TRISB = 0xF0; // in out pins
    ANSELB = 0x00; // set to digital
    PORTB = 0x00;  // set RB0 to RB3 to 0
    INTCON2bits.RBPU = 0; // activating pull up resistor
    INTCONbits.RBIE = 1;  // enabling interruption on PORTB
    INTCONbits.RBIF = 0;

    // Testingggggggg
    TRISC = 0x00;
    ANSELC = 0x00;
    PORTC = 0x00;

    // PORTD (output)
    TRISD = 0x00;
    ANSELD = 0x00;
    PORTD = 0xFF;

    // PORTE
    TRISE = 0x00;
    ANSELE = 0x00;
    PORTE = 0x02;

    // Interrupt
    INTCONbits.GIE = 1; // enable global interrupt

    // LCD init
    InitLCD();
    DispRomStr(Ln1Ch0, (ROM)"Passcode: ");
}

char GetKeyCode(){
    char i = 0;
    while(RawKey!=ButtonsContent[i]){
        i++;
    }
    if(i<10){
        return i;
    }
    return 0xFF;
}


void CheckPasscode(){
    while(PORTAbits.RA0 == 0){
        do_event = 1;
    }
    if(do_event){
        char i = 0, isEqual = 1;
        Delay10KTCYx(2);
        while(i<4){
            if((digits[i]<<4)!= (passCode[i]<<4)){
                isEqual = 0;
                break;
            }
            i++;
        }

        if(isEqual == 1){
            PORTEbits.RE1 = 0;
            PORTEbits.RE0 = 1;
            DispRomStr(Ln1Ch0, (ROM)"CORRECT PASSCODE");
            DispRomStr(Ln2Ch0, (ROM)"CONGRATULATIONS!");
            msgDisp = 1;
            while(1);
        }
        else{
            DispRomStr(Ln1Ch0, (ROM)"WRONG PASSCODE!");
            DispRomStr(Ln2Ch0, (ROM)"   Try Again   ");
            msgDisp = 1;
            trials--;
            if(trials == 0x00){
                DispRomStr(Ln1Ch0, (ROM)" YOU SPENT ALL ");
                DispRomStr(Ln2Ch0, (ROM)"  YOUR TRIALS! ");
                while(1);
            }
        }
        do_event = 0;
    }
}

void Clear(){
    char i;
    currentPosition = 0;
    for(i=0; i<4; i++){
        digits[i] = 0;
    }
    DispRomStr(Ln1Ch0, (ROM)"Passcode:       ");
    DispRomStr(Ln2Ch0, (ROM)"                ");
    PORTEbits.RE0 = 0;
    PORTEbits.RE1 = 1;
}


#pragma code ISR = 0x0008
#pragma interrupt SFR
void SFR(){
    // Getting Keycode
    char StringDigit[4];
    Delay10KTCYx(2);
    if ((~PORTB&0xF0)!= 0){
        RawKey = PORTB&0xF0;
        TRISB=0xFF;
        TRISB=0x0F;
        RawKey|=PORTB;
        digit = GetKeyCode();
        if(msgDisp==1){
            Clear();
            msgDisp = 0;
        }
        if(digit != 0xFF && currentPosition<4 && trials !=0){
            Bin2AscE(digit, StringDigit);  // Convert the digit to a string
            StringDigit[0] = StringDigit[4];
            digits[currentPosition] = StringDigit[0];
            DispVarStr(digits, Ln2Ch0, 5);
            currentPosition++;
        }
        TRISB=0xF0;
    }
    INTCONbits.RBIF=0;
}

int main(){
    Setup();
    while(1){
        if(PORTAbits.RA1 == 0)
            Clear();
        CheckPasscode();
    };
}

