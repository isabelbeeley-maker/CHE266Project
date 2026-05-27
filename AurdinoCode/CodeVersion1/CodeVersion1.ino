// Libraries for GPS
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// Libraries for display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//initialize Display object
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
// OLED reset pin, usually not used for I2C modules
#define OLED_RESET -1
// Most 128x64 I2C OLEDs use address 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//Initialize GPS object
SoftwareSerial gpsSerial(8, 9); // RX, TX
TinyGPSPlus gps;

unsigned long lastPrint = 0;

float origin_lat = 0;
float origin_lng = 0;

void setup() {

  //Setup GPS
  Serial.begin(115200);
  gpsSerial.begin(9600);

  Wire.begin();

  //Setup Ultrasound
  pinMode(12, OUTPUT);  // trigger pulse
  pinMode(11, INPUT);   // echo pulse


  //Create initial display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    while (true); 
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Waiting for GPS"));
  display.println(F("origin fix..."));
  display.display();

  setGPSOrigin();

  Serial.print(F("Origin set: "));
  Serial.print(origin_lat, 6);
  Serial.print(F(", "));
  Serial.println(origin_lng, 6);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Origin set"));
  display.print(F("Lat: "));
  display.println(origin_lat, 6);
  display.print(F("Lng: "));
  display.println(origin_lng, 6);
  display.display();

  delay(1500);
  Serial.print(F("Origin set: "));
  Serial.print(origin_lat, 6);
  Serial.print(F(", "));
  Serial.println(origin_lng, 6);
}
  


void loop() {
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Origin set"));
  
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  float distance_m = getGPSDistanceFromOrigin();
  if (distance_m >= 0) {
    Serial.print("Distance from origin: ");
    Serial.print(distance_m, 2);
    Serial.println(" m");
    display.print(distance_m);
    display.println(F(" m"));

  } else {
    Serial.println("No GPS fix yet");
  }
  
  float depth = gettDepth();
  if (depth >= 0) {
    Serial.print(depth);
    Serial.println(" cm");
    display.print(depth);
    display.println(F(" cm"));
  }

  display.display();

  delay(300);
}


//Set GPS starting location and save it as the global fields of origin
void setGPSOrigin() {
  while (!gps.location.isValid()) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 1000) {
      lastStatus = millis();

      Serial.print(F("Waiting for GPS fix... chars = "));
      Serial.print(gps.charsProcessed());

      Serial.print(F(", sats = "));
      if (gps.satellites.isValid()) {
        Serial.println(gps.satellites.value());
      } else {
        Serial.println(F("NA"));
      }
    }

    if (millis() > 10000 && gps.charsProcessed() < 10) {
      Serial.println(F("No GPS data. Check TX/RX wiring."));
    }
  }

  origin_lat = gps.location.lat();
  origin_lng = gps.location.lng();
}

//returns the distnace from origin
float getGPSDistanceFromOrigin() {
  // Always read GPS data first
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Only calculate distance if GPS has a valid location
  if (gps.location.isValid()) {
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    float distance_m = TinyGPSPlus::distanceBetween(
      lat,
      lng,
      origin_lat,
      origin_lng
    );

    return distance_m;
  }

  // Return -1 if no valid GPS fix yet
  return -1;
}


float gettDepth() {
  // Trigger pulse
  digitalWrite(12, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(12, LOW);

  unsigned long timeoutStart = micros();

  // Wait for echo to go HIGH, but do not wait forever
  while (!digitalRead(11)) {
    if (micros() - timeoutStart > 30000) {
      return -1;
    }
  }

  unsigned long StartTime = micros();

  // Wait for echo to go LOW, but do not wait forever
  while (digitalRead(11)) {
    if (micros() - StartTime > 30000) {
      return -1;
    }
  }

  unsigned long CurrentTime = micros();
  unsigned long HighLevelTime = CurrentTime - StartTime;

  float distance = HighLevelTime * 340.0 / 20000.0;  // cm

  return distance;
}
