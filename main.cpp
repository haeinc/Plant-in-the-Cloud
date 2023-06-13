#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DHT20.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "Wire.h"
#include "Adafruit_seesaw.h"

//ssh -i \Users\veron\Downloads\lab4_cs147.pem ubuntu@ec2-18-191-167-91.us-east-2.compute.amazonaws.com
// . venv/bin/activate
// export FLASK_APP=server.py
// python3 -m flask run --host=0.0.0.0


#define RED 33
#define YELLOW 32
#define GREEN 12
#define BLUE 13

TFT_eSPI tft = TFT_eSPI();
int width = 240;
int height = 135;

Adafruit_seesaw soil;

DHT20 DHT;

int min_uv = 0;
int max_uv = 0;
int curr;

char ssid[50] = "SETUP-B953";
char pass[50] = "brush3043engine";


const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

void connect_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}
 

void initialize_sensors() {
  DHT.begin();
  soil.begin(0x36);
  Wire.begin();

  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  digitalWrite(YELLOW, LOW);
  digitalWrite(BLUE, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(RED, LOW);
}


void initialize_screen() {
  tft.init();
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setRotation(1);
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  

  delay(1000);
  connect_wifi();
  initialize_sensors();

  initialize_screen();
  
}


void check_moisture(int moisture) {
  // too low
  if (moisture < 500) {
    digitalWrite(RED, HIGH);
    digitalWrite(BLUE, LOW);
    digitalWrite(GREEN, LOW);
  }
  // perfect
  else if (moisture > 500 && moisture < 800) {
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
    digitalWrite(RED, LOW);
  }
  // too high
  else if (moisture > 800) {
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
  }
}


uint8_t count = 0;
int line_count = 1;

void loop() {
  int err = 0;
  WiFiClient c;
  HttpClient http(c);
  
  char buffer[100];
  if (millis() - DHT.lastRead() >= 1000)
  {
    // READ DATA
    int status = DHT.read();

    if ((count % 10) == 0)
    {
      count = 0;
    }
    count++;

    float humidity = DHT.getHumidity();
    float temp = DHT.getTemperature();
    temp = (temp * 1.8) + 32;

    int moisture = soil.touchRead(0);

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_MAGENTA);
    String humid_s = "Humidity: " + String(humidity);
    tft.drawCentreString(humid_s, int(tft.width()/2), 10, 4);

    tft.setTextColor(TFT_SKYBLUE);
    String temp_s = "Temperature: " + String(temp);
    tft.drawCentreString(temp_s, int(tft.width()/2), 40, 4);

    tft.setTextColor(TFT_CYAN);
    String soil_s = "Soil Moisture: " + String(moisture);
    tft.drawCentreString(soil_s, int(tft.width()/2), 70, 4);

    check_moisture(moisture);



    //sprintf(buffer, "/?%s, %s, %s", humid_s, temp_s, soil_s);
    sprintf(buffer, "/?var=%s", String(moisture)); //, String(line_count));
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(", Temperature: ");
    Serial.print(temp);
    Serial.print(", Soil Moisture: ");
    Serial.print(moisture);

    Serial.print("\n");
  }
  

  
  err = http.get("18.222.219.231", 5000, buffer, NULL);
  
  if (err == 0) {
    Serial.println();
    Serial.println("startedRequest ok");
    err = http.responseStatusCode();

    if (err >= 0) {
      Serial.print("Got status code: ");
      Serial.println(err);
      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get
      err = http.skipResponseHeaders();

      if (err >= 0) {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;

        // Whilst we haven't timed out & haven't reached the end of the body
        while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (http.available()) {
            c = http.read();
            // Print out this character
            Serial.print(c);
            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          } else {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }

        }
      } else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connect failed: ");
    Serial.println(err);
    Serial.println(http.responseStatusCode());
  }
  http.stop(); 
  delay(1000);
}

