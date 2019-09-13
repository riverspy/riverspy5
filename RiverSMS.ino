
int NewSMScmd()
{
	int SMSno;
	TrackCP(CP_CHECK_SMS);
	SMSno = CheckForSMS();			// test for sms received
	delay(500);										// so that you can see the message on the screen
	
	if (SMSno != -1)
	{
		wdt_reset();
		TrackCP(CP_DELETE_SMS);
		DelSMS(SMSno);
		wdt_reset();
		TrackCP(CP_HANDLE_SMS);
		HandleSMScmd();
		wdt_reset();
	}
	
	return(SMSno);
}

int CheckForSMS()
{
	int SMSindex=0;
/*	for (SMSindex=0; SMSindex<14; n++)
		Sender[SMSindex] = 0;
	for (SMSindex=0; SMSindex<15; n++)
		SMScmd[SMSindex] = 0;
	for (SMSindex=(MSG_LEN-1); SMSindex>=0; SMSindex--)
		msg[SMSindex] = 0; */

	lcd.setCursor(0,5);
	lcd.clearLine();
	lcd.print(F("Check SMS "));
	emptyRXbuffer();
	Serial.println(F("AT+CMGL=\"REC UNREAD\""));						// read messages

	if (Serial.findUntil("+CMGL:", OKstr))		// read until just before the index number
	{
		msg[Serial.readBytesUntil(',', msg, 4)]=0;			// read the index
		SMSindex = atoi(msg);
		lcd.print(SMSindex);
		
		if (Serial.findUntil("READ\",\"", OKstr))
		{
			Sender[Serial.readBytesUntil('"', Sender, 13)]=0;
			Serial.find(CRLF);					// skip to past the CR-LF
			SMScmd[Serial.readBytesUntil(0x0D,SMScmd, MAX_CMD_LEN-1)] = 0;					// read the text portion

			Serial.find(OKstr);
			
			lcd.setCursor(0,2);
			lcd.clearLine();
			lcd.print(Sender);
			
			lcd.setCursor(0,3);
			lcd.clearLine();
			lcd.print(SMScmd);
			wdt_reset();
			delay(2000);
			return(SMSindex);
		}
	}
	else
	{
		lcd.setCursor(0,5);
		lcd.clearLine();
		lcd.print(F("No new SMS"));
		wdt_reset();
		delay(1000);
		return (-1);		// no message waiting
	}
}

void HandleSMScmd()
{
	char *argPtr = &SMScmd[7];
	
	lcd.clear();
	lcd.print(F("Process Cmd "));
	lcd.setCursor(0,1);
	lcd.print(Sender);
	lcd.setCursor(0,2);
	lcd.print(SMScmd);
	lcd.setCursor(0,4);
	delay(1000);
	wdt_reset();
	
	if (atoi(SMScmd) != PassCode)
	{
		lcd.print(atoi(SMScmd));
		lcd.print(F("denied"));
	}
	else
	{
		if (SMScmd[5] =='O' || SMScmd[5] == 'o')
		{
			Offset = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_OFFSET, Offset);
			Levels[Slot] = (int)(Offset - ScaleFactor * Total);		// scale to cm and add any offset due to sensor being above the 0cm level of the river
			sprintf(msg,"Offset %d Level now %dcm",Offset,Levels[Slot]);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}

		else if (SMScmd[5] =='C' || SMScmd[5] == 'c')
		{
			Offset = Offset + atoi(argPtr) - Levels[Slot];
			Levels[Slot] = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_OFFSET, Offset);
			Levels[Slot] = (int)(Offset - ScaleFactor * Total);		// scale to cm and add any offset due to sensor being above the 0cm level of the river
			sprintf(msg,"Cal %d Level now %dcm",atoi(argPtr),Levels[Slot]);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}

		else if (SMScmd[5] =='R' || SMScmd[5] == 'r')
		{
			Readings = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_READS, Readings);
			sprintf(msg,"Readings %d",Readings);
			lcd.print(msg);
			SendSMS(Sender,msg);
			ScaleFactor = 1.0 * eeprom_read_word((uint16_t*)E_SCALE1000) / (Readings * 1000.0);
		}
		
		else if (SMScmd[5] =='F' || SMScmd[5] == 'f')
		{
			eeprom_write_word((uint16_t*)E_SCALE1000, atoi(argPtr));
			ScaleFactor = 1.0 * atoi(argPtr) / (Readings * 1000.0);			
			Levels[Slot] = (int)(Offset - ScaleFactor * Total);		// scale to cm and add any offset due to sensor being above the 0cm level of the river
			sprintf(msg,"ScaleFac %d Level now %dcm",atoi(argPtr),Levels[Slot]);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}
		
		else if (SMScmd[5] =='H' || SMScmd[5] == 'h')
		{
			Heartbeat = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_HEARTBEAT, Heartbeat);
			sprintf(msg,"Heartbeat %d",Heartbeat);
			lcd.print(msg);
			SendSMS(Sender,msg);
			TrackCP(CP_CONFIG_REBOOT);
			Reboot();	// need to start again
		}
		
		else if (SMScmd[5] =='U' || SMScmd[5] == 'u')
		{
			UpdateInterval = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_UPINT, UpdateInterval);
			sprintf(msg,"Update Interval %d",UpdateInterval);
			lcd.print(msg);
			SendSMS(Sender,msg);
			TrackCP(CP_CONFIG_REBOOT);
			Reboot();	// need to start again
		}
		
		else if (SMScmd[5] =='G' || SMScmd[5] == 'g')
		{
			GaugeID = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_GAUGE_ID, GaugeID);
			sprintf(msg,"GaugeID %d",GaugeID);
			lcd.print(msg);
			SendSMS(Sender,msg);

		}

		else if (SMScmd[5] =='P' || SMScmd[5] == 'p')
		{
			PassCode = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_PASSCODE, PassCode);
			sprintf(msg,"PassCode %d",PassCode);
			lcd.print(msg);
			SendSMS(Sender,msg);
			eeprom_write_word((uint16_t*)E_PASSCODE, PassCode);
		}


		else if (SMScmd[5] =='T' || SMScmd[5] == 't')
		{
			TriggerLevel = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_TRIGGERLEVEL, TriggerLevel);
			sprintf(msg,"Trigger %d",TriggerLevel);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}

		
		else if (SMScmd[5] =='L' || SMScmd[5] == 'l')
		{
			sprintf(msg,"G:%d, L:%d, O:%d, R:%d, F:%d, T:%d, H:%d, U:%d",GaugeID,Levels[Slot],Offset,Readings,(int)(ScaleFactor*Readings*1000),TriggerLevel,Heartbeat,UpdateInterval);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}
		
		else if (SMScmd[5] =='D' || SMScmd[5] == 'd')
		{
			DebugLevel = atoi(argPtr);
			eeprom_write_word((uint16_t*)E_DEBUGLEVEL, DebugLevel);
			sprintf(msg,"DebugLevel:%d",DebugLevel);
			lcd.print(msg);
			SendSMS(Sender,msg);
		}

		else
		SendSMS(Sender,"Que?");
		
		Levels[Slot] = (int)(Offset - ScaleFactor * Total);		// scale to cm and add any offset due to sensor being above the 0cm level of the river
	} 
}

void ClearSMS()
{
	lcd.setCursor(0,5);
	lcd.clearLine();
	lcd.print(F("Delete SMS"));
	Serial.println(F("AT+CMGD=0,4"));					// delete all messages
	Serial.flush();
	delay(100);		// let the sim900 prepare for the next command
}

void DelSMS(int Sindex)
{
	lcd.setCursor(0,5);
	lcd.clearLine();
	lcd.print(F("Del SMS "));
	lcd.print(Sindex);
	Serial.print(F("AT+CMGD="));
	Serial.print(Sindex);
	Serial.println(",0");					// delete specific message
	if (Serial.findUntil(OKstr, ERRstr))		// read until OK
	lcd.print(F("->OK"));
	else
	lcd.print(F("->bad"));
	delay(500);
} 

void SendSMS(char* Num, char* SMSmsg)
{
	lcd.clear();
	lcd.print(F("Send SMS to \""));
	lcd.print(Num);
	lcd.print('"');
	lcd.setCursor(0,2);
	lcd.print(SMSmsg);
	emptyRXbuffer();
	Serial.print(F("AT+CMGS=\""));
	Serial.print(Num);
	Serial.println("\"");
	if (Serial.findUntil(">", ERRstr))
		lcd.print(" >");
	delay(100);		// let the sim900 prepare for the next command
	Serial.print(SMSmsg);
	Serial.println((char)0x1a);		// finish with Ctrl-Z
	delay(1000);
	wdt_reset();
	lcd.print("->");
	if (Serial.findUntil(OKstr, ERRstr))
		lcd.print(F("OK"));
	else
	{
		lcd.print(F("bad"));
		wdt_reset();
		delay(2000);		
	}
	wdt_reset();
	Serial.println();
	emptyRXbuffer();
};
