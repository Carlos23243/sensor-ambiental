/* 
   UNIVERSIDAD ESTATAL AMAZÓNICA (UEA)
   Asignatura: Sistemas Digitales
   Proyecto: Sistema Inteligente de Monitoreo Ambiental
*/


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// CONFIGURACIÓN DE PINES Y HARDWARE

// Pin digital del sensor DHT11 / DHT22

#define DHTPIN 2  
// Cambiar a DHT22 si usas dicho sensor        
#define DHTTYPE DHT11     
 // Pin analógico de la fotorresistencia LDR
#define LDR_PIN A0       
// Pin digital para el LED verde
#define PIN_LED 4 
 // Pin digital para el Buzzer/Zumbador
#define BUZZER 6 


// UMBRALES DE ALERTA (Modifica según tus pruebas)

const float TEMP_ADVERTENCIA = 28.0; 
const float TEMP_CRITICA     = 32.0;
const float HUM_ADVERTENCIA  = 70.0; 
const float HUM_CRITICA      = 85.0; 


// MÁQUINA DE ESTADOS FINITOS (FSM)

enum EstadoSistema {
  ESTADO_INICIAL,
  ESTADO_MONITOREO,
  ESTADO_ADVERTENCIA,
  ESTADO_ALARMA
};

EstadoSistema estadoActual = ESTADO_INICIAL;


// OBJETOS Y VARIABLES GLOBALES

LiquidCrystal_I2C lcd(0x27, 16, 2); 
DHT dht(DHTPIN, DHTTYPE);

float temperatura = 0.0;
float humedad = 0.0;
int luzRaw = 0;

unsigned long ultimoTiempoLectura = 0;
// Leer cada 1.5 segundos
const unsigned long INTERVALO_LECTURA = 1500; 

// DECLARACIÓN PREVIA DE FUNCIONES (PROTOTIPOS)

void evaluarEstado();
void actualizarActuadores();
void actualizarPantallaLCD();
void enviarReporteUART();


// SETUP / INICIALIZACIÓN

void setup() {
  Serial.begin(9600);
  Serial.println(F("================================================"));
  Serial.println(F("SISTEMA INTELIGENTE DE MONITOREO AMBIENTAL - UEA"));
  Serial.println(F("================================================"));

  pinMode(PIN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(PIN_LED, LOW);
  digitalWrite(BUZZER, LOW);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("  SISTEMA UEA   ");
  lcd.setCursor(0, 1);
  lcd.print("  INICIANDO...  ");
  delay(2000);

  dht.begin();

  estadoActual = ESTADO_MONITOREO;
  lcd.clear();
}

// LOOP PRINCIPAL

void loop() {
  if (millis() - ultimoTiempoLectura >= INTERVALO_LECTURA) {
    ultimoTiempoLectura = millis();

    leerSensores();
    evaluarEstado();
    actualizarActuadores();
    actualizarPantallaLCD();
    enviarReporteUART();
  }
}


// DEFINICIÓN DE FUNCIONES AUXILIARES


void leerSensores() {
  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();
  luzRaw = analogRead(LDR_PIN);

  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println(F("¡Error al leer el sensor DHT!"));
  }
}

void evaluarEstado() {
  if (temperatura >= TEMP_CRITICA || humedad >= HUM_CRITICA) {
    estadoActual = ESTADO_ALARMA;
  }
  else if (temperatura >= TEMP_ADVERTENCIA || humedad >= HUM_ADVERTENCIA) {
    estadoActual = ESTADO_ADVERTENCIA;
  }
  else {
    estadoActual = ESTADO_MONITOREO;
  }
}

void actualizarActuadores() {
  switch (estadoActual) {
    case ESTADO_MONITOREO:
      digitalWrite(PIN_LED, HIGH); 
      digitalWrite(BUZZER, LOW);  
      break;

    case ESTADO_ADVERTENCIA:
      digitalWrite(PIN_LED, (millis() / 500) % 2); 
      digitalWrite(BUZZER, LOW);
      break;

    case ESTADO_ALARMA:
      digitalWrite(PIN_LED, (millis() / 150) % 2); 
      digitalWrite(BUZZER, HIGH); 
      break;

    case ESTADO_INICIAL:
    default:
      digitalWrite(PIN_LED, LOW);
      digitalWrite(BUZZER, LOW);
      break;
  }
}

void actualizarPantallaLCD() {
  lcd.setCursor(0, 0);
  lcd.print("T:");
  if (!isnan(temperatura)) lcd.print((int)temperatura); else lcd.print("--");
  lcd.print((char)223); 
  lcd.print("C H:");
  if (!isnan(humedad)) lcd.print((int)humedad); else lcd.print("--");
  lcd.print("% L:");
  
  int luzPorcentaje = map(luzRaw, 0, 1023, 0, 99);
  if (luzPorcentaje < 10) lcd.print("0");
  lcd.print(luzPorcentaje);

  lcd.setCursor(0, 1);
  switch (estadoActual) {
    case ESTADO_MONITOREO:
      lcd.print("Estado: NORMAL  ");
      break;
    case ESTADO_ADVERTENCIA:
      lcd.print("Est: ADVERTENCIA");
      break;
    case ESTADO_ALARMA:
      lcd.print("Est: !! ALARMA !");
      break;
    default:
      lcd.print("Est: INICIAL    ");
      break;
  }
}

void enviarReporteUART() {
  Serial.print(F("Temp: "));
  Serial.print(temperatura);
  Serial.print(F(" C | Hum: "));
  Serial.print(humedad);
  Serial.print(F(" % | Luz (ADC): "));
  Serial.print(luzRaw);
  Serial.print(F(" | Estado: "));

  switch (estadoActual) {
    case ESTADO_MONITOREO:   Serial.println(F("NORMAL")); break;
    case ESTADO_ADVERTENCIA: Serial.println(F("ADVERTENCIA")); break;
    case ESTADO_ALARMA:      Serial.println(F("ALARMA")); break;
    default:                 Serial.println(F("INICIAL")); break;
  }
}
