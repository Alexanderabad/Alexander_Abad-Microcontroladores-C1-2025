#include <Arduino.h>

// Definición de pines
#define LED_ROJO 2
#define LED_VERDE 4
#define LED_ROJO_2 16   // LED rojo adicional (parpadeo mientras se mueve)
#define LED_VERDE_2 17  // LED verde adicional (indica puerta abierta)
#define PUENTE_H_A 5
#define PUENTE_H_B 18

// Estados de la máquina de estado
typedef enum {
    PUERTA_CERRADA,
    ABRIENDO_PUERTA,
    PUERTA_ABIERTA,
    CERRANDO_PUERTA
} EstadoPuerta;

EstadoPuerta estadoActual = PUERTA_CERRADA;
bool ledRojoEstado = false;
bool ledRojo2Estado = false;
unsigned long tiempoInicioMovimiento = 0;

// Temporizador de 50ms
void IRAM_ATTR onTimer() {
    if (estadoActual == ABRIENDO_PUERTA || estadoActual == CERRANDO_PUERTA) {
        ledRojoEstado = !ledRojoEstado;
        digitalWrite(LED_ROJO, ledRojoEstado);

        // También parpadea el LED rojo adicional
        ledRojo2Estado = !ledRojo2Estado;
        digitalWrite(LED_ROJO_2, ledRojo2Estado);
    }
}

hw_timer_t *timer = NULL;

void setup() {
    pinMode(LED_ROJO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_ROJO_2, OUTPUT);
    pinMode(LED_VERDE_2, OUTPUT);
    pinMode(PUENTE_H_A, OUTPUT);
    pinMode(PUENTE_H_B, OUTPUT);

    Serial.begin(115200);
    Serial.println("Sistema Iniciado...");
    Serial.println("Escriba 'abrir' para abrir la puerta o 'cerrar' para cerrarla.");

    // Configurar temporizador cada 50ms
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (1us por tick)
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 50000, true);  // 50ms
    timerAlarmEnable(timer);
}

void abrirPuerta() {
    if (estadoActual == PUERTA_CERRADA) {
        Serial.println("-> Iniciando apertura de puerta...");
        estadoActual = ABRIENDO_PUERTA;
        tiempoInicioMovimiento = millis();
        digitalWrite(PUENTE_H_A, HIGH);
    }
}

void cerrarPuerta() {
    if (estadoActual == PUERTA_ABIERTA) {
        Serial.println("-> Iniciando cierre de puerta...");
        estadoActual = CERRANDO_PUERTA;
        tiempoInicioMovimiento = millis();
        digitalWrite(PUENTE_H_B, HIGH);
    }
}

void loop() {
    // Verifica si hay datos en el puerto serie
    if (Serial.available()) {
        String comando = Serial.readStringUntil('\n');
        comando.trim();

        if (comando.equalsIgnoreCase("abrir")) {
            abrirPuerta();
        } else if (comando.equalsIgnoreCase("cerrar")) {
            cerrarPuerta();
        }
    }

    switch (estadoActual) {
        case PUERTA_CERRADA:
            digitalWrite(LED_ROJO, HIGH);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_ROJO_2, LOW);
            digitalWrite(LED_VERDE_2, LOW);
            digitalWrite(PUENTE_H_A, LOW);
            digitalWrite(PUENTE_H_B, LOW);
            Serial.println("Estado: PUERTA CERRADA - Escriba 'abrir' para abrir.");
            break;

        case ABRIENDO_PUERTA:
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERDE_2, LOW);
            Serial.println("Estado: ABRIENDO PUERTA - LED rojo parpadeando...");

            // Simular fin de apertura en 5s
            if (millis() - tiempoInicioMovimiento > 5000) {
                estadoActual = PUERTA_ABIERTA;
                digitalWrite(PUENTE_H_A, LOW);
                digitalWrite(LED_ROJO, LOW);
                digitalWrite(LED_ROJO_2, LOW);
                Serial.println("-> Puerta completamente abierta.");
            }
            break;

        case PUERTA_ABIERTA:
            digitalWrite(LED_ROJO, LOW);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERDE_2, HIGH);  // LED verde adicional encendido
            digitalWrite(LED_ROJO_2, LOW);
            digitalWrite(PUENTE_H_A, LOW);
            digitalWrite(PUENTE_H_B, LOW);
            Serial.println("Estado: PUERTA ABIERTA - Escriba 'cerrar' para cerrarla.");
            break;

        case CERRANDO_PUERTA:
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERDE_2, LOW);
            Serial.println("Estado: CERRANDO PUERTA - LED rojo parpadeando...");

            // Simular fin de cierre en 5s
            if (millis() - tiempoInicioMovimiento > 5000) {
                estadoActual = PUERTA_CERRADA;
                digitalWrite(PUENTE_H_B, LOW);
                Serial.println("-> Puerta completamente cerrada.");
            }
            break;
    }

    delay(500);  // Reducir la frecuencia de los mensajes en el monitor serie
}
