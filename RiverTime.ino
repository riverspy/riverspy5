#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

ISR(WDT_vect)	// Watchdog Interrupt Service. This is executed when watchdog timed out.
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
  else
  {
    Serial.println(F("WDT Overrun!!!"));
  }
}



void Reboot(void)
{
	lcd.clear();
	lcd.print(F("-RIVERSPY.NET-"));
		
	lcd.setCursor(0,2);
	lcd.print(F("Reboot now..."));
	delay(2000);
	wdt_setup_1s_with_reset();
	delay(2000);					// this won't have time to finish
	lcd.print(F("still here :("));
}


void enterSleep(void)	// Enters the arduino into sleep mode.
{
  power_all_disable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}



void wdt_setup_1s_with_reset()
{
	/*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
    
  /* Enable the reset */
  WDTCSR |= 1<<WDE; 
  WDTCSR &= ~(1<<WDIE);
}

void wdt_setup_1s()
	{
		/*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
    
  /* Enable the WD interrupt (note no reset). */
  WDTCSR &= ~(_BV(WDE));
  WDTCSR |= _BV(WDIE);
  
}

void wdt_setup_8s_with_reset()
{
  /*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP3 | 1<<WDP0; /* 8.0 seconds */
  
  /* Enable the reset */
  WDTCSR |= 1<<WDE;
  WDTCSR &= ~(1<<WDIE);
 
}


long GetRealTime(int GID)											
{									
	byte Hour,Min,Sec,Day,Month;
	int Year;
	
	emptyRXbuffer();	
	Serial.println(F("\r\n"));		// wakey wakey
	Serial.println(F("AT+CCLK?"));
	wdt_reset();
	if (!Serial.find("+CCLK:"))  return -1;
	if ((Year = Serial.parseInt()) < 0)	return -1;
	if ((Month = Serial.parseInt()) < 0)	return -1;
	if ((Day = Serial.parseInt()) < 0)	return -1;
	if ((Hour = Serial.parseInt()) < 0)	return -1;
	if ((Min = Serial.parseInt()) < 0)	return -1;
	if ((Sec = Serial.parseInt()) < 0)	return -1;
		
	setTime(Hour,Min,Sec,Day,Month,Year);
	adjustTime(GID*3);		// add a custom offset of 3secs per gauge so that they all don't update together	

	return now();	
}

void UpdateScreenTime()
{
	lcd.setCursor(0,5);
	lcd.clearLine();
	sprintf(msg, "%02d/%02d %02d:%02d:%02d", day(), month(), hour(), minute(), second());
//	sprintf(msg, "%d %02d:%02d:%02d", TicksToGo, hour(), minute(), second());
	lcd.print(msg);	
}

void UpdateScreenData(int Level, int LastLevel)
{
	lcd.begin(84, 48);				// reset lcd every cycle
	lcd.print(F("-RIVERSPY.NET-"));
		
	lcd.setCursor(0,2);
	lcd.print(F("Level: "));
	lcd.print(Level);
	lcd.print(F("cm"));
	
	lcd.setCursor(0,3);
	lcd.print(F("Trend: "));
	if (Level>LastLevel)
	lcd.print('+');
	if (Level==LastLevel)
	lcd.print(F("Steady"));
	else
	{
		lcd.print(Level-LastLevel);
		lcd.print(F("cm"));
	}
	
	lcd.setCursor(0,4);
	if (mV != 0){				// do not display if we do not receive a credible reading, it will be zero
		lcd.print(F("VBatt: "));
		lcd.print(mV/1000);
		lcd.print(F("."));
		lcd.print((mV%1000)/100);
		lcd.print((mV%100)/10);
		lcd.print(F("V")); 		
	}  
}

void ReadServerTime()
{
	lcd.clear();
	lcd.print(F("-RIVERSPY.NET-"));
	lcd.setCursor(0,1);
	lcd.print(F("4: Get time at"));		// read the time from riverspy.net
	lcd.print(TIME_URL);
	lcd.print(getHTTPbody(TIME_URL, TIME_SERVER, msg, MSG_LEN, 2));		
	msg[20] = 0;	
	wdt_reset();				// the timestamp should only be 20 chars long
	emptyRXbuffer();
	Serial.print(F("AT+CCLK=\""));
	Serial.print(msg);
	Serial.println("\"");			// update the Real Time Clock (RTC) of the SIM900
	if (Serial.findUntil(OKstr, ERRstr))
	{
		lcd.print(F(" Good Time"));
	}
	else
	{
		lcd.print(F(" Bad Time"));
	};
	wdt_reset();
	delay(500);			
	lcd.clearLine();
	lcd.setCursor(0,4);
	lcd.clearLine();
	lcd.print(msg);					// write the time received to the LCD
	delay(500);
}
