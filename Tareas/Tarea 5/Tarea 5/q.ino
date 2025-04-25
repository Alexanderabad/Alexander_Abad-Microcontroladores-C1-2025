#include <Arduino.h>

// Definición de pines
#define BUTTON_PIN 12    // GPIO12 para el botón
#define LED_PIN 4        // GPIO4 para el LED
#define DEBOUNCE_TIME 50 // Tiempo de debounce en ms

// Variables globales
volatile uint32_t buttonPressDuration = 0;
volatile bool buttonPressed = false;
volatile bool blinkingEnabled = false;
volatile uint32_t blinkInterval = 0;
volatile bool ledState = false;

// Timer para medición del botón
hw_timer_t *buttonTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Timer para parpadeo del LED
hw_timer_t *ledTimer = NULL;
portMUX_TYPE ledTimerMux = portMUX_INITIALIZER_UNLOCKED;

// ISR para el timer del botón
void IRAM_ATTR buttonTimerISR() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (digitalRead(BUTTON_PIN) == LOW) {
    buttonPressDuration++;
  } else {
    if (buttonPressDuration > 0) {
      blinkInterval = buttonPressDuration;
      blinkingEnabled = true;
      buttonPressDuration = 0;
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

// ISR para el timer del LED
void IRAM_ATTR ledTimerISR() {
  portENTER_CRITICAL_ISR(&ledTimerMux);
  if (blinkingEnabled) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
  portEXIT_CRITICAL_ISR(&ledTimerMux);
}

void setupTimer(hw_timer_t **timer, uint32_t frequency, void (*fn)(void)) {
  *timer = timerBegin(frequency);
  timerAttachInterrupt(*timer, fn);
  timerAlarm(*timer, 1000000 / frequency, true, 0); // 1,000,000 us / frequency
  timerStart(*timer);
}

void setup() {
  Serial.begin(115200);
  
  // Configurar pines
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Configurar timers
  setupTimer(&buttonTimer, 1000, &buttonTimerISR);  // 1000 Hz = 1ms
  setupTimer(&ledTimer, 1, &ledTimerISR);           // Inicialmente 1 Hz
  
  Serial.println("Sistema iniciado");
}

void loop() {
  // Actualizar intervalo de parpadeo
  static uint32_t lastBlinkInterval = 0;
  
  if (blinkingEnabled && (blinkInterval != lastBlinkInterval)) {
    portENTER_CRITICAL(&ledTimerMux);
    timerAlarm(ledTimer, blinkInterval * 1000, true, blinkInterval * 1000);
    lastBlinkInterval = blinkInterval;
    portEXIT_CRITICAL(&ledTimerMux);
    
    Serial.print("Intervalo de parpadeo: ");
    Serial.print(blinkInterval);
    Serial.println(" ms");
  }
  
  // Control de botón con debounce
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  if (currentButtonState != lastButtonState) {
    delay(DEBOUNCE_TIME);
    currentButtonState = digitalRead(BUTTON_PIN);
    
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      buttonPressed = true;
      blinkingEnabled = false;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Botón presionado");
    } else if (currentButtonState == HIGH && lastButtonState == LOW) {
      buttonPressed = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("Botón liberado");
    }
    lastButtonState = currentButtonState;
  }
  
  delay(10);
}