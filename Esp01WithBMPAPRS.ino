// Do not remove the include below
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SparkFunBME280.h>
#include <OneWire.h>
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug

RemoteDebug Debug;

#define MAX_SRV_CLIENTS 1
#define MAX_TIME_INACTIVE 60000000

const char* ssid     = ""; //write your wifi connection name
const char* password = ""; // write your wifi password
float tempC, tempF, hum, pres, alt = -99;
String TitlePage = "Gauge PP5ERE-13 WX"; //write your html title
String msg = "";

unsigned long ElapsedTime = 0;

//Telnet Configuration from APRS server
const String USER    = ""; //write your aprs callsign
const String PAS     = ""; // write your aprs password
const String LAT     = "2736.01S"; //write your latitude
const String LON     = "04841.11W"; //write your longitute
const String COMMENT = "APRS on ESP-01"; //write some comment
//const String SERVER  = "brazil.d2g.com";
const String SERVER  = "brazil.aprs2.net"; // write the address of the aprs server
const int    PORT    = 14579; //write the aprs server port

BME280 bmp; //Uses I2C address 0x76 (jumper closed)

ESP8266WebServer server(80);

void(* resetFunc) (void) = 0; //restart

//The setup function is called once at startup of the sketch
void setup()
{
  
    Debug.println("Start setup()");  
    startConnection();
    
    startBMP();
    delay(10);
    Debug.begin("PP5ERE"); // Initiaze the telnet server

    Debug.setResetCmdEnabled(true); // Enable the reset command
}

// The loop function is called in an endless loop
void loop()
{
  if (WiFi.status() != WL_CONNECTED){
    Debug.println("Start WiFi.status() = !WL_CONNECTED");  
    startConnection();
  }
  //listen to client request
  server.handleClient();
  Debug.handle();

  sendAPRSPacketEvery(600000); //run every 10 minutes
  // Give a time for ESP

    yield();
}

void startConnection(){
  int count = 0;
  Debug.println("Try to connect(). Tentativa: "+String(count));
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);
  Debug.println("Run: WiFi.begin(ssid, password);");  
  while(WiFi.status() != WL_CONNECTED && count < 100) {
    Debug.println("Run: while(WiFi.status() != WL_CONNECTED && count < 100)");
    delay(500);
    Debug.println("WiFi.status() = "+ String(WiFi.status())+"\nTry = "+String(count)+"\n");
    Debug.println("Try Connecting... ");
    if (WiFi.status() == WL_NO_SSID_AVAIL){
      Debug.println("WiFi.status() == WL_NO_SSID_AVAIL"+ String(WiFi.status())+ "\nReset\n");
      delay(5000);
      resetFunc();
    }
    count++;
  }
  if (count == 100 && WiFi.status() != WL_CONNECTED){
    Debug.println("try == 100 && WiFi.status() != WL_CONNECTED\nReset...");
      delay(5000);
    resetFunc();
  }
  Debug.println("Connected!");
  IPAddress ip(192,168,1,35);
  Debug.println("Set IP = 192.168.1.35");
  IPAddress gate(192,168,1,1);
  Debug.println("Set Gateway = 192.168.1.1");
  IPAddress sub(255,255,255,0);
  Debug.println("Set mask = 255.255.255.0");
  //IPAddress dns1(8,8,8,8);
  //IPAddress dns2(8,8,4,4);
  WiFi.config(ip,gate,sub);//,dns1,dns2);
  Debug.println("Config the values: Wifi.config(ip,gate,sub)");
  server.on("/weather", HTTP_GET, getJson);
  Debug.println("Set server.on(/weather)");
  server.on("/", HTTP_GET, getPage);
  Debug.println("Set server.on(/)");
  //server.on("/log", HTTP_GET, getLog);
  Debug.println("Set server.on(log)");
  server.onNotFound(onNotFound);
  Debug.println("Set server.onNoFound");
  if ( MDNS.begin ( "esp8266" ) ) {
    Debug.println ( "MDNS started" );
  }
  server.begin();
  Debug.println("Run server.begin()");
  
}

void sendAPRSPacketEvery(unsigned long t){
  unsigned long currentTime;
  currentTime = millis();
  if (currentTime < ElapsedTime){
    Debug.println("Run: (currentTime = millis()) < ElapsedTime.\ncurrentTime ="+String(currentTime)+"\nElapsedTime="+String(ElapsedTime));
    ElapsedTime = 0;    
    Debug.println("Set ElapsedTime=0");
  }
  if ((currentTime - ElapsedTime) >= t){
    Debug.println("Tried : (currentTime - ElapsedTime) >= t.\ncurrentTime ="+String(currentTime)+"\nElapsedTime="+String(ElapsedTime));
    clientConnectTelNet();
    ElapsedTime = currentTime;  
    Debug.println("Set ElapsedTime = currentTime");
  }
}

void clientConnectTelNet(){
  WiFiClient client;
  int count = 0;
  String packet, aprsauth, tempStr, humStr, presStr;
  Debug.println("Run clientConnectTelNet()");
  getDataFromBMP();
  while (!client.connect(SERVER.c_str(), PORT) && count <20){
    Debug.println("Didn't connect with server: "+String(SERVER)+" Port: "+String(PORT));
    delay (1000);
    client.stop();
    client.flush();
    Debug.println("Run client.stop");
    Debug.println("Trying to connect with server: "+String(SERVER)+" Port: "+String(PORT));
    count++;
    Debug.println("Try: "+String(count));
  }
  if (count == 20){
    Debug.println("Tried: "+String(count)+" don't send the packet!");
  }else{
    Debug.println("Connected with server: "+String(SERVER)+" Port: "+String(PORT));
    tempStr = getTemp(tempF);
    humStr = getHum(hum);
    presStr = getPres(pres);
    Debug.println("Leu tempStr="+tempStr+" humStr="+humStr+" presStr="+presStr);
    while (client.connected()){ //there is some problem with the original code from WiFiClient.cpp on procedure uint8_t WiFiClient::connected()
      // it don't check if the connection was close, so you need to locate and change the line below:
      //if (!_client ) to: 
      //if (!_client || _client->state() == CLOSED)
      delay(1000);
      Debug.println("Run client.connected()");
      if(tempStr != "-999" || presStr != "-99999" || humStr != "-99"){
          aprsauth = "user " + USER + " pass " + PAS + "\n";
          client.write(aprsauth.c_str());
          delay(500);
          Debug.println("Send client.write="+aprsauth);
                  
          packet = USER + ">APRMCU,TCPIP*,qAC,T2BRAZIL:=" + LAT + "/" + LON +
               "_.../...g...t" + tempStr +
               "r...p...P...h" + humStr +
               "b" + presStr + COMMENT + "\n";
        
          client.write(packet.c_str());
          delay(500);
          Debug.println("Send client.write="+packet);
       
          client.stop();
          client.flush();
          Debug.println("Telnet client disconnect ");
      }
    }
  }
  
}

void getDataFromBMP(){
  int count = 0;
  tempC = bmp.readTempC();
  tempF = bmp.readTempF();
  hum = bmp.readFloatHumidity();
  pres = bmp.readFloatPressure();
  alt = bmp.readFloatAltitudeMeters();
  Debug.println("Read tempC="+String(tempC)+" tempF="+String(tempF)+" hum="+String(hum)+" pres="+String(pres)+" alt="+String(alt));
  while ((isnan(tempC) || isnan(tempF) || isnan(pres) || isnan(hum) || isnan(alt) || hum == 0) && count < 1000){
    Debug.println("Read (isnan(tempC) || isnan(tempF) || isnan(pres) || isnan(hum) || isnan(alt) || hum == 0) && count < 1000");
    tempC = bmp.readTempC();
    tempF = bmp.readTempF();
    hum = bmp.readFloatHumidity();
    pres = bmp.readFloatPressure();
    alt = bmp.readFloatAltitudeMeters();
    Debug.println("Trying read again tempC="+String(tempC)+" tempF="+String(tempF)+" hum="+String(hum)+" pres="+String(pres)+" alt="+String(alt)+" count="+String(count));
    
    delay(2);
    count++;
  }
}
void getJson(){
  getDataFromBMP();
  
  String json = "{\"TempC\":" + String(tempC) + 
                ",\"TempF\":" + String(tempF) + 
                ",\"Hum\":" + String(hum) + 
                ",\"Pres\":" + String(pres/100) + 
                ",\"Alt\":" + String(alt) + 
                "}";
  Debug.println("Create json="+json);
  server.send (200, "application/json", json);
  Debug.println("Run server.send (200, \"application/json\", json);");
}

void getPage(){
  String html = "";
  html += "<!doctype html> \n";
  html += "<html>\n";
  html += "<head>\n";
  html += "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n";
  html += "    <title>"+TitlePage+"</title>\n";
  html += "   <!--<link rel=\"stylesheet\" href=\"fonts.css\">-->\n";
  html += "    <script src=\"https://canvas-gauges.com/download/latest/all/gauge.min.js\"></script>\n";
  html += "    <script src=\"https://code.jquery.com/jquery-3.3.1.min.js\"></script>\n";
  html += "</head>\n";
  html += "<body style=\"background: #222\" onload=animateGauges()>\n";
  html += "<canvas id=\"tempC\"\n";
  html += "    data-type=\"radial-gauge\"\n";
  html += "        data-width=\"300\"\n";
  html += "        data-height=\"300\"\n";
  html += "        data-units=\"°C\"\n";
  html += "        data-title=\"Temperature\"\n";
  html += "        data-value=\"0\"\n";
  html += "        data-min-value=\"-50\"\n";
  html += "        data-max-value=\"50\"\n";
  html += "        data-major-ticks=\"[-50,-40,-30,-20,-10,0,10,20,30,40,50]\"\n";
  html += "        data-minor-ticks=\"2\"\n";
  html += "        data-stroke-ticks=\"true\"\n";
  html += "        data-highlights=\'[\n";
  html += "                    {\"from\": -50, \"to\": 0, \"color\": \"rgba(0,0, 255, .3)\"},\n";
  html += "                    {\"from\": 0, \"to\": 50, \"color\": \"rgba(255, 0, 0, .3)\"}\n";
  html += "                ]\'\n";
  html += "        data-ticks-angle=\"225\"\n";
  html += "        data-start-angle=\"67.5\"\n";
  html += "        data-color-major-ticks=\"#ddd\"\n";
  html += "        data-color-minor-ticks=\"#ddd\"\n";
  html += "        data-color-title=\"#eee\"\n";
  html += "        data-color-units=\"#ccc\"\n";
  html += "        data-color-numbers=\"#eee\"\n";
  html += "        data-color-plate=\"#222\"\n";
  html += "        data-border-shadow-width=\"0\"\n";
  html += "        data-borders=\"true\"\n";
  html += "        data-needle-type=\"arrow\"\n";
  html += "        data-needle-width=\"2\"\n";
  html += "        data-needle-circle-size=\"7\"\n";
  html += "        data-needle-circle-outer=\"true\"\n";
  html += "        data-needle-circle-inner=\"false\"\n";
  html += "        data-animated-value=\"true\"\n";
  html += "        data-animation-duration=\"1500\"\n";
  html += "        data-animation-rule=\"linear\"\n";
  html += "        data-color-border-outer=\"#333\"\n";
  html += "        data-color-border-outer-end=\"#111\"\n";
  html += "        data-color-border-middle=\"#222\"\n";
  html += "        data-color-border-middle-end=\"#111\"\n";
  html += "        data-color-border-inner=\"#111\"\n";
  html += "        data-color-border-inner-end=\"#333\"\n";
  html += "        data-color-needle-shadow-down=\"#333\"\n";
  html += "        data-color-needle-circle-outer=\"#333\"\n";
  html += "        data-color-needle-circle-outer-end=\"#111\"\n";
  html += "        data-color-needle-circle-inner=\"#111\"\n";
  html += "        data-color-needle-circle-inner-end=\"#222\"\n";
  html += "        data-color-value-box-rect=\"#222\"\n";
  html += "        data-color-value-box-rect-end=\"#333\"\n";
  html += "        data-font-value=\"Led\"\n";
  html += "        data-font-numbers=\"Led\"\n";
  html += "        data-font-title=\"Led\"\n";
  html += "        data-font-units=\"Led\"\n";
  html += "></canvas>\n";
  html += "<canvas id=\"hum\"\n";
  html += "    data-type=\"radial-gauge\"\n";
  html += "        data-width=\"300\"\n";
  html += "        data-height=\"300\"\n";
  html += "        data-units=\"%\"\n";
  html += "        data-title=\"Humidity\"\n";
  html += "        data-value=\"10\"\n";
  html += "        data-min-value=\"10\"\n";
  html += "        data-max-value=\"100\"\n";
  html += "        data-major-ticks=\"10,20,30,40,50,60,70,80,90,100\"\n";
  html += "        data-minor-ticks=\"2\"\n";
  html += "        data-stroke-ticks=\"true\"\n";
  html += "        data-highlights=\'[{\"from\": 90, \"to\": 100, \"color\": \"rgba(200, 50, 50, .75)\"}]\'\n";
  html += "        data-color-plate=\"#222\"\n";
  html += "        data-color-major-ticks=\"#f5f5f5\"\n";
  html += "        data-color-minor-ticks=\"#ddd\"\n";
  html += "        data-color-title=\"#fff\"\n";
  html += "        data-color-units=\"#ccc\"\n";
  html += "        data-color-numbers=\"#eee\"\n";
  html += "        data-color-needle-start=\"rgba(240, 128, 128, 1)\"\n";
  html += "        data-color-needle-end=\"rgba(255, 160, 122, .9)\"\n";
  html += "        data-value-box=\"true\"\n";
  html += "        data-font-value=\"Repetition\"\n";
  html += "        data-font-numbers=\"Repetition\"\n";
  html += "        data-animated-value=\"true\"\n";
  html += "        data-animation-duration=\"1500\"\n";
  html += "        data-animation-rule=\"linear\"\n";
  html += "        data-borders=\"false\"\n";
  html += "        data-border-shadow-width=\"0\"\n";
  html += "        data-needle-type=\"arrow\"\n";
  html += "        data-needle-width=\"2\"\n";
  html += "        data-needle-circle-size=\"7\"\n";
  html += "        data-needle-circle-outer=\"true\"\n";
  html += "        data-needle-circle-inner=\"false\"\n";
  html += "        data-ticks-angle=\"250\"\n";
  html += "        data-start-angle=\"55\"\n";
  html += "        data-color-needle-shadow-down=\"#333\"\n";
  html += "        data-value-box-width=\"45\"\n";
  html += "></canvas>\n";
  html += "<canvas id=\"pres\"\n";
  html += "    data-type=\"radial-gauge\"\n";
  html += "        data-width=\"300\"\n";
  html += "        data-height=\"300\"\n";
  html += "        data-units=\"hPa\"\n";
  html += "        data-title=\"Pressure\"\n";
  html += "        data-value=\"960\"\n";
  html += "        data-min-value=\"960\"\n";
  html += "        data-max-value=\"1060\"\n";
  html += "        data-major-ticks=\"[960,970,980,990,1000,1010,1020,1030,1040,1050,1060]\"\n";
  html += "        data-minor-ticks=\"10\"\n";
  html += "        data-stroke-ticks=\"true\"\n";
  html += "        data-highlights=\'[\n";
  html += "        {\"from\": 960, \"to\": 990, \"color\": \"rgba(0, 0, 255, .3)\"},\n";
  html += "        {\"from\": 990, \"to\": 1030, \"color\": \"rgba(0, 255, 0, .3)\"},\n";
  html += "        {\"from\": 1030, \"to\": 1060, \"color\": \"rgba(255, 0, 0, .3)\"}\n";
  html += "                ]\'\n";
  html += "        data-ticks-angle=\"225\"\n";
  html += "        data-start-angle=\"67.5\"\n";
  html += "        data-color-major-ticks=\"#ddd\"\n";
  html += "        data-color-minor-ticks=\"#ddd\"\n";
  html += "        data-color-title=\"#eee\"\n";
  html += "        data-color-units=\"#ccc\"\n";
  html += "        data-color-numbers=\"#eee\"\n";
  html += "        data-color-plate=\"#222\"\n";
  html += "        data-border-shadow-width=\"0\"\n";
  html += "        data-borders=\"true\"\n";
  html += "        data-font-Numbers-Size=\"14\"\n";
  html += "        data-needle-type=\"arrow\"\n";
  html += "        data-needle-width=\"2\"\n";
  html += "        data-needle-circle-size=\"7\"\n";
  html += "        data-needle-circle-outer=\"true\"\n";
  html += "        data-needle-circle-inner=\"false\"\n";
  html += "        data-animated-value=\"true\"\n";
  html += "        data-animation-duration=\"1500\"\n";
  html += "        data-animation-rule=\"linear\"\n";
  html += "        data-color-border-outer=\"#333\"\n";
  html += "        data-color-border-outer-end=\"#111\"\n";
  html += "        data-color-border-middle=\"#222\"\n";
  html += "        data-color-border-middle-end=\"#111\"\n";
  html += "        data-color-border-inner=\"#111\"\n";
  html += "        data-color-border-inner-end=\"#333\"\n";
  html += "        data-color-needle-shadow-down=\"#333\"\n";
  html += "        data-color-needle-circle-outer=\"#333\"\n";
  html += "        data-color-needle-circle-outer-end=\"#111\"\n";
  html += "        data-color-needle-circle-inner=\"#111\"\n";
  html += "        data-color-needle-circle-inner-end=\"#222\"\n";
  html += "        data-color-value-box-rect=\"#222\"\n";
  html += "        data-color-value-box-rect-end=\"#333\"\n";
  html += "        data-font-value=\"Led\"\n";
  html += "        data-font-numbers=\"Led\"\n";
  html += "        data-font-title=\"Led\"\n";
  html += "        data-font-units=\"Led\"\n";
  html += "></canvas>\n";
  html += "<script>\n";
  html += "var timers = [];\n";
  html += "function animateGauges() {\n";
  html += "  var url = window.location.origin+\"/weather\";\n";
  html += "  document.getElementById(\"tempC\").setAttribute(\"data-value\", 0);\n";
  html += "    document.getElementById(\"hum\").setAttribute(\"data-value\", 10);\n";
  html += "    document.getElementById(\"pres\").setAttribute(\"data-value\", 960);\n";
  html += "    document.gauges.forEach(function(gauge) {\n";
  html += "        timers.push(setInterval(function() {\n";
  html += "           $.getJSON(url, function(data) {    \n";
  html += "                if (gauge.options.maxValue == 50){\n";
  html += "                    //document.getElementById(\"tempC\").setAttribute(\"data-value\",  data.TempC);\n";
  html += "                    gauge.value = data.TempC;\n";
  html += "               }\n";
  html += "               if (gauge.options.maxValue == 100){\n";
  html += "                    //document.getElementById(\"hum\").setAttribute(\"data-value\", data.Hum);        \n";
  html += "                    gauge.value = data.Hum;\n";
  html += "               }\n";
  html += "               if (gauge.options.maxValue == 1060){\n";
  html += "                   gauge.value = data.Pres;\n";
  html += "               }\n";
  html += "           });   \n"; 
              
  html += "        }, gauge.animation.duration + 50));\n";
  html += "    });\n";
  html += "}\n";
  html += "</script>\n";
  html += "</body>\n";
  html += "</html>\n";
  Debug.println("Write html page");

  server.send (200, "text/html", html);
  Debug.println("Run server.send (200, \"text/html\", html);");
}

void onNotFound()
{
  server.send(404, "text/plain", "Not Found service in /" );
  Debug.println("Run server.send(404, \"text/plain\", \"Not Found service in /\" );");
}

String getTemp(float pTemp){
  String strTemp;
  int intTemp;
  Debug.println("Run getTemp(float pTemp)");
  intTemp = (int)pTemp;
  strTemp = String(intTemp);
  //strTemp.replace(".", "");
  
  switch (strTemp.length()){
  case 1:
    return "00" + strTemp;
    break;
  case 2:
    return "0" + strTemp;
    break;
  case 3:
    return strTemp;
    break;
  default:
    return "-999";
  }

}

String getHum(float pHum){
  String strHum;
  int intHum;
  Debug.println("Run getHum(float pHum)");
  intHum = (int)pHum;
  strHum = String(intHum);
  
  switch (strHum.length()){
  case 1:
    return "0" + strHum;
    break;
  case 2:
    return strHum;
    break;
  case 3:
    if (intHum == 100){
       return "00";
    }else {
       return "-99";
    }
    break;
  default:
    return "-99";
  }
}

String getPres(float pPress){
  String strPress;
  int intPress = 0;
  intPress = (int)(pPress/10);
  strPress = String(intPress);
  Debug.println("Run getPres(float pPress)");
  
  //strPress.replace(".", "");
  ///strPress = strPress.substring(0,5);
  //return strPress;
  
  switch (strPress.length()){
  case 1:
    return "0000" + strPress;
    break;
  case 2:
    return "000" + strPress;
    break;
  case 3:
    return "00" + strPress;
    break;
  case 4:
    return "0" + strPress;
    break;
  case 5:
    return strPress;
    break;
  default:
    return "-99999";
  }
}
/*
void addLog(String pMsg){
  msg +=pMsg;
}
void getLog(){
  
  server.send (200, "application/json", msg);
}*/

void startBMP(){
  int fail = 0;
  bool isBegin = false;
  //I2C stuff
    Debug.println("Run startBMP();");
    bmp.reset();
    Debug.println("Run bmp.reset();");
    Wire.pins(0, 2);
    Debug.println("Run Wire.pins(0, 2);");
    Wire.begin(0, 2);
    Debug.println("Run Wire.begin(0, 2);");
    
    //BMP280
    bmp.setI2CAddress(0x76);
    Debug.println("Run bmp.setI2CAddress(0x76);");
    isBegin = bmp.beginI2C();
    Debug.println("Read: bmp.beginI2C()");
    while (!isBegin && fail < 10){
      Debug.println("BMP280 don't start. Fail: " + String(fail));
        bmp.reset();
        Debug.println("Try run again bmp.reset();");
        Wire.pins(0, 2);
        Debug.println("Try run again Wire.pins(0, 2);");
        Wire.begin(0, 2);
        Debug.println("Try run again Wire.begin(0, 2);");
        delay(100);
        bmp.setI2CAddress(0x76);
        Debug.println("Read: bmp.setI2CAddress(0x76)");
        isBegin = bmp.beginI2C();
        fail++;
      }
      if (!isBegin){
        resetFunc();
      }
}
