#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "rf_model.h"

// -------- WIFI --------
const char* ssid = "your wifi ssid";
const char* password = "your wifi password";

// -------- TWILIO --------
String accountSID = "your twilio account sid";
String authToken  = "your twilio auth token";
String fromNumber = "your twilio phone number";
String toNumber   = "your receiver phone number";

// -------- OBJECTS --------
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
MPU6050 mpu;
WebServer server(80);

// -------- ML --------
#define WINDOW_SIZE 200
float ax_buf[WINDOW_SIZE], ay_buf[WINDOW_SIZE], az_buf[WINDOW_SIZE];
float gx_buf[WINDOW_SIZE], gy_buf[WINDOW_SIZE], gz_buf[WINDOW_SIZE];
float features[24];
int index_sample = 0;

// -------- STATUS --------
String statusText = "Normal Activity";
String latText = "N/A";
String lonText = "N/A";

unsigned long fallDisplayTime = 0;
bool smsSent = false;

// -------- GPS MEMORY --------
float lastLat = 0.0;
float lastLon = 0.0;
bool hasValidGPS = false;

// -------- UI CONTROL --------
unsigned long lastUIUpdate = 0;
const int UI_REFRESH_INTERVAL = 500;

// -------- FUNCTION DECLARATIONS --------
void compute_features();
void calc_stats(float *buffer, float *out);
void sendSMS(String message);

// -------- SETUP --------
void setup() {

  Serial.begin(115200);
  Wire.begin(21, 22);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting WiFi...");
  }

  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());

  // MPU
  mpu.initialize();
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_8);

  // GPS
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // Web UI
  server.on("/", []() {
    String page = "<html><head>";
    page += "<meta http-equiv='refresh' content='2'>";
    page += "<style>";
    page += "body{font-family:Arial;text-align:center;background:#f2f2f2;}";
    page += ".card{background:white;padding:20px;margin:20px auto;width:300px;border-radius:10px;}";
    page += ".status{font-size:20px;font-weight:bold;color:red;}";
    page += "</style></head><body>";

    page += "<h1>Fall Detection System</h1>";
    page += "<div class='card'>";
    page += "<p class='status'>Status: " + statusText + "</p>";
    page += "<p>Latitude: " + latText + "</p>";
    page += "<p>Longitude: " + lonText + "</p>";
    page += "</div>";

    page += "</body></html>";

    server.send(200, "text/html", page);
  });

  server.begin();
}

// -------- LOOP --------
void loop() {

  server.handleClient();

  // -------- SENSOR --------
  int16_t ax_raw, ay_raw, az_raw;
  int16_t gx_raw, gy_raw, gz_raw;

  mpu.getMotion6(&ax_raw, &ay_raw, &az_raw,
                 &gx_raw, &gy_raw, &gz_raw);

  float ax = ax_raw / 4096.0;
  float ay = ay_raw / 4096.0;
  float az = az_raw / 4096.0;

  float gx = gx_raw / 131.0;
  float gy = gy_raw / 131.0;
  float gz = gz_raw / 131.0;

  float mag = sqrt(ax*ax + ay*ay + az*az);

  // -------- GPS READ --------
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {

    lastLat = gps.location.lat();
    lastLon = gps.location.lng();

    latText = String(lastLat, 6);
    lonText = String(lastLon, 6);

    hasValidGPS = true;

  } else {

    if (hasValidGPS) {
      latText = String(lastLat, 6);
      lonText = String(lastLon, 6);
    } else {
      latText = "N/A";
      lonText = "N/A";
    }
  }

  // -------- FALL DETECTION --------
  static bool freeFall = false;
  static bool impact = false;
  static unsigned long impactTime = 0;

  if (mag < 0.5) freeFall = true;

  if (freeFall && mag > 3.0) {
    impact = true;
    impactTime = millis();
  }

  if (impact && millis() - impactTime > 3000) {

    compute_features();
    int prediction = rf_predict(features);

    if (prediction == 1) {

      statusText = "FALL DETECTED";
      fallDisplayTime = millis();

      if (!smsSent) {

        String msg = "FALL DETECTED!\n";

        if (hasValidGPS) {

          String mapLink = "https://www.google.com/maps?q=" + latText + "," + lonText;

          if (gps.location.isValid()) {
            msg += "Live Location:\n";
          } else {
            msg += "Last Known Location:\n";
          }

          msg += "Lat: " + latText + "\n";
          msg += "Lon: " + lonText + "\n";
          msg += mapLink;

        } else {
          msg += "Location not available";
        }

        sendSMS(msg);
        smsSent = true;
      }

    } else {
      statusText = "Normal Activity";
      smsSent = false;
    }

    freeFall = false;
    impact = false;
    index_sample = 0;
  }

  if (statusText == "FALL DETECTED" && millis() - fallDisplayTime > 5000) {
    statusText = "Normal Activity";
    smsSent = false;
  }

  // -------- STORE DATA --------
  ax_buf[index_sample] = ax;
  ay_buf[index_sample] = ay;
  az_buf[index_sample] = az;

  gx_buf[index_sample] = gx;
  gy_buf[index_sample] = gy;
  gz_buf[index_sample] = gz;

  index_sample++;
  if (index_sample >= WINDOW_SIZE) index_sample = 0;

  // -------- UI CONTROL --------
  if (millis() - lastUIUpdate > UI_REFRESH_INTERVAL) {
    lastUIUpdate = millis();
  }

  delay(10);
  yield();
}

// -------- SMS FUNCTION --------
void sendSMS(String message) {

  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String url = "https://api.twilio.com/2010-04-01/Accounts/" + accountSID + "/Messages.json";

    String postData = "To=" + toNumber +
                      "&From=" + fromNumber +
                      "&Body=" + message;

    http.begin(client, url);
    http.setAuthorization(accountSID.c_str(), authToken.c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    Serial.print("Twilio Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}

// -------- FEATURE EXTRACTION --------
void compute_features() {

  int idx = 0;

  calc_stats(ax_buf, &features[idx]); idx += 4;
  calc_stats(ay_buf, &features[idx]); idx += 4;
  calc_stats(az_buf, &features[idx]); idx += 4;

  calc_stats(gx_buf, &features[idx]); idx += 4;
  calc_stats(gy_buf, &features[idx]); idx += 4;
  calc_stats(gz_buf, &features[idx]);
}

// -------- STATS --------
void calc_stats(float *buffer, float *out) {

  float sum = 0;
  float max_val = buffer[0];
  float min_val = buffer[0];

  for (int i = 0; i < WINDOW_SIZE; i++) {
    sum += buffer[i];
    if (buffer[i] > max_val) max_val = buffer[i];
    if (buffer[i] < min_val) min_val = buffer[i];
  }

  float mean = sum / WINDOW_SIZE;

  float variance = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) {
    variance += (buffer[i] - mean) * (buffer[i] - mean);
  }

  float std = sqrt(variance / WINDOW_SIZE);

  out[0] = mean;
  out[1] = std;
  out[2] = max_val;
  out[3] = min_val;
}