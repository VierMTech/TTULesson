//**********************************************************
//*               Ex9_1_TMR1_INT.c
//*	設計一個0.5秒讓 PORTD 的 LED 所顯示的二進位數字自動加一的程式
//**********************************************************
#include 	<p18f4520.h> 	//納入微控制器定義檔
#include 	<timers.h> 		//納入計時器函式庫定義檔
#include 	<adc.h>
#include 	<stdlib.h>
#include	<stdio.h>
#include	<math.h>
// 結構位元定義
#pragma config	OSC=HS, BOREN=OFF, BORV = 2, PWRT=ON, WDT=OFF, LVP=OFF

#define TMR1_VAL 	65536-328		// Timer1 設定為 100ms 中斷一次
#define PORTA4ButtonPress() 	!((PORTA & 0x10) == 0x10)
#define PORTA4ButtonRelease() 	((PORTA & 0x10) == 0x10)

//宣告函式原型
void Init_TMR1(void);
void timer1_isr(void);
void MyTimerFunction(void);
//變數宣告
unsigned int CounterA = 0;

unsigned int ButtonA4Counter = 0;
unsigned char ButtonA4Flag = 0;
unsigned int ButtonA4Event = 0;

unsigned int SystemStep = 0;

#pragma code

void MyTimerFunction(void)
{
	if (ButtonA4Counter > 0) ButtonA4Counter--;
	if (CounterA > 0) CounterA--;

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
unsigned char PORTINCStep = 0;
unsigned char PORTINC_index = 0;
unsigned char PORTINC_Times = 0;
unsigned char PORTINC_TotalTimes = 7;
unsigned char PORD_Shift = 0;
unsigned int PORD_Buff = 0;

unsigned char PORTD_INV_Buff = 0;
unsigned char Loopi = 0;
unsigned char BitRev(unsigned char c)
{
	unsigned char s = 0;
	int i;
	for (i = 0; i < 8; ++i)
	{
		s <<= 1;
		s |= c & 1;
		c >>= 1;
	}
	return s;
}
void PORT_SHIFT_Init(void)
{
	PORTINCStep = 0;
}
char PORT_SHIFT(char Direction)
{

	if (CounterA == 0)
	{
		CounterA = 10;
		switch (PORTINCStep)
		{
		case 0: //Lock
			PORTINCStep++;
			break;
		case 1:
			PORTINC_TotalTimes = 7;
			PORTINC_Times = 7;
			PORTD_INV_Buff = 0x01;
			PORD_Buff = 0;
			PORD_Shift = 0x01;
			PORTINCStep++;
			break;
		case 2:
			if (PORTINC_Times != 0)
			{
				PORTINC_Times--;
				PORD_Shift = PORD_Shift << 1;
				PORTD_INV_Buff = PORD_Shift | PORD_Buff;
			}
			else	//跑完一次
			{
				if (PORTINC_TotalTimes > 0)
				{
					PORTINC_TotalTimes--;
				}
				if (PORTINC_TotalTimes == 0)
				{
					PORTINCStep++;
				}
				PORD_Buff = PORTD_INV_Buff;
				PORD_Shift = 0x01;
				PORTINC_Times = PORTINC_TotalTimes;
			}
			break;
		case 3:
			PORTD_INV_Buff |= 0x01;
			PORTINCStep = 100;
			break;
		case 100:
			//完成LOCK
			break;

		}

		if (Direction == 0)
		{
			PORTD = PORTD_INV_Buff;
		}
		else
		{
			PORTD = BitRev(PORTD_INV_Buff);
		}
	}
	return PORTINCStep;
}
void main(void)
{
	Init_TMR1();			// 初始化設定Timer1函式
	INTCONbits.PEIE = 1;	// 開啟周邊中斷功能
	INTCONbits.GIE = 1;		// 開啟全域中斷控制

	TRISD = 0;
	PORTD = 0;
	TRISA |= 0x10;

	PORT_SHIFT_Init();
	while (1)
	{
		ButtonProcess();


		switch (SystemStep)
		{
		case 0:
			if (PORT_SHIFT(0) == 100)
			{
				SystemStep++;
				ButtonA4Event = 0;
			}
			break;
		case 1:
			if (ButtonA4Event == 1)
			{
				ButtonA4Event = 0;
				PORT_SHIFT_Init();
				SystemStep++;

			}
			break;
		case 2:
			if (PORT_SHIFT(1) == 100)
			{
				SystemStep++;
				ButtonA4Event = 0;
			}
			break;
		case 3:
			if (ButtonA4Event == 1)
			{
				ButtonA4Event = 0;
				PORT_SHIFT_Init();
				SystemStep++;
			}
			break;
		default:
			SystemStep = 0 ;
			break;
		}

	};

}

//宣告並安排中斷執行程式記憶體位置
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

	PIR1bits.TMR1IF = 0;	// 清除中斷旗標
	WriteTimer1(TMR1_VAL);	// 當將計數器觸發次數歸零寫入預設值

	MyTimerFunction();

}
void Init_TMR1(void)
{

	OpenTimer1(TIMER_INT_ON &		// 使用C18編譯器timer函式庫
			T1_16BIT_RW &		// 初始化設定Timer1
			T1_SOURCE_EXT &		// 並開啟TIMER1中斷功能(PIE1bits.TMR1IE=1)
			T1_PS_1_1 & T1_OSC1EN_ON & T1_SYNC_EXT_ON);
	WriteTimer1(TMR1_VAL);			// 寫入預設值
	PIR1bits.TMR1IF = 0;			// 清除中斷旗標

}
#pragma code
