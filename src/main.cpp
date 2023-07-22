#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

boolean readPMSdata(Stream *s);
void lcd_Init();
void wifi_Init();
void GSM_Init();
void send_SMS(String sms);
void check_MQ135();
void check_DHT11();
void check_PMS();


#define BUZZER 23
#define MQ135_D 18
#define DHT_SENSOR_PIN  19 
#define DHT_SENSOR_TYPE DHT22
#define RL1 32
#define RL2 33


bool ledState = 0, buzzerState = 0, RL1State = 0;
bool gasDetect = 0;
bool fireAlert = 0, airAlert = 0;
int pm1State = 0, pm25State = 0, pm10State = 0;
const char* ssid = "Minh Khoi";
const char* password = "Minhkhoi2019";
char default_num[13] = {'+', '8', '4', '9', '8', '9', '3', '4', '3', '1', '1', '4' };
float humi = 0, tempC = 0, tempF = 0;

struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
struct pms7003data data;

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
LiquidCrystal_I2C lcd(0x27,20,4);
AsyncWebServer server(80);

void setup() 
{
  pinMode(MQ135_D, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RL1, OUTPUT);
  pinMode(RL2,OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(RL1, LOW);
  digitalWrite(RL2, LOW);

  Serial.begin(9600);
  dht_sensor.begin();
  Serial2.begin(9600);

  lcd_Init();
  wifi_Init();
  GSM_Init();
   // Initialize SPIFFS
  if(!SPIFFS.begin()){
    return;
  }
  //server_Init();
    // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(tempC).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humi).c_str());
  });
  server.on("/pm25", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(data.pm25_env).c_str());
  });
  server.on("/pm10", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(data.pm10_env).c_str());
  });
  server.on("/pm100", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(data.pm100_env).c_str());
  });
  // Start server
  server.begin();
  
}

void loop() {
  check_MQ135();
  check_DHT11();
  check_PMS();
  if (tempC > 40 && pm10State == pm25State == pm1State == 3 && fireAlert == 0){
    send_SMS("Fire detected!");
    fireAlert = 1;
  }
  else fireAlert == 0;

  if ( pm25State == 3 && airAlert == 0){
    send_SMS("Air quantity: BAD");
    airAlert = 1;
  }
  if (pm25State == 1 && airAlert == 1 ) 
    airAlert == 0;

  //server.handleClient();
}
void lcd_Init(){
  lcd.init();         
  lcd.backlight();
  lcd.cursor_off();
  lcd.setCursor(3,0);
  lcd.print("Air Monitoring");
  lcd.setCursor(6,2);
  lcd.print("Nhom: 3");
  delay(1500);
}

void wifi_Init(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing...");
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED);
  lcd.setCursor(0,1);
  lcd.print("WiFi connected...!");
  lcd.setCursor(0,2);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
}

void GSM_Init(){
  Serial.println("AT+CMGF=1");  // Configuring TEXT mode
  delay(100);
  Serial.println("AT&W");
  delay(100);
}

void send_SMS(String sms)
{
  Serial.print("AT+CMGS=\"");
  Serial.print(default_num);
  Serial.println("\"");
  delay(50);
  Serial.print(sms); //text content
  Serial.write(26);
}

// void server_Init(){
//   server.on("/", handle_OnConnect);
//   server.onNotFound(handle_NotFound);
//   server.begin();
//   delay(1000);

// }

// void handle_OnConnect(){
//   server.send(200, "text/html", SendHTML(data.pm10_env, data.pm25_env, data.pm100_env, tempC, humi));
// }

// void handle_NotFound(){
//   server.send(404, "text/plain", "Not found");
// }

// String SendHTML(int pm1, int pm25, int pm10, float tempC, float humi){
//   String ptr = "<!DOCTYPE html> <html>\n";
//   ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
//   ptr += "<title>Air monitoring</title>\n";
//   ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
//   ptr += "body{margin-top: 24px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
//   ptr += "p {font-size: 20px;color: #444444;margin-bottom: 10px;}\n";
//   ptr += "</style>\n";
//   ptr += "<script>\n";
//   ptr += "setInterval(loadDoc,1000);\n";
//   ptr += "function loadDoc() {\n";
//   ptr += "var xhttp = new XMLHttpRequest();\n";
//   ptr += "xhttp.onreadystatechange = function() {\n";
//   ptr += "if (this.readyState == 4 && this.status == 200) {\n";
//   ptr += "document.body.innerHTML =this.responseText}\n";
//   ptr += "};\n";
//   ptr += "xhttp.open(\"GET\", \"/\", true);\n";
//   ptr += "xhttp.send();\n";
//   ptr += "}\n";
//   ptr += "</script>\n";
//   ptr += "</head>\n";
//   ptr += "<body>\n";
//   ptr += "<div id=\"webpage\">\n";

//   ptr += "<div style=\" display: inline; \">";
//   ptr += "<p style=\"font-size: 15px; \">Sinh Viên: Lê Văn Dũng</p>";
//   ptr += "<p style=\"font-size: 15px; \">MSSV: 1912950</p>";
//   ptr += "</div>";
//   ptr += "<h1>Air Monitoring</h1>\n";
 

//   ptr += "<p>PM1.0: ";
//   ptr += pm1;
//   ptr += " ug/m3</p>";
 
//   ptr += "<p>PM2.5: ";
//   ptr += pm25;
//   ptr += " ug/m3</p>";
 
//   ptr += "<p>PM10: ";
//   ptr += pm10;
//   ptr += " ug/m3</p>";


//   ptr += "<p>Temp: ";
//   ptr += tempC;
//   ptr += " °C</p>";
//   ptr += "<p>Humi: ";
//   ptr += humi;
//   ptr += "% </p>";
 
//   ptr += "</div>\n";
//   ptr += "</body>\n";
//   ptr += "</html>\n";
//   return ptr;
// }


void check_MQ135(){
  if (!digitalRead(MQ135_D) && gasDetect == 0)
  {
    delay(20);
    if (!digitalRead(MQ135_D))
    {
      gasDetect = 1;
      digitalWrite(LED_BUILTIN, HIGH);
      ledState = 1;
      digitalWrite(BUZZER, HIGH);
      buzzerState = 1;
      digitalWrite(RL2, HIGH);
      send_SMS("Gas detected!");
    }
  }

  if (digitalRead(MQ135_D) && gasDetect == 1)
  {
    delay(20);
  if (digitalRead(MQ135_D))
    {
      gasDetect = 0;
      digitalWrite(LED_BUILTIN, LOW);
      ledState = 0;
      digitalWrite(BUZZER, LOW);
      buzzerState = 0;
      digitalWrite(RL2, LOW);
    }
  }
}

void check_DHT11(){

  humi  = dht_sensor.readHumidity();
  tempC = dht_sensor.readTemperature();
  tempF = dht_sensor.readTemperature(true);

    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(tempC);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(12,0);
    lcd.print("H:");
    lcd.print(humi);
    lcd.print("%");

    if (tempC >= 37 || humi > 80 && RL1State == 0){
      digitalWrite(RL1, HIGH);
      RL1State = 1;
    }
    if (tempC < 37 && humi < 80 && RL1State == 1){
      digitalWrite(RL1, LOW);
      RL1State = 0;
    }
}

void check_PMS(){
  if(readPMSdata(&Serial2)) {

    lcd.setCursor(1,1);
    lcd.print("PM1.0:");
    lcd.setCursor(7,1); 
    lcd.print("   ");
    lcd.setCursor(7,1); 
    lcd.print(data.pm10_env);
    lcd.setCursor(10,1);
    lcd.print("ug/m3");
    lcd.setCursor(16,1);
    if (data.pm10_env <= 12){
      lcd.print("PER");
      pm1State = 1;
    }
    else if (data.pm10_env > 35){
      lcd.print("BAD");
      pm1State = 3;
    }
    else {
      lcd.print("MED");
      pm1State = 2;
    }

    lcd.setCursor(1,2);   
    lcd.print("PM2.5:");
    lcd.setCursor(7,2); 
    lcd.print("   ");
    lcd.setCursor(7,2); 
    lcd.print(data.pm25_env);
    lcd.setCursor(10,2);
    lcd.print("ug/m3");
    lcd.setCursor(16,2);
    if (data.pm25_env <= 12){
      lcd.print("PER");
      pm25State = 1;
    }
    else if (data.pm25_env > 35){
      lcd.print("BAD");
      pm25State = 3;
    }
    else {
      lcd.print("MED");
      pm25State = 2;
    }

    lcd.setCursor(1,3);   
    lcd.print("PM10 :"); 
    lcd.setCursor(7,3); 
    lcd.print("   ");
    lcd.setCursor(7,3);
    lcd.print(data.pm100_env);
    lcd.setCursor(10,3);
    lcd.print("ug/m3");
    lcd.setCursor(16,3);
    if (data.pm100_env <= 54){
      lcd.print("PER");
      pm10State = 1;
    }
    else if (data.pm100_env > 154){
      lcd.print("BAD");
      pm10State = 3;
    }
    else{
      lcd.print("MED");
      pm10State = 2;
    }
  }
}

boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  /* debugging
    for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
    }
    Serial.println();
  */

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    //Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

