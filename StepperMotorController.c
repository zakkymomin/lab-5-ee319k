// StepperMotorController.c starter file EE319K Lab 5
// Runs on TM4C123
// Finite state machine to operate a stepper motor.  
// Jonathan Valvano
// September 2, 2019

// Hardware connections (External: two input buttons and four outputs to stepper motor)
//  PA5 is Wash input  (1 means pressed, 0 means not pressed)
//  PA4 is Wiper input  (1 means pressed, 0 means not pressed)
//  PE5 is Water pump output (toggle means washing)
//  PE4-0 are stepper motor outputs 
//  PF1 PF2 or PF3 control the LED on Launchpad used as a heartbeat
//  PB6 is LED output (1 activates external LED on protoboard)

#include "SysTick.h"
#include "TExaS.h"
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

void EnableInterrupts(void);
// edit the following only if you need to move pins from PA4, PE3-0      
// logic analyzer on the real board
#define PA4       (*((volatile unsigned long *)0x40004040))
#define PORTA54 			(*((volatile unsigned long *)0x400040C0))
#define PE50      (*((volatile unsigned long *)0x400240FC))
#define PF3 			(*((volatile unsigned long *)0x40025020))
void PortE_Init (void){
volatile unsigned long delay; 
SYSCTL_RCGC2_R |= 0x10;  // 1) activate Port E
delay = SYSCTL_RCGC2_R;          // allow time for clock to stabilize
GPIO_PORTE_AMSEL_R = 0x00;       // 3) disable analog functionality on PE7-0
GPIO_PORTE_PCTL_R = 0x00000000;  // 4) configure PB7-0 as GPIO
GPIO_PORTE_DIR_R = 0x3F;         // 5) make PE5-0 out
GPIO_PORTE_AFSEL_R = 0x00;       // 6) disable alt funct on PE7-0
GPIO_PORTE_DR8R_R = 0xFF;        // enable 8 mA drive on PE7-0
GPIO_PORTE_DEN_R = 0x3F;         // 7) enable digital I/O on PE7-0
}

void PortF_Init (void){
volatile unsigned long delay; 
SYSCTL_RCGC2_R |= 0x20;  // 1) activate Port F
delay = SYSCTL_RCGC2_R;          // allow time for clock to stabilize
GPIO_PORTF_DIR_R |= 0x08;         // 5) make PB6 out
GPIO_PORTF_DEN_R |= 0x08;         // 7) enable digital I/O on PB6
}
void PortB_Init (void){
volatile unsigned long delay; 
SYSCTL_RCGC2_R |= 0x02;  // 1) activate Port B
delay = SYSCTL_RCGC2_R;          // allow time for clock to stabilize
GPIO_PORTB_AMSEL_R &= ~0x40;       // 3) disable analog functionality on PB6
GPIO_PORTB_PCTL_R &= ~0x00F00FF;  // 4) configure PB7-0 as GPIO
GPIO_PORTB_DIR_R |= 0x40;         // 5) make PB6 out
GPIO_PORTB_AFSEL_R = 0x00;       // 6) disable alt funct on PB7-0
GPIO_PORTB_DR8R_R = 0xFF;        // enable 8 mA drive on PB7-0
GPIO_PORTB_DEN_R |= 0x40;         // 7) enable digital I/O on PB6
}
void PortA_Init (void){
volatile unsigned long delay; 
SYSCTL_RCGC2_R |= 0x01;  // 1) activate Port A
delay = SYSCTL_RCGC2_R;          // allow time for clock to stabilize
GPIO_PORTA_AMSEL_R &= ~0x30;       // 3) disable analog functionality on PA4 PA5
GPIO_PORTA_PCTL_R &= ~0x00FF0FF;  // 4) configure PA45 as GPIO
GPIO_PORTA_DIR_R &= ~0x30;         // 5) make PA4-5 input
GPIO_PORTA_AFSEL_R = ~0x30;       // 6) disable alt funct on PA4-5
GPIO_PORTA_DEN_R |= 0x30;         // 7) enable digital I/O on PA4-5
}
void SendDataToLogicAnalyzer(void){
  UART0_DR_R = 0x80|(PA4<<2)|PE50;
}

struct State {
	unsigned char out; 
	unsigned short wait; 
	unsigned char next[4];
};

typedef const struct State StateType;

StateType Fsm[39] = {
	{0x00, 5, {0, 1, 20, 20}},// 
	{0x01, 5, {2, 2, 21, 21}},
	{0x02, 5, {3, 3, 22, 22}},
	{0x04, 5, {4, 4, 23, 23}},
	{0x08, 5, {5, 5, 24, 24}},
	{0x10, 5, {6, 6, 25, 25}},
	{0x01, 5, {7, 7, 26, 26}},
	{0x02, 5, {8, 8, 27, 27}},
	{0x04, 5, {9, 9, 28, 28}},
	{0x08, 5, {10, 10, 29, 29}},
	{0x10, 5, {11, 11, 30, 30}},
	{0x08, 5, {12, 12, 31, 31}},	
	{0x04, 5, {13, 13, 32, 32}},
	{0x02, 5, {14, 14, 33, 33}},
	{0x01, 5, {15, 15, 34, 34}},
	{0x10, 5, {16, 16, 35, 35}},
	{0x08, 5, {17, 17, 36, 36}},
	{0x04, 5, {18, 18, 37, 37}},
	{0x02, 5, {19, 19, 38, 38}},
	{0x01, 5, {0, 2, 20, 20}},
	{0x21, 5, {2, 2, 21, 21}},
	{0x22, 5, {3, 3, 22, 22}},
	{0x24, 5, {4, 4, 23, 23}},
	{0x28, 5, {5, 5, 24, 24}},
	{0x30, 5, {6, 6, 25, 25}},
	{0x21, 5, {7, 7, 26, 26}},
	{0x22, 5, {8, 8, 27, 27}},
	{0x24, 5, {9, 9, 28, 28}},
	{0x28, 5, {10, 10, 29, 29}},
	{0x30, 5, {11, 11, 30, 30}},
	{0x28, 5, {12, 12, 31, 31}},
	{0x24, 5, {13, 13, 32, 32}},
	{0x22, 5, {14, 14, 33, 33}},
	{0x21, 5, {15, 15, 34, 34}},
	{0x30, 5, {16, 16, 35, 35}},
	{0x28, 5, {17, 17, 36, 36}},
	{0x24, 5, {18, 18, 37, 37}},
	{0x22, 5, {19, 19, 38, 38}},
	{0x21, 5, {0, 2, 21, 21}},
	
	
};
	
unsigned char cState;
//unsigned char input;

int main(void){ 
  TExaS_Init(&SendDataToLogicAnalyzer);    // activate logic analyzer and set system clock to 80 MHz
	SysTick_Init(); 	
	PortE_Init();
	PortB_Init();
	PortA_Init();
	PortF_Init();
	cState= 0;	
// you initialize your system here
	
  EnableInterrupts();  
  while(1){
		
		PF3 ^= 0x08;
		GPIO_PORTE_DATA_R = Fsm[cState].out; 
		//GPIO_PORTB_DATA_R = Fsm[cState].out<<1; 
		//GPIO_PORTB_DATA_R &= 0X40; 
		SysTick_Wait(4000000);
		GPIO_PORTE_DATA_R &= 0xFDF; 
		SysTick_Wait10ms (Fsm[cState].wait);
		//input = PORTA54;
		cState = Fsm[cState].next[PORTA54>>4];
		
// output
// wait
// input
// next		
  }
}


