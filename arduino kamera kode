#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid      = "Nokia";
const char* password  = "noob1234";
const char* serverUrl = "http://172.20.10.14:5000/upload";
const int   BUTTON_PIN = 13;

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

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init FAILED");
    while (true) delay(1000);
  }
  Serial.println("Camera ready");
}

void sendImage() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { Serial.println("Capture failed"); return; }
  Serial.printf("Captured %u bytes\n", fb->len);

  HTTPClient http;
  http.begin(serverUrl);
  http.setTimeout(10000);

  String boundary = "boundary123";
  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();
  uint8_t* buf = (uint8_t*)malloc(totalLen);
  if (!buf) { Serial.println("malloc failed"); esp_camera_fb_return(fb); return; }

  memcpy(buf, head.c_str(), head.length());
  memcpy(buf + head.length(), fb->buf, fb->len);
  memcpy(buf + head.length() + fb->len, tail.c_str(), tail.length());

  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
  int code = http.POST(buf, totalLen);

  Serial.printf("Server response: %d\n", code);
  if (code == 200) Serial.println("Image sent successfully!");
  else Serial.println(http.getString());

  free(buf);
  esp_camera_fb_return(fb);
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initCamera();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  Serial.println("Press button to capture and send.");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      sendImage();
      delay(2000);
    }
  }
}
