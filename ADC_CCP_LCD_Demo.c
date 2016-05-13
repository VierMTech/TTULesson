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

#define TMR1_VAL 	65536-3277		// Timer1 �]�w�� 500ms ���_�@��

//�ŧi�禡�쫬
void Init_TMR1(void);
void timer1_isr(void);

void Init_TMR1(void) {

	OpenTimer1(TIMER_INT_ON &		// �ϥ�C18�sĶ��timer�禡�w
			T1_16BIT_RW &		// ��l�Ƴ]�wTimer1
			T1_SOURCE_EXT &		// �ö}��TIMER1���_�\��(PIE1bits.TMR1IE=1)
			T1_PS_1_1 & T1_OSC1EN_ON & T1_SYNC_EXT_ON);
	WriteTimer1(TMR1_VAL);			// �g�J�w�]��
	PIR1bits.TMR1IF = 0;			// �M�����_�X��

}

/*
 * init var
 */

unsigned int ADC_Active_Timer;
unsigned int ADC_VoltageFirst, ADC_VoltageLast;
unsigned int ADC_result;
char STR_Value[17];
float ADC_Voltage;

/*
 * PWM
 */
unsigned long PWM_Width = 0 ;

//�ŧi�æw�Ƥ��_����{���O�����m
#pragma code high_vector=0x08
void high_interrupt(void) {
	_asm GOTO
	timer1_isr _endasm
}
#pragma code

#pragma interrupt timer1_isr

void timer1_isr(void) {
	PIR1bits.TMR1IF = 0;	// �M�����_�X��
	WriteTimer1(TMR1_VAL);	// ��N�p�ƾ�Ĳ�o�����k�s�g�J�w�]��

	if (ADC_Active_Timer > 0) ADC_Active_Timer--;
}

#pragma code

void InitializePORT(void) {

	PORTA = 0x00;
	TRISA = 0b11011011;			// RA2 as LCD-E control, RA5 as CS Control  for SPI
	PORTD = 0x00;
	TRISD = 0x00;					// Set PORTD as Output port
}
void SetPWMDuty(unsigned long input)
{
	CCPR1L = (input & 0x3FC) >> 2;
	CCP1CON &= ~0b00110000;
	CCP1CON |= (input & 0x03) << 4 ;
}
void main(void) {

	PORTA = 0x00;
	TRISA = 0b11011011;	// RA2 as LCD-E control, RA5 as CS Control  for SPI
	PORTD = 0x00;
	TRISD = 0x00;		// Set PORTD as Output port

	Init_TMR1();			// ��l�Ƴ]�wTimer1�禡

	INTCONbits.PEIE = 1;	// �}�ҩP�䤤�_�\��
	INTCONbits.GIE = 1;		// �}�ҥ��줤�_����

	ADC_Active_Timer = 0;

	ADCON0 = 0x01;					// ���AN0�q�D�ഫ�A�}��ADC�Ҳ�
	ADCON1 = 0x0E;					//0b00001110		// �ϥ�VDD�AVSS���Ѧҹq���A�]�wAN0�������J
	ADCON2 = 0xBA;					//0b10111010		// ���G�V���a���ó]�w�ഫ�ɶ���Fosc/32�A�ļˮɶ���20TAD



	OpenLCD();

	LCD_Set_Cursor(0, 0);						// Put LCD Cursor on Line 1
	putrsLCD("                ");

	LCD_Set_Cursor(0, 0);						// Put LCD Cursor on Line 2
	putrsLCD("                ");

	//PWM
	/*
	 *
	 * 		T2CON => 6-3bit  0000 = 1:1 Postscale
	 * 				 2  bit  0    = Timer2 is on
	 * 				 1-9bit  01   = Prescaler is 4
	 *		CCP1CON => 	bit 5-4 PWM Duty Cycle bit , no use now.
	 *					bit 3-0 11xx = PWM mode
	 *
	 * 		Fosc = 16MHz = 16,000,000Hz
	 * 		Timer2 Clock Source = Fosc/4 = 4,000,000Hz
	 *
	 *		PWM�g��(HI��HI)�p��覡
	 *		PMW�g�� 	= (PR2 + 1) * 4 * Tosc * Timer2 Prescaler
	 *				= (PR2 + 1) * 4 * (1/4,000,000MHz) * 4
	 *				= (PR2 + 1) * 4 * 0.25(us) * 4
	 *
	 *		$�Ʊ� �g���� 1024us$
	 *		1024 	= (PR2 + 1) * 4 * 0.25(us) * 4
	 *				PR2 = 1024/4/0.25/ 4 -1
	 *				    = 255
	 *
	 *		���߽ļe��(�ɶ�)	= CCPRxL:CCPxCON<5:4> * Tosc * Timer2 Prescaler
	 *					= CCPRxL:CCPxCON<5:4> * 0.25 * 4
	 *
	 *		CCPRxL:CCPxCON<5:4>�p����
	 *		void SetPWMDuty(unsigned long input)
	 *		{
	 *			CCPR1L = (input & 0x3FC) >> 2;
	 *			CCP1CON &= ~0b00110000;
	 *			CCP1CON |= (input & 0x03) << 4 ;
	 *		}
	 *
	 *		$�Ʊ�߽ļe��580us
	 *
	 *		Pwm_Width = CCPRxL:CCPxCON<5:4>
	 *
	 *		580us = Pwm_Width *0.25 * 4
	 *		Pwm_Width = 580us /0.25 /4
	 *		Pwm_Width = 580
	 *
	 *		�]�w��k�G
	 *		�I�s   SetPWMDuty(580);  �Y�i
	 *
	 */
	T2CON = 0b00000101;
	CCP1CON = 0b00001100 ;
	TRISCbits.RC2 = 0;


	PR2 = 255; // ((PR2)+1)* 4 * 0.25us * 4 = 1024(us) = 1.024ms

	PWM_Width = 128 ;
	SetPWMDuty(PWM_Width) ;		//Pulse Width = PWM_Width * 0.25us * 4
								// 255us = <1020> * 0.0625 * 4


	while (1) {


		if (ADC_Active_Timer == 0) {
			ADC_Active_Timer = 1;

			ADC_result = 0;
			Delay10TCYx(5);	//�ɶ�����H�����ļ�
			ConvertADC();	//�i��T���ഫ
			while (BusyADC()) {
			};	//�����ഫ����

			ADC_result = ReadADC();

			ADC_Voltage = ADC_result * 4.55;
			ADC_Voltage /= 1024;

			//ADC_VoltageFirst,ADC_VoltageLast
			ADC_VoltageFirst = (unsigned int) floor(ADC_Voltage);
			ADC_VoltageLast = (ADC_Voltage - ADC_VoltageFirst) * 100;

			memset(STR_Value, 0, sizeof(STR_Value));
			sprintf(STR_Value, "ADC:%04d,V=%d.%d", ADC_result, ADC_VoltageFirst, ADC_VoltageLast);
			LCD_Set_Cursor(1, 0);						// Put LCD Cursor on Line 2
			putsLCD(STR_Value);


			memset(STR_Value, 0, sizeof(STR_Value));
			sprintf(STR_Value, "Hi Duty:%04d", ADC_result);
			LCD_Set_Cursor(0, 0);						// Put LCD Cursor on Line 2
			putsLCD(STR_Value);

			SetPWMDuty(ADC_result);

		}

	};

}
#pragma code
