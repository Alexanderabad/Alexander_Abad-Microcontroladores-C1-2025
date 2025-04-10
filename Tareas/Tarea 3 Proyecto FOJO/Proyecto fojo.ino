#include <Arduino.h>
#include <Ticker.h>  // Usamos Ticker como alternativa compatible

// Definición de estados
enum SystemState {
  STATE_IDLE,
  STATE_SENSOR_READING,
  STATE_MOTOR_CONTROL,
  STATE_BUZZER_ACTIVE,
  STATE_LAMP_CONTROL,
  STATE_ERROR
};

// Variables globales
volatile SystemState currentState = STATE_IDLE;
volatile bool timerFlag = false;

// Configuración de pines (ajustar según conexiones reales)
const int sensorPin = 34;     // Pin del sensor (ADC1)
const int motorPin1 = 12;     // Control motor 1
const int motorPin2 = 13;     // Control motor 2
const int buzzerPin = 14;     // Pin del buzzer
const int lampPin = 15;       // Pin de la lámpara
const int relayPin = 16;      // Pin del relé

Ticker timer;  // Objeto Timer alternativo

// Función de callback del timer
void onTimer() {
  timerFlag = true;
}

void setup() {
  Serial.begin(115200);
  
  // Configurar pines
  pinMode(sensorPin, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lampPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  
  // Configurar timer con Ticker (alternativa compatible)
  timer.attach_ms(50, onTimer);  // Timer de 50ms
  
  Serial.println("Sistema FOJO inicializado");
}

void loop() {
  if(timerFlag) {
    timerFlag = false;
    stateMachine();
  }
}

void stateMachine() {
  static unsigned long lastSensorRead = 0;
  static int sensorValue = 0;
  
  switch(currentState) {
    case STATE_IDLE:
      currentState = STATE_SENSOR_READING;
      break;
      
    case STATE_SENSOR_READING:
      sensorValue = analogRead(sensorPin);
      Serial.print("Valor sensor: ");
      Serial.println(sensorValue);
      
      // Lógica de transición
      if(sensorValue > 2000) {
        currentState = STATE_MOTOR_CONTROL;
      } else if(sensorValue < 500) {
        currentState = STATE_BUZZER_ACTIVE;
      } else {
        currentState = STATE_LAMP_CONTROL;
      }
      break;
      
    case STATE_MOTOR_CONTROL:
      digitalWrite(motorPin1, HIGH);
      digitalWrite(motorPin2, LOW);
      delay(10);
      currentState = STATE_IDLE;
      break;
      
    case STATE_BUZZER_ACTIVE:
      tone(buzzerPin, 1000, 100);  // Buzzer a 1kHz por 100ms
      currentState = STATE_IDLE;
      break;
      
    case STATE_LAMP_CONTROL:
      digitalWrite(lampPin, HIGH);
      delay(50);
      digitalWrite(lampPin, LOW);
      currentState = STATE_IDLE;
      break;
      
    case STATE_ERROR:
      // Manejo de errores
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, LOW);
      digitalWrite(lampPin, LOW);
      noTone(buzzerPin);
      break;
  }
}