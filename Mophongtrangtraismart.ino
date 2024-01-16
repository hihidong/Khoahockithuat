#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BH1750.h>

#define DHTPIN 2 //Chân D2 kết nối với cảm biến DHT
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

BH1750 lightMeter(0x23); 
LiquidCrystal_I2C lcd(0x27, 20, 4); // Khai báo địa chỉ LCD và kích thước LCd(20x4)

const int soilMoisturePin = A0;  //Chân A0 kết nối với cảm biến độ ẩm
const int relayPin = 7;          // Chân D7 kết nối với IN2 của relay (đèn)
const int ledPin1 = 3;           // Chân D3 kết nối với đèn LED của đèn
const int ledPin2 = 4;           // Chân D4 kết nối với đèn Led của máy bơm
const int pumpRelayPin = 8;      // Chân D8 kết nối với IN4 của relay (máy bơm)
const int manualButtonPin1 = 11; // Chân D11 kết nối với nút bấm thủ công của đèn
const int manualButtonPin2 = 12; // Chân D12 kết nối với nút bấm thủ công của máy bơm 
const int lcdButtonPin = 13;     // Chân D13 kết nối với nút bấm thủ công của màn hình LCD
const int lcdControlPin = A4;    // Chân A4 SCL kết nối với màn hình LCD

int pushButton1State = HIGH;      // Trạng thái nút bấm đèn
int pushButton2State = HIGH;      // Trạng thái nút bấm máy bơm
int pushButtonState = HIGH;       // Trạng thái nút bấm

bool manualModeLight = false;    // Chế độ thủ công đèn
bool manualModePump = false;     // Chế độ thủ công máy bơm

bool relayState = false;         // Trạng thái của relay đèn (true - bật, false - tắt)
bool pumpState = false;          // Trạng thái của relay máy bơm (true - bật, false - tắt)

bool lcdState = false;    //Trạng thái màn hình LCD (true - on, false - off)

void setup() {
  Serial.begin(115200);
  dht.begin();    //Khời động cảm biến 
  lightMeter.begin();   //Khởi động cảm biến

  pinMode(relayPin, OUTPUT);
  pinMode(pumpRelayPin, OUTPUT);
  pinMode(lcdButtonPin, INPUT);
  pinMode(manualButtonPin1, INPUT);
  pinMode(manualButtonPin2, INPUT);
  pinMode(lcdControlPin, OUTPUT);

  lcd.init();
  lcd.backlight();  //Bật đèn nền của LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Data:");
  delay(2000);
}

// Đọc dữ liệu  nhiệt độ cảm biến DHT
float readTemperature() {
  return dht.readTemperature();
}
//Đọc dữ liệu độ ẩm không khí cảm biến DHT
float readHumidity() {
  return dht.readHumidity();
}
//Đọc dữ liệu cảm biến độ ẩm đất
int readSoilMoisture() {
  int value = analogRead(soilMoisturePin);
  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1; // Ensure value is within 0-100 range
  return value;
}
//Đọc dữ liệu cảm biến BH1750
float readLightIntensity() {
  return lightMeter.readLightLevel();
}
//Gửi dữ liệu qua ESP
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
//Hiển thị lên màn hình lcd
void displaySensorData(float temperature, float humidity, int soilMoisture, float lightIntensity) {
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(temperature);
  lcd.print(" *C");

  lcd.setCursor(0, 1);
  lcd.print("Humid:");
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(0, 2);
  lcd.print("Soil:");
  lcd.print(soilMoisture);
  lcd.print("%");

  lcd.setCursor(0, 3);
  lcd.print("Light: ");
  lcd.print(lightIntensity);
  lcd.print(" lux");
}
// Điều khiển nút bấm vật lý
void checkPhysicalButtons() {
  // Nút bấm đèn
  if (digitalRead(manualButtonPin1) == LOW) {
    if (pushButton1State != LOW) {
      manualModeLight = !manualModeLight;
      digitalWrite(relayPin, manualModeLight ? HIGH : LOW);
    }
    pushButton1State = LOW;
  } else {
    pushButton1State = HIGH;
  }

  // Nút bấm máy bơm
  if (digitalRead(manualButtonPin2) == LOW) {
    if (pushButton2State != LOW) {
      manualModePump = !manualModePump;
      digitalWrite(pumpRelayPin, manualModePump ? HIGH : LOW);
    }
    pushButton2State = LOW;
  } else {
    pushButton2State = HIGH;
  }

  // Nút bấm màn hình LCD
  if (digitalRead(lcdControlPin) == LOW) {
    if (pushButtonState != LOW) {
      lcdState = !lcdState;
    }
    pushButtonState = LOW;
  } else {
    pushButtonState = HIGH;
  }
}

void loop() {
  float temperature = readTemperature();
  float humidity = readHumidity();
  int soilMoisture = readSoilMoisture();
  float lightIntensity = readLightIntensity();

  checkPhysicalButtons();

  //Hiển thị hoặc ẩn màn hình LCD tùy thuộc vào trạng thái của lcdState
  if (lcdState) {
    displaySensorData(temperature, humidity, soilMoisture, lightIntensity); //Gọi hàm hiện thị dữ liệu
  } else {
    lcd.clear();  //Nếu tắt màn hình, xóa dữ liệu hiển thị
  }

   // Nếu đang ở chế độ tự động và độ ẩm đất thấp, bật máy bơm và đèn LED
  if (!manualModePump && soilMoisture < 40) {
    digitalWrite(pumpRelayPin, HIGH);  // Bật relay máy bơm
    pumpState = true;
    digitalWrite(ledPin1, HIGH);        // Bật đèn LED của đèn
  } else {
    digitalWrite(pumpRelayPin, LOW);   // Tắt relay máy bơm
    pumpState = false;
    digitalWrite(ledPin1, LOW);         // Tắt đèn LED của đèn
  }

  // Nếu đang ở chế độ tự động và cường độ ánh sáng thấp, bật đèn và đèn led
  if (!manualModeLight && lightIntensity < 500) {
    digitalWrite(relayPin, HIGH);
    relayState = true;
    digitalWrite(ledPin2, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
    relayState = false;
    digitalWrite(ledPin2, LOW);
  }

  displaySensorData(temperature, humidity, soilMoisture, lightIntensity);
  sendDataToESP(temperature, humidity, soilMoisture, lightIntensity);

  delay(5000); // Adjust delay as needed
}
