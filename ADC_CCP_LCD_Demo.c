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

#define TMR1_VAL 	65536-3277		// Timer1 設定為 500ms 中斷一次

//宣告函式原型
void Init_TMR1(void);
void timer1_isr(void);

void Init_TMR1(void) {

	OpenTimer1(TIMER_INT_ON &		// 使用C18編譯器timer函式庫
			T1_16BIT_RW &		// 初始化設定Timer1
			T1_SOURCE_EXT &		// 並開啟TIMER1中斷功能(PIE1bits.TMR1IE=1)
			T1_PS_1_1 & T1_OSC1EN_ON & T1_SYNC_EXT_ON);
	WriteTimer1(TMR1_VAL);			// 寫入預設值
	PIR1bits.TMR1IF = 0;			// 清除中斷旗標

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

//宣告並安排中斷執行程式記憶體位置
#pragma code high_vector=0x08
void high_interrupt(void) {
	_asm GOTO
	timer1_isr _endasm
}
#pragma code

#pragma interrupt timer1_isr

void timer1_isr(void) {
	PIR1bits.TMR1IF = 0;	// 清除中斷旗標
	WriteTimer1(TMR1_VAL);	// 當將計數器觸發次數歸零寫入預設值

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

	Init_TMR1();			// 初始化設定Timer1函式

	INTCONbits.PEIE = 1;	// 開啟周邊中斷功能
	INTCONbits.GIE = 1;		// 開啟全域中斷控制

	ADC_Active_Timer = 0;

	ADCON0 = 0x01;					// 選擇AN0通道轉換，開啟ADC模組
	ADCON1 = 0x0E;					//0b00001110		// 使用VDD，VSS為參考電壓，設定AN0為類比輸入
	ADCON2 = 0xBA;					//0b10111010		// 結果向左靠齊並設定轉換時間為Fosc/32，採樣時間為20TAD



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
	 *		PWM週期(HI到HI)計算方式
	 *		PMW週期 	= (PR2 + 1) * 4 * Tosc * Timer2 Prescaler
	 *				= (PR2 + 1) * 4 * (1/4,000,000MHz) * 4
	 *				= (PR2 + 1) * 4 * 0.25(us) * 4
	 *
	 *		$希望 週期為 1024us$
	 *		1024 	= (PR2 + 1) * 4 * 0.25(us) * 4
	 *				PR2 = 1024/4/0.25/ 4 -1
	 *				    = 255
	 *
	 *		正脈衝寬度(時間)	= CCPRxL:CCPxCON<5:4> * Tosc * Timer2 Prescaler
	 *					= CCPRxL:CCPxCON<5:4> * 0.25 * 4
	 *
	 *		CCPRxL:CCPxCON<5:4>計算函數
	 *		void SetPWMDuty(unsigned long input)
	 *		{
	 *			CCPR1L = (input & 0x3FC) >> 2;
	 *			CCP1CON &= ~0b00110000;
	 *			CCP1CON |= (input & 0x03) << 4 ;
	 *		}
	 *
	 *		$希望脈衝寬度580us
	 *
	 *		Pwm_Width = CCPRxL:CCPxCON<5:4>
	 *
	 *		580us = Pwm_Width *0.25 * 4
	 *		Pwm_Width = 580us /0.25 /4
	 *		Pwm_Width = 580
	 *
	 *		設定方法：
	 *		呼叫   SetPWMDuty(580);  即可
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
			Delay10TCYx(5);	//時間延遲以完成採樣
			ConvertADC();	//進行訊號轉換
			while (BusyADC()) {
			};	//等待轉換完成

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
