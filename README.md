# Arduino Aprs Weather Station

This project creates a simple Aprs Weather Station for ham radio. In this project I used:

- 1 BME280

<img src="http://img.dxcdn.com/productimages/sku_436672_1.jpg" width="400" height="400"/>

- 1 ESP-01 and 1 USB - UART do ESP8266 - ESP01 Programmer Adapter

<img src="https://img2.bgxcdn.com/thumb/large/oaupload/banggood/images/35/E5/89466d3a-fe96-42db-ac23-28625ecabb9d.jpg" width="400" height="400"/>

## Details

To run this code you need to write some important things, like your ssid and password wifi

```
const char* ssid     = "";
const char* password = ""; 	
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

If you want to check the weather data, you can access the main ip address on your preferential browser and check the weather page, like the example below:

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
# How I builded the Arduino Sketch

![Legenda](https://raw.githubusercontent.com/pp5ere/AprsWeatherStation/master/esp-01_BMP280.png)
