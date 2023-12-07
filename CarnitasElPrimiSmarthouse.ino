#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#define BUZZER_PASIVO 5
#define GAS_PIN 26
// Definiciones de notas musicales

#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define MOTION_SENSOR_PIN  17
#define LED_PIN            14
#define DEBOUNCE_TIME 500 //
unsigned long lastDebounceTime = 0;

// Variables para el sensor de movimiento
int motionStateCurrent  = LOW;
int motionStatePrevious = LOW;
String serverSmoke = "http://192.168.1.73:7800/smoke";
String serverName = "http://192.168.1.73:7800/";
String serverLed = "http://192.168.1.73:7800/led"; 
String serverMotion = "http://192.168.1.73:7800/motion"; 

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

int contador = 0; // Variable para el contador inicializada en 0

void getLedState()
{
  HTTPClient http;
  http.begin(serverLed);

  int httpResponseCode = http.GET();

  if (httpResponseCode == 200)
  {
    // Leer el JSON de la respuesta
    String payload = http.getString();
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, payload);

    // Obtener el estado del LED desde el JSON
    bool ledState = jsonDoc["status"];
    
    // Actualizar el estado del LED en la placa Arduino
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    Serial.print("Estado del LED: ");
    Serial.println(ledState);
  }
  else
  {
    Serial.print("Error al obtener el estado del LED. Código de respuesta: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void post_motion(bool motionDetected)
{
  DynamicJsonDocument json_doc(1024);
  json_doc["motionDetected"] = motionDetected; // true si se detecta movimiento, false si no

  String json_str;
  serializeJson(json_doc, json_str);

  HTTPClient http;
  http.begin(serverMotion);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(json_str);


  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code (motion): ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code (motion): ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void post_smoke(bool smokeDetected)
{
  DynamicJsonDocument json_doc(1024);
  json_doc["smokeDetected"] = smokeDetected; // true si se detecta humo, false si no

  String json_str;
  serializeJson(json_doc, json_str);

  HTTPClient http;
  http.begin(serverSmoke);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(json_str);

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code (smoke): ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code (smoke): ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

int lastGasState = -1; // Estado inicial desconocido
int lastMotionState = -1; // Estado inicial desconocido

void setup() {
  lastGasState = digitalRead(GAS_PIN);
  lastMotionState = digitalRead(MOTION_SENSOR_PIN);
  Serial.begin(9600);
  ledcSetup(0, 5000, 8); // Canal 0, 5000 Hz como frecuencia base, 8 bits de resolución
  ledcAttachPin(BUZZER_PASIVO, 0); // Asigna el pin del buzzer al canal 0
  // Configuraciones para el sensor de gas y el buzzer
  pinMode(GAS_PIN, INPUT);
  pinMode(BUZZER_PASIVO, OUTPUT);

  Serial.println("Warming up the MQ2 sensor");
  delay(20000);

  // Configuraciones para el sensor de movimiento y LED
  pinMode(MOTION_SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Apagar el LED al inicio

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  getLedState();

  // Control de tiempo para la solicitud GET (si es necesario)
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      // Aquí iría el código para la solicitud GET si es necesario
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  // Lógica del sensor de gas
  int gasState = digitalRead(GAS_PIN);
  bool isGasDetected = (gasState == LOW); // Asume que LOW significa que se detectó gas
  if (gasState != lastGasState) {
    post_smoke(isGasDetected);
    lastGasState = gasState;

    if (isGasDetected) {
      Serial.println("The gas is present");
      playJingleBells();
    } else {
      Serial.println("The gas is NOT present");
      noTone(BUZZER_PASIVO);
    }
  }

  // Lógica del sensor de movimiento
  motionStatePrevious = motionStateCurrent;
  motionStateCurrent = digitalRead(MOTION_SENSOR_PIN);
  bool isMotionDetected = (motionStatePrevious == LOW && motionStateCurrent == HIGH);
  if (motionStateCurrent != lastMotionState) {
    post_motion(isMotionDetected);
    lastMotionState = motionStateCurrent;

    if (isMotionDetected) {
      Serial.println("Motion detected!, turns LED ON");
      digitalWrite(LED_PIN, HIGH);
    } else if (motionStatePrevious == HIGH && motionStateCurrent == LOW) {
      Serial.println("Motion stopped!, turns LED OFF");
      digitalWrite(LED_PIN, LOW);
    }
  }
  delay(1000); 
}


void playJingleBells() {
int melodia[] = {
  NOTE_D4, NOTE_D4, NOTE_D5, NOTE_A4, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_C4, NOTE_C4, NOTE_D5, NOTE_A4, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_B3, NOTE_B3, NOTE_D5, NOTE_A4, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_AS3, NOTE_AS3, NOTE_D5, NOTE_A4, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
};

int duraciones[] = {
  6, 7, 5, 3, 5, 5, 6, 9, 9, 9,
  6, 7, 5, 3, 5, 5, 6, 9, 9, 9,
  6, 7, 5, 3, 5, 5, 6, 9, 9, 9,
  6, 7, 5, 3, 5, 5, 6, 9, 9, 9
};

  for (int i = 0; i < 40; i++) {
    int duracion = 1000 / duraciones[i];
    ledcWriteTone(0, melodia[i]); // Usa ledcWriteTone en lugar de tone
    int pausa = duracion * 1.30;
    delay(pausa);
    ledcWriteTone(0, 0); // Detiene el tono
  }
}
