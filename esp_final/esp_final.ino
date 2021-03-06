#include <RTClib.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include "NTPClient.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "HTTPSRedirect.h"
#include "SPI.h"
#include "SD.h"
/*
////////////////////////***********************************/////////////////////////////////////
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
//Variables
int i = 0;
int statusCode;
const char* ssid = "Default_SSID";
const char* passphrase = "Default_Password";
String st;
String content;

//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);
///////////////////////***********************************//////////////////////////////////////
struct data {
  int Year = 0;
  int Month = 0;
  int Date = 0;
  int Day = 0;
  int H = 0;
  int M = 0;
  int S = 0;
  bool Connection = 0 ;// connection to internet
  float Analog0 = 0;
  bool Digital0 = 0;
};
struct data fData;

String SerialprintDataStruct(struct data d);
bool connect_to_wifi(const char* wifi_name, const char* password);
void get_ntp_time();
DateTime get_rtc_time();
void set_rtc_time();
void sync_clock();
void log_data();
void connect_to_gs();
void log_cloud(String datastr);
void showNewData();
void recvWithStartEndMarkers();
const int chipSelect = D8;
bool debug=false;
//
const byte numChars = 64;
char receivedChars[numChars];
boolean newData = false;
//

/*
 * TODO:
 * implement log_cloud function
*/


// Enter network credentials:
const char* wifi_name     = "Flynn";
const char* password = "01778661848";

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
const long utcOffsetInSeconds = 6 * 60 * 60;//seconds + 6 GMT
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


//
bool connected, RTC, sdcard;
////////////////////////////////////////////////////////////////////////////////////////
// Enter Google Script Deployment ID:

//AKfycbyHb6GvqEiTJwx8WL0OcgvWVeLwvcwp2Wc2MMGz8zl8IHDoq5-sa8iCUc31hhCBDmwWmw
//https://docs.google.com/spreadsheets/d/1M3xaxG51xiiUtlAeIqKY_2oDXReSYx8Zt0Uc16y3_Ok/edit#gid=0
const char *GScriptId = "AKfycbyHb6GvqEiTJwx8WL0OcgvWVeLwvcwp2Wc2MMGz8zl8IHDoq5-sa8iCUc31hhCBDmwWmw";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec?cal";
HTTPSRedirect* client = nullptr;
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//





String logfile_name;




 bool sdebug = true;
void setup() {
  delay(5000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Disconnecting current wifi connection");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  //---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }
  Serial.println();
  Serial.println("Waiting.");
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
    if ((WiFi.status() != WL_CONNECTED))

  connected = true;
  delay(5000);
  if (connected)
  get_ntp_time();
  delay(1000);

  if (! rtc.begin()) {
    if (debug)
    {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    }
    RTC = false;
  }
  else
    RTC = true;

  //??
  if (RTC)
  get_rtc_time();
  if (RTC && connected)
  
  sync_clock();

  pinMode(A0, INPUT); //A0
  pinMode(D0, INPUT); //D0


  char buff [64];
DateTime now = rtc.now(); 
//Updated now.day to now.date
sprintf(buff, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year()); 


logfile_name = buff;
logfile_name += String(".txt");
if(sdebug)
Serial.println(logfile_name);
  if (SD.begin(chipSelect))
  {
sdcard = true;
  Serial.println("SD card Found");

  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    
    dataFile.println(logfile_name);
    dataFile.close();
    // print to the serial port too:
    if(debug)
    Serial.println(logfile_name);
  }
  }
  else{
  sdcard=false;
  Serial.println("No SD card");
  }
  connect_to_gs();
}
void loop() {
  // put your main code here, to run repeatedly:
  //main loop

  if(( WiFi.status() == WL_CONNECTED )){
  connected = true;
  }
  else{
  connected = false;
  }
  fData.Connection = connected;
  if(connected){
   sync_clock();
  }
  
  DateTime timestamp = get_rtc_time();
fData.Year=timestamp.year();
fData.Month=timestamp.month();
fData.Date=timestamp.day();
fData.Day=timestamp.dayOfTheWeek();
fData.H=timestamp.hour();
fData.M=timestamp.minute();
fData.S=timestamp.second();
//get pinState A0, D0 
fData.Analog0 = (float)(analogRead(A0)/1024)*3.3;
fData.Digital0 = digitalRead(D0);
  
  // send incomplete struct to nano
  //SerialprintDataStruct(Data);
  //implement log_local() here
///
if(sdcard){
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
   if(sdebug)
Serial.println(SerialprintDataStruct(fData));
if (dataFile) {

  dataFile.println(SerialprintDataStruct(fData));
  dataFile.close();
}
  else {
    Serial.println("error opening datalog.txt");
  }
}
///
  //Serial.println(SerialprintDataStruct(fData));
    if(connected){
  log_cloud(SerialprintDataStruct(fData));//(TBI)
  }


  delay(1000);

}


bool connect_to_wifi(const char* wifi_name, const char* password)
{
  if (debug) {
    Serial.println("Wifi connecting to : ");
    Serial.println( wifi_name );
  }
  WiFi.begin(wifi_name, password);

  if ( WiFi.status() == WL_CONNECTED ) {
    if (debug) {
      Serial.println("connected");
      Serial.println("NodeMCU IP Address : ");
      Serial.println(WiFi.localIP() );
    }
    return true;
  }
  else {
    if (debug) {
      Serial.println("not connected");

    }
    return false;
  }
}

void get_ntp_time()
{
    int Year, Month, Date, Day,H, M, S;

  timeClient.update();
  if (debug){
    Year = timeClient.getYear();
    Month = timeClient.getMonth();
    Date = timeClient.getDate();
    Day = timeClient.getDay();
    H = timeClient.getHours();
    M = timeClient.getMinutes();
    S = timeClient.getSeconds();
    Serial.println("NTP time : ");
    Serial.print(Year);
    Serial.print('/');
    Serial.print(Month);
    Serial.print('/');
    Serial.print(Date);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[Day]);
    Serial.print(") ");
    Serial.print(H);
    Serial.print(':');
    Serial.print(M);
    Serial.print(':');
    Serial.print(S);
    Serial.println();
  }
}

DateTime get_rtc_time() {
  DateTime now = rtc.now();
  if (debug){
    Serial.println("RTC time : ");
    Serial.print(now.year());
    Serial.print('/');
    Serial.print(now.month());
    Serial.print('/');
    Serial.print(now.day());
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour());
    Serial.print(':');
    Serial.print(now.minute());
    Serial.print(':');
    Serial.print(now.second());
    Serial.println();
    }
    return now;
    }
void set_rtc_time() {
  int Year, Month, Date, Day,H, M, S;
  get_ntp_time();
  Year = timeClient.getYear();
  Month = timeClient.getMonth();
  Date = timeClient.getDate();
  H = timeClient.getHours();
  M = timeClient.getMinutes();
  S = timeClient.getSeconds();
  rtc.adjust(DateTime(Year, Month, Date, H, M, S));

}

void sync_clock()
{
  if(debug)
  Serial.println("syncing clock");

  DateTime Time_RTC = get_rtc_time();
   int Year, Month, Date, Day,H, M, S;
  get_ntp_time();
  Year = timeClient.getYear();
  Month = timeClient.getMonth();
  Date = timeClient.getDate();
  H = timeClient.getHours();
  M = timeClient.getMinutes();
  S = timeClient.getSeconds();
  
  if(Year != Time_RTC.year() || Month != Time_RTC.month() || Date != Time_RTC.day() || H!=Time_RTC.hour() || M != Time_RTC.minute() || S != Time_RTC.second()){
  if (debug)
  Serial.println("Synced R");
  set_rtc_time();
  }
}


String SerialprintDataStruct(struct data d)

{  String dataString = "";
dataString += String("[");
dataString += String(d.Year);
dataString += String("-");
dataString += String(d.Month);
dataString += String("-");
dataString += String(d.Date);
dataString += String("-");
dataString += String(d.Day);
dataString += String("-");
dataString += String(d.H);
dataString += String("-");
dataString += String(d.M);
dataString += String("-");
dataString += String(d.S);
dataString += String("]");
dataString += String(",");
dataString += String(d.Digital0);
dataString += String(",");
dataString += String(d.Analog0);
//dataString += String("");
if(debug)
Serial.println(dataString);
return dataString;
}


  
void connect_to_gs()
{
   // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
 // Serial.print("Connecting to ");
 // Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       //Serial.println("Connected");
       break;
    }
    //else()
      //Serial.println("Connection failed. Retrying...");
  }
  if (!flag){
   // Serial.print("Could not connect to server: ");
    //Serial.println(host);
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  }

void log_cloud(String datastr)
{
static bool flag = false;
if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
    }
  }
  else{
    //Serial.println("Error creating client object!");
  }
  
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + datastr + "\"}";
  
  // Publish data to Google Sheets
  //Serial.println("Publishing data...");
  if(client->POST(url, host, payload)){ 
    // do stuff here if publish was successful
      //Serial.println(payload);

  }
  else{
    // do stuff here if publish was not successful
   // Serial.println("Error while connecting");
  }
}
void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}
void showNewData() {
  if (newData) {
    if (newData){
    Serial.print("R: ");
    Serial.println(receivedChars);
    }

    newData = false;
  }
}
//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("ICON_datalogger_v1", "");
  Serial.println("Initializing_softap_for_wifi credentials_modification");
  launchWeb();
  Serial.println("over");
}
void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}
