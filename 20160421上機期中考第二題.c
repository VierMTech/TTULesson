//**********************************************************
//*               Ex9_1_TMR1_INT.c
//*	�]�p�@��0.5���� PORTD �� LED ����ܪ��G�i��Ʀr�۰ʥ[�@���{��
//**********************************************************
#include 	<p18f4520.h> 	//�ǤJ�L����w�q��
#include 	<timers.h> 		//�ǤJ�p�ɾ��禡�w�w�q��
#include 	<adc.h>
#include 	<stdlib.h>
#include	<stdio.h>
#include	<math.h>
// ���c�줸�w�q
#pragma config	OSC=HS, BOREN=OFF, BORV = 2, PWRT=ON, WDT=OFF, LVP=OFF

#define TMR1_VAL 	65536-328		// Timer1 �]�w�� 100ms ���_�@��
#define PORTA4ButtonPress() 	!((PORTA & 0x10) == 0x10)
#define PORTA4ButtonRelease() 	((PORTA & 0x10) == 0x10)

//�ŧi�禡�쫬
void Init_TMR1(void);
void timer1_isr(void);
void MyTimerFunction(void);
//�ܼƫŧi
unsigned int CounterA = 0;

unsigned int ButtonA4Counter = 0;
unsigned char ButtonA4Flag = 0;
unsigned int ButtonA4Event = 0;

unsigned int SystemStep = 0;

unsigned char RD0Flag = 0;

#pragma code

void MyTimerFunction(void)
{
	if (ButtonA4Counter > 0) ButtonA4Counter--;

}

void ButtonProcess(void)
{
	if (PORTA4ButtonPress())
	{
		ButtonA4Counter = 1;
		ButtonA4Flag = 1;
	}
	else if (PORTA4ButtonRelease() && ButtonA4Flag == 1 && ButtonA4Counter == 0)
	{
		ButtonA4Flag = 0;
		ButtonA4Event = 1;
	}
}
void main(void)
{
	Init_TMR1();			// ��l�Ƴ]�wTimer1�禡
	INTCONbits.PEIE = 1;	// �}�ҩP�䤤�_�\��
	INTCONbits.GIE = 1;		// �}�ҥ��줤�_����

	TRISD = 0;
	PORTD = 0;
	TRISA |= 0x10;

	while (1)
	{
		ButtonProcess();

		if (ButtonA4Event == 1)
		{
			ButtonA4Event = 0;
			if (RD0Flag == 0)
			{
				RD0Flag = 1;
				PORTD |= 0x01;
			}
			else
			{
				RD0Flag = 0;
				PORTD &= ~0x01;
			}
		}

	};

}

//�ŧi�æw�Ƥ��_����{���O�����m
#pragma code high_vector=0x08
void high_interrupt(void)
{
	_asm GOTO
	timer1_isr _endasm
}
#pragma code

#pragma interrupt timer1_isr

void timer1_isr(void)
{

	PIR1bits.TMR1IF = 0;	// �M�����_�X��
	WriteTimer1(TMR1_VAL);	// ��N�p�ƾ�Ĳ�o�����k�s�g�J�w�]��

	MyTimerFunction();

}
void Init_TMR1(void)
{

	OpenTimer1(TIMER_INT_ON &		// �ϥ�C18�sĶ��timer�禡�w
			T1_16BIT_RW &		// ��l�Ƴ]�wTimer1
			T1_SOURCE_EXT &		// �ö}��TIMER1���_�\��(PIE1bits.TMR1IE=1)
			T1_PS_1_1 & T1_OSC1EN_ON & T1_SYNC_EXT_ON);
	WriteTimer1(TMR1_VAL);			// �g�J�w�]��
	PIR1bits.TMR1IF = 0;			// �M�����_�X��

}
#pragma code
