// Arduino Timezone Library Copyright (C) 2018 by Jack Christensen and
// licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Arduino Timezone Library example sketch.
// Self-adjusting clock for one time zone.
// TimeChangeRules can be hard-coded or read from EEPROM, see comments.
// Jack Christensen Mar 2012

#include <Timezone.h>    //https://github.com/JChristensen/Timezone
#include <bitBangedSPI.h>
#include <MAX7219_Dot_Matrix.h>
#include <ESP8266WiFi.h>            // ESP8266 Wifi library
#include <ESP8266WebServer.h>       // Web server functions
#include <ESP8266mDNS.h>            // Web server functions
#include <WiFiUdp.h>

MAX7219_Dot_Matrix display (4, 15);  // Chips / LOAD

const byte chips = 4;               // Number of 8x8 modules linked together
//const int msglen = 500;             // Maximum length of the charas

const char* ssid = "Parzival";
const char* password = "EF13FCBC9D";
//const char* ssid = "Demo";
//const char* password = "kasonvaja";

static const char ntpServerName[] = "europe.pool.ntp.org";

WiFiUDP Udp;
uint16_t localPort;  // local port to listen for UDP packets

// You can comment this section out for using DHCP
//IPAddress ip(192, 168, 1, 85); // where xx is the desired IP Address
//IPAddress gateway(192, 168, 1, 254); // set gateway to match your network
//IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

MDNSResponder mdns;
ESP8266WebServer server(80);

//EET Tallinn
TimeChangeRule myDST = {"EEST", Second, Sun, Mar, 25, +180};    //Daylight time = UTC + 3 hours
TimeChangeRule mySTD = {"EET", First, Sun, Oct, 29, +120};     //Standard time = UTC + 2 hours
Timezone myTZ(myDST, mySTD);

//If TimeChangeRules are already stored in EEPROM, comment out the three
//lines above and uncomment the line below.
//Timezone myTZ(100);       //assumes rules stored at EEPROM address 100

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev


int st = 0;
char charas [] = "";
char message [] = "";
long cycler_helper;
unsigned long lastMoved = 0;
unsigned long MOVE_INTERVAL = 40;  // Default delay in miliseconds
int static_scroll = 1;  // Default to use scroll
int use_clock = 1;  // Default to use scroll
int use_cycle = 1;  // Default to use cycle
byte intensity = 1;                // Default intensity 0-15

int cycle_state = 1;
int  messageOffset;
String webPage = "";
String webStat = "";
String webFooter = "";
String str;
String messageString;

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  display.begin();
  display.setIntensity(intensity);
  display.sendSmooth ("", 0);


  connectWiFi();
}

void handleMainPage() {
  webPage = "<html><body><h1>MAX7219 Display and Text Scroller</h1><p>This sketch scrolls a custom charas on a MAX7219 driven 8x8 pixel display units chained one after the other.</p>";
  webPage += "<p><b>Change text</b><br/>Current text: ";
  webPage += messageString;
  webPage += "<br/><form action=\"/set\"><input type=\"text\" name=\"text\" value=\"";
  //webPage += messageString;
  webPage += "\"><input type=\"submit\" value=\"Update\"></form></p>";

  webPage += "<p><b>Cycle/Message only</b>";
  webPage += "<br/>Change to: <a href=\"set?cycle=1\">Cycle</a> | <a href=\"set?cycle=0\">Show message only</a>";
  webPage += "</p>";

  webPage += "<p><b>Static/Scoll</b>";
  webPage += "<br/>Change to: <a href=\"set?state=0\">Static</a> | <a href=\"set?state=1\">Scroll</a>";
  webPage += "</p>";

  webPage += "<p><b>Change scroll speed</b><br/>Current delay: ";
  webPage += MOVE_INTERVAL;
  webPage += "<br/>Change to: <a href=\"set?delay=10\">10</a> | <a href=\"set?delay=20\">20</a> | <a href=\"set?delay=30\">30</a> | <a href=\"set?delay=40\">40</a> | <a href=\"set?delay=50\">50</a> | <a href=\"set?delay=60\">60</a> | <a href=\"set?delay=70\">70</a> | <a href=\"set?delay=80\">80</a> | <a href=\"set?delay=10000\">90</a> | <a href=\"set?delay=100\">100</a>";
  webPage += "</p>";

  webPage += "<p><b>Change brightness</b><br/>Current brightness: ";
  webPage += intensity;
  webPage += "<br/>Change to: <a href=\"set?intensity=1\">1</a> | <a href=\"set?intensity=2\">2</a> | <a href=\"set?intensity=3\">3</a> | <a href=\"set?intensity=4\">4</a> | <a href=\"set?intensity=5\">5</a> | <a href=\"set?intensity=6\">6</a> | <a href=\"set?intensity=7\">7</a> | <a href=\"set?intensity=8\">8</a> | <a href=\"set?intensity=9\">9</a> | <a href=\"set?intensity=10\">10</a> | <a href=\"set?intensity=11\">11</a> | <a href=\"set?intensity=12\">12</a> | <a href=\"set?intensity=13\">13</a> | <a href=\"set?intensity=14\">14</a> | <a href=\"set?intensity=15\">15</a>";
  webPage += "</p>";

  webStat = "<p style=\"font-size: 90%; color: #FF8000;\">RSSI: ";
  webStat += WiFi.RSSI();
  webStat += "<br/>";
  webStat += "Uptime [min]: ";
  webStat += millis() / (1000 * 60);
  webStat += "</p>";

  webFooter = "<p style=\"font-size: 80%; color: #08088A;\">MAX7219 Text Scroller v1.0 | <a href=\"mailto:csongor.varga@gmail.com\">email me</a> | <a href=\"https://github.com/nygma2004/max7219scroller\">GitHub</a></p></body></html>";
  server.send(200, "text/html", webPage + webStat + webFooter);
  Serial.println("Web page request");
}

void handleSetCommand() {
  String response = "";
  if (server.args() == 0) {
    response = "No parameter";
  } else {
    if (server.argName(0) == "delay") {
      MOVE_INTERVAL = server.arg("delay").toInt();
      response = "Delay updated to ";
      response += MOVE_INTERVAL;
    }
    if (server.argName(0) == "cycle") {
      use_cycle = server.arg("cycle").toInt();
      response = "cycle updated to ";
      response += use_cycle;
    }
    if (server.argName(0) == "state") {
      static_scroll = server.arg("state").toInt();
      response = "State updated to ";
      response += static_scroll;
    }
    if (server.argName(0) == "intensity") {
      intensity = (byte)server.arg("intensity").toInt();
      response = "Intensity updated to ";
      display.setIntensity(intensity);
      response += intensity;
    }
    if (server.argName(0) == "text") {
      if (server.arg("text") == "removeStr") {
        messageString = "";
        response = "message removed";
      } else if (server.argName(0) != "") {
        messageString = server.arg("text");
        messageOffset = - chips * 8;
        response = "message updated";
      }
    }
    if (response == "" ) {
      response = "invalid parameter";
    }
  }
  Serial.print("Change request: ");
  Serial.println(response);
  //  response = "<html><head><meta http-equiv=\"refresh\" content=\"2; url=/\"></head><body>" + response + "</body></html>";
  //  server.send(200, "text/html", response);          //Returns the HTTP response
}

void updateDisplay() {
  if (static_scroll) {
    display.sendSmooth (message, messageOffset);
  }
  if (!static_scroll) {
    display.sendString (message);
  }

  // next time show one pixel onwards
  if (messageOffset++ >= (int) (strlen (message) * 8))
    messageOffset = - chips * 8;
}

void updateClock() {
  for (int cycler = 0; cycler < 3; cycler++) {
    time_t utc = now();    
    time_t ee = myTZ.toLocal(utc, &tcr);
    //time_t utc, ee;
    ee = myTZ.toLocal(utc);
    printTime(ee);
    delay(1000);
  }
  cycle_state = 2;
}

void connectWiFi() {
  //WiFi.config(ip, gateway, subnet);   // Comment this line for using DHCP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  long conStart = millis();
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - conStart >= 10000) {
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal [RSSI]: ");
    Serial.println(WiFi.RSSI());

    if (mdns.begin("max7219_display", WiFi.localIP())) {
      Serial.println("MDNS responder started");
    }
    server.on("/", handleMainPage);
    server.on("/set", handleSetCommand);        // Handling parameter set
    server.begin();
    Serial.println("HTTP server started");
    messageOffset = - chips * 8;

    setTime(myTZ.toUTC(compileTime()));
    localPort = random(1024, 65535);
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(5 * 60);
  }
}

void loop(void)
{
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (use_cycle == 1) {
    if (cycle_state == 1) {
      updateClock();
      messageOffset = - chips * 8;                                                        // reset scroller to start msg from edge
      cycler_helper = millis();
    }
    if (cycle_state == 2) {
      if (messageString == "") {
        cycle_state = 1;                                                                  // if no msg, skip scroll
      } else {
        messageString.toCharArray(message, messageString.length() + 1);
        if (millis () - lastMoved >= MOVE_INTERVAL) {                                     // start scroll
          updateDisplay ();
          lastMoved = millis ();
        }
        if (millis() - cycler_helper >= ((messageString.length() + 1) / 2.8) * 1000) {   // scroll as long to read the msg x1
          cycle_state = 1;
        }
      }
    }
  } else if (use_cycle == 0) {                                                             // scroll indefinitely
    messageString.toCharArray(message, messageString.length() + 1);
    if (millis () - lastMoved >= MOVE_INTERVAL) {
      updateDisplay ();
      lastMoved = millis ();
    }
  }
  server.handleClient();                                                                  // Handle HTTP server requests
}

//Function to return the compile date and time as a time_t value
time_t compileTime(void)  {
  tmElements_t tm;
  time_t t;

  breakTime(now(), tm);
  t = makeTime(tm);
  return t;
}

//Function to print time with time zone
void printTime(time_t t)
{
  sPrintI00(hour(t));
  sPrintDigits(minute(t));
  sPrintDigits(second(t));
  Serial.print(' ');
  Serial.print(dayShortStr(weekday(t)));
  Serial.print(' ');
  sPrintI00(day(t));
  Serial.print(' ');
  Serial.print(monthShortStr(month(t)));
  Serial.print(' ');
  Serial.print(year(t));
  Serial.print(" | Current msg: " + messageString + " | Size: " + messageString.length());
  Serial.println();

  sCalcI00(hour(t), minute(t));
  display.sendString (charas);
}

void sCalcI00(int hour, int minute)
{
  if (hour < 10) {
    str = "0";
  }
  str += hour;
  if (minute < 10) {
    str += "0";
  }
  str += minute;
  str.toCharArray(charas, 5);
  str = "";
  return;
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
  return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
  Serial.print(':');
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress timeServerIP; // time.nist.gov NTP server address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.print(F("Transmit NTP Request "));
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  Serial.println(timeServerIP);

  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while ((millis() - beginWait) < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println(F("Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  Serial.println(F("No NTP Response :-("));
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
