#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>

// Configuración de pines ESP32-CAM
#define BUTTON_PIN 13  // GPIO13 para botón
#define LED_PIN 4      // GPIO4 (LED integrado)
#define DEBOUNCE_MS 50 // Tiempo antirebote

// Handles para RTOS
TimerHandle_t buttonTimer;
TimerHandle_t blinkTimer;
QueueHandle_t eventQueue;

// Estructura para eventos
typedef struct {
  uint32_t duration;
  bool start_blinking;
} ButtonEvent;

// Interrupción del botón
void IRAM_ATTR buttonISR() {
  static uint32_t lastPressTime = 0;
  uint32_t currentTime = millis();
  
  if (currentTime - lastPressTime > DEBOUNCE_MS) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (digitalRead(BUTTON_PIN) == LOW) {
      // Botón presionado
      xTimerStartFromISR(buttonTimer, &xHigherPriorityTaskWoken);
      digitalWrite(LED_PIN, HIGH);
    } else {
      // Botón liberado
      uint32_t duration = xTimerGetExpiryTime(buttonTimer) - xTimerGetPeriod(buttonTimer);
      ButtonEvent event = {duration, true};
      xQueueSendFromISR(eventQueue, &event, &xHigherPriorityTaskWoken);
      digitalWrite(LED_PIN, LOW);
    }
    
    lastPressTime = currentTime;
    if (xHigherPriorityTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
  }
}

// Callbacks de los timers
void buttonTimerCallback(TimerHandle_t xTimer) {
  // Vacío - solo necesitamos medir tiempo
}

void blinkTimerCallback(TimerHandle_t xTimer) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// Tarea de control del LED (CORREGIDA)
void ledControlTask(void *pvParameters) {
  ButtonEvent event;
  
  while(1) {
    if (xQueueReceive(eventQueue, &event, portMAX_DELAY)) {  // Paréntesis corregido aquí
      if (event.start_blinking) {
        xTimerChangePeriod(blinkTimer, pdMS_TO_TICKS(event.duration), 0);
        xTimerStart(blinkTimer, 0);
        
        vTaskDelay(pdMS_TO_TICKS(event.duration * 2));
        xTimerStop(blinkTimer, 0);
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configuración de hardware
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Crear cola RTOS
  eventQueue = xQueueCreate(5, sizeof(ButtonEvent));
  
  // Configurar timers
  buttonTimer = xTimerCreate(
    "ButtonTimer",
    pdMS_TO_TICKS(0xFFFFFFFF),
    pdTRUE,
    (void*)0,
    buttonTimerCallback
  );
  
  blinkTimer = xTimerCreate(
    "BlinkTimer",
    pdMS_TO_TICKS(1000),
    pdTRUE,
    (void*)1,
    blinkTimerCallback
  );
  xTimerStop(blinkTimer, 0);
  
  // Crear tarea
  xTaskCreate(
    ledControlTask,
    "LEDControl",
    2048,
    NULL,
    2,
    NULL
  );
  
  // Configurar interrupción
  attachInterrupt(BUTTON_PIN, buttonISR, CHANGE);
  
  Serial.println("Sistema listo");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}