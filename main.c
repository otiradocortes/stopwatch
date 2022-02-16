//Osvaldo Tirado-Cortes
//Lab 5-3 Fully functional stopwatch: Button 1 Pauses and Resume Counter function, while Button 2 resets timer in paused or continuous state.
#include <msp430fr6989.h>
#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7
#define BUT1 BIT1                           // Button S1 at Port 1.1
#define BUT2 BIT2                           // Button S2 at Port 1.2


void config_ACLK_to_32KHz_crystal();        //function declaration
void Initialize_LCD();
void PauseLCDtimer();
void InternalLCDtimer();
void display_num_lcd(unsigned int n);
int timerCounter = 0, loopCounter;

const unsigned char LCD_Num[10] = {0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE0, 0xFF, 0xE7}; //character array containing all numbers for display

int main(void) {                        //main function start
    WDTCTL = WDTPW | WDTHOLD;           // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;               // Enable GPIO pins
    P1DIR |= redLED;                        //direct pins as output
    P9DIR |= greenLED;                      //
    P1OUT &= ~redLED;                       //red LED off
    P9OUT |= greenLED;                     //green LED on
    P1DIR &= ~(BUT1|BUT2);                  // 0: input
    P1REN |= (BUT1|BUT2);                   // 1: enable built-in resistors
    P1OUT |= (BUT1|BUT2);                   // 1: built-in resistor is pulled up to Vcc
    P1IE  |= (BUT1|BUT2);                   // 1: enable interrupts
    P1IES |= (BUT1|BUT2);                   // 1: interrupt on falling edge
    P1IFG &= ~(BUT1|BUT2);                  // 0: clear the interrupt flags

    config_ACLK_to_32KHz_crystal();         // Configure ACLK to the 32 KHz crystal
    Initialize_LCD();                       // Initializes the LCD_C module
    LCDCMEMCTL = LCDCLRM;                   // Clears all the segments

    // Configure Channel 0 for up mode with interrupt
    TA0CCR0 = 32767;                           // 1 second @ 32 KHz
    TA0CCTL0 |= CCIE;                          // Enable Channel 0 CCIE bit
    TA0CCTL0 &= ~CCIFG;                        // Clear Channel 0 CCIFG bit


    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;    //ACLK, divide by 1, up mode, clear TAR
    _low_power_mode_3();
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR(void) {
    if(loopCounter==0){                     //loop dependent on button ISR loop counter; 0 to count
        InternalLCDtimer();
        TA0CCTL0 &= ~CCIFG;                 //reset interrupt flag
    }
    else{
        display_num_lcd(timerCounter);      //else display current number until loop statement is true again
    }
}

#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void) {

    if((P1IN&BUT1)==0) {                //loop to check if button 1 has been pressed
           P1IFG &= ~BUT1;              //reset button flag
           if(loopCounter == 0) {       //runs after loop counter has been set to 0
               loopCounter++;              //sets counter to 1 to pause counting
               TA0CCTL0 &= ~CCIFG;         //resets interrupt flag

               P1OUT |= redLED;            //turns on red LED
               P9OUT &= ~greenLED;         //turns off green LED

               }

           else{            //else should always be running (on other button press)
                   P9OUT ^= greenLED;       //green LED toggles opposite red LED
                   P1OUT &= ~redLED;        //red is set to off
                   TA0CCTL0 &= ~CCIFG;      //resets interrupt flag
                   loopCounter = 0;         //sets counter to 0 to allow for continuous counting
               }
       }

    if((BUT2 & P1IN)==0) {                  //if button 2 is pressed to reset
        LCDCMEMCTL = LCDCLRM;               //clears all segements on LCD display
        timerCounter=0;                     //sets global timer counter to 0
        display_num_lcd(timerCounter);      //displays current timerCounter in a stopped state or has a short pause in continuous count
        P1IFG &= ~BUT2;                     //reset button flag
    }

}




void display_num_lcd(unsigned int n){
    int i;                  //loop variable to allow for 0 condition
    int temp;               //dividing variable to determine length of passed number
    temp = n;               // sets temp equal to passed

    while(temp!=0) {       //temp is divided into 10s, i equals total divisions
        temp = temp/10;
        i++;                //returns count greater than 0
    }

    if(i>=0) {                  //condition to make sure 0 always prints
        LCDM8 = LCD_Num[n%10];  //modulo pos[0] number
        n = n/10;               //divide by passed
    }

    if(n!=0){                   //condition to drop leading zero
        LCDM15 = LCD_Num[n%10]; //modulo pos[1] number
        n = n/10;               //divide by passed
    }

    if(n!=0){
        LCDM19 = LCD_Num[n%10]; //modulo pos[2]
        n = n/10;
    }

    if(n!=0){
        LCDM4 = LCD_Num[n%10]; //modulo pos[3]
        n = n/10;
    }

    if(n!=0){
        LCDM6 = LCD_Num[n%10]; //modulo pos[4]
        n = n/10;
    }
}

void Initialize_LCD() {
    PJSEL0 = BIT4 | BIT5; // For LFXT
    // Initialize LCD segments 0 - 21; 26 - 43
    LCDCPCTL0 = 0xFFFF;
    LCDCPCTL1 = 0xFC3F;
    LCDCPCTL2 = 0x0FFF;
    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8; // Unlock CS registers
    CSCTL4 &= ~LFXTOFF; // Enable LFXT
    do {
            CSCTL5 &= ~LFXTOFFG; // Clear LFXT fault flag
            SFRIFG1 &= ~OFIFG;
    }
    while (SFRIFG1 & OFIFG);   // Test oscillator fault flag
    CSCTL0_H = 0;               // Lock CS registers
    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
    LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled
    LCDCMEMCTL = LCDCLRM; // Clear LCD memory
    //Turn LCD on
    LCDCCTL0 |= LCDON;
    return;
}

void config_ACLK_to_32KHz_crystal() {
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;
    CSCTL0 = CSKEY;                             // Unlock CS registers
    do {
        CSCTL5 &= ~LFXTOFFG;                    // Local fault flag
        SFRIFG1 &= ~OFIFG;                      // Global fault flag
    }
    while((CSCTL5 & LFXTOFFG) != 0);
    CSCTL0_H = 0;                               // Lock CS registers
    return;
}

void InternalLCDtimer() {
    unsigned int n,j;
    if(timerCounter!=65535) {       //checks condition to  make sure int isn't at rollover point
    display_num_lcd(timerCounter);  //displays number
    timerCounter++;                 //keeps counting
    }
    else{
   display_num_lcd(65535);
   for(n=0; n<=3; n++) {for(j=0; j<=60000; j++) {}} //cheap delay loop to allow the appearance of rolling over
   LCDCMEMCTL = LCDCLRM;            //if it is at rollover, clears the LCD
   timerCounter=0;                  //sets global variable to 0
   display_num_lcd(timerCounter);   //displays to LCD to continue counting.
   timerCounter++;
    }

   return;
}
