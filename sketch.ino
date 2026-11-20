#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
 
// --- DEFINIÇÃO DE PINOS E INTERFACES ---
const int PIN_POT      = A0;  // Entrada analógica do potenciômetro
const int PIN_TRIG     = 12;  // Disparo do pulso ultrassônico
const int PIN_ECHO     = 11;  // Retorno do eco ultrassônico
const int PIN_SERVO    = 9;   // Modulação PPM do servomotor
const int PIN_BUZZER   = 8;   // Alarme acústico de proximidade
const int PIN_LED_R    = 6;   // PWM Canal Vermelho (Red)
const int PIN_LED_G    = 5;   // PWM Canal Verde (Green)
const int PIN_LED_B    = 3;   // PWM Canal Azul (Blue)
 
// --- INSTANCIAÇÃO DE OBJETOS ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gateServo;
 
// --- TEMPORIZAÇÃO DETERMINÍSTICA NÃO-BLOQUEANTE (MILLIS) ---
unsigned long prevSonarTime     = 0;
const unsigned long SONAR_RATE  = 60;   // Amostragem do sonar a cada 60ms
unsigned long prevTelemetryTime = 0;
const unsigned long TELEM_RATE  = 100;  // Envio serial USB a cada 100ms
unsigned long prevLcdTime       = 0;
const unsigned long LCD_RATE    = 250;  // Taxa de refresh do LCD a 250ms
 
// --- VARIÁVEIS DE CONTROLE E FILTRAGEM DIGITAL ---
float emaFilteredPot    = 0.0;   // Armazenamento do filtro passa-baixas EMA
const float EMA_ALPHA   = 0.15;  // 15% peso novo, 85% amortecimento histórico
float currentDistanceCm = 400.0; // Distância métrica inicial
int manualServoAngle    = 0;     // Posição angular recebida por downlink
bool overrideMode       = false; // Bandeira de controle manual remoto
String inputBuffer      = "";    // Buffer de recepção serial de caracteres
 
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
 
  gateServo.attach(PIN_SERVO);
  gateServo.write(0); // Cancela fechada na inicialização
 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESTACAO ACESSO");
  lcd.setCursor(0, 1);
  lcd.print("INICIALIZANDO...");
 
  emaFilteredPot = analogRead(PIN_POT);
  delay(400); // Aguardo de estabilização do LCD
  lcd.clear();
}
 
void loop() {
  unsigned long currentMillis = millis();
 
  // 1. Aquisição Periódica do Sensor Ultrassônico e Potenciômetro
  if (currentMillis - prevSonarTime >= SONAR_RATE) {
    prevSonarTime = currentMillis;
    readSensors();
  }
 
  // 2. Atualização dos Atuadores Físicos (Servo, LED RGB e Buzzer)
  updateActuators();
 
  // 3. Envio de Telemetria Formatada (Uplink Serial USB -> p5.js)
  if (currentMillis - prevTelemetryTime >= TELEM_RATE) {
    prevTelemetryTime = currentMillis;
    sendTelemetry();
  }
 
  // 4. Renderização Alfanumérica no Display Serial I2C
  if (currentMillis - prevLcdTime >= LCD_RATE) {
    prevLcdTime = currentMillis;
    updateLcdDisplay();
  }
 
  // 5. Escuta Contínua de Comandos de Downlink da Aplicação Multimídia
  processIncomingSerial();
}
 
void readSensors() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
 
  long duration = pulseIn(PIN_ECHO, HIGH, 25000);
  if (duration > 0) {
    currentDistanceCm = (duration * 0.0343) / 2.0;
  } else {
    currentDistanceCm = 400.0;
  }
 
  int rawPot = analogRead(PIN_POT);
  emaFilteredPot = (EMA_ALPHA * rawPot) + ((1.0 - EMA_ALPHA) * emaFilteredPot);
}
 
void updateActuators() {
  int targetAngle = 0;
  if (overrideMode) {
    targetAngle = manualServoAngle;
  } else {
    if (currentDistanceCm < 30.0) {
      targetAngle = 90;
    } else {
      targetAngle = map((int)emaFilteredPot, 0, 1023, 0, 180);
    }
  }
  gateServo.write(constrain(targetAngle, 0, 180));
 
  if (currentDistanceCm < 15.0) {
    setRgbColor(255, 0, 0);
    tone(PIN_BUZZER, 2000, 25);
  } else if (currentDistanceCm < 45.0) {
    setRgbColor(255, 160, 0);
    noTone(PIN_BUZZER);
  } else {
    setRgbColor(0, 255, 0);
    noTone(PIN_BUZZER);
  }
}
 
void setRgbColor(byte r, byte g, byte b) {
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}
 
void updateLcdDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("DIST: ");
  lcd.print((int)currentDistanceCm);
  lcd.print("cm   ");
 
  lcd.setCursor(0, 1);
  if (overrideMode) {
    lcd.print("REMOTO: ");
    lcd.print(manualServoAngle);
    lcd.print((char)223);
    lcd.print("   ");
  } else {
    lcd.print("POT: ");
    lcd.print((int)emaFilteredPot);
    lcd.print(" AUTO ");
  }
}
 
void sendTelemetry() {
  Serial.print("DIST:");
  Serial.print((int)currentDistanceCm);
  Serial.print(",POT:");
  Serial.print((int)emaFilteredPot);
  Serial.print(",SERVO:");
  Serial.print(gateServo.read());
  Serial.print(",MODE:");
  Serial.println(overrideMode ? "REMOTE" : "AUTO");
}
 
void processIncomingSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        inputBuffer.trim();
        if (inputBuffer.startsWith("SERVO:")) {
          int angle = inputBuffer.substring(6).toInt();
          manualServoAngle = constrain(angle, 0, 180);
          overrideMode = true;
        } else if (inputBuffer.equals("MODE:AUTO")) {
          overrideMode = false;
        }
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}
