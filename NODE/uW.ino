#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// Cài đặt thông số LoRa
#define SCK     PA5
#define MISO    PA6
#define MOSI    PA7
#define SS      PA4
#define RST     PB0
#define DIO0    PB1
#define TSS PA1
#define TdsSensorPin PA2 
#define ONE_WIRE_BUS PA3  
#define RELAY1_PIN PB6  // Chân relay 1
#define RELAY2_PIN PB7  // Chân relay 2

#define VREF 3.3          
#define SCOUNT 30 
#define PH_PIN PA0
#define ArrayLenth  40 
const float U0 = 3300.0;  // Điện áp chuẩn tại 0 NTU (360 mV), khi module không chạm nước hoặc trong nước lọc
float Um;                 // Điện áp đo được từ cảm biến
float f;                  // Tỷ số f = Um / U0
float NTU;                // Giá trị độ đục (NTU)
float phValue;
float voltage;
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;


byte localAddress = 0xBB;  // Địa chỉ của thiết bị gửi
byte destination = 0xFF;   // Địa chỉ của thiết bị nhận
byte msgCount = 0;         // Đếm số tin nhắn gửi đi
int s_node2 = 0;
String outgoing;              // tin nhắn gửi đi
String mymessage = "";
bool relay1State = false; // Trạng thái relay 1
bool relay2State = false; // Trạng thái relay 2

unsigned long relay1StartTime = 0;
unsigned long relay2StartTime = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int analogBuffer[SCOUNT]; 
float temperature = 25.0; 

//ham gui du lieu cam bien
void sendMessage(String outgoing) {
    LoRa.beginPacket();
    LoRa.write(destination);  // Địa chỉ nhận
    LoRa.write(localAddress); // Địa chỉ gửi
    LoRa.write(msgCount);     // ID tin nhắn
    LoRa.write(outgoing.length());  // Độ dài tin nhắn
    LoRa.print(outgoing);
    LoRa.endPacket();

    Serial.println("Sent Data: " + outgoing);
    msgCount++;
}
//ham doc cam bien
void sendSensorData() {
// cam bien turbidity water --------------------------------------
  // Đọc giá trị từ cảm biến (0 - 1023) và chuyển đổi ra mV
  int sensorValue = analogRead(PA1);
  Um = (sensorValue / 4095.0) * 3300.0;  // Chuyển đổi giá trị từ 0-1023 thành mV (giả sử Vcc = 5V)
  // Tính tỷ số f
  f = Um / U0;
  // Quy đổi f ra NTU
  if (f >= 0.98 && f <= 1.000) {
    NTU = 0; // Tương ứng với 0 NTU
  } 
  // ngoài mức này thì tính ra NTU
  else { 
    NTU = map(f * 100, 0, 100, 40, 0);  //Quy đổi tỉ lệ phần trăm sang NTU,
  }
  
  
  //cảm biến tds và ds------------------------------------------------------
  sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    if (temperature == -127.0) {  // Kiểm tra lỗi DS18B20
        Serial.println("Lỗi DS18B20! Kiểm tra kết nối.");
        return;
    }

    int sum = 0;
    for (int i = 0; i < SCOUNT; i++) {
        analogBuffer[i] = analogRead(TdsSensorPin);
        delay(10);
    }
    for (int i = 0; i < SCOUNT; i++) {
        sum += analogBuffer[i];
    }
    int adcValue = sum / SCOUNT;


    float averageVoltage = adcValue * (VREF / 4095.0);  // ADC 12-bit (0-4095)

    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                     - 255.86 * compensationVoltage * compensationVoltage
                     + 857.39 * compensationVoltage) * 0.5;

    
  //cảm biên ph---------------------------------------------------------------
  //   {pHArray[pHArrayIndex++]=analogRead(PH_PIN);
  // if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
    voltage = (analogRead(PH_PIN)/4096.0)*3.3;
 float phValue = 7.86 * voltage - 7.;
 Serial.println(voltage);
// Serial.println(voltage);
// Serial.println(analogRead(PH_PIN));
//  Serial.println(phValue,2);
//  delay(3000);
 
 
    if (s_node2 == 0) {
      mymessage = String(temperature) + ","  + String(tdsValue, 0) + ","+ String(NTU) + "," + String(phValue) + ","  + String(averageVoltage, 2) + "," + String(sensorValue)
                 + "," + String(Um) + "," + String(f, 4) + "," ;
      sendMessage(mymessage);
      } else if (s_node2 == 1) {
      delay(3000);
      s_node2 = 0;
      Serial.println("Disable");
    }
    }

//ham tinh tb ph------------------

// double avergearray(int* arr, int number){
//   int i;
//   int max,min;
//   double avg;
//   long amount=0;
//   if(number<=0){
//     Serial.println("Error number for the array to avraging!/n");
//     return 0;
//   }
//   if(number<5){   //less than 5, calculated directly statistics
//     for(i=0;i<number;i++){
//       amount+=arr[i];
//     }
//     avg = amount/number;
//     return avg;
//   }else{
//     if(arr[0]<arr[1]){
//       min = arr[0];max=arr[1];
//     }
//     else{
//       min=arr[1];max=arr[0];
//     }
//     for(i=2;i<number;i++){
//       if(arr[i]<min){
//         amount+=min;        //arr<min
//         min=arr[i];
//       }else {
//         if(arr[i]>max){
//           amount+=max;    //arr>max
//           max=arr[i];
//         }else{
//           amount+=arr[i]; //min<=arr<=max
//         }
//       }//if
//     }//for
//     avg = (double)amount/(number-2);
//   }//if
//   return avg;}


// ham dk relay

void activateRelay1() {
    relay1State = true;
    digitalWrite(RELAY1_PIN, HIGH);
    relay1StartTime = millis();
    Serial.println("Relay 1 BẬT (15s)");
}

void activateRelay2() {
    relay2State = true;
    digitalWrite(RELAY2_PIN, HIGH);
    relay2StartTime = millis();
    Serial.println("Relay 2 BẬT (15s)");
}

// ham dem giay delay
void checkRelayTimeout() {
    if (relay1State && millis() - relay1StartTime >= 60000) {
        digitalWrite(RELAY1_PIN, LOW);
        relay1State = false;
        Serial.println("Relay 1 TẮT");
    }
    if (relay2State && millis() - relay2StartTime >= 52000) {
        digitalWrite(RELAY2_PIN, LOW);
        relay2State = false;
        Serial.println("Relay 2 TẮT");
    }    
}
//ham nhan button
void onReceive(int packetSize) {
    if (packetSize == 0) return;

    byte recipient = LoRa.read();
    byte sender = LoRa.read();
    byte incomingMsgId = LoRa.read();
    byte incomingLength = LoRa.read();

    String incoming = "";
    while (LoRa.available()) {
        incoming += (char)LoRa.read();
    }

    if (incomingLength != incoming.length()) {
        Serial.println("Lỗi: Độ dài tin nhắn không khớp!");
        return;
    }

    if (recipient != localAddress && recipient != 0xFF) {
        Serial.println("Tin nhắn này không dành cho tôi.");
        return;
    }

    Serial.println("Lệnh nhận từ " + String(sender, HEX) + ": " + incoming);

    if (incoming == "ON1") {
        activateRelay1();
    }else if (incoming == "ON2"){
      activateRelay2();
    }
}

void setup() {
    analogReadResolution(12);
    Serial.begin(115200);
    pinMode(TdsSensorPin, INPUT);
    sensors.begin(); 
    pinMode(RELAY1_PIN, OUTPUT);
    digitalWrite(RELAY1_PIN, LOW);
    pinMode(RELAY2_PIN, OUTPUT);
    digitalWrite(RELAY2_PIN, LOW);    
  
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa init failed!");
        while (1);
    }

    Serial.println("LoRa Receiver Ready");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        onReceive(packetSize);
    }

    checkRelayTimeout();

    static unsigned long lastSendTime = 0;
    if (millis() - lastSendTime >= 5000) {
        lastSendTime = millis();
        sendSensorData();
    }
}
