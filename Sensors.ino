#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BH1750.h>
#include <Wire.h>

#define DHTPIN 2      // Chân kết nối cảm biến DHT22
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
const int soilMoisturePin = A0;  // Chân A0 kết nối với cảm biến độ ẩm
BH1750 lightMeter(0x23);
LiquidCrystal_I2C lcd(0x27, 20, 4);
int receivedPumpStatus = 0;
int receivedLampStatus = 0;

void setup() {
  Serial.begin(115200);  // Baud rate ở đây phải khớp với ESP8266
  dht.begin();
  lightMeter.begin();   // Khởi động cảm biến
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Data:");
}
float readTemperature() {
  return dht.readTemperature();
}

float readHumidity() {
  return dht.readHumidity();
}

int readSoilMoisture() {
  int value = analogRead(soilMoisturePin);
  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1;
  return value;
}

float readLightIntensity() {
  return lightMeter.readLightLevel();
}
void displaySensorData(float temperature, float humidity, int soilMoisture, float lightIntensity) {
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(temperature);
  lcd.print("*C");

  lcd.setCursor(10, 1);
  lcd.print("H:");
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(0, 2);
  lcd.print("S:");
  lcd.print(soilMoisture);
  lcd.print("%");

  lcd.setCursor(10, 2);
  lcd.print("L:");
  lcd.print(lightIntensity);
  lcd.print("lux");
}
void sendDataToESP(float temperature, float humidity, int soilMoisture, float lightIntensity) {
  Serial.print("T:");
  Serial.print(temperature);
  Serial.print("H:");
  Serial.print(humidity);
  Serial.print("S:");
  Serial.print(soilMoisture);
  Serial.print("L:");
  Serial.println(lightIntensity);
}
void loop() {
  float temperature = readTemperature();
  float humidity = readHumidity();
  int soilMoisture = readSoilMoisture();
  float lightIntensity = readLightIntensity();
  if (Serial.available() > 0) {
        String data = Serial.readStringUntil('\n');
        Serial.println("Received data: " + data);

        if (data.startsWith("PumpStatus:")) {
            receivedPumpStatus = data.substring(12).toInt();
        } else if (data.startsWith("LampStatus:")) {
            receivedLampStatus = data.substring(12).toInt();
        }
    }

    // Hiển thị trạng thái bơm và đèn trên LCD
    lcd.setCursor(0, 3);
    lcd.print("P: ");
    lcd.print(receivedPumpStatus == HIGH ? "ON " : "OFF");

    lcd.setCursor(10, 3);
    lcd.print("Lamp: ");
    lcd.print(receivedLampStatus == HIGH ? "ON " : "OFF");

  sendDataToESP(temperature, humidity, soilMoisture, lightIntensity);
  displaySensorData(temperature, humidity, soilMoisture, lightIntensity);
  delay(5000);
}
  

