#define AdminPhone "+41763305009"
#define SENSOR_ON 9
#define SENSOR_SIGNAL A1

#define PRESSUP 3
#define PRESSDOWN 2
#define PRESSBACK A5
#define PRESSENTER A4

// The buttons were originally designed to use A0, A1, A2 and A3
// An error during pcb layout instead connected the signals to A7, A6, A5 and A4
// A7 and A6 are purely analog pins that cannot be used for digital functions
// The up and down buttons have been shorted to pins D2 and D3 to get the buttons working

#define GSM_ON 6
#define GSM_RESET 7

#define MSG_LEN 130
#define MAX_CMD_LEN 25
#define SERIAL_BAUD 19200
#define SERIAL_TIMEOUT 2500
#define LEVELARRAY 96					// store up to 96 readings between web updates, equivalent of 24 hours at 15min intervals
#define LOW_POWER_mV 3800				// go into low power mode, updating every 3 hours if voltage is below 3.900V

#define UPDATE_URL "http://www.riverspy.net/updaterz.cgi"
//updaterz.cgi will send on the data to riverzone.eu
#define UPDATE_SERVER "www.riverspy.net"
#define TIME_URL "http://www.riverspy.net/time.cgi"
#define TIME_SERVER "www.riverspy.net"

#define SIM_PIN				"0000"		// PIN code of the GSM SIM card
#define Default_Contrast	60			// was 127
#define Default_Heartbeat	900			// default is 900 seconds = 15 minutes
#define Default_Readings	10
#define Default_Offset		600			// River Level = Offset - (Vertical distance measured by LIDAR) (default 600cm)
#define Default_Scale		1000		// 1000 for vertical LIDAR, 1000.Cos(Theta) if at an angle
#define Default_GaugeID		10			// 3 for River Styx test setup, 10 for Rein da Medel
#define Default_Pass		1234		// crude security feature to deter malicious web updates
#define Default_Trigger		100			// Send web updates every reading when the level is above trigger (default 100cm)
#define Default_Alert		2000		// this will alert up to 10 VIPs by SMS when the river exceeds this level
#define Default_UpInt		12			// send updates to web every nth reading unless river level above trigger
#define Default_DebugLevel	0			// 0001 => just the startup admin sms 
#define DebugBit_SMS		0			// Bit 0 => send Admin SMS at boot time
#define DebugBit_CP			1			// Bit 1 => track checkpoint in EEPROM - caution EEPROM can burn out after 100,000 writes
#define DebugBit_24hrON		2			// Bit 2 => leave the phone on - only use if powered from mains
#define DebugBit_freeRAM	3			// Bit 3 => log the free RAM instead of water level if checking for a memory leak
#define VBAT_SCALE			18		// scale 1024 to 18.3V
#define NumVIPs				10
#define VIPlength			14

// this is the memory map for where variables are stored in EEPROM
#define E_PASSCODE 0			// 2 bytes (int) for passcode
#define E_HEARTBEAT 2			// 2 bytes (int) for heartbeat
#define E_OFFSET 4				// 2 bytes  for offset
#define E_SCALE1000 6			// 2 bytes for scale factor, (ADC * E_SCALE1000 / 1000 = level in cm)
								// 4V x (33k / (33k+15k)) = 2.75V at ADC input
								// 500cm is 4V at sensor, is 2.75V at ADC input , is 853 at ADC output
								// nominal value for ESCALE1000 is 586
#define E_PROXYPORT 8			// 2 bytes (int) for proxy port
#define E_APN 10
#define E_APN_LEN 30			// 30 bytes for APN
#define E_APN_USER 40
#define E_APN_USER_LEN 15			// 15 bytes for APN username
#define E_APN_PASS 55
#define E_APN_PASS_LEN 15			// 15 bytes for APN password
#define E_SERVER 70
#define E_SERVER_LEN 20			// 20 bytes for server name or IP address
#define E_GAUGE_ID 90			// 2 bytes (int) for Gauge ID
#define E_TRIGGERLEVEL 92			// 2 bytes (int) for trigger level
#define E_VIPS 94					// 10x14 = 140 bytes for those that receive sms alerts
#define E_READS 234				// 2 bytes  for readings
#define E_UPINT 236				// 2 bytes  for update interval (1 to 96)
#define E_ALERTLEVEL 238		// 2 bytes (int) for alert level
#define E_CHECKPOINT 240		// 2 bytes (int) for checkpoint
// CheckPoint holds the latest point that called a watchdog timeout to track where the timeout occurred
// Make sure not to modify CheckPoint until the administrator sms message has been sent after the new boot
#define E_DEBUGLEVEL 242		// 2 bytes (int) Bit 0=>send admin sms at boot time, Bit 1=>track CheckPoints

#define myCID 1

//  Settings for an O2 Sim
//#define myAPN "Internet"
//#define myProxy "62.40.32.40"
//#define myProxyPort 8080
//#define myUser ""
//#define myPass ""

//  Settings for a tescomobile sim
//#define myAPN "tescomobile.liffeytelecom.com"
//#define myProxy "10.1.11.19"
//#define myUser ""
//#define myPass ""
//#define myProxyPort 8080

//  Settings for a vodafone sim
//#define myAPN "isp.vodafone.ie"
//#define myProxy "www.riverspy.net"
//#define myProxyPort 80
//#define myUser "vodafone"
//#define myPass "vodafone"

//  Settings for a Swiss CoopMobile sim (untested)
#define myAPN "gprs.swisscom.ch"
#define myProxy "www.riverspy.net"
#define myProxyPort 80
#define myUser ""
#define myPass ""

// list of checkpoints useful for finding out where code has timed out, causing the watchdog to reboot the system
#define CP_OFF			0
#define CP_1S_LOOP		100
#define CP_SET_TO_8S	200
#define CP_TURN_ON_PHONE	250
#define CP_PHONE_IS_ON	300
#define CP_PIN_IS_NEXT	400
#define CP_PIN_IS_GOOD	500
#define CP_SEND_TO_VIP	600
#define CP_MONTHLY_REBOOT	700
#define CP_BATT_REBOOT	800
#define CP_SEND_UPDATE	900
#define CP_UPDATE_GPRS	1000
#define CP_UPDATE_TCP	1100
#define CP_TCP_OPEN		1150
#define CP_CHECK_SMS	1200
#define CP_DELETE_SMS	1300
#define CP_HANDLE_SMS	1400
#define CP_CONFIG_REBOOT	1500
#define CP_READING_LEVEL	1600
