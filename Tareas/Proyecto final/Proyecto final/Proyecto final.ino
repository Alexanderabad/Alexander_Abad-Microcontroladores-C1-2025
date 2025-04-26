#include <WiFi.h>
#include <WebServer.h>
#include <driver/ledc.h>

// Configuración WiFi
const char* ssid = "ESP32-555-Emulator";
const char* password = "password123";
#define WIFI_TIMEOUT_MS 20000

WebServer server(80);

// Pines ESP32-CAM
const int pwmPin = 12;    // GPIO12 - Salida PWM
const int potPin = 13;     // GPIO13 - Entrada analógica
const int buttonPin = 14;  // GPIO14 - Botón
const int ledPin = 4;      // GPIO4 - LED flash

// Variables configurables
float r1 = 10000;     // Resistencia R1 en ohms
float r2 = 10000;     // Resistencia R2 en ohms
float capacitor = 1e-6; // Capacitor en faradios (1µF)
int pulseWidth = 1000; // Ancho de pulso en ms (monostable)

// Variables globales
int pwmValue = 0;
volatile bool triggered = false;
bool isRunning = false;
String currentMode = "astable";

// Prototipos de funciones
void IRAM_ATTR buttonISR();
void setupWiFi();
void setupWebServer();
void setupPWM();
void handleRoot();
void handleAstable();
void handleMonostable();
void handleConfig();
void handleStart();
void handleStop();
void handleStatus();
float calculateFrequency();
void updatePWM();

void setup() {
  Serial.begin(115200);
  
  // Configurar pines
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
  
  // Configurar PWM
  setupPWM();
  
  // Iniciar WiFi
  setupWiFi();
  
  // Configurar servidor web
  setupWebServer();
  
  Serial.println("Sistema iniciado");
}

void setupPWM() {
  // Configuración del canal PWM
  ledc_timer_config_t timer_conf = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 5000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {
    .gpio_num = pwmPin,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&channel_conf);
}

void loop() {
  server.handleClient();
  
  if (isRunning) {
    updatePWM();
    
    if (currentMode == "monostable" && triggered) {
      // Implementa la lógica Monostable
      digitalWrite(ledPin, HIGH);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      
      delay(pulseWidth);
      
      digitalWrite(ledPin, LOW);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      triggered = false;
    }
  }
}

void IRAM_ATTR buttonISR() {
  if (!digitalRead(buttonPin)) {
    triggered = true;
  }
}

void setupWiFi() {
  WiFi.softAP(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    digitalWrite(ledPin, HIGH);
  } else {
    Serial.println("\nNo se pudo conectar al WiFi");
    digitalWrite(ledPin, LOW);
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/astable", handleAstable);
  server.on("/monostable", handleMonostable);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Emulador NE555</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:20px;}";
  html += ".container{max-width:600px;margin:0 auto;}";
  html += "form{margin-bottom:20px;}";
  html += "label{display:block;margin:10px 0;}";
  html += "input[type='number']{width:100%;padding:8px;}";
  html += "button{padding:10px 15px;background:#4CAF50;color:white;border:none;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>Emulador NE555 con ESP32-CAM</h1>";
  html += "<p>Modo actual: " + currentMode + "</p>";
  html += "<p>Estado: " + String(isRunning ? "En ejecución" : "Detenido") + "</p>";
  
  // Formulario de configuración
  html += "<form action='/config' method='post'>";
  html += "<h2>Configuración</h2>";
  
  // Configuración para modo astable
  html += "<div id='astable-config'>";
  html += "<label>Resistencia R1 (Ω): <input type='number' name='r1' value='" + String(r1) + "' step='100'></label>";
  html += "<label>Resistencia R2 (Ω): <input type='number' name='r2' value='" + String(r2) + "' step='100'></label>";
  html += "</div>";
  
  // Configuración para modo monostable
  html += "<div id='monostable-config' style='display:none;'>";
  html += "<label>Resistencia R (Ω): <input type='number' name='r' value='" + String(r1) + "' step='100'></label>";
  html += "<label>Ancho de pulso (ms): <input type='number' name='pulse' value='" + String(pulseWidth) + "'></label>";
  html += "</div>";
  
  html += "<label>Capacitor (µF): <input type='number' name='cap' value='" + String(capacitor*1e6) + "' step='0.1'></label>";
  
  // Selector de modo
  html += "<label>Modo: <select name='mode' onchange='toggleMode()'>";
  html += "<option value='astable'" + String(currentMode=="astable"?" selected":"") + ">Astable</option>";
  html += "<option value='monostable'" + String(currentMode=="monostable"?" selected":"") + ">Monostable</option>";
  html += "</select></label>";
  
  html += "<button type='submit'>Guardar Configuración</button>";
  html += "</form>";
  
  // Controles de ejecución
  html += "<a href='/start'><button>Iniciar</button></a>";
  html += "<a href='/stop'><button>Detener</button></a>";
  
  // Mostrar valores calculados
  if(currentMode == "astable") {
    html += "<p>Frecuencia calculada: " + String(calculateFrequency()) + " Hz</p>";
  }
  
  html += "<script>";
  html += "function toggleMode() {";
  html += "var mode = document.getElementsByName('mode')[0].value;";
  html += "document.getElementById('astable-config').style.display = mode=='astable'?'block':'none';";
  html += "document.getElementById('monostable-config').style.display = mode=='monostable'?'block':'none';";
  html += "}";
  html += "toggleMode();"; // Ejecutar al cargar
  html += "</script>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleAstable() {
  currentMode = "astable";
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleMonostable() {
  currentMode = "monostable";
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleConfig() {
  if(server.method() == HTTP_POST) {
    currentMode = server.arg("mode");
    
    if(currentMode == "astable") {
      r1 = server.arg("r1").toFloat();
      r2 = server.arg("r2").toFloat();
    } else {
      r1 = server.arg("r").toFloat();
      pulseWidth = server.arg("pulse").toInt();
    }
    
    capacitor = server.arg("cap").toFloat() * 1e-6; // Convertir µF a F
    
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

void handleStart() {
  isRunning = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStop() {
  isRunning = false;
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  String json = "{";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"running\":" + String(isRunning ? "true" : "false") + ",";
  json += "\"r1\":" + String(r1) + ",";
  
  if(currentMode == "astable") {
    json += "\"frequency\":" + String(calculateFrequency()) + ",";
    json += "\"r2\":" + String(r2);
  } else {
    json += "\"pulseWidth\":" + String(pulseWidth);
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

float calculateFrequency() {
  if(currentMode == "astable") {
    return 1.44 / ((r1 + 2*r2) * capacitor);
  }
  return 0;
}

void updatePWM() {
  if(currentMode == "astable") {
    // En modo astable, el PWM se controla por la resistencia variable
    int potValue = analogRead(potPin);
    float dutyCycle = (r1 + r2) / (r1 + 2*r2);
    pwmValue = dutyCycle * 255;
    
    // Ajustar frecuencia según los componentes
    float freq = calculateFrequency();
    if(freq > 0) {
      ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwmValue);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
  }
}