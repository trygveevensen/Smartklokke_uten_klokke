//    IELS2003 Ingeniørprosjekt III - Vår 2026
//    Gruppe 1

//    Koden tilhører Invisi-Cane LITE, et navigasjonsverktøy for blinde og synshemmede.
//    Systemet består av tre vibrasjonsmotorer, et potensiometer, 
//      en ES32-C6 SuperMini, ToF-sensor VL53L5CX og spenningsforsyning.
// -------------------------------------

#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

const int MOTOR_VENSTRE = 2; 
const int MOTOR_MIDT    = 3; 
const int MOTOR_HOYRE   = 4;  
const int POT_PIN       = 1; 

// Globale variabler
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData;

const int MIN_DIST = 300;
const int MAX_DIST = 1000;

// Timer
hw_timer_t * timer = NULL;
volatile bool timerFlag = false;

void IRAM_ATTR onTimer() { 
  timerFlag = true; 
}

void setup() {
  Serial.begin(115200);

  // Initierer motor-utganger
  pinMode(MOTOR_VENSTRE, OUTPUT);
  pinMode(MOTOR_MIDT,    OUTPUT);
  pinMode(MOTOR_HOYRE,   OUTPUT);

  // Oppsett av ToF-sensoren med I2C
  Wire.begin(7, 6); 
  Wire.setClock(400000); 
  
  if (myImager.begin() == false) {
    Serial.println("ToF feilet - sjekk kabling!");
    while (1) delay(1000);
  }

  myImager.setResolution(8 * 8);  // For 8x8 rutenett
  myImager.setRangingFrequency(15); // Maks oppdateringsfrekves, 15 Hz
  myImager.startRanging();

  // Konfiguerer timer til å trigge 15 ganger i sekundet, for henting av verdier
  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 66667, true, 0); 
  
  Serial.println("System startet: Potensiometer styrer sensitivitet.");
}

void loop() {
  // Kjør 15 ganger i sekundet på interrupt:
  if (timerFlag) {
    timerFlag = false; 

    // Les sensitivitet fra potmeter (0 - 4095 for ESP32)
    int potVal = analogRead(POT_PIN);
    
    /*
       Sensitivitet justere med "threshold" (hvor mange punkter som trengs for 50% PWM).
       - Ved 0V: 1 punkt = 50% vibrasjon.
       - Ved 3.3V: 50% av sonens punkter = 50% vibrasjon.
    */

    // Hent alle 64 verdier fra sensor, både avstand og status
    if (myImager.isDataReady() && myImager.getRangingData(&measurementData)) {
      int countV = 0, countM = 0, countH = 0;

      for (int i = 0; i < 64; i++) {
        int dist = measurementData.distance_mm[i];
        int status = measurementData.target_status[i];

        // Hvis punktet har gyldig status og avstand mellom 30cm og 100cm, øk tilhørende sone-telling med 1
        if ((status == 5 || status == 9) && dist >= MIN_DIST && dist <= MAX_DIST) {
          int rad = i / 8; 
          if (rad <= 1)      countV++; // Sone venstre (16 punkter totalt)
          else if (rad >= 6) countH++; // Sone høyre (16 punkter totalt)
          else               countM++; // Sone midt (32 punkter totalt)
        }
      }

      // Mapper verdi fra potensitiometer til faktor som brukes i PWM
      float sensFactor = map(potVal, 0, 4095, 127, 16);    // Ved 3.3V trengs 8 punkter for 50% vibrasjon, 16*8=128 

      int pwmV = constrain(countV * sensFactor, 0, 255);
      int pwmM = constrain(countM * (sensFactor / 2.0), 0, 255); // Dele på 2 fordi sonen er dobbelt så stor
      int pwmH = constrain(countH * sensFactor, 0, 255);

      analogWrite(MOTOR_VENSTRE, pwmV);
      analogWrite(MOTOR_MIDT,    pwmM);
      analogWrite(MOTOR_HOYRE,   pwmH);
    }
  }
}
