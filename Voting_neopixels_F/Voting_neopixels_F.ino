#include <WiShield.h>
#include <WiServer.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6  // LED strip signal pin

#define WIRELESS_MODE_INFRA	1
#define WIRELESS_MODE_ADHOC	2

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60,PIN,NEO_GRB+NEO_KHZ800); // declaring neopixel strip object

String string = "";            // contains incoming data from server
String prevString = "";        // contains previous data from server
int dimmer = 0;                // value used to determine led brightness
unsigned long E = 0;           // incoming E vote tally
unsigned long F = 0;           // incoming F vote tally
unsigned int totalTally = 0;  // total votes
int red = 0;                  // red color component
int green = 0;                // green color component
int blue = 0;                 // blue color component

// Wireless configuration parameters ----------------------------------------
unsigned char local_ip[] = {192,168,43,2};	// IP address of WiShield
unsigned char gateway_ip[] = {192,168,43,1};	// router or gateway IP address
unsigned char subnet_mask[] = {255,255,255,0};	// subnet mask for the local network
const prog_char ssid[] PROGMEM = {"yhtomit"};		// max 32 bytes

unsigned char security_type = 3;	// 0 - open; 1 - WEP; 2 - WPA; 3 - WPA2

// WPA/WPA2 passphrase
const prog_char security_passphrase[] PROGMEM = {"12345678"};	// max 64 characters

// WEP 128-bit keys
// sample HEX keys
prog_uchar wep_keys[] PROGMEM = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,	// Key 0
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Key 1
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Key 2
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	// Key 3
				};

// setup the wireless mode
// infrastructure - connect to AP
// adhoc - connect to another WiFi device
unsigned char wireless_mode = WIRELESS_MODE_INFRA;

unsigned char ssid_len;
unsigned char security_passphrase_len;
// End of wireless configuration parameters ----------------------------------------


// Function that prints data from the server
void printData(char* data, int len) {
  int counter = 0;
  // Print the data returned by the server
  // Note that the data is not null-terminated, may be broken up into smaller packets, and 
  // includes the HTTP header. 
  while (len-- > 0) {
      *(data++);
      if (*data == '$')counter++;
      if (counter == 1) string += *data;
      //Serial.print(counter);
  } 
  /*while (len-- > 0) {
    Serial.print(*(data++));
  }*/
}


// IP Address for hosting server 
uint8 ip[] = {185,27,134,202};

// A request that gets the latest data from vote process
GETrequest getVotes(ip, 80, "www.kisese.byethost7.com", "/kura/vote_results.php");

void setup() {
    // Initialize WiServer (we'll pass NULL for the page serving function since we don't need to serve web pages) 
  WiServer.init(NULL);
  strip.begin();              // initialize strip
  // Enable Serial output and ask WiServer to generate log messages (optional)
  Serial.begin(57600);
  
  // Have the processData function called when data is returned by the server
  getVotes.setReturnFunc(printData);
}


// Time (in millis) when the data should be retrieved 
long updateTime = 0;

void loop(){

  // Check if it's time to get an update
  if (millis() >= updateTime) {
    getVotes.submit();    
    // Get another update one hour from now
    updateTime += 1000;
  }
  else{
    if (string != "" && string != prevString){
      Serial.println(F("******************************************************************"));
      Serial.println(string);
      Serial.println(F("******************************************************************"));
      getParameters(string); //get incoming vote parameters
      
      totalTally = E + F;                // get the total tally of votes
      
      if (((float(F)/float(totalTally)) * 100) > 74)
        dimmer = 1;
      else if (((float(F)/float(totalTally)) * 100) < 75 && ((float(F)/float(totalTally)) * 100) > 49)
        dimmer = 2;
      else if (((float(F)/float(totalTally)) * 100) < 50 && ((float(F)/float(totalTally)) * 100) > 24)
        dimmer = 3;
      else if (((float(F)/float(totalTally)) * 100) < 25 && ((float(F)/float(totalTally)) * 100) >= 0)
        dimmer = 4;
      
      setColor();                        // set led strip color
      
      for (uint8_t i = 0; i < 60 ; i++){         // set neopixel color
        strip.setPixelColor(i, strip.Color((red/dimmer), (green/dimmer), (blue/dimmer)));
      }
      strip.show();                              // light up the neopixels
      
      prevString = string;
    }
    string = "";
  }
  
  // Run WiServer
  WiServer.server_task();
  delay(10);                               // delay for 10 microseconds
}

void getParameters(String incoming){
  char charBuffer[10];
  E = 0;
  F = 0;
  int startIndex = incoming.indexOf("E");
  int endIndex = incoming.indexOf(",", startIndex);
  String valueE = incoming.substring(startIndex+1, endIndex);
  valueE.toCharArray(charBuffer, 10); // convert string value to character
  for (int i = 0; i < valueE.length(); i++){
    E = (E*10) + (charBuffer[i] - '0');
  }
  if (valueE != ""){
    Serial.print("E = ");
    Serial.println(E);
  }
  
  startIndex = incoming.indexOf("F"); 
  endIndex = incoming.indexOf("," , startIndex);
  String valueF = incoming.substring(startIndex+1, endIndex);
  valueF.toCharArray(charBuffer, 10); // convert string value to character
  for (int i = 0; i < valueF.length(); i++){
    F = (F*10) + (charBuffer[i] - '0');
  }
  if (valueF != ""){
    Serial.print("F = ");
    Serial.println(F);
  }
}

void setColor(){
  if (E == F){     // if E is equal to F
    green = 255;
    red = 0;
    blue = 0;
  }
  else if (E > F){  // E is greater than F
    red = 0;
    blue = map(E, 0, totalTally/2, 255, 0);
    green = map(E, 0, totalTally/2, 0, 255);    
  }
  else if (E < F){   // E is less than F
    blue = 0;
    red = map(E, totalTally/2, totalTally, 0, 255);
    green = map(E, totalTally/2, totalTally, 255, 0);
  }
}
