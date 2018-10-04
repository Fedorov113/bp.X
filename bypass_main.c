/* 
 * File:   bypass_main.c
 * Author: fedor
 *
 * Created on March 19, 2018, 1:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
// Supplementary libraries 
#include <stdint.h>
#include <xc.h> 
// Include configuration bits
#include "header.h"

//#define LED  PORTCbits.RC6
#define PAR_LED  LATC6
#define EF_1_LED LATC1
#define EF_2_LED  LATA5

#define EF_1_B  PORTAbits.RA3
#define CENTR_B  PORTBbits.RB4
#define EF_2_B  PORTAbits.RA0

#define K1  LATC3
#define K2  LATA2
#define K3  LATB5
#define K4  LATC2
#define K5  LATC4
#define K6  LATA1
#define K7  LATC5
#define K8  LATC0
#define GLUH  LATC7

const unsigned long gluh_wait = 100;

void now_change(int state,  uint8_t ef1, uint8_t ef2) {
    //Here, for relay 1 means UP and 0 means DOWN (According to schematic)
    EF_1_LED = ef1;
    EF_2_LED = ef2;    
    K4 = ef1;
    K3 = ef2;
    if (state == 0) {//E1 -> E2 || clean, no loop
        PAR_LED = 0;
        K1 = 1;
        K2 = 1;
        K5 = 0;
        K6 = 0;
        K7 = 1;
        K8 = 0;
    } else if (state == 1) {//E2 -> E1 no loop
        PAR_LED = 0;
        K1 = 0;
        K2 = 0;
        K5 = 1;
        K6 = 1;
        K7 = 0;
        K8 = 1;
    } else if (state == 2) {//E1 || E2
        PAR_LED = 1;
        K1 = 1;
        K2 = 0;
        K5 = 0;
        K6 = 1;
        K7 = 0;
        K8 = 1;
    } else if (state == 3) {//CLEAN PAR, NO LOOP
        K1 = 1;
        K2 = 1;
        K5 = 1;
        K6 = 1;
        K7 = 0;
        K8 = 1;
    } 
    else if (state == 10) { //special case for debugging
        EF_1_LED = 1;
        EF_2_LED = 1;    
        PAR_LED = 1;
        K1 = 0;
        K2 = 0;
        K3 = 0;
        K4 = 0;
        K5 = 0;
        K6 = 0;
        K7 = 0;
        K8 = 0;
    }else {
        K6 = 0;
        K1 = 1;
        __delay_ms(200);
        K6 = 1;
        K1 = 1;
        __delay_ms(200);
    }
}

void all_on(int state, uint8_t gluh, uint8_t ef1, uint8_t ef2) {
    
    if(gluh == 1){
        GLUH = 1; // photoFET on
        __delay_ms(20);
    }

    now_change(state, ef1, ef2);
    
    if(gluh == 1){
        __delay_ms(20);
        GLUH = 0; // photoFET on
    }

}

int main(int argc, char** argv) {
    OSCCONbits.IRCF = 0b110; //set freq mode 4
    ADCON0 = 0b00001000; // Disable A/D mode
    ADCON1 = 0b00001111; // All digital  I/O    
    
    CMCON  = 0b00000111; // comparators off
    CVRCON = 0b00000000;
    
    T1CON  = 0; //disable timer 1

    PORTC = 0b00000000;
    TRISC = 0b00000000;

    TRISA = 0b00001001; // All PORTA pins except 0 and 3 are configured as outputs
    PORTA = 0b00000000; // All PORTA pins are cleared
    
    PORTB = 0b00000000;
    TRISB = 0b00010000;

    uint8_t state = 0; //on/off state of the pedal; 1 - on, 0 - off
    uint8_t pred_state = 0;
    uint8_t pred_pred_state = 0;
    uint8_t changestate = 0; // to change status of the pedal
    uint8_t press_switch = 0; // variable to detect if the switch is pressed

    uint8_t press_switch_ar[3] = {0, 0, 0};
    all_on(0, 1, 1, 1);
    uint8_t SERIES = 1;
    uint8_t LOOP = 0;

    uint8_t e1_state = 1;
    uint8_t change_e1_state = 0;
    
    uint8_t e2_state = 1;
    uint8_t change_e2_state = 0;
    
    while (1) {
        //BUTTON EFFECT 1
        if (EF_1_B == 0) {//button pressed
            __delay_ms(15); // debouncing
            if (EF_1_B == 0) {// still pressed
                press_switch_ar[0] = press_switch_ar[0] + 1; // switch is on
                if (press_switch_ar[0] > 10) {
                    press_switch_ar[0] = 10; // max value for press_switch
                }
            }
        }
        if (press_switch_ar[0] == 2) { // switch is pressed : lets turn the pedal on or off
            changestate = 1;
            e1_state = !e1_state;
            press_switch_ar[0] = 3; // avoid bugs if press_switch stays at 1
        }
        if (EF_1_B == 1) {
            __delay_ms(15); // debouncing
            if (EF_1_B == 1) {
                press_switch_ar[0] = 0;
            }
        }
        
        //CENTRAL BUTTON
        if (CENTR_B == 0) {//button pressed
            __delay_ms(15); // debouncing
            if (CENTR_B == 0) {
                press_switch_ar[1] = press_switch_ar[1] + 1; // switch is on
                if (press_switch_ar[1] > 10) {
                    press_switch_ar[1] = 10; // max value for press_switch
                }
            }
        }
        if (press_switch_ar[1] == 1) { // switch is pressed : lets turn the pedal on or off
            changestate = 1;
            state = 2;
            if(pred_state == 2)
                state = 0;
            press_switch_ar[1] = 2; // avoid bugs if press_switch stays at 1
        }
        if (CENTR_B == 1) {
            __delay_ms(15); // debouncing
            if (CENTR_B == 1) {
                press_switch_ar[1] = 0;
            }
        }
        
        //BUTTON FOR 2 EFFECT 
        if (EF_2_B == 0) {//button pressed
            __delay_ms(15); // debouncing
            if (EF_2_B == 0) {
                press_switch_ar[2] = press_switch_ar[2] + 1; // switch is on
                if (press_switch_ar[2] > 10) {
                    press_switch_ar[2] = 10; // max value for press_switch
                }
            }
        }
        if (press_switch_ar[2] == 2) { // switch is pressed : lets turn the pedal on or off
            changestate = 1;
            e2_state = !e2_state;
            press_switch_ar[2] = 3; // avoid bugs if press_switch stays at 1
        }
        if (EF_2_B == 1) {
            __delay_ms(15); // debouncing
            if (EF_2_B == 1) {
                press_switch_ar[2] = 0;
            }
        }
        
        // Changing state of the pedal
        if (changestate == 1) {
            all_on(state, 1, e1_state, e2_state); // relay on 
            pred_state = state;
            __delay_ms(10);
            changestate = 0; // reset changestate
        }
//        
    }
    return (EXIT_SUCCESS);
}



//        if (change_e2_state == 1) {
//            PAR_LED = !e2_state;
//            EF_2_LED = !e2_state;
//            K3=e2_state; // relay on 
//            __delay_ms(10);
//            change_e2_state = 0; // reset changestate
//        }
//        
//        if (change_e1_state == 1) {
//            PAR_LED = !e1_state;
//            EF_1_LED = !e1_state;
//            K3=e1_state; // relay on 
//            __delay_ms(10);
//            change_e1_state = 0; // reset changestate
//        }
//        
//        // To let the pedal in the good state (on or off)
//        //all_on(state, 0);
//        if (state == 1) { // effect on
//            //LED = 1; // LED on
//            //all_on(1); // relay on
//        } else { // effect off
//            //LED = 0; // LED off
//            //all_on(0); // relay off
//        }