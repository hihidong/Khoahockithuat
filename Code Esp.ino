#define BLYNK_TEMPLATE_ID "TMPL6fPT-Rkrm"
#define BLYNK_TEMPLATE_NAME "Mô phỏng trang trại thông minh"
#define BLYNK_AUTH_TOKEN "LGCzrnRa0rm25rTmJHdZx3nl3r4qw_fG"
#include <Wire.h>
#include <BlynkSimpleEsp8266.h>
#include <Bounce2.h>

char ssid[] = "Dongpro";
char pass[] = "dua50ngan";

WidgetLED PUMP(V3);  // Đèn trạng thái bơm
WidgetLED LAMP(V4);  // Đèn trạng thái đèn

int SoilMoisture = 0;
float lightIntensity = 0;
float Humidity = 0;
float Temperature = 0;
#define DRY_SOIL_THRESHOLD 60  // Giả sử ngưỡng độ ẩm đất thấp là 50
#define LIGHT_THRESHOLD 400     // Giả sử ngưỡng độ sáng thấp là 400

// Pin cho pump
const int pumpPin = D5;
const int pumpButtonPin = D3;
const int pumpStatusLed = D4;
int pumpStatus = 0;
unsigned long pumpTimerStart = 0;  // Thêm biến để lưu thời gian bắt đầu bơm
bool manualControl = false;  // Flag để kiểm soát bằng Blynk

// Pin cho lamp
const int lampPin = D6;
const int lampButtonPin = D7;
const int lampStatusLed = D8;
int lampStatus = 0;

// Thời gian bơm (10 giây)
const unsigned long pumpDuration = 10000;
unsigned long pumpStartTime = 0;

// Biến đếm số lần nút được nhấn
int pumpButtonClickCount = 0;
int lampButtonClickCount = 0;

// Tạo đối tượng Bounce cho nút bấm pump và lamp
Bounce pumpButton = Bounce();
Bounce lampButton = Bounce();

void setup() {
    Serial.begin(115200);
    pinMode(pumpPin, OUTPUT);
    pinMode(lampPin, OUTPUT);
    pinMode(pumpButtonPin, INPUT_PULLUP);
    pinMode(lampButtonPin, INPUT_PULLUP);
    pinMode(pumpStatusLed, OUTPUT);
    pinMode(lampStatusLed, OUTPUT);
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    Blynk.syncAll();  // Synchronize hardware with app state

    // Tắt pump và lamp khi khởi động
    digitalWrite(pumpPin, LOW);
    digitalWrite(lampPin, LOW);
    updateLedStatus();
    pumpButton.attach(pumpButtonPin);
    pumpButton.interval(20);  // 20 milliseconds debounce time
    lampButton.attach(lampButtonPin);
    lampButton.interval(20);

    // Map Blynk Virtual Pins
    Blynk.virtualWrite(V3, pumpStatus); // Virtual Pin for Pump LED
    Blynk.virtualWrite(V4, lampStatus); // Virtual Pin for Lamp LED
}
void loop() {
    Blynk.run();
    updateLedStatus();
    checkPhysicalButton();

    if (Serial.available() > 0) {
        String data = Serial.readStringUntil('\n');
        Serial.println("Received data: " + data);

        Temperature = getValue(data, 'T');
        Humidity = getValue(data, 'H');
        SoilMoisture = getValue(data, 'S');
        lightIntensity = getValue(data, 'L');

        Blynk.virtualWrite(V0, Temperature);
        Blynk.virtualWrite(V1, Humidity);
        Blynk.virtualWrite(V2, SoilMoisture);

        Serial.print("PumpStatus:");
        Serial.println(pumpStatus);
        Serial.print("LampStatus:");
        Serial.println(lampStatus);
        delay(500);
    }
}

float getValue(String data, char identifier) {
    int index = data.indexOf(identifier) + 2;
    String valueString = data.substring(index);
    float value = valueString.toFloat();
    Serial.println("Identifier: " + String(identifier) + ", Value: " + value);
    return value;
}

void updateLedStatus() {
    digitalWrite(pumpStatusLed, digitalRead(pumpPin));
    digitalWrite(lampStatusLed, digitalRead(lampPin));
}

void checkPhysicalButton() {
    pumpButton.update();
    if (pumpButton.fell()) {
        manualControl = true;  // Set manual control flag
        // Nếu nút được nhấn, tăng biến đếm lên
        pumpButtonClickCount++;
        if (pumpButtonClickCount % 2 == 1) {
            // Nếu số lần nhấn là lẻ, bật pump và bắt đầu thời gian bơm
            pumpStartTime = millis();
            digitalWrite(pumpPin, HIGH);
            Blynk.virtualWrite(V4, HIGH);  // Update Blynk App state
        } else {
            // Nếu số lần nhấn là chẵn, tắt pump
            digitalWrite(pumpPin, LOW);
            pumpStartTime = 0;
            Blynk.virtualWrite(V4, LOW);  // Update Blynk App state
        }
    }

    lampButton.update();
    if (lampButton.fell()) {
        manualControl = true;  // Set manual control flag
        lampButtonClickCount++;
        if (lampButtonClickCount % 2 == 1) {
            digitalWrite(lampPin, HIGH);
            Blynk.virtualWrite(V3, HIGH);  // Update Blynk App state
        } else {
            digitalWrite(lampPin, LOW);
            Blynk.virtualWrite(V3, LOW);  // Update Blynk App state
        }
    }

    if (pumpStartTime > 0 && millis() - pumpStartTime >= pumpDuration) {
        digitalWrite(pumpPin, LOW);
        pumpStartTime = 0;
        manualControl = false;  // Reset manual control flag
        Blynk.virtualWrite(V4, LOW);  // Update Blynk App state
    }
    updateLedStatus();
}

void checkAutoControl() {
    // Auto control based on light and soil moisture thresholds
    if (lightIntensity < LIGHT_THRESHOLD && SoilMoisture < DRY_SOIL_THRESHOLD) {
        // Turn on pump and lamp automatically
        digitalWrite(pumpPin, HIGH);
        digitalWrite(lampPin, HIGH);
    } else {
        // Turn off pump and lamp automatically
        digitalWrite(pumpPin, LOW);
        digitalWrite(lampPin, LOW);
    }
}

BLYNK_WRITE(V3) {  // Blynk App changes Pump button state
    int pumpControl = param.asInt();
    if (pumpControl == HIGH) {
        // Turn on pump only if not in manual control
        if (!manualControl) {
            pumpStartTime = millis();
            digitalWrite(pumpPin, HIGH);
            Blynk.virtualWrite(V5, HIGH);  // Update Blynk App state
        }
    } else {
        // Turn off pump
        digitalWrite(pumpPin, LOW);
        pumpStartTime = 0;
        Blynk.virtualWrite(V5, LOW);  // Update Blynk App state
    }
    updateLedStatus();
}

BLYNK_WRITE(V4) {  // Blynk App changes Lamp button state
    int lampControl = param.asInt();
    if (lampControl == HIGH) {
        // Turn on lamp only if not in manual control
        if (!manualControl) {
            digitalWrite(lampPin, HIGH);
            Blynk.virtualWrite(V6, HIGH);  // Update Blynk App state
        }
    } else {
        // Turn off lamp
        digitalWrite(lampPin, LOW);
        Blynk.virtualWrite(V6, LOW);  // Update Blynk App state
    }
    updateLedStatus();
}