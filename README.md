# Arduino Aprs Weather Station

This project creates a simple Aprs Weather Station for ham radio. In this project I used:

- 1 BME280

<img src="http://img.dxcdn.com/productimages/sku_436672_1.jpg" width="400" height="400"/>

- 1 ESP-01

<img src="https://img2.bgxcdn.com/thumb/large/oaupload/banggood/images/35/E5/89466d3a-fe96-42db-ac23-28625ecabb9d.jpg" width="400" height="400"/>

- 1 USB - UART do ESP8266 - ESP01 Programmer Adapter

# How the system works

The system gets weather data from BME-280 module and sends it to APRS server using telnet connection every 10 minutes. If you want to change this time, change the value below:

```
sendAPRSPacketEvery(600000); //run every 10 minutes
```

The weather data sent to the APRS network is shown at https://aprs.fi/#!lat=0&lng=0. The system has a html page that show the weather data in realtime and a web service that response is a json weather

## Details before record the code

Before record this code you need to write some important things, like your ssid and password wifi

```
const char* ssid     = "pp5ere"; //put your wifi ssid here
const char* password = "000000"; //put your wifi password here
```

Then you need to configure the main IP from the web service and network data addresses below

```
IPAddress ip(192,168,1,35); //sevice IP
IPAddress gate(192,168,1,1); //gateway IP
IPAddress sub(255,255,255,0); //subnet mask
```

The web service is listening on port 80, if you want another one change the line:

```
ESP8266WebServer server(80);
```

Then you need to inform some aprs network data, like callsign (USER), password (PAS), latitude (LAT), longitude (LON) and comment (COMMENT). If you want, change the server aprs and port

```
const String USER    = "PP5ERE-13";
const String PAS     = "00000";
const String LAT     = "2736.01S";
const String LON     = "04841.11W";
const String COMMENT = "APRS on ESP-01";
const String SERVER  = "brazil.aprs2.net";
const int    PORT    = 14579;
```
As we know, the APRS protocol uses Latitude and Longitude like:
```
DDMM.hhN/DDDMM.hhW

where DDMM.hhN is Latitude
DD are 2 digits for degrees
MM are 2 digits for minutes
hh are 2 digits for hundreds of minutes
N is 1 digit for North or South(N, S)

where DDDMM.hhW is Longitude
DDD are 3 digits for degrees
MM are 2 digits for minutes
hh are 2 digits for hundreds of minutes
W is 1 digit for East or West(E, W)

```
If you want to check the weather data in real time, you can access the main IP address in your favorite browser and check the weather page, as in the example below:

```
http://192.168.1.35
```

If you want to get data from ESP-01 to implement some application, you need access the URI weather. 

```
http://192.168.1.35/weather
```

This URI is http get request and show a json data:

```
{"TempC":26.53,"TempF":79.75,"Hum":69.25,"Pres":1023.99,"Alt":-64485.50}
```
# How I built the Arduino Sketch

![Legenda](https://raw.githubusercontent.com/pp5ere/AprsWeatherStation/master/esp-01_BMP280.png)

All you need to do is build this circuit and record the program on Esp-01

# Telnet Debug with RemoteDebug

If you have some problem, you can see what's going on inside your Esp-01 using telnet port. To do this, just access your favorite terminal and type:

```
$ telnet 192.168.1.35
```

You'll see the messages write into the code if you want know more about RemoteDebug, please click here https://github.com/JoaoLopesF/RemoteDebug
