#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include "GM65_scanner.h"

#define CAMERA_MODEL_AI_THINKER 

#include "camera_pins.h"

const char* ssid = "S216_0526";
const char* password = "phong0526";

const char *mqtt_broker = "f42bfb9b.ala.us-east-1.emqxsl.com";
const char *mqtt_username = "gelcard";
const char *mqtt_password = "gelcard";
const int mqtt_port = 8883;

const char* topic_barcode_req = "barcode/request";
const char* topic_barcode_res = "barcode/response";
const char* topic_wifi = "wifi/request";

IPAddress local_IP(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   
IPAddress secondaryDNS(8, 8, 4, 4); 

#define QR Serial1

TaskHandle_t Task1;

#define BUTTON_PIN 2

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

const char* ca_cert= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

volatile bool isScanning = false;

void startCameraServer();
void setupLedFlash(int pin);
void connectWifi();
void connectMQTT();
void checkConnections();

GM65_scanner gm65(&QR);

bool check = false;

void IRAM_ATTR buttonInterrupt() {
  // xTaskCreatePinnedToCore(loop2_escucha_escaner_qr, "Task1", 10000, NULL, 3, &Task1, 0);
}

void loop2_escucha_escaner_qr(void *pvParameters) {
  for (;;) {

    if (QR.available() > 0) {
      String barcode = "";

      while (QR.available()) {
        char input = QR.read();
        barcode += input;
        delay(5);
      }
      barcode.trim();

      if (!barcode.isEmpty()) {
        mqttClient.publish(topic_barcode_res, barcode.c_str());
        String ipAddress = WiFi.localIP().toString();
        mqttClient.publish(topic_wifi, ipAddress.c_str());
      }
    }
    delay(5);
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  String receivedPayload(reinterpret_cast<char*>(payload), length);
  Serial.print("Received message from topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(receivedPayload);
  
  if (String(topic) == topic_barcode_req && receivedPayload == "barcode") {
    mqttClient.publish(topic_barcode_res, receivedPayload.c_str());
    xTaskCreatePinnedToCore(loop2_escucha_escaner_qr, "Task1", 10000, NULL, 3, &Task1, 0);
  }
}

void connectWifi(){
  // WiFiManager wm;
  // bool res;
  // wm.resetSettings();
  // res = wm.autoConnect("AutoConnectAP","password");

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif


  connectWifi();

  connectMQTT();

  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, RISING);

  QR.begin(9600, SERIAL_8N1, 1, 3);
  xTaskCreatePinnedToCore(loop2_escucha_escaner_qr, "Task1", 10000, NULL, 3, &Task1, 0);
}

void connectMQTT(){
  wifiClient.setCACert(ca_cert);
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(callback);
  while (!mqttClient.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (mqttClient.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        Serial.println("Public EMQX MQTT broker connected");
    } else {
        Serial.print("failed with state ");
        Serial.print(mqttClient.state());
        delay(2000);
    }
  }

  mqttClient.publish(topic_barcode_res, "Hi, I'm ESP32 ^^ - Barcode");
  mqttClient.subscribe(topic_barcode_req);
}

void checkConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectWifi();
  }

  if (!mqttClient.connected()) {
    Serial.println("MQTT connection lost. Reconnecting...");
    connectMQTT();
  }
}

void loop() {
  checkConnections();
  mqttClient.loop();
}