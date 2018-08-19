/**************************************************************************************
 * NEO PIXEL CONTROLLER
 * main.c
 * For PIC32MX795 compiled with XC32 V1.30
 *		
 * 8-19-18: Checked it again with 4x15 RGB rings
 * G7,G6,G5,G4,G3,G2,G1,G0, R7,R6,R5,R4,R3,R2,R1,R0, B7,B6,B5,B4,B3,B2,B1,B0
 * Reads three pots, controls RGB. Also UART set to 57600 baud.
 * Refreshes LEDs using Timer 1 interrupt @ 100 Hz
 **************************************************************************************/
 
#include <plib.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "DELAY32.H"
#define GetSystemClock() (80000000ul)	// 80 Mhz clock

#define false FALSE
#define true TRUE

#define TEST_OUT LATBbits.LATB0

#define MAXPOTS 3
unsigned short arrPots[MAXPOTS];
unsigned char dataReady = false;

// UART FOR PC SERIAL PORT
#define HOSTuart UART2
#define HOSTbits U2STAbits
#define HOST_VECTOR _UART_2_VECTOR

#define MAXBUFFER 128
unsigned char HOSTRxBuffer[MAXBUFFER];
unsigned char HOSTRxBufferFull = false;
void ConfigAd(void);

#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config FSOSCEN = OFF, IESO = OFF, FPBDIV = DIV_1, FCKSM = CSDCMD
#pragma config POSCMOD = HS, FNOSC = PRIPLL
#pragma config  DEBUG = OFF, PWP = OFF, BWP = OFF, CP = OFF

#define RED 1
#define GREEN 2
#define BLUE 0

typedef struct {
    union {
        unsigned char LED[3];
        unsigned long color;
    };
} LEDtype;

#define NUM_PIXELS 60
LEDtype arrPixels[NUM_PIXELS];
#define NUMPOTS 3
unsigned char arrADdata[NUMPOTS];
extern void writePixels(void);
void InitializeSystem(void);

int main(void) {
unsigned short i, displayCounter = 200;
unsigned char REDval, GREENval, BLUEval;

    for(i=0; i<NUM_PIXELS; i++){
		arrPixels[i].LED[RED]=0x00;
		arrPixels[i].LED[GREEN]=0x00;
		arrPixels[i].LED[BLUE]=0xFF;        
    }
 
    InitializeSystem();    
    DelayMs(100);    
    printf("\rTimer interrupts - refresh 100 Hz");
    
	while(1){		 
        if (HOSTRxBufferFull)
        {
            HOSTRxBufferFull = false;
            printf("\rReceived: %s", HOSTRxBuffer);
        }
        
        if (dataReady)
        {
            dataReady = false;
            REDval = arrPots[0]/4;
            GREENval = arrPots[1]/4;
            BLUEval = arrPots[2]/4;
            
            if (displayCounter) displayCounter--;
            else
            {
                displayCounter = 50;
                // printf ("\rR: %02X, G: %02X, B: %02X", REDval, GREENval, BLUEval);
            }
            mAD1IntEnable(INT_ENABLED);   
        }        
        
        for (i = 0; i < NUM_PIXELS; i++)
        {
        	arrPixels[i].LED[RED] = REDval;
            arrPixels[i].LED[GREEN] = GREENval;
            arrPixels[i].LED[BLUE] = BLUEval;            
        }                            
        DelayMs(1);         
	} 
}
    
void __ISR(_ADC_VECTOR, ipl6) ADHandler(void) {
    unsigned short offSet;
    unsigned char i;

    mAD1IntEnable(INT_DISABLED);
    mAD1ClearIntFlag();

    // Determine which buffer is idle and create an offset
    offSet = 8 * ((~ReadActiveBufferADC10() & 0x01));

    for (i = 0; i < MAXPOTS; i++)
        arrPots[i] = (unsigned short) ReadADC10(offSet + i); // read the result of channel 0 conversion from the idle buffer
    dataReady = TRUE;
}

void ConfigAd(void)
{

    mPORTBSetPinsAnalogIn(BIT_3 | BIT_8 | BIT_9);

    // ---- configure and enable the ADC ----

    // ensure the ADC is off before setting the configuration
    CloseADC10();

    // define setup parameters for OpenADC10
    //                 Turn module on | ouput in integer | trigger mode auto | enable autosample
#define PARAM1  ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON

    // ADC ref external    | disable offset test    | enable scan mode | perform  samples | use dual buffers | use only mux A
#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_4 | ADC_ALT_BUF_ON | ADC_ALT_INPUT_OFF

    //                   use ADC internal clock | set sample time
#define PARAM3  ADC_CONV_CLK_INTERNAL_RC | ADC_SAMPLE_TIME_31

#define PARAM4    ENABLE_AN3_ANA | ENABLE_AN8_ANA | ENABLE_AN9_ANA

// USE AN3, AN8, AN9    
#define PARAM5 SKIP_SCAN_AN0 | SKIP_SCAN_AN1 | SKIP_SCAN_AN2 |\
SKIP_SCAN_AN4 | SKIP_SCAN_AN5 | SKIP_SCAN_AN6 | SKIP_SCAN_AN7 |\
SKIP_SCAN_AN10 |\
SKIP_SCAN_AN11 | SKIP_SCAN_AN12 | SKIP_SCAN_AN13 | SKIP_SCAN_AN14 | SKIP_SCAN_AN15


    // set negative reference to Vref for Mux A
    SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF);

    // open the ADC
    OpenADC10(PARAM1, PARAM2, PARAM3, PARAM4, PARAM5);

    ConfigIntADC10(ADC_INT_PRI_2 | ADC_INT_SUB_PRI_2 | ADC_INT_ON);

    // clear the interrupt flag
    mAD1ClearIntFlag();

    // Enable the ADC
    EnableADC10();
}

#define CR 13
#define LF 10
#define BACKSPACE 8

// HOST UART interrupt handler it is set at priority level 2
void __ISR(HOST_VECTOR, ipl2) IntHostUartHandler(void) {
    unsigned char ch;
    static unsigned short HOSTRxIndex = 0;

    if (HOSTbits.OERR || HOSTbits.FERR) {
        if (UARTReceivedDataIsAvailable(HOSTuart))
            ch = UARTGetDataByte(HOSTuart);
        HOSTbits.OERR = 0;
        HOSTRxIndex = 0;
    } else if (INTGetFlag(INT_SOURCE_UART_RX(HOSTuart))) {
        INTClearFlag(INT_SOURCE_UART_RX(HOSTuart));
        if (UARTReceivedDataIsAvailable(HOSTuart)) {
            ch = toupper(UARTGetDataByte(HOSTuart));            
            if (ch == '$') HOSTRxIndex = 0;
            if (ch == LF || ch == 0);
            else if (ch == BACKSPACE) {
                while (!UARTTransmitterIsReady(HOSTuart));
                UARTSendDataByte(HOSTuart, ' ');
                while (!UARTTransmitterIsReady(HOSTuart));
                UARTSendDataByte(HOSTuart, BACKSPACE);
                if (HOSTRxIndex > 0) HOSTRxIndex--;
            } else if (ch == CR) {
                if (HOSTRxIndex < MAXBUFFER) {
                    HOSTRxBuffer[HOSTRxIndex] = CR;
                    HOSTRxBuffer[HOSTRxIndex + 1] = '\0'; 
                    HOSTRxBufferFull = true;
                }
                HOSTRxIndex = 0;
            }                
            else if (HOSTRxIndex < MAXBUFFER)
                HOSTRxBuffer[HOSTRxIndex++] = ch;
        }
    }
    if (INTGetFlag(INT_SOURCE_UART_TX(HOSTuart))) {
        INTClearFlag(INT_SOURCE_UART_TX(HOSTuart));
    }
}

void __ISR(_TIMER_1_VECTOR, ipl2) Timer1Handler(void){
static short intCounter = 10;    
    
    mT1ClearIntFlag(); // Clear interrupt flag
    
    if (intCounter)intCounter--;
    if (!intCounter)
    {    
        writePixels();
        intCounter = 10;
        if (TEST_OUT) TEST_OUT = 0;
        else TEST_OUT = 1;
    }
}
 
void InitializeSystem(void)
{
    // Enable optimal performance
    SYSTEMConfigPerformance(GetSystemClock());
    mOSCSetPBDIV(OSC_PB_DIV_1); // Use 1:1 CPU Core:Peripheral clocks

    // Turn off JTAG so we get the pins back
    mJTAGPortEnable(false);
    
    PORTSetPinsDigitalOut(IOPORT_F, BIT_0);        
    PORTSetPinsDigitalOut(IOPORT_B, BIT_0);
        
    // Set up HOST UART
    UARTConfigure(HOSTuart, UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(HOSTuart, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(HOSTuart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(HOSTuart, GetSystemClock(), 57600);
    UARTEnable(HOSTuart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // Configure HOST UART Interrupts
    INTEnable(INT_SOURCE_UART_TX(HOSTuart), INT_DISABLED);
    INTEnable(INT_SOURCE_UART_RX(HOSTuart), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(HOSTuart), INT_PRIORITY_LEVEL_2);
    INTSetVectorSubPriority(INT_VECTOR_UART(HOSTuart), INT_SUB_PRIORITY_LEVEL_0);
    
    ConfigAd();
    
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_64, 1250);    

    // Turn on the interrupts
    INTEnableSystemMultiVectoredInt();            
}   
