#include <Arduino.h>
#include <HardwareSerial.h>

#define RS485_DE_RE_PIN 2
#define LED_FLASH 4
HardwareSerial rs485(2);

// Variables Modbus
uint16_t holdingRegisters[10] = {0}; // Registros Modbus

void setup() {
  Serial.begin(115200);
  rs485.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);
  pinMode(LED_FLASH, OUTPUT);
  digitalWrite(LED_FLASH, LOW);
  
  // Inicializar algunos valores de ejemplo
  holdingRegisters[0] = 1234;
  holdingRegisters[1] = 5678;
  
  Serial.println("ESP32-CAM Modbus RTU Listo");
}

uint16_t calculateCRC(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)data[pos];
    for (uint8_t i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void processModbusRequest() {
  uint8_t buffer[8];
  uint8_t len = 0;
  
  while (rs485.available()) {
    buffer[len++] = rs485.read();
    if (len >= 8) break;
  }
  
  if (len >= 8) {
    // Verificar CRC
    uint16_t receivedCRC = (buffer[len-2] << 8) | buffer[len-1];
    uint16_t calculatedCRC = calculateCRC(buffer, len-2);
    
    if (receivedCRC == calculatedCRC) {
      // Función 03: Leer registros holding
      if (buffer[1] == 0x03) {
        uint16_t startAddr = (buffer[2] << 8) | buffer[3];
        uint16_t regCount = (buffer[4] << 8) | buffer[5];
        
        if (startAddr < 10 && regCount <= 10) {
          uint8_t response[5 + regCount * 2];
          response[0] = buffer[0]; // ID esclavo
          response[1] = 0x03;      // Función
          response[2] = regCount * 2; // Byte count
          
          for (uint16_t i = 0; i < regCount; i++) {
            response[3 + i*2] = holdingRegisters[startAddr + i] >> 8;
            response[4 + i*2] = holdingRegisters[startAddr + i] & 0xFF;
          }
          
          uint16_t crc = calculateCRC(response, 3 + regCount * 2);
          response[3 + regCount * 2] = crc >> 8;
          response[4 + regCount * 2] = crc & 0xFF;
          
          digitalWrite(RS485_DE_RE_PIN, HIGH);
          rs485.write(response, 5 + regCount * 2);
          rs485.flush();
          digitalWrite(RS485_DE_RE_PIN, LOW);
          
          digitalWrite(LED_FLASH, HIGH);
          delay(10);
          digitalWrite(LED_FLASH, LOW);
        }
      }
    }
  }
}

void loop() {
  processModbusRequest();
  
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    holdingRegisters[0] = random(0, 100); // Simular lectura de sensor
    Serial.print("Registro 0 actualizado: ");
    Serial.println(holdingRegisters[0]);
  }
}