#include <Arduino.h>

// Definición de estados
enum SystemState {
  STATE_INICIAL,
  STATE_ESPERA,
  STATE_LECTURA_SENSOR,
  STATE_PROCESAMIENTO,
  STATE_ACCION_MOTOR,
  STATE_ALERTA,
  STATE_ERROR
};

// Variables globales
SystemState currentState = STATE_INICIAL;
unsigned long lastStateChange = 0;

// Configuración de pines para ESP32-CAM
const int sensorPin = 12;     // GPIO12 (ADC2)
const int motorPin = 13;      // GPIO13
const int ledPin = 4;         // GPIO4 (LED Flash integrado)
const int buttonPin = 14;     // GPIO14

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);
  Serial.println("Máquina de estados ESP32-CAM inicializada");
}

void loop() {
  switch(currentState) {
    case STATE_INICIAL: inicialState(); break;
    case STATE_ESPERA: esperaState(); break;
    case STATE_LECTURA_SENSOR: lecturaSensorState(); break;
    case STATE_PROCESAMIENTO: procesamientoState(); break;
    case STATE_ACCION_MOTOR: accionMotorState(); break;
    case STATE_ALERTA: alertaState(); break;
    case STATE_ERROR: errorState(); break;
  }
}

void inicialState() {
  Serial.println("Estado INICIAL: Iniciando sistema");
  for(int i=0; i<3; i++) {
    digitalWrite(ledPin, HIGH); delay(200);
    digitalWrite(ledPin, LOW); delay(200);
  }
  currentState = STATE_ESPERA;
}

void esperaState() {
  static bool buttonPressed = false;
  if(digitalRead(buttonPin) == LOW && !buttonPressed) {
    buttonPressed = true;
    currentState = STATE_LECTURA_SENSOR;
    Serial.println("Botón presionado -> LECTURA_SENSOR");
  } else if(digitalRead(buttonPin) == HIGH) {
    buttonPressed = false;
  }
  if(millis() - lastStateChange > 500) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    lastStateChange = millis();
  }
}

void lecturaSensorState() {
  int sensorValue = analogRead(sensorPin);
  Serial.print("Lectura sensor: "); Serial.println(sensorValue);
  if(sensorValue == -1) currentState = STATE_ERROR;
  else if(sensorValue > 2000) currentState = STATE_PROCESAMIENTO;
  else currentState = STATE_ALERTA;
}

void procesamientoState() {
  Serial.println("Procesando datos..."); delay(300);
  currentState = STATE_ACCION_MOTOR;
}

void accionMotorState() {
  Serial.println("Activando motor...");
  digitalWrite(motorPin, HIGH); delay(800);
  digitalWrite(motorPin, LOW);
  currentState = STATE_ESPERA;
}

void alertaState() {
  Serial.println("¡ALERTA! Valor bajo del sensor");
  for(int i=0; i<5; i++) {
    digitalWrite(ledPin, HIGH); delay(100);
    digitalWrite(ledPin, LOW); delay(100);
  }
  currentState = STATE_ESPERA;
}

void errorState() {
  Serial.println("ERROR: Reiniciando...");
  for(int i=0; i<10; i++) {
    digitalWrite(ledPin, HIGH); delay(100);
    digitalWrite(ledPin, LOW); delay(100);
  }
  currentState = STATE_INICIAL;
}