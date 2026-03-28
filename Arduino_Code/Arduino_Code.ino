#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// --- CONFIGURATION ---
const char* ssid = "Test";
const char* pass = "12345678";
const char* server = "api.thingspeak.com";
String apiKey = "A6X2ZSQCBT2U6YK4"; 

// Pins
const int BUTTON_PIN = D5; 
const int ALARM_PIN = D1;  

SoftwareSerial gpsSerial(D6, D7); // RX=D6, TX=D7
TinyGPSPlus gps;
WiFiClient client;

int pressCount = 0;
unsigned long lastPressTime = 0;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  Serial.println("\n--- SYSTEM STARTING ---");
  
  WiFi.begin(ssid, pass);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println("Ready on D5 (Button) and D1 (Buzzer)");
}

void loop() {
  // GPS Reading
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Button Logic
  int btnState = digitalRead(BUTTON_PIN);
  if (btnState == HIGH) {
    delay(50); 
    if (digitalRead(BUTTON_PIN) == HIGH) {
      pressCount++;
      lastPressTime = millis();
      Serial.print("Click: "); Serial.println(pressCount);
      
      // Chota beep feedback ke liye
      digitalWrite(ALARM_PIN, HIGH);
      delay(100);
      digitalWrite(ALARM_PIN, LOW);
      
      while(digitalRead(BUTTON_PIN) == HIGH);
    }
  }

  // Actions
  if (pressCount > 0 && (millis() - lastPressTime > 800)) {
    if (pressCount == 1) {
      Serial.println("Action: Local Alarm");
      digitalWrite(ALARM_PIN, HIGH);
      delay(2000);
      digitalWrite(ALARM_PIN, LOW);
    } 
    else if (pressCount >= 2) {
      Serial.println("Action: Sending to ThingSpeak...");
      sendToThingSpeak();
    }
    pressCount = 0;
  }
}

void sendToThingSpeak() {
  if (gps.location.isValid()) {
    float lat = gps.location.lat();
    float lng = gps.location.lng();
    
    Serial.println("GPS Locked! Sending Data...");

    if (client.connect(server, 80)) {
      // Direct Link for Serial Monitor (Backup)
      Serial.print("Map Link: https://www.google.com/maps?q=");
      Serial.print(lat, 6); Serial.print(","); Serial.println(lng, 6);

      String url = "/update?api_key=" + apiKey;
      url += "&field1=" + String(lat, 6);
      url += "&field2=" + String(lng, 6);
      
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + server + "\r\n" +
                   "Connection: close\r\n\r\n");
      
      // Success Beep
      digitalWrite(ALARM_PIN, HIGH); delay(500); digitalWrite(ALARM_PIN, LOW);
    }
  } else {
    Serial.println("Waiting for GPS Lock... (Go near window)");
  }
}