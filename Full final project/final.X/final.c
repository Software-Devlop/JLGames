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
char changePasscode = 0x00;
char resetMode = 0x00;
char alarming = 0; // no alarm set (by default)

// Displaying Properties
char RawKey = 0x00;
char lcdPositions[] = {Ln2Ch0, Ln2Ch1, Ln2Ch2, Ln2Ch3};
char currentPosition = 0;
char digits[4];
char digit;
char msgDisp = 0; // a flag that specify is a message is displayed on the LCD

// Sets up the pins and ports that are going to be used in this project
void Setup(){
    // PORTA
    TRISA = 0xFF;
    ANSELA = 0x00;

    // PORTB
    TRISB = 0xF0;
    ANSELB = 0x00;
    PORTB = 0x00;
    INTCON2bits.RBPU = 0;
    INTCONbits.RBIE = 1;
    INTCONbits.RBIF = 0;

    // PORTC (output - for number of trials)
    TRISC = 0x00;
    ANSELC = 0x00;
    PORTC = trials;

    // PORTD (output)
    TRISD = 0x00;
    ANSELD = 0x00;
    PORTD = 0xFF;

    // PORTE
    TRISE = 0x00;
    ANSELE = 0x00;
    PORTE = 0x02;

    // Interrupt
    INTCONbits.TMR0IE = 1; //enable timer0 interrupt
    INTCONbits.GIE = 1; // enable global interrupt

    // LCD init
    InitLCD();
    DispRomStr(Ln1Ch0, (ROM)"Passcode: ");
}

// Gets the specefic keycode whenever a specefic keypad is pressed
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

// Clears the code entered by the user from both the memory and the lcd screen
void Clear(){
    char i;
    if(resetMode == 0x02){
        // set the array elements to 0xFF to detect void characters
         for(i=0; i<4; i++){
            digits[i] = 0xFF;
        }
         DispRomStr(Ln1Ch0, (ROM)"New Passcode:   ");
    }
    else{
        for(i=0; i<4; i++){
            digits[i] = 0;
        }
        if(resetMode == 0x00)
            DispRomStr(Ln1Ch0, (ROM)"Passcode:       ");
        else if(resetMode == 0x01)
            DispRomStr(Ln1Ch0, (ROM)"Current Passcode");
    }
    
    currentPosition = 0;
    DispRomStr(Ln2Ch0, (ROM)"                ");
    PORTEbits.RE0 = 0;
    PORTEbits.RE1 = 1;
}


// Checks whether the entered passcode is correct or not and display the appropriate action accordingly
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

        if(isEqual && resetMode == 0x00){
            PORTEbits.RE1 = 0;
            PORTEbits.RE0 = 1;
            DispRomStr(Ln1Ch0, (ROM)"CORRECT PASSCODE");
            DispRomStr(Ln2Ch0, (ROM)"CONGRATULATIONS!");
            msgDisp = 1;
            while(1);
        }
        else if(isEqual && resetMode == 0x01){
            Clear();
            DispRomStr(Ln1Ch0, (ROM)"New Passcode:   ");
            resetMode = 0x02;
            msgDisp = 1;
        }
        else if(resetMode == 0x02){
            char voidElement = 0;
            i = 0;
            while(i<4){
                if((digits[i]<<4)== (0xFF<<4)){
                    voidElement = 1;
                    break;
                }
                i++;
            }
            if(voidElement == 0){
                for(i=0; i<4; i++){
                    passCode[i] = digits[i];
                }
                DispRomStr(Ln1Ch0, (ROM)"PASSCODE CHANGE ");
                DispRomStr(Ln2Ch0, (ROM)"   SUCCEEDED!   ");
                resetMode = 0x00;
                msgDisp = 1;
            }
            else{
                DispRomStr(Ln1Ch0, (ROM)"PASSCODE SHOULD ");
                DispRomStr(Ln2Ch0, (ROM)"BE 4 DIGITS ONLY");
                msgDisp = 1;
            }
        }
        else{
            DispRomStr(Ln1Ch0, (ROM)"WRONG PASSCODE! ");
            DispRomStr(Ln2Ch0, (ROM)"   Try Again    ");
            msgDisp = 1;
            trials--;
            PORTC = trials;
            if(trials == 0x00){
                DispRomStr(Ln1Ch0, (ROM)" YOU SPENT ALL ");
                DispRomStr(Ln2Ch0, (ROM)"  YOUR TRIALS! ");
                alarming = 1;
                INTCONbits.T0IF=0;
                T0CON = 0b10010010;
                TMR0H = (65536 - 25344)/256;
                TMR0L = (65536 - 65156)%256;
            }
        }
        do_event = 0;
    }

}


void ChangePasscode(){
    if(PORTAbits.RA2==0){
        while(PORTAbits.RA2 == 0){
            changePasscode = 1;
        }
        if(changePasscode && resetMode==0x00){
            Clear();
            DispRomStr(Ln1Ch0, (ROM)"Current Passcode");
            DispRomStr(Ln2Ch0, (ROM)"                ");
            resetMode = 0x01;
        }
        changePasscode = 0;
    }
}

#pragma code ISR = 0x0008
#pragma interrupt ISR
void ISR(){
    // Getting Keycode
    char StringDigit[4];
    if(!alarming){
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
                if(resetMode == 0x00)
                    DispRomStr(Ln1Ch0, (ROM)"Passcode:       ");
                else if(resetMode == 0x01)
                    DispRomStr(Ln1Ch0, (ROM)"Current Passcode");
                else if(resetMode == 0x02)
                    DispRomStr(Ln1Ch0, (ROM)"New Passcode:   ");
            }
            if(digit != 0xFF && currentPosition<4 && trials !=0){
                Bin2AscE(digit, StringDigit);
                StringDigit[0] = StringDigit[4];
                digits[currentPosition] = StringDigit[0];
                DispVarStr(digits, Ln2Ch0, 5);
                currentPosition++;
            }
            TRISB=0xF0;
        }
        INTCONbits.RBIF=0;
    }
    else{
        TMR0H = (65536 - 25344)/256;
        TMR0L = (65536 - 65156)%256;
        INTCONbits.T0IF=0;
        PORTEbits.RE1 = !PORTEbits.RE1;
    }
}

int main(){
    Setup();
    while(1){
        if(!alarming){
            if(PORTAbits.RA1 == 0)
                Clear();
            CheckPasscode();
            ChangePasscode();
        }
    };
}