/*
 * File:   main.c
 * Author: Ferdinand Thomas
 *
 * Created on December 5, 2018, 11:19 PM
 */


#ifndef F_CPU
#define F_CPU 8000000 // 16 MHz clock speed
#endif

#define D4 eS_PORTB3
#define D5 eS_PORTB2
#define D6 eS_PORTB1
#define D7 eS_PORTB0
#define RS eS_PORTB5
#define EN eS_PORTB4

#define tstarthraddr 0x00
#define tstartminaddr 0x01
#define tstophraddr 0x02
#define tstopminaddr 0x03


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "i2cmaster.h"
#include <stdbool.h>

  int sec;
  int min;
  int hour;
  int day;
  int daynum;
  int month;
  int year;
  char time[]="00:00:00";
  char date[]="00-00-2000";
  char timerbeg[]="00:00";
  char timerend[]="00:00";
  
  int timerstarthr=0;
  int timerstartmin=0;
  int timerstophr=0;
  int timerstopmin=0;
  int outval=0;
  
void EEPROM_write(unsigned int ucAddress, unsigned char ucData)
{
while(EECR & (1<<EEPE));
EECR = (0<<EEPM1)|(0<<EEPM0);
EEARL = ucAddress;
EEDR = ucData;
EECR |= (1<<EEMPE);
EECR |= (1<<EEPE);
}

unsigned char EEPROM_read(unsigned int ucAddress)
{
while(EECR & (1<<EEPE));
EEARL = ucAddress;
EECR |= (1<<EERE);
return EEDR;
}
  
int  BCD_2_DEC(int to_convert)
{
   return (to_convert >> 4) * 10 + (to_convert & 0x0F);
}

int DEC_2_BCD (int to_convert)
{
   return ((to_convert / 10) << 4) + (to_convert % 10);
}

void Set_Rtc()
{
  i2c_start_wait(0xD0);
  i2c_write(0x00);
  i2c_write(DEC_2_BCD(sec));
  i2c_write(DEC_2_BCD(min));
  i2c_write(DEC_2_BCD(hour));
  i2c_write(DEC_2_BCD(day));
  i2c_write(DEC_2_BCD(daynum)&~(1<<6));
  i2c_write(DEC_2_BCD(month));
  i2c_write(DEC_2_BCD(year));
  i2c_stop();
}

void Update_From_Rtc()
{
    i2c_start_wait(0xD0);
    i2c_write(0x00);
    i2c_rep_start(0XD1);
    sec= BCD_2_DEC(i2c_readAck());
    min= BCD_2_DEC(i2c_readAck());
    hour=BCD_2_DEC(i2c_readAck());
    day= BCD_2_DEC(i2c_readAck());
    daynum= BCD_2_DEC(i2c_readAck());
    month= BCD_2_DEC(i2c_readAck());
    year= BCD_2_DEC(i2c_readNak());
    i2c_stop();
    time[7]=(char)(sec%10+'0');
    time[6]=(char)(sec/10+'0');
    time[4]=(char)(min%10+'0');
    time[3]=(char)(min/10+'0');
    time[1]=(char)(hour%10+'0');
    time[0]=(char)(hour/10+'0');
    date[1]=(char)(daynum%10+'0');
    date[0]=(char)(daynum/10+'0');
    date[4]=(char)(month%10+'0');
    date[3]=(char)(month/10+'0');   
    date[9]=(char)(year%10+'0');
    date[8]=(char)(year/10+'0');  
}

void Lcd_Main_Screen()
{
    Lcd4_Set_Cursor(1,3);
    Lcd4_Write_String(date);
    Lcd4_Set_Cursor(2,4);
    Lcd4_Write_String(time);
}

void Lcd_Timerset1()
{
    int pos=14;
    Lcd4_Clear();
    Lcd4_Set_Cursor(1,0);
    Lcd4_Write_String("Timer");
    Lcd4_Set_Cursor(1,11);
    Lcd4_Write_String("hh:mm");
    Lcd4_Set_Cursor(2,0);
    Lcd4_Write_String("Start:");
    while((PIND&(1<<PD1))==0)
    {
        Lcd4_Set_Cursor(2,pos);
        Lcd4_Write_String("  ");
        _delay_ms(5);
        Lcd4_Set_Cursor(2,11);
        Lcd4_Write_Char((char)timerstarthr/10+48);
        Lcd4_Write_Char((char)timerstarthr%10+48);
        Lcd4_Write_Char(':');
        Lcd4_Write_Char((char)timerstartmin/10+48);
        Lcd4_Write_Char((char)timerstartmin%10+48);
        _delay_ms(5);
        if((PIND&(1<<PD4))!=0)
        {
            pos==14?(timerstartmin=(timerstartmin+1)%60):(timerstarthr=(timerstarthr+1)%24);
            _delay_ms(10);
        }
        if((PIND&(1<<PD0))!=0)pos==14?(pos=11):(pos=14);
    }
    timerbeg[0]=(char)timerstarthr/10+48;
    timerbeg[1]=(char)timerstarthr%10+48;
    timerbeg[3]=(char)timerstartmin/10+48;
    timerbeg[4]=(char)timerstartmin%10+48;   
    EEPROM_write(tstarthraddr,(char)timerstarthr);
    EEPROM_write(tstartminaddr,(char)timerstartmin);
}

void Lcd_Timerset2()
{
    int pos=14;
    Lcd4_Clear();
    Lcd4_Set_Cursor(1,0);
    Lcd4_Write_String("Timer");
    Lcd4_Set_Cursor(1,11);
    Lcd4_Write_String("hh:mm");
    Lcd4_Set_Cursor(2,0);
    Lcd4_Write_String("Stop:");
    while((PIND&(1<<1))==0)
    {
        Lcd4_Set_Cursor(2,pos);
        Lcd4_Write_String("  ");
        _delay_ms(5);
        Lcd4_Set_Cursor(2,11);
        Lcd4_Write_Char((char)timerstophr/10+48);
        Lcd4_Write_Char((char)timerstophr%10+48);
        Lcd4_Write_Char(':');
        Lcd4_Write_Char((char)timerstopmin/10+48);
        Lcd4_Write_Char((char)timerstopmin%10+48);
        _delay_ms(5);
        if((PIND&(1<<PD4))!=0)
        {
            pos==14?(timerstopmin=(timerstopmin+1)%60):(timerstophr=(timerstophr+1)%24);
            _delay_ms(10);
        }
        if((PIND&(1<<PD0))!=0)pos==14?(pos=11):(pos=14);
    }
    timerend[0]=(char)timerstophr/10+48;
    timerend[1]=(char)timerstophr%10+48;
    timerend[3]=(char)timerstopmin/10+48;
    timerend[4]=(char)timerstopmin%10+48; 
    EEPROM_write(tstophraddr,(char)timerstophr);
    EEPROM_write(tstopminaddr,(char)timerstopmin);
}

void Timeset()
{
    int pos=14;
    Lcd4_Clear();
    Lcd4_Set_Cursor(1,0);
    Lcd4_Write_String("Time Adjust");
    while((PIND&(1<<1))==0)
    {
        Update_From_Rtc();
        Lcd4_Set_Cursor(2,pos);
        Lcd4_Write_String("  ");
        _delay_ms(1);
        Lcd4_Set_Cursor(2,8);
        Lcd4_Write_Char((char)hour/10+48);
        Lcd4_Write_Char((char)hour%10+48);
        Lcd4_Write_Char(':');
        Lcd4_Write_Char((char)min/10+48);
        Lcd4_Write_Char((char)min%10+48);        
        Lcd4_Write_Char(':');
        Lcd4_Write_Char((char)sec/10+48);
        Lcd4_Write_Char((char)sec%10+48);
        _delay_ms(1);
        if((PIND&(1<<PD4))!=0)
        {
            if(pos==14)
            {
                sec=0;
                Set_Rtc();
            }
            else if(pos==11)
            {
                min=(min+1)%60;
                Set_Rtc();
            }
            else
            {
                hour=(hour+1)%24;
                Set_Rtc();
            }
        }
        if((PIND&(1<<PD0))!=0)pos==14?(pos=11):(pos==11?(pos=8):(pos=14));
    }
    Update_From_Rtc();
}

int calmaxdays(int month)
{
	if( month == 2)
	{
		if(year%4==0)	
			return 29;
		else	
			return 28;
	}
	//months which has 31 days
	else if(month == 1 || month == 3 || month == 5 || month == 7 || month == 8
	||month == 10 || month==12)	
		return 31;
	else 		
		return 30;
}

void Dateset()
{
    int pos=12;
    Lcd4_Clear();
    Lcd4_Set_Cursor(1,0);
    Lcd4_Write_String("Date Adjust");
    while((PIND&(1<<1))==0)
    {
        if(pos==12)
        {
            Lcd4_Set_Cursor(2,pos);
            Lcd4_Write_String("    ");
        }
        else
        {
            Lcd4_Set_Cursor(2,pos);
            Lcd4_Write_String("  ");    
        }
        _delay_ms(1);
        Lcd4_Set_Cursor(2,6);
        Lcd4_Write_Char((char)daynum/10+48);
        Lcd4_Write_Char((char)daynum%10+48);
        Lcd4_Write_Char('-');
        Lcd4_Write_Char((char)month/10+48);
        Lcd4_Write_Char((char)month%10+48);
        Lcd4_Write_Char('-');
        Lcd4_Write_Char('2');
        Lcd4_Write_Char('0');
        Lcd4_Write_Char((char)year/10+48);
        Lcd4_Write_Char((char)year%10+48);
        _delay_ms(1);
        if((PIND&(1<<PD4))!=0)
        {
            pos==12?(year=(year+1)%100):(pos==9?(month=(month%12)+1):(daynum=(daynum%calmaxdays(month))+1));
            _delay_ms(10);
        }
        if((PIND&(1<<PD0))!=0)pos==12?(pos=9):((pos==9)?(pos=6):(pos=12));
    }
    Set_Rtc();
    Update_From_Rtc();
}

void Menu()
{
    Lcd_Timerset1();
    Lcd_Timerset2();
    Timeset();
    Dateset();
    Lcd4_Clear();
}

void checktimer()
{
    if(timerstarthr<=timerstophr)
    {
    if(outval==1 && timerstophr<=hour)
    {    
        if(timerstopmin<=min)
        {
            outval=0;
            PORTD=PORTD & ~(1<<PD5);
        }
    }
    else  if(outval==0 && timerstarthr<=hour && timerstophr>=hour)
    {
        if(timerstartmin<=min && timerstopmin>min)
        {
            outval=1;
            PORTD=PORTD | (1<<PD5);
        }
    }
    }
   
    
}

ISR(INT0_vect)
{
    SMCR=0x00;
    cli();
    Update_From_Rtc();
    Lcd_Main_Screen();
    checktimer();
    sei();
}

ISR(INT1_vect)
{
    SMCR=0x00;
    cli();
    Menu();
    Lcd4_Clear();
    sei();
}

int main(void)
{
  DDRB = 0xFF;
  DDRD = 0xE0;
  Lcd4_Init();
  i2c_init();
  
  i2c_start_wait(0xD0);
  i2c_write(0x0E);
  i2c_write(0x00);
  i2c_stop();
  
  sei();
  EICRA=0x0E;
  EIMSK=0x03;
  PRR=0b00101101;
  
  Update_From_Rtc();
  Lcd_Main_Screen();
  timerstarthr=EEPROM_read(0x00);
  timerstartmin=EEPROM_read(0x01);
  timerstophr=EEPROM_read(0x02);
  timerstopmin=EEPROM_read(0x03);
  while(1)
  {
      SMCR=0x05;
  }
}