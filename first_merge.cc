#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid      = "Nokia";
const char* password  = "noob1234";
const char* serverUrl = "http://172.20.10.14:5000/upload";

// I/O pins
const int MOTOR_VENSTRE = 12; 
const int MOTOR_MIDT    = 13;
const int MOTOR_HOYRE   = 2;  
const int BUTTON_PIN    = 4;  // Knappen på blits-pinnen

// Kamera Pins
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// Global:
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData;
bool isUploading = false; 

const int MIN_DIST = 300;
const int MAX_DIST = 1000;

hw_timer_t * timer = NULL;
volatile bool timerFlag = false;

void IRAM_ATTR onTimer() { timerFlag = true; }

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk      = XCLK_GPIO_NUM;
  config.pin_pclk      = PCLK_GPIO_NUM;
  config.pin_vsync     = VSYNC_GPIO_NUM;
  config.pin_href      = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset     = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Kamera feilet!");
    while (true) delay(1000);
  }
}

void sendImage() {
  isUploading = true; 
  
  // Slå av motorer for å gi all strøm/fokus til WiFi
  analogWrite(MOTOR_VENSTRE, 0);
  analogWrite(MOTOR_MIDT, 0);
  analogWrite(MOTOR_HOYRE, 0);

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { isUploading = false; return; }
  
  HTTPClient http;
  http.begin(serverUrl);
  http.setTimeout(15000);

  String boundary = "boundary123";
  String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();
  uint8_t* buf = (uint8_t*)malloc(totalLen);
  
  if (buf) {
    memcpy(buf, head.c_str(), head.length());
    memcpy(buf + head.length(), fb->buf, fb->len);
    memcpy(buf + head.length() + fb->len, tail.c_str(), tail.length());

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    int res = http.POST(buf, totalLen);
    Serial.printf("Opplasting ferdig, kode: %d\n", res);
    free(buf);
  }

  esp_camera_fb_return(fb);
  http.end();
  isUploading = false; 
}

void setup() {
  Serial.begin(115200);
  
  pinMode(MOTOR_VENSTRE, OUTPUT);
  pinMode(MOTOR_MIDT, OUTPUT);
  pinMode(MOTOR_HOYRE, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Bruker intern pullup på pin 4

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  initCamera();

  Wire.begin(15, 14);
  Wire.setClock(400000); 
  if (myImager.begin() == false) {
    while (1) delay(1000);
  }
  myImager.setResolution(8 * 8);
  myImager.setRangingFrequency(15);
  myImager.startRanging();

  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 66667, true, 0); 
}

void loop() {
  // 1. ToF Prosessering (Pauses under bilde-sending)
  if (timerFlag && !isUploading) {
    timerFlag = false; 

    if (myImager.isDataReady() && myImager.getRangingData(&measurementData)) {
      int countV = 0, countM = 0, countH = 0;

      for (int i = 0; i < 64; i++) {
        int dist = measurementData.distance_mm[i];
        int status = measurementData.target_status[i];

        if ((status == 5 || status == 9) && dist >= MIN_DIST && dist <= MAX_DIST) {
          int rad = i / 8; 
          if (rad <= 1)      countV++;
          else if (rad >= 6) countH++;
          else               countM++;
        }
      }

      analogWrite(MOTOR_VENSTRE, map(countV, 0, 16, 0, 255));
      analogWrite(MOTOR_MIDT,    map(countM, 0, 32, 0, 255));
      analogWrite(MOTOR_HOYRE,   map(countH, 0, 16, 0, 255));
    }
  }

  // 2. Knappesjekk (Pin 4 er LOW når knappen trykkes)
  if (digitalRead(BUTTON_PIN) == LOW && !isUploading) {
    delay(100); // Litt lengre debounce for Pin 4
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Knapp trykket - tar bilde...");
      sendImage();
    }
  }
}
