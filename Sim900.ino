#define ON_TRIES 5
#define REG_TRIES 10
#define CSTT_TRIES 5
#define OPEN_WAIT 30
#define TCP_WAIT 30
#define CIPS_TRIES 3
#define HTTPGET_WAIT 10

#define INIT_ERR -2
#define CSTT_ERR -4
#define CIICR_ERR -6
#define CIPSTART_ERR -7
#define CLOSE_ERR -11
#define COMMS_ERR -12
#define REG_ERR -13
#define CIPSEND_ERR -14

void sim900_setup(int GBaud, int GTimeOut)
{
    pinMode(GSM_ON, OUTPUT);
    pinMode(GSM_RESET, OUTPUT);
    Serial.begin(GBaud);
    Serial.setTimeout(GTimeOut);
}

int sim900_on(){

	uint8_t t=0;


	Serial.println("\r\n\r\n\r\n\r\n");	// allow the SIM900 to sync to the baud rate
	emptyRXbuffer();
	Serial.println("AT");
	
	if (!Serial.find(OKstr))
	{
		wdt_reset();
		// power on pulse
		digitalWrite(GSM_ON,HIGH);
		delay(1500);				// the data sheet recommends an on pulse of greater than 1s
		digitalWrite(GSM_ON,LOW);
		
		wdt_reset();
		delay(2500);	// the data sheet recommends waiting at least 2.2s before the serial port is ready
		wdt_reset();
		

		// waits for an answer from the module
		t = 0;
		while(t < ON_TRIES)
		{
			wdt_reset();
			// Send AT every three seconds and wait for the answer
			Serial.println(F("AT"));
			if (Serial.find(OKstr))
				t = ON_TRIES;
			t++;
		}
		if (t!=(ON_TRIES+1))
		{
			digitalWrite(GSM_RESET,HIGH);
			delay(1000);
			digitalWrite(GSM_RESET,LOW);
			return (COMMS_ERR);
		}
	}
					
		// enable echo
	Serial.println(F("ATE 1"));
		// set error reporting level
	Serial.println(F("AT+CMEE=1"));	
		// sets the PIN code
	emptyRXbuffer();	// wait for everything to respond

	return 0;
}

int wait_for_reg(char* PIN){
	
	char ch;
	uint8_t t=0;
	
	if (PIN != NULL)
	{
		Serial.print(F("AT+CPIN="));
		Serial.println(PIN);
		if (!Serial.find(OKstr))
		{
			lcd.print(F("Bad PIN"));
			return (REG_ERR);
		}

	};
	delay(100);
	
	t = 0;
	while(t < REG_TRIES)
	{
		wdt_reset();
		// Send AT+CREG every three seconds and wait for the registration
		Serial.println(F("AT+CREG?"));					// wait for response of 0,1
		if (Serial.find("+CREG: 0,"))
		{
			delay(10);				// wait for the next char to arrive
			ch = Serial.read();
			if ((ch=='1') || (ch=='5'))
			t = REG_TRIES;
			else
			delay(2000);		// if not ready, try again in 3 seconds
		};
		t++;
	}
	if (t!=(REG_TRIES+1))
	return (REG_ERR);

	emptyRXbuffer();
	Serial.println(F("AT+DDET=0"));	// enable detection of DTMF dialing tones
	Serial.find(OKstr);
	Serial.println(F("AT+CMGF=1"));	// Set SMS mode to text
	Serial.find(OKstr);
	
	return (0);
}

int sim900_off()
{
	Serial.print((char)(0x1A));		// send a ctrl-z in case it is not expecting AT commands
	Serial.println(F("\r\n\r\n"));		// make sure that its listening
	Serial.println(F("AT+CPOWD=1"));	// send the power down command
	Serial.flush();					// let all of the chars go out in case a sleep follows the return
	return 0;
}

int openGPRS(char* IPaddr)
{
	int n=0;

	emptyRXbuffer();
	while(n < CSTT_TRIES)
	{
		wdt_reset();
		// Send the start task command every three seconds and wait for the answer
		Serial.print(F("AT+CSTT=\""));						// start GPRS task
		Serial.print(F(myAPN));
		Serial.print(F("\",\""));
		Serial.print(F(myUser));
		Serial.print(F("\",\""));
		Serial.print(F(myPass));
		Serial.println(F("\""));
		if (Serial.find(OKstr))
			n = CSTT_TRIES;
		n++;
	};
	if (n != CSTT_TRIES+1)
		return CSTT_ERR;

	
	Serial.println(F("AT+CIICR"));					// bring up the wireless connection
	delay(100);
	while((n<OPEN_WAIT) && (Serial.available()<3))
	{
		wdt_reset();
		delay(1000);			// check every second to see if response has come back
		n++;
	}
	if (!Serial.find(OKstr))
		return CIICR_ERR;

	emptyRXbuffer();
	Serial.println(F("AT+CIFSR"));						// read the IP address from the SIM900
	Serial.find(CRLF);									// skip past the first CRLF
	delay(10);											// wait 10ms
	wdt_reset();
	while (Serial.peek()==' ' || Serial.peek()==0x0A || Serial.peek()==0x0D)
	{			// skip past the white space
		Serial.read();
		delay(10);			// it takes about 10ms per char at 9600 Baud
	};
	IPaddr[Serial.readBytesUntil('\r',IPaddr, 30)]=0;	

	return (0);
}


int closeGPRS()
{
	Serial.flush();
	Serial.println(F("AT+CIPSHUT"));
	if (!Serial.find(OKstr))
		return CLOSE_ERR;

	return (0);
}


int openTCPproxy(char* reply)
{
	int n=0, t=0;
	
	while(t < CIPS_TRIES)
	{
		emptyRXbuffer();
		Serial.print(F("AT+CIPSTART=\"TCP\",\""));						// start TCP connection
		Serial.print(F(myProxy));
		Serial.print(F("\",\""));						// start TCP connection
		Serial.print(myProxyPort);	
		Serial.println(F("\""));
		Serial.find(OKstr);					// skip past the first OK
			
		while((n<TCP_WAIT) && (Serial.available()<10))
		{
			wdt_reset();
			delay(100);			// check every 100ms to see if response has come back
			n++;
		};
		
		if (Serial.available()>=10)
		{
			Serial.find(CRLF);			// skip past the  next CR
			reply[Serial.readBytesUntil('\r', reply, 30)]=0;	
			t = CIPS_TRIES;		
		}
		else
		{
			closeTCP();
			wdt_reset();
			delay(2000);
			wdt_reset();
		}

		t++;		
	}
	if (t != CIPS_TRIES+1)
		return (CIPSTART_ERR);
						
	return(0);
}



int getHTTPbody(char* path, char* server, char* result, int resultlength, char ReTries)
{
	int		n=0;
	char	t=0;
	char	*ret;
	time_t	TimeOut;

	while (n==0 && t<=ReTries)
	{
		wdt_reset();
		Serial.println(F("AT+CIPSEND"));
		if (!Serial.find(">"))
		{
			t++;
			if (t==ReTries)
			{
				lcd.clearLine();
				lcd.print(F("CIPSEND Error"));
				delay(1000);
				result[0] = 0x00;
				return (CIPSEND_ERR);
			}
		}
		else
		{
			lcd.print('>');
			Serial.print(F("GET "));
			Serial.print(path);
			Serial.println(F(" HTTP/1.1"));
		
			Serial.print(F("Host: "));
			Serial.println(server);
		
			Serial.println();
			Serial.print((char)(0x1A));		// finish with ctrl-z

			result[0] = 0;

			TimeOut = now() + HTTPGET_WAIT;		
			while(Serial.available()<20  && now()<TimeOut)
				{
					delay(50);
					wdt_reset();
				};	// await a response
		
			if (Serial.find("<body>"))							// skip to the body content
			{
				while(Serial.available()<3  && now()<TimeOut)
				{
					delay(50);
					wdt_reset();
				};	// wait for at least 3 more characters

				while (Serial.peek()==' ' || Serial.peek()==0x0A || Serial.peek()==0x0D){			// skip past any white space
					Serial.read();
					delay(10);			// it takes about 10ms per char at 9600 Baud
				};
				wdt_reset();
				Serial.setTimeout(500);		// wait up to half a second for more characters
				n = Serial.readBytes(result, resultlength);		
				result[n] = 0x00;	
				ret = strstr(result,"</body>");
				if (ret != NULL) ret[0] = 0x00;
				Serial.setTimeout(SERIAL_TIMEOUT);	
			}
			else
			{
				n=-1;
				t++;
			};
		}
	}
	return(n);
}


int closeTCP()
{
	Serial.flush();
	Serial.println(F("AT+CIPCLOSE"));
	if (!Serial.find(OKstr))
		return CLOSE_ERR;
	return(0);
}

void emptyRXbuffer()
{
	Serial.flush();		// wait for any TX characters to go out
	delay(100);			// wait 100ms for the gsm module to say OK etc
	
	while(Serial.available())
	{
		Serial.read();	// read the chars that are available now asap
		delay(10);		// allow 5ms between characters for ongoing transmissions		
	};
}

int ReadTemp()
{
	Serial.println(F("AT+CMTE?"));		// check temperature
	if (Serial.find(","))	// skip to where the temperature comes just after a comma
	{
		msg[Serial.readBytesUntil('\r',msg, MSG_LEN)]=0;	//
		return (atoi(msg));								// the chip is a bit warmer than ambient
	}
	else
		return (-99);
}

int ReadVoltage()
{
	Serial.println(F("AT+CBC"));		// check battery voltage
	if (Serial.find("+CBC:"))
	{
		Serial.find(",");	//skip the charge mode
		Serial.find(",");	//skip the percentage
		msg[Serial.readBytesUntil('\r', msg, MSG_LEN)]=0;	// read the battery voltage in mV
		return(atoi(msg));
	}	
	else
		return(0);	
	
}
