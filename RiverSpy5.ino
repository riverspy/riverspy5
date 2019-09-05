// successfully uploaded with Arduino 1.8.9
// using an arduino Fio as 'Arduino as ISP' (Burn ArduinoISP sketch onto it, add Capacitor)
// Gboard 800 is 3.3V so use a 3.3V Arduino as Programmer


#include <TFMini.h> #add through Library Manager, TFMini by Peter Jansen, 0.1.0
#include <TimeLib.h> 
#include <PCD8544.h> #add through Library Manager, Carlos Rodrigues, 1.4.3
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "RiverSpy.h"
#include <SoftwareSerial.h>


int mV=12000, PassCode, Temperature, ret, validReads;
char n,v;
int Levels[LEVELARRAY];
char LatestUpdateSlot, Slot, StartMonth;
int Heartbeat, GaugeID, TriggerLevel, AlertLevel, UpdateInterval;
boolean LeavePhoneOn, AlertActive;
int Offset, Readings, Scale1000;
int dist, strength;
int TicksToGo, DebugLevel;

char msg[MSG_LEN+1], Ch;
// char Sender[VIPlength];
char SMScmd[20], OKstr[5]="OK\r\n", ERRstr[6]="ERROR", CRLF[3]="\r\n";
boolean CmdWaiting, ReTry;
volatile int f_wdt;	// watchdog timer flag
long nextSlot, turnOnDelay, readingStart, Total;
float ScaleFactor, timeFactor;

uint8_t PhoneOK=0, GprsOn=0, TCPopen=0;

// A custom glyph (a smiley)...
static const byte glyph[] = { B00010000, B00110100, B00110000, B00110100, B00010000 };
// static PCD8544 lcd(13,11,8,9,12);   // SCLK:D13, SDIN:D11, D/C:D8, RST:D9, SCE: D12 GBoard900
static PCD8544 lcd(13,11,12,11,8);   // SCLK:D13, SDIN:D11, D/C:D12, RST:RST, SCE: D8

// Setup software serial port
SoftwareSerial mySerial(A2, A3);      // Uno RX (TFMINI TX), Uno TX (TFMINI RX)
TFMini tfmini;

void setup(){
	wdt_enable(WDTO_8S);
		
	lcd.begin(84, 48, CHIP_ST7576);
	lcd.print(F("-RIVERSPY.NET-"));
	lcd.print(F("    v5.01     "));
	lcd.print(F("     by       "));
	lcd.print(F(" Daithi Power "));
	lcd.print(F("   Aug. 2019  "));
	delay (1000);
	lcd.setContrast(Default_Contrast);
		
	
	f_wdt=1;	// watchdog timer flag		
	timeFactor = 1.0;
	CmdWaiting = false;
	ReTry = true;
	AlertActive = true;
//	LatestUpdateSlot = -1;    // this will cause lots of updates at boot time
	MCUSR = 0;							// clear any reset flags
	pinMode(SENSOR_ON, OUTPUT);
	digitalWrite(SENSOR_ON, HIGH);		// a low signal cuts the 5V to the sensor
//	ADCSRA = ADCSRA | bit (ADEN);		// enable ADC	    

	pinMode(PRESSUP, INPUT_PULLUP);		// configure pushbutton signals as inputs with pull ups enabled
	pinMode(PRESSDOWN, INPUT_PULLUP);
	pinMode(PRESSBACK, INPUT_PULLUP);
	pinMode(PRESSENTER, INPUT_PULLUP);
	digitalWrite(PRESSUP, HIGH);		// turn on the pull ups
	digitalWrite(PRESSDOWN, HIGH);		// turn on the pull ups
	digitalWrite(PRESSBACK, HIGH);		// turn on the pull ups
	digitalWrite(PRESSENTER, HIGH);		// turn on the pull ups
	
	digitalWrite(GSM_ON,HIGH);			// cycle the power on pin
	delay (1500);
	digitalWrite(GSM_ON,LOW);
	wdt_reset();  // watchdog reset

	PassCode = eeprom_read_word(E_PASSCODE);
	if (PassCode==-1)
	{
		lcd.clear();		
		lcd.print(F("Reset EEPROM>"));
		SetDefaults();
		delay(2000);
		wdt_reset();
		lcd.clearLine();
	}
	else
	{
		GaugeID = eeprom_read_word((uint16_t*)E_GAUGE_ID);
		Heartbeat = eeprom_read_word((uint16_t*)E_HEARTBEAT);
		Offset = eeprom_read_word((uint16_t*)E_OFFSET);
		Readings = eeprom_read_word((uint16_t*)E_READS);
		Scale1000 = eeprom_read_word((uint16_t*)E_SCALE1000);
		ScaleFactor = 1.0 * Scale1000 / 1000.0;
		TriggerLevel = eeprom_read_word((uint16_t*)E_TRIGGERLEVEL);		
		AlertLevel = eeprom_read_word((uint16_t*)E_ALERTLEVEL);
		UpdateInterval = eeprom_read_word((uint16_t*)E_UPINT);
		DebugLevel = eeprom_read_word((uint16_t*)E_DEBUGLEVEL);
	};

	lcd.clear();
	lcd.print(F("-RIVERSPY.NET-"));	
	lcd.print(F("Gauge: "));
	lcd.print(GaugeID);
	lcd.setCursor(0,2);
	lcd.print(F("HeartBt: "));
	lcd.print(Heartbeat);
	lcd.setCursor(0,3);
	lcd.print(F("Offset: "));
	lcd.print(Offset);	
	lcd.setCursor(0,4);
	lcd.print(F("Scale: "));
	lcd.print(Scale1000);
	lcd.setCursor(0,5);			// show 
	lcd.print(F("Tr:"));
	lcd.print(TriggerLevel);
	lcd.print(F(" Al:"));
	lcd.print(AlertLevel);	

	delay(2200);
	wdt_reset();
	digitalWrite(GSM_RESET,HIGH);			// cycle the reset pin
	delay (500);
	digitalWrite(GSM_RESET,LOW);

	/*   Don't bother with the admin function for now. Its purpose is to send texts to up to 10 numbers if the level exceeds alertlevel
	n=0;
	while (n<= NumVIPs-5)	
	{
		wdt_reset();
		for (v=0; v<5; v++)
		{
			lcd.setCursor(0,v+1);
			lcd.clearLine();
			lcd.print(n+v);
			lcd.print(':');
			ReadVIP(n+v,Sender);
			lcd.print(Sender);
		}
		n=n+5;
		delay(2000);
	}  */

	// Initialize the data rate for the SoftwareSerial port
	mySerial.begin(TFMINI_BAUDRATE);
	// Initialize the TF Mini sensor
	tfmini.begin(&mySerial);
	
	lcd.clear();
	lcd.print(F("-RIVERSPY.NET-"));
	lcd.setCursor(0,2);
	lcd.print(F("Press and hold ENTER for setup mode"));
	delay(1500);
	wdt_reset();
	lcd.clear();	
	

	// Take one TF Mini distance measurement
	dist = tfmini.getDistance();
	strength = tfmini.getRecentSignalStrength();
			  
	if (EnterPress())  // Press enter button here to enter setup mode
 	{
		lcd.println(F("SETUP"));
		lcd.setCursor(0,1);
		lcd.println(F("Back = exit")); 
		lcd.setCursor(0,2);
		lcd.println(F("Dist"));
		lcd.setCursor(0,3);
		lcd.println(F("SigStr"));  		
		lcd.setCursor(0,4);
		lcd.println(F("Offset")); 		
		lcd.setCursor(0,5);
		lcd.println(F("Level"));  
		
		while(!BackPress())
		{
			// Display the measurement
			lcd.setCursor(48,2);
			lcd.print(dist);
			lcd.print(" ");
			lcd.setCursor(48,3);
			lcd.print(strength);
			lcd.print(" ");
			lcd.setCursor(48,4);
			lcd.print(Offset);
			lcd.print(" ");
			lcd.setCursor(48,5);
			lcd.print(Offset - int(dist * ScaleFactor));
			lcd.print(" ");

			// Wait some short time before taking the next measurement
			delay(500);
			wdt_reset();
      
      
		  if (UpPress())  // Increase Offset
		  {
			while(UpPress())
			{
			  Offset = Offset + 1;
			  lcd.setCursor(48,4);
			  lcd.print(Offset);
			  lcd.print(" ");
			  delay(100);
			  wdt_reset();;
			}
		  }
		  if (DownPress())  // Decrease Offset
		  {
			while(DownPress())
			{
			  Offset = Offset - 1;
			  lcd.setCursor(48,4);
			  lcd.print(Offset);
			  lcd.print(" ");
			  delay(100);
			  wdt_reset();
			}
		  }
     
			// Take one TF Mini distance measurement
			dist = tfmini.getDistance();
			strength = tfmini.getRecentSignalStrength();
		}
		eeprom_write_word((uint16_t*)E_OFFSET, Offset);
	 }
	 
	digitalWrite(SENSOR_ON, LOW);			// turn off the 5V supply to the underwater/LIDAR sensor
	pinMode(A3,INPUT);	// make sure no power is being drawn by the LIDAR through the serial TX
		  
	lcd.clear();
	lcd.print(F("-RIVERSPY.NET-"));
					
	PhoneOK = !PhoneOn();
	delay(1000);
	wdt_reset();
	if (PhoneOK){
		ret = wait_for_reg(SIM_PIN);
		lcd.setCursor(0,1);
		lcd.clearLine();
		if (ret){
			lcd.print(F("1: Check SIM"));	
			PhoneOK = 0;
		}
		else
			lcd.print(F("1: Phone Ready"));
	};
	wdt_reset();
	delay(3000);	

	wdt_reset();
	if (PhoneOK){		 
		lcd.setCursor(0, 2);	
		lcd.print(F("2: Test GPRS"));		// open a gprs connection to the APN
		lcd.setCursor(0, 3);			
		lcd.print(F(myAPN));
		n = openGPRS(msg);
		GprsOn = (n == 0);
		lcd.clearLine();
		if (GprsOn)
			lcd.print(F("->OK"));
		else
		{
			lcd.print(F("->error "));
			lcd.print(n);
		}
		delay(2000);			// wait 2 seconds to view the result on the screen
	}
	
	wdt_reset();
	if (GprsOn)
	{
		lcd.clear();	
		lcd.print(F("-RIVERSPY.NET-"));
		lcd.setCursor(0,1);			
		lcd.print(F("IP"));				// print IP address
		lcd.print(msg);
		
		lcd.setCursor(0, 2);
		lcd.print(F("3:TCP to "));
		lcd.print(F(myProxy));
		lcd.print(F(":"));
		lcd.print(myProxyPort);
		if (openTCPproxy(msg))	// test TCP connection to server
			lcd.print(F(" bad start"));
		else
			lcd.print(msg);
		wdt_reset();
		delay(2000);
		wdt_reset();
	
		ReadServerTime();	// get the time from the web server and set the RTC in the SIM900
		if (closeTCP())
			lcd.print(F("TCP_CLOSE_ERROR"));
		if (closeGPRS())
			lcd.print(F("GPRS_CLOSE_ERROR"));

		GprsOn = 0;
		
		delay(100);
		wdt_reset();
	}
	
	mV = ReadVoltage();
	if (bitRead(DebugLevel, DebugBit_SMS))
	{	
		sprintf(msg,"Station %05d is %dcm Batt %dmV CP %d",GaugeID,(int)(Offset - ScaleFactor * Total), mV, eeprom_read_word((uint16_t*)E_CHECKPOINT));
		// send me a text when booting up. If this is a reboot, CheckPoint helps to pinpoint where the code timed out
		SendSMS(AdminPhone,msg);
	}
	
//	LeavePhoneOn = true;		// stay powered up for the first cycle
	CmdWaiting = false;			// no SMS commands to process yet
	wdt_reset();
/*	if (PhoneOK)
		NewSMScmd(); */
				
	GetRealTime(GaugeID);				// read the time from the SIM900	
	StartMonth = month();
	PhoneOff();
	
	Slot = SetSlot();	
	LatestUpdateSlot = Slot-2;			// send the last 2 readings on the first update
	if (LatestUpdateSlot<0) LatestUpdateSlot = LatestUpdateSlot + LEVELARRAY;
	lcd.setCursor(0,0);
	lcd.print(F("Slot:"));
	lcd.print((int)Slot);
	Levels[Slot] = Offset - int(dist * ScaleFactor);		// scale to vertical cm and subtract from height of gauge above datum
//	Levels[Slot] = freeRAM();								// useful for debugging

	wdt_reset(); 
	} 

void loop()
{	
	UpdateScreenData(Levels[Slot], LastLevel(Slot));
		
//	if (LatestUpdateSlot==-1)			// just to speed up debugging
//		TicksToGo=25;
//	else
		TicksToGo = (int)(timeFactor * (nextSlot - now()));	// estimate number of sleep cycles to next update
		
	Total = 0;
	wdt_setup_1s();								// configure the watchdog timer for 1s sleep followed by wake to interrupt	
	f_wdt=0;
	TrackCP(CP_1S_LOOP);
	digitalWrite(SENSOR_ON,LOW);
	
	while (TicksToGo > 0)					// enter a one second sleep-loop cycle until nextSlot
	{
		/* Enter sleep mode. */
		enterSleep();	
			
		if(f_wdt == 1)		// watchdog woke us up
		{
			adjustTime(1);	// add a second to the clock
			UpdateScreenTime();	
															
			/* Don't forget to clear the flag. */
			f_wdt = 0;		
			TicksToGo--;  
		}
		else					// some other interrupt woke us up
		{
			/* Do nothing. */
		}
	}
	turnOnDelay = now();				// we need to know the time difference between wake-up and setting the clock later
	wdt_setup_8s_with_reset();	// a wdt timeout now would cause a reboot
	
	//				ADCSRA = ADCSRA | bit (ADEN);		// enable ADC
	digitalWrite(SENSOR_ON, HIGH);		// turn on the 5V supply to the underwater/LIDAR sensorr
	pinMode(A3,OUTPUT);	// enable the softserial TX line
	tfmini.begin(&mySerial);
	delay(100);	
	
	while (mySerial.available()>0)
	{
		Ch = mySerial.read();	// clear the serial port buffer
	}
	lcd.setCursor(0,4);
	lcd.clearLine();
	lcd.print(F(" reading level"));
	lcd.setCursor(0,5);
	
	readingStart = now();
	Total = 0;		// accumulator total of valid reading values
	validReads = 0;			// number of valid readings
	TrackCP(CP_READING_LEVEL);
	
	while((now()-readingStart) < Readings)// keep reading continuously for Readings seconds
	{
		dist = tfmini.getDistance();	
		if (dist>0)							// ignore any returns of -1
		{
			Total = Total + dist;
			validReads++;
			lcd.clearLine();
			lcd.print(validReads);
			lcd.print(F("  "));		// update the last line of the display with the number of valid readings and the current reading
			lcd.print(dist);
			delay(50);
		}	
		wdt_reset();	
	}
	if (validReads != 0) 
		Total = Total / validReads;		// use the average value of the readings

	digitalWrite(SENSOR_ON, LOW);			// turn off the 5V supply to the underwater/LIDAR sensor
	pinMode(A3,INPUT);	// make sure no power is being drawn by the LIDAR through the serial TX
	wdt_enable(WDTO_8S);			// reboot if inactive for 8 seconds	
	TrackCP(CP_SET_TO_8S);
	lcd.print(F("cm"));
	delay(1000);		// allow user to read the screen at the end of the read phase

	Slot = SetSlot();	// set the time for the next slot and update the variable Slot
	if (validReads != 0) 	
		Levels[Slot] = Offset - (int)(ScaleFactor * Total);		// scale to cm and add any offset due to sensor being above the 0cm level of the river	
	else
		Levels[Slot] = 0;
	
	if (LatestUpdateSlot == Slot)			// check if the circular buffer is full
		LatestUpdateSlot++;
	if (LatestUpdateSlot == LEVELARRAY)
		LatestUpdateSlot = 0;

	if (bitRead(DebugLevel, DebugBit_freeRAM))
	{	
		Levels[Slot] = freeRAM();	// useful for debugging a memory leak, log the free memory instead of the water level
	};
	
	UpdateScreenData(Levels[Slot], LastLevel(Slot));
	TrackCP(CP_TURN_ON_PHONE);	
			
	if(PhoneOK = !PhoneOn())
	{		
		turnOnDelay = now() - turnOnDelay;			// update the time from the real time clock on the SIM900
		wdt_reset();
		TrackCP(CP_PHONE_IS_ON);
		if (GetRealTime(GaugeID) > 0)
		{						// if we get a valid time, update the time correction factor
			timeFactor = timeFactor * (nextSlot - (now()-turnOnDelay)) / Heartbeat;
			if (timeFactor<0.8)
				timeFactor=0.8;		// put some reasonable bounds on this
			if (timeFactor>1.1)
				timeFactor=1.1;		
			UpdateScreenTime();
		}
		else
		{
			wdt_reset();
			lcd.print(F("Bad time"));	
			delay(2000);
		}
		wdt_reset();
		   
		Temperature = ReadTemp();
		mV = ReadVoltage();
		wdt_reset();
		TrackCP(CP_PIN_IS_NEXT);
				
		if (Slot%UpdateInterval==0 || ReTry || Levels[Slot]>=TriggerLevel)
		{
			if (wait_for_reg(SIM_PIN))
				ReTry = true;
			else
			{
				lcd.setCursor(0,1);
				lcd.clearLine();
				
				lcd.print(F("Phone is Ready"));
				wdt_reset();
				delay(5000);
				wdt_reset();
				TrackCP(CP_PIN_IS_GOOD);
/*				LeavePhoneOn = (NewSMScmd() != -1);		// if we get an sms cmd, process it and then leave the modem on for the next cycle
														// -1 implies that there was no SMS command waiting to be processed
				if (Slot%48 == 0)		// leave it on for a slot every 12 hours to make sure that texts get through
					LeavePhoneOn = true;
					
				wdt_reset();
				delay(1000); */
				TrackCP(CP_SEND_UPDATE);
				ReTry = SendLevelUpdate();
				delay(500);
			
/*				if (Levels[Slot] < (AlertLevel-5))
				AlertActive = true;			// reset the TriggerActive flag once the river drops
				if (Levels[Slot] >= AlertLevel && AlertActive)
				{
					AlertActive = false;
					sprintf(msg, "Gauge %d is %dcm, up %dcm in %dmin", GaugeID, Levels[Slot], (Levels[Slot]-LastLevel(Slot)),Heartbeat/60);
					for (v=0; v<NumVIPs; v++)
					{
						TrackCP(CP_SEND_TO_VIP + v);
						wdt_reset();
						ReadVIP(v,Sender);
						if (Sender[0] != 0)			// it contains a phone number
							SendSMS(Sender, msg);
					}
				}	*/
			};
			if ((mV<LOW_POWER_mV) && (UpdateInterval<12) && !bitRead(DebugLevel, DebugBit_24hrON))
			{
				eeprom_write_word((uint16_t*)E_UPINT, 12);		// go to once every 3 hours if battery is low
				TrackCP(CP_BATT_REBOOT);
				PhoneOff();
				Reboot();	
			}
		}
	}

	if (bitRead(DebugLevel, DebugBit_24hrON))
		LeavePhoneOn = true;

	if (!LeavePhoneOn || ReTry || (StartMonth != month()))		// if something fails, always reboot phone before the next go
		PhoneOff();


	if (StartMonth != month()) 		// reboot everything once a month to avoid any arduino 49 day timer issues
	{
		TrackCP(CP_MONTHLY_REBOOT);
		Reboot();
	}
}

int SetSlot()
{	
		nextSlot = (((now()+60) / Heartbeat )+1) * Heartbeat;
		return(((now()+60) / Heartbeat) % LEVELARRAY);		// figure out the current slot (0 to 95 usually)
}

boolean PhoneOn()
{
	boolean NoAnswer;
	
	wdt_reset();
	sim900_setup(SERIAL_BAUD, SERIAL_TIMEOUT);						// setup the On and Reset pins, the serial port and the timeout	
	
	lcd.setCursor(0,1);
	lcd.clearLine();
	
	lcd.print(F("1: Turn on GSM"));
	NoAnswer = sim900_on();				// turn on the phone and check that it responds
	lcd.setCursor(0,1);
	lcd.clearLine();
	if (NoAnswer)
		lcd.print(F("1: No answer - check jumpers"));
	else
		lcd.print(F("1: Phone is ON"));
	return (NoAnswer);
}

void PhoneOff()
{
		lcd.setCursor(0,1);
		lcd.clearLine();
		lcd.print(F("--Phone OFF--"));
	
		sim900_off();
		PhoneOK = false;
		// disable ADC
//		ADCSRA = ADCSRA & ~(bit (ADEN));		// disable ADC
		
		// disable serial port
		UCSR0B = 0;
		wdt_reset();
		delay(2000);
		wdt_reset();
}

void SetDefaults()
{
	PassCode = Default_Pass;
	eeprom_write_word(E_PASSCODE, Default_Pass);
	
	Heartbeat = Default_Heartbeat;
	eeprom_write_word((uint16_t*)E_HEARTBEAT, Heartbeat);
	Offset = Default_Offset;
	eeprom_write_word((uint16_t*)E_OFFSET, Offset);
	Readings = Default_Readings;
	eeprom_write_word((uint16_t*)E_READS, Readings);
	GaugeID = Default_GaugeID;
	eeprom_write_word((uint16_t*)E_GAUGE_ID, GaugeID);
	ScaleFactor = 1.0 * Default_Scale / 1000.0;
	eeprom_write_word((uint16_t*)E_SCALE1000, Default_Scale);
	Scale1000 = Default_Scale;
	TriggerLevel = Default_Trigger;
	eeprom_write_word((uint16_t*)E_TRIGGERLEVEL, TriggerLevel);	
	AlertLevel = Default_Alert;
	eeprom_write_word((uint16_t*)E_ALERTLEVEL, AlertLevel);
	UpdateInterval = Default_UpInt;
	eeprom_write_word((uint16_t*)E_UPINT, UpdateInterval);
	for (v=0; v<NumVIPs; v++)
		for (n=0; n<VIPlength; n++)												// zero all of the VIP phone numbers
			eeprom_write_byte((uint8_t*)(E_VIPS + v*VIPlength + n), 0);	

	eeprom_write_block( "0863075017", (uint8_t*)(E_VIPS), 11);
	eeprom_write_word((uint16_t*)E_DEBUGLEVEL, Default_DebugLevel);
	DebugLevel = Default_DebugLevel;
	eeprom_write_word((uint16_t*)E_CHECKPOINT, CP_OFF);		
}

int LastLevel(char slot)
{
	if (slot==0)
		return Levels[LEVELARRAY-1];
	else
		return Levels[slot-1];
}

int SendLevelUpdate()
{ 
	int Tries;
	char TrySlot;
	long SlotTime;
	
	Tries = 3;	
	while ((Tries>0) && (LatestUpdateSlot!=Slot))
	{ 
		wdt_reset();
		TrackCP(CP_UPDATE_GPRS + Tries);
		lcd.setCursor(0, 1);
		lcd.clearLine();
		lcd.print(F("Open GPRS"));		// open a gprs connection to the APN
		GprsOn = !openGPRS(msg);
		if (GprsOn)
			lcd.print(F("->OK"));
		else
		{
			lcd.print(F("->bad"));
			Tries--;
		}
		delay(500);			// wait 0.5 second to view the result on the screen

 
		while (GprsOn && (LatestUpdateSlot!=Slot))
		{	
			wdt_reset();
			TrackCP(CP_UPDATE_TCP + Tries);
			lcd.setCursor(0, 1);
			lcd.clearLine();
			lcd.print(F("Open TCP"));
			if (!TCPopen)
				TCPopen = !(openTCPproxy(msg));
			
			emptyRXbuffer();
			if (!TCPopen)
			{
				ret=0;
				lcd.print(F("->Bad"));
			}
			else
			{				
				wdt_reset();
				TrackCP(CP_TCP_OPEN + Tries);
				emptyRXbuffer();
				if (LatestUpdateSlot==-1)		// -1 => we just booted and LatestUpdateSlot has not been set yet
					TrySlot = Slot-UpdateInterval;				// if this is the case, send a packet of recent slots
				else
					TrySlot = LatestUpdateSlot + 1;
				
				if (TrySlot<0)					// this could happen if this is the first transmission
					TrySlot = TrySlot + LEVELARRAY;
				if (TrySlot == LEVELARRAY)		// Levels[LEVELARRAY] is a circular buffer, go back to 0 on overflow
					TrySlot = 0;

				if (Slot>=TrySlot)
					SlotTime = nextSlot - (1+Slot-TrySlot)*Heartbeat;	// nextSlot has already moved on by 1 heart beat
				else
					SlotTime = nextSlot - (1+LEVELARRAY+Slot-TrySlot)*Heartbeat;			

				lcd.print(F("->OK"));
				lcd.setCursor(0,4);
				lcd.clearLine();
				lcd.print('S');
				lcd.print((int)(TrySlot));
				lcd.print('>');
					
				sprintf(msg,"%s?river=%05d&pass=%04d&time=%ld&level=%d&mvolts=%d&temp=%d&vr=%d", UPDATE_URL, GaugeID, PassCode, SlotTime, Levels[TrySlot], mV, Temperature, validReads);
				ret = getHTTPbody(msg, UPDATE_SERVER, msg, 50, 3);		// 4 characters should be enough to hold level but read em all
				wdt_reset();
				
				lcd.print(ret);
				lcd.print(':');
				msg[30] = 0;		// if it is very long, truncate
				if (ret>0)
				{
					lcd.print(msg);
					delay(1000);			// wait one second to see the message
				}
				else
				{
					lcd.print(F("Faic"));
					closeTCP();		
					TCPopen=0;	
				}
			}
			
			if (ret > 0)
			{
				if (atoi(msg)==Levels[TrySlot])
				{
					LatestUpdateSlot = TrySlot;						 
					if (TrySlot!=Slot)
					{
						TrySlot++;
						if (TrySlot>=LEVELARRAY)
							TrySlot = 0;						
					}
				}
				else
				{	
					closeTCP();
					TCPopen=0;
					Tries--;
					wdt_reset();
				}
			}
			else
			{
				closeGPRS();
				GprsOn = 0;
				Tries--;
			}							
		} 

		
		if ((year() < 2015) && GprsOn)
		{
			wdt_reset();
			delay(500);
			if (!openTCPproxy(msg))
			{
				ReadServerTime();
				closeTCP();
				delay(500);
				wdt_reset();
				GetRealTime(GaugeID);
			}
		}
	
		wdt_reset();
		if (TCPopen)
		{
			closeTCP();
			TCPopen = 0;
		};
		if (GprsOn)
		{
			closeGPRS();
			GprsOn = 0;
		};


		if (LatestUpdateSlot!=Slot)
		{
			wdt_reset();
			lcd.clearLine();
			lcd.print(F("Go again.."));
			delay(5000);
			wdt_reset();
			emptyRXbuffer();
			lcd.clear();
		}
	}
	return (LatestUpdateSlot!=Slot);
}

void TrackCP(int CP)
{
	// CheckPoint holds the latest point that called a watchdog timeout to track where the timeout occurred
	// Make sure not to modify CheckPoint until the administrator sms message has been sent after the new boot
	// CheckPoint tracking is only enabled if Bit 1 of DebugLevel is set
	// Excessive writing to EEPROM when not debugging could potentially cause a failure after 100,000 EEPROM cycles
	
	if (bitRead(DebugLevel, DebugBit_CP))
		eeprom_write_word((uint16_t*)E_CHECKPOINT, CP);
}

int freeRAM()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/* void ReadVIP(char index, char* vip)
{
	char digit;
	for (digit=0; digit<VIPlength; digit++)
			vip[digit] = eeprom_read_byte((uint8_t*)(E_VIPS + index*VIPlength + digit));
	vip[VIPlength-1] = 0;	
} */

boolean UpPress()
{
	return (digitalRead(PRESSUP)==LOW);
}
boolean DownPress()
{
	return (digitalRead(PRESSDOWN)==LOW);
}
boolean BackPress()
{
	return (digitalRead(PRESSBACK)==LOW);
}
boolean EnterPress()
{
	return (digitalRead(PRESSENTER)==LOW);
}
