#include "WiFi.h"
#include "HTTPClient.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// Wi-Fi
const char *ssid = "Group32";
const char *password = "Group32Password";

// Pins
#define WIFI_PIN 5
#define POWER_PIN 25
#define LED_PIN 17

#define UP_PIN 18
#define RIGHT_PIN 21
#define DOWN_PIN 23
#define LEFT_PIN 19
#define LEFT_BUMPER_PIN 16
#define RIGHT_BUMPER_PIN 22

#define VRX_PIN_1 26
#define VRY_PIN_1 27
#define VRX_PIN_2 14
#define VRY_PIN_2 12

// Timing
const unsigned long wifiHoldTime = 1500;
const unsigned long powerHoldTime = 1500;
const unsigned long joystickInterval = 100;

// States
bool wifiActionTaken = false;
bool powerActionTaken = false;
unsigned long wifiPressStart = 0;
unsigned long powerPressStart = 0;
unsigned long lastJoystickCheck = 0;

// Joystick values
int xValue1 = 0, yValue1 = 0, xValue2 = 0, yValue2 = 0;

struct Button {
  const char* name;
  int pin;
  int index;
  bool lastState;
};

Button buttons[] = {
  {"UP", UP_PIN, 0, HIGH},
  {"RIGHT", RIGHT_PIN, 1, HIGH},
  {"DOWN", DOWN_PIN, 2, HIGH},
  {"LEFT", LEFT_PIN, 3, HIGH},
  {"LEFT_BUMPER", LEFT_BUMPER_PIN, 4, HIGH},
  {"RIGHT_BUMPER", RIGHT_BUMPER_PIN, 5, HIGH}
};

// Function declarations
void connectToWiFi();
void reconnectWifi();
void handleWifiButton(unsigned long now);
void handlePowerButton(unsigned long now);
void handleButtons();
void buttonPressed(int index);
void handleJoystick();
void sendJoystickInfo();
void flashAndSleep();
void adc_power_off();

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(WIFI_PIN, INPUT_PULLUP);
  pinMode(POWER_PIN, INPUT_PULLUP);

  for (int i = 0; i < 6; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  pinMode(VRX_PIN_1, INPUT);
  pinMode(VRY_PIN_1, INPUT);
  pinMode(VRX_PIN_2, INPUT);
  pinMode(VRY_PIN_2, INPUT);

  // Setup wake from deep sleep
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_PIN, 0);

  delay(500);
  connectToWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  handleWifiButton(currentMillis);
  handlePowerButton(currentMillis);
  handleButtons();
  handleJoystick();

  if (currentMillis - lastJoystickCheck >= joystickInterval) {
    sendJoystickInfo();
    lastJoystickCheck = currentMillis;
  }
}

void handleJoystick() {
  xValue1 = analogRead(VRX_PIN_1);
  yValue1 = analogRead(VRY_PIN_1);
  xValue2 = analogRead(VRX_PIN_2);
  yValue2 = analogRead(VRY_PIN_2);
}

void handleButtons() {
  for (int i = 0; i < 6; i++) {
    bool current = digitalRead(buttons[i].pin);
    if (buttons[i].lastState == HIGH && current == LOW) {
      Serial.printf("%s pressed!\n", buttons[i].name);
      buttonPressed(buttons[i].index);
    }
    buttons[i].lastState = current;
  }
}

void handleWifiButton(unsigned long now) {
  bool wifiPressed = digitalRead(WIFI_PIN) == LOW;

  if (wifiPressed) {
    if (!wifiActionTaken) {
      if (wifiPressStart == 0) {
        wifiPressStart = now;
      } else if (now - wifiPressStart >= wifiHoldTime) {
        Serial.println("WiFi button held - reconnecting...");
        reconnectWifi();
        wifiActionTaken = true;
      }
    }
  } else {
    wifiPressStart = 0;
    wifiActionTaken = false;
  }
}

void handlePowerButton(unsigned long now) {
  bool powerPressed = digitalRead(POWER_PIN) == LOW;

  if (powerPressed) {
    if (!powerActionTaken) {
      if (powerPressStart == 0) {
        powerPressStart = now;
      } else if (now - powerPressStart >= powerHoldTime) {
        Serial.println("Power button held - entering deep sleep...");
        flashAndSleep();
        powerActionTaken = true;
      }
    }
  } else {
    powerPressStart = 0;
    powerActionTaken = false;
  }
}

void reconnectWifi() {
  WiFi.disconnect(true);
  delay(100);
  connectToWiFi();
}

void connectToWiFi() {
  Serial.println("Attempting WiFi connection...");
  
  // Add WiFi event handlers
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[WiFi] Event: %d - ", event);
    switch(event) {
      case SYSTEM_EVENT_STA_START:
        Serial.println("STA Started");
        break;
      case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("Connected to AP");
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("Disconnected from AP");
        WiFi.reconnect();
        break;
    }
  });

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // More robust connection check
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttemptTime < 20000) { // 20s timeout
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    WiFi.setSleep(false); // Disable sleep for better stability
    Serial.println("\nConnected to Rover");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("\nFailed to connect - restarting");
    delay(1000);
    ESP.restart();
  }
}

void flashAndSleep() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }

  adc_power_off();
  digitalWrite(LED_PIN, LOW);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);  
  delay(100);

  esp_deep_sleep_start();
}

void buttonPressed(int index) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://192.168.4.1/buttonpress");
    http.addHeader("Content-Type", "application/json");
    http.POST("{\"buttonIndex\": " + String(index) + "}");
    http.end();
  }
}

void sendJoystickInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://192.168.4.1/controllerdata");
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"joystickX1\": " + String(xValue1) +
                      ", \"joystickY1\": " + String(yValue1) +
                      ", \"joystickX2\": " + String(xValue2) +
                      ", \"joystickY2\": " + String(yValue2) + "}";
    Serial.println("X1: " + String(xValue1));
    Serial.println("Y1: " + String(yValue1));
    Serial.println("X2: " + String(xValue2));
    Serial.println("Y2: " + String(yValue2));
    int httpResponseCode = http.POST(jsonData);
    http.end();
  }
}

void adc_power_off() {
  rtc_gpio_isolate(GPIO_NUM_26);
  rtc_gpio_isolate(GPIO_NUM_27);
  rtc_gpio_isolate(GPIO_NUM_14);
  rtc_gpio_isolate(GPIO_NUM_12);
}