/* This is a modification of the Original Venduino Vending machine by Ryan Bates available at:
 *  http://www.retrobuiltgames.com/diy-kits-shop/venduino/
 *  
 *  The Modifications to the Original are:
 *  The Arduino Uno is replaced with a Arduino AT Mega 2560
 *  An Ethernet Shield is mounted on top of the AT Mega
 *  An MFRC-522 RFID Reader is mounted at the back of the coin slot. The coin slot is hidden with Company Logo Sticker
 *  An RGB Led is mounted in the LED hole on the front panel.
 *  The servos are Futaba S3003 transformed as continuous rotation.
 *  The power is supplied through the USB input, using a USB "battery pack".
 *  
 *  The Venduino RFID-MQTT connects through an MQTT Broker to an InterSystems Ensemble server that 
 *    - contains the RFID Card details (CardHolder Name and Card Balance)
 *    - stores all transactions
 *    - allows to centrally manage the pricing of the Venduino-RFID-MQTT
 *    
 *   Before deploying this code, the Server IP Address needs to be changed
 */
 
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>

#include <SPI.h>
#include <Ethernet.h>

//--- Arduino MQTT Client
#include <PubSubClient.h>

//--- RFID RC522
#include <MFRC522.h>

// A struct used for passing the UID of a PICC.
  typedef struct {
    byte    size;     // Number of bytes in the UID. 4, 7 or 10.
    byte    uidByte[10];
    byte    sak;      // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
  } Uid;

//--- Servos
#include <Servo.h>

//--- Nokia 5110 LCD Screen
#include <LCD5110_Graph.h>


//---------------------------------------------------------------
//----------- VALUES TO RECONFIGURE BEFORE DEPLOYMENT -----------
//---------------------------------------------------------------
//--- Debug to Serial?
#define DEBUGSERIAL
#ifdef DEBUGSERIAL
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x) 
#define DEBUGLN(x) 
#endif

//--- The IP and Port of the Mosquitto Server:
IPAddress MqttServer(191,1,1,2);   
int MqttServerPort=1883;


//--- The Contrast for the LCD
#define CONTRAST 125

//--- Ethernet Shield Configuration values (if DHCP Fails)
byte mac[] = { 0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 176);  //A local IP Address, IP In case DHCP does not return one
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

//---------------------------------------------------------------
//-----------------------  PINS ---------------------------------
//---------------------------------------------------------------

//--- SPI PIN CONFIGURATIONS for MEGA2560 Card. For RFID Reader RC522, and Ethernet Shield
#define PIN_RFID_RST   5      //Typical Pin layout for Mega
#define PIN_RFID_SS    49     //PYD: (8)fails, changed to 53. SPI SlaveSelect for RFID Shield. Avoid using 10 as the Eth Lib uses SS hardcoded to this in some places.

#define PIN_ETH_SS     53     //SPI SlaveSelect for Ethernet Shield
#define PIN_USED_ETH_LIBRARY_SS 10  //Do not use this PIN. ETH Library hardcodes its use in w5100.h as "Slave Select" port
#define PIN_ETHSD_SS    4     //SPI SlaSelect for SD Card on Ethernet Shield

//--- Local Pins for Sensors and Outputs -----------
//-- the 4 Buttons for the 4 trays
#define PIN_BUTTON_A   12
#define PIN_BUTTON_B   7
#define PIN_BUTTON_C   8
#define PIN_BUTTON_D   2

//-- the RGB Led at the top of the front panel
#define PIN_LED_RED   A10
#define PIN_LED_GREEN A9
#define PIN_LED_BLUE   A8

//--- These pins control the servo, PWM enabled pins
#define PIN_SERVO_A    9
#define PIN_SERVO_B    11
#define PIN_SERVO_C    3
#define PIN_SERVO_D    6

//We only enable one servo at a time. 
//This avoids having the servo move if their "neutral" point is not correctly set.
Servo servo;

//--- These Pins for the NOKIA5110 LCD
#define PIN_LCD_1  A0  //Nokia_SCK      
#define PIN_LCD_2  A1  //Nokia_MOSI     
#define PIN_LCD_3  A2  //Nokia_DC
#define PIN_LCD_4  A3  //Nokia_RST
#define PIN_LCD_5  A4  //NOkia_CS  
  

/*
 * SPI PINs on Arduino Mega:
 * RST/Reset      5 
 * SPI SS (SDA)   53  
 * SPI MOSI       51
 * SPI MISO       50
 * SPI SCK        52
*/

//-----------------------------------------------------------------
//-------------------------- PIN COLORS AND CABLING  --------------
//-----------------------------------------------------------------
/* NOKIA 5110
 * A0-A4: Nokia5110 LCD, with Resistor
 * LCD5110(int SCK, int MOSI, int DC, int RST, int CS)
 *        (7Yellow, 6Green, 5Blue, 4Violet, 3Gray) 
 *        ( A0    , A1    , A2  ,  A3     , A4)
 * Cabling Done: 
 *    8- LED        Orange -- (Backlight to 5V)
 *    7- SCLK       Yellow --(Arduino A0)
 *    6- CHKMD/MOSI Green -- (Arduino A1)
 *    5- D/C        Blue  -- (Arduino A2)
 *    4- Rst        Violet --(Arduino A3) 
 *    3- SCE        gray  -- (Arduino A4)
 *    2- Gnd        White -- GND
 *    1- VCC        Black -- 3.3V
 */

 /* RFID
  *             Gray  -- 3.3V
  *  5- RST     Violet
  *  GND        Blue
  *  N/A IRQ    Green
  *  50- MISO   Yellow
  *  51- MOSI   Orange
  *  52- SCK    Red
  *  49- SS     Brown  
  *  
  */

/*  Buttons
 *  Green: Upper Left   cable Violet  Pin 12
 *  Green: Upper Right  cable Blue    Pin 7
 *  Red:   Lower left   cable white   Pin 13
 *  Red:   Lower rigth  cable green   Pin 2
 *  Ground              cable Gray    Pin GND
 */

 /* Servos
  *  
  *  Servo Upper Right: 
  *                   white - cable red         Pin 6
  *                   Red   - cable Brown       5V
  *                   black - cable Black       Gnd
  * Servo Lower Right:
  *                   white  - cble Violet      Pin 11   - Cable Yellow
  *                   red    - cable Gray       5V
  *                   black  - cable white      Gnd
  *                   
  *  Servo Upper Left: 
  *                    white - cable Blue       Pin 3
  *                    Red:  - cable Green      5V
  *                    black - cable Yellow     Gnd
  *  Servo Lower Left
  *  
  *                   white  -cable Brown       Pin 9   -Cable Orange
  *                   red    - cable red        5V
  *                   black  - cable Orange     Gnd
  *                   
  *  
  */

  /* Led RGB - with 220 Ohm resistor
   *   Red    - cabke Green    A10
   *   Green  -. cable Violet  A8
   *   Blue   -  cable Blue    A9
   *   Minus  - cable Gray     GND
   */
  /* Temperature Sensor LM35
   *  Reading : cable yellow   A11
   */
//-----------------------------------------------------------------
//--------------------------    CODE          ---------------------
//-----------------------------------------------------------------

 LCD5110 glcd(A0,A1,A2,A3,A4);
 extern uint8_t SmallFont[];
  
//--- RFID Shield Configuration
MFRC522 mfrc522(PIN_RFID_SS, PIN_RFID_RST);  // Create MFRC522 instance

EthernetClient ethClient;
//unsigned long lastConnectionTime=0;
//const unsigned long postingInterval=30000; //30 secs


//--- MQTT Commnunication, Client and Topics
PubSubClient client(ethClient); 
char MQTTClientName[20]="Venduino-A";
const String topicPrefix="Venduino/Ens/";
String inTopicRoot="Venduino/Dev/A/";
String inTopicWildCard="Venduino/Dev/A/#";

//--- Local State variables

//volatile float tempCelsius;
//bool isStart=true;
bool cardIsPresent=false;     //Keeps trak os whether card enabled other tasks
String cardUID="";            //Maintains the UID of the last Card
String cardName="Salvador";   //Maintans the Customer name
int cardCredit=3;             //The Credit on the Card
int itemPrice=2;              //The Price of each item in Vending machine (all items of all trays have the same price!)
unsigned long cardMillis;     //Time at wich the card was swiped/responded 
bool isServoEnabled=false;
unsigned long lastCardInsertTime=0;
unsigned long maxSelectionTime=10000;  //The user has 10 Seconds to select product after inserting the card


//--- Setup the Correct Configuration for all the connected PINS
void initPins()
{
  //---RFID Pins, SPI Slave mode
  pinMode(PIN_RFID_SS,OUTPUT);

  //---Ethernet Shield and its SDCard, SPI Slave mode.
  pinMode(PIN_ETH_SS, OUTPUT);
  pinMode(PIN_ETHSD_SS,OUTPUT);

  //---RGB Led
  pinMode(PIN_LED_RED,OUTPUT);
  pinMode(PIN_LED_GREEN,OUTPUT);
  pinMode(PIN_LED_BLUE,OUTPUT);
  
  //--- Set the Initial Values for all Output PINS
  delay(100);
  digitalWrite(PIN_RFID_SS,HIGH);
  digitalWrite(PIN_ETH_SS,HIGH);
  digitalWrite(PIN_ETHSD_SS,HIGH);  
 
  
  pinMode(PIN_BUTTON_A,INPUT_PULLUP);
  pinMode(PIN_BUTTON_B,INPUT_PULLUP);
  pinMode(PIN_BUTTON_C,INPUT_PULLUP);
  pinMode(PIN_BUTTON_D,INPUT_PULLUP);
  
  delay(100); 
  
}
//--- Change the RGB Led color
void ledColor(int red,int green, int blue)
{
  analogWrite(PIN_LED_RED,red);
  analogWrite(PIN_LED_GREEN,green);
  analogWrite(PIN_LED_BLUE,blue);
  
}

//--- Init LCD Nokia 5110
void initLCD()
{
  glcd.InitLCD();
  glcd.setFont(SmallFont);
  glcd.setContrast(CONTRAST);
  out("VENDUINO MQTT ",1);
}

//--- Init the SPI and RFID522
void initRFID()
{
  SPI.begin();            // Init SPI bus
  mfrc522.PCD_Init();     // Init MFRC522 module
  DEBUGLN(F("MFRC522 Digital self test:"));
  #ifdef DEBUGSERIAL
      mfrc522.PCD_DumpVersionToSerial();  // Show version of PCD - MFRC522 Card Reader
  #endif
  
}

//--- Init Ethernet Shield
void initETH()
{
  DEBUG("Starting inet ...");  
  if (!Ethernet.begin(mac)){
    Ethernet.begin(mac,ip);
  }
  DEBUG("Done, IP:");
  DEBUGLN(Ethernet.localIP());
  
  out(ipToString(Ethernet.localIP()),2);  
}

//--- Init the Servio Objects and Bind them to their PIN, one by one (Avoid pulling too much current).
void initServos()
{
   servo.attach(PIN_SERVO_A); servo.write(90); delay(200); servo.detach(); delay(100);
   servo.attach(PIN_SERVO_B); servo.write(90); delay(200); servo.detach(); delay(100);
   servo.attach(PIN_SERVO_C); servo.write(90); delay(200); servo.detach(); delay(100);
   servo.attach(PIN_SERVO_D); servo.write(90); delay(200); servo.detach(); delay(100);
   
}

//--- the setup routine runs once when you press reset:
void setup() 
{
  //--- initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  DEBUGLN("setup: Serial Interface started");
  
  //--- Set the PINS to correct Input/Output LOW/HIGH values
  initPins();

  //---
  
  
  //--- Init the Nokia5110 LCD, print initial Message
  initLCD();
  
   //--- Check RFID,before Ethernet Shield
  initRFID();
  
  //--- Configure Ethernet
  initETH();
  delay(1000);

  //--- Init Servos
  initServos();
 
  //--- Configure MQTT Client
  client.setServer(MqttServer, MqttServerPort);
  client.setCallback(callback);
  DEBUGLN("MQTT Config Done");
  out("MQTT OK",3); 
  sendMQTTString("STATUS",(String)"ONLINE");
  
  ledColor(255,0,0);delay(600);ledColor(0,255,0);delay(600);ledColor(0,0,255);delay(600);ledColor(0,0,0);
 
  delay(1000); 
  showInitialMessage(); 

}

//--- Display the Initial Message
void showInitialMessage()
{
  glcd.clrScr(); 
  out("Venduino MQTT",1);
  out("Pasar Targeta",3);
  glcd.update();
}
//--- Message when card Read
void showCard()
{
  glcd.clrScr();
  glcd.update();
  out("ID:"+cardUID,3);
  glcd.update();
}

//-- Message when ID Back
void showCustomerInfo()
{
  glcd.clrScr();
  out(String("Hola ")+cardName,1);
  out(String("Saldo: ")+cardCredit+ " $",2);
  
  out (String("Precio: ")+itemPrice+ "$",4);
  glcd.update();
}

//-- Special Offer Message()
void showSpecial()
{
  glcd.clrScr();
  outCenter("Oferta!",2);
  outCenter(String("Precio:")+itemPrice+"$",3);
  glcd.drawRoundRect(5,5,75,30);
  glcd.update();
  ledColor(255,0,255);delay(500);ledColor(0,0,0);
  ledColor(255,0,255);delay(500);ledColor(0,0,0);
  ledColor(255,0,255);delay(500);ledColor(0,0,0);
    
}
///--- Out message on LCD Screen
void out(String msg,int line)
{
  glcd.print(msg,0,(line-1)*10);
  glcd.update();
  //delay(200);
}

void outCenter(String msg,int line)
{
  glcd.print(msg,CENTER,(line-1)*10);
  glcd.update();
  //delay(200);
}

//--- Transform an IP Address to a String for display
String ipToString(IPAddress ip)
{
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

//--- Transform a Card Addres to a String (for display and Sending)
String cardToString(Uid *uid) {
  String s;
  char bufptr[5];
  for (byte i = 0; i < uid->size; i++) {
    sprintf(bufptr, "%02X", uid->uidByte[i]);
    s += String(bufptr[0])+String(bufptr[1])+"-";
  } 
  return s.substring(0,s.length()-1);
}

//---- the loop routine runs over and over again forever:
void loop() { 
   
  // Are we connected to MQTT broker?
  if (!client.connected()) {
      reconnect();
      
  }else {
    // Connected to MQTT broker, process subscriptions
    client.loop();
  }

  //--Look for RFID Card and get Data
   checkForCard();

  //--Check if Status is
  while (isServoEnabled) {
    if (millis() - lastCardInsertTime < maxSelectionTime) {
        serveProduct();  
    }else {
      isServoEnabled=false;
      showInitialMessage();
      cardName="";
      cardCredit=0;
    }
    
  }

  
}

//-------------------------------------
// RFID Reading Functions
//-------------------------------------
void checkForCard() {
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  DEBUG("Card found:");
  #ifdef DEBUGSERIAL
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  #endif
  
  //---Update Card Information
  cardIsPresent=true;
  cardUID=cardToString((Uid *)&(mfrc522.uid));
  showCard();
  
  mfrc522.PICC_HaltA();

  //---Call MQTT Server to get customer info and Balance
  sendMQTTString("CARD",cardUID);
  
}
//-------------------------------------
// Buttons and Servo Functions
//-------------------------------------

//Proceed to turn the given servo
void vending(int type) 
{
  DEBUGLN((String)"Vending type:"+(String) type);
  sendMQTTString("BUY",(String)cardUID+"|"+(String)type+"|"+(String)itemPrice);
  ledColor(255,0,0);delay(1000);
  glcd.clrScr();
  glcd.print((String)"Procesando "+(String)type,CENTER,0);
  glcd.update();

  //Choose which Servo
  int servoNum;
  switch (type) {
    case 1:
      servoNum=PIN_SERVO_A;
      break;
    case 2:
      servoNum=PIN_SERVO_B;
      break;
    case 3:
      servoNum=PIN_SERVO_C;
      break;
    case 4:
      servoNum=PIN_SERVO_D;
      break;
    default:
      DEBUGLN("Error in vending rtn, DEFAULT case");
  }
  //Switch the Servo on and Turn it
  ledColor(0,0,0);delay(1000);
  servo.attach(servoNum); servo.write(90); delay(100); 
  servo.write(120);

  //--- You may need to adjust the time each servo rotates, and the rotation direction
  for (int i=0;i<5;i++) 
  {
    ledColor(0,0,255);
    delay(205);
    ledColor(0,0,0);
    delay(205);
  }
  servo.detach(); delay(100);
  
  isServoEnabled=false;
  cardName="";
  cardCredit=0;
  showInitialMessage();
  DEBUGLN("SOLD ITEM");  
}

//---Scans the Buttons and wait for selection
void serveProduct() 
{
  ledColor(0,255,0);
  while ((digitalRead(PIN_BUTTON_A)==HIGH || digitalRead(PIN_BUTTON_B)==HIGH || digitalRead(PIN_BUTTON_C)==HIGH ||digitalRead(PIN_BUTTON_D)== HIGH)&& isServoEnabled)
  {
    if (millis() - lastCardInsertTime > maxSelectionTime) {
      isServoEnabled=false;
      DEBUGLN("ServeProduct: TIMEOUT");
    }
    if (digitalRead(PIN_BUTTON_A)==LOW) {
      vending(1);
      break;
    }
    if (digitalRead(PIN_BUTTON_B)==LOW) {
      vending(2);
      break;
    }
    if (digitalRead(PIN_BUTTON_C)==LOW) {
      vending(3);
      break;
    }
    if (digitalRead(PIN_BUTTON_D)==LOW) {
      vending(4);
      break;
    }
  } //while
  ledColor(0,0,0);
  cardName="";
  cardCredit=0;
  showInitialMessage();
}
//-------------------------------------
// MQTT Functions
//-------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String strTopic = String((char*)topic);
  String strPayload="";
  for (int i=0;i<length;i++) {  strPayload=strPayload+(char)payload[i]; }
  
  DEBUG((String)"MQTT Message ["+strTopic+"]:");
  DEBUGLN(strPayload);
  if (strTopic==(inTopicRoot+"CARD")) {
    DEBUGLN("CARD Detected");
    cardUID=strPayload.substring(0,strPayload.indexOf("|"));
    DEBUGLN((String)"CardID Parsed:"+cardUID);
    String remains=strPayload.substring(strPayload.indexOf("|")+1,strPayload.length());
    cardName=remains.substring(0,remains.indexOf("|"));
    DEBUGLN((String)"Name Parsed:"+cardName);
    String txt =remains.substring(remains.indexOf("|")+1,remains.length());
    DEBUGLN((String)"Credit Parsed:"+txt);
    cardCredit=txt.toInt();
    showCustomerInfo();
    lastCardInsertTime=millis(); //Start Counting time toward autocancel
    
    if (cardUID=="") {
      outCenter("Desconocido",5);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      delay(2000);
      cardName="";
      cardCredit=0;
      showInitialMessage();
      return;
    }
    
    if (cardCredit>=itemPrice) {
      isServoEnabled=true;
    }else {
      outCenter("Sin Saldo",5);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      ledColor(255,0,0);delay(500);ledColor(0,0,0);
      delay(2000);
      cardName="";
      cardCredit=0;
      showInitialMessage();
    }
    return;
  }
  if (strTopic==(inTopicRoot+"OFFER")) {
    DEBUGLN("OFFER Detected");
    itemPrice=strPayload.toInt();
    showSpecial();
    
    return;
  }
  if (strTopic=(inTopicRoot+"PRICE")) {
    DEBUG("END OF OFFER NEW Price:");
    itemPrice=strPayload.toInt();
    showInitialMessage();
    return;
  }
} 

//--- Reconnect (non-blocking) to MQTT broker
boolean reconnect() {
  DEBUGLN("Reconnect");
  if (client.connect(MQTTClientName)) {
    // Subscribe to inTopic to receive messages
    int lt=inTopicWildCard.length()+1;    
    char topicChars[lt];
    inTopicWildCard.toCharArray(topicChars,lt);
    client.subscribe(topicChars);
    DEBUGLN("Subscribed to:"+inTopicWildCard);
  }
  return client.connected();
}
//--- Helper function to send MQTT messages using Strings for both topic and content
void sendMQTTString(String topic, String content) {
    // Prepend prefix to topic
    String topicFull=topicPrefix+topic;

    DEBUGLN((String)"sendMQTTString: ["+topicFull+"]="+content);
    
    // Convert topic string to char array
    int lt=topicFull.length()+1;    
    char topicChars[lt];
    topicFull.toCharArray(topicChars,lt);

    // Convert content string to char array
    int lc=content.length()+1;       
    char contentChars[lc];
    content.toCharArray(contentChars,lc);

    // Send out MQTT message
    client.publish(topicChars,contentChars);
}
// ------------------------------------
