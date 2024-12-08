#include <WiFi.h>
#include "esp_camera.h"

// Define the camera model's pins (AI-Thinker)
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "ESP32-Access-Point"; // SSID for the Access Point
const char* password = "123456789"; // Password for the Access Point

#define LED_GPIO_NUM     4 // LED pin

WiFiServer server(80);

bool ledState = false; // Track LED state
bool streamActive = false; // Track stream state

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Start the Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Configure LED pin
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW); // Ensure the LED is off initially

  // Camera configuration
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  server.begin();
}

void handleLEDControl(WiFiClient& client) {
  ledState = !ledState; // Toggle LED state
  digitalWrite(LED_GPIO_NUM, ledState ? HIGH : LOW);

  // Send response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.printf("{\"led\": %s}", ledState ? "true" : "false");
}

void handleStreamControl(WiFiClient& client) {
  // Toggle stream state
  streamActive = !streamActive;

  // Send response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.printf("{\"stream\": %s}", streamActive ? "started" : "stopped");
}

void streamCamera(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Connection: keep-alive");
  client.println();

  while (client.connected() && streamActive) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    client.printf("--frame\r\n");
    client.printf("Content-Type: image/jpeg\r\n\r\n");
    size_t written = client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (written != fb->len) {
      Serial.println("Frame not fully sent");
      break;
    }

    client.printf("\r\n");
    delay(100);
  }
  client.stop();
  Serial.println("Client disconnected");
}

void handleRootPage(WiFiClient& client) {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 Camera Control</title>
      <style>
          body {
              font-family: Arial, sans-serif;
              text-align: center;
              background-color: #f0f0f0;
          }
          .container {
              margin-top: 50px;
          }
          .controls button {
              padding: 10px 20px;
              font-size: 16px;
              margin: 10px;
              cursor: pointer;
          }
          #stream {
              max-width: 100%;
              margin-top: 20px;
              border: 2px solid #ccc;
              border-radius: 10px;
          }
      </style>
      <script>
          // jQuery Code Embedded Directly for Offline Usage
          var $ = function(id) { return document.getElementById(id); };

          window.onload = function() {
              let streamActive = false;
              let ledStatus = false;
              const baseUrl = "http://192.168.4.1"; // IP Address of ESP32

              $('toggleStreamBtn').onclick = function() {
                  if (streamActive) {
                      $('stream').src = '';  // Stop stream
                      $('toggleStreamBtn').innerText = 'Start Stream';
                      streamActive = false;
                  } else {
                      $('stream').src = `${baseUrl}/start_stream`;  // Start stream
                      $('toggleStreamBtn').innerText = 'Stop Stream';
                      streamActive = true;
                  }
              };

              $('toggleLedBtn').onclick = function() {
                  ledStatus = !ledStatus;
                  const ledUrl = ledStatus ? `${baseUrl}/led=true` : `${baseUrl}/led=false`;

                  var xhr = new XMLHttpRequest();
                  xhr.open('GET', ledUrl, true);
                  xhr.onload = function() {
                      const ledText = ledStatus ? 'Turn LED Off' : 'Turn LED On';
                      $('toggleLedBtn').innerText = ledText;
                  };
                  xhr.send();
              };
          };
      </script>
  </head>
  <body>
      <div class="container">
          <div class="stream-container">
              <h2>ESP32 Camera Stream</h2>
              <img id="stream" src="" alt="Camera Stream">
          </div>

          <div class="controls">
              <button id="toggleStreamBtn">Start Stream</button>
              <button id="toggleLedBtn">Turn LED On</button>
          </div>
      </div>
  </body>
  </html>
  )rawliteral";

  client.println(html);
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/led") >= 0) {
    handleLEDControl(client);
  } else if (request.indexOf("/start_stream") >= 0) {
    streamActive = true;
    streamCamera(client);
  } else if (request.indexOf("/stop_stream") >= 0) {
    streamActive = false;
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"stream\": \"stopped\"}");
  } else if (request.indexOf("/") >= 0) {
    handleRootPage(client);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
}
