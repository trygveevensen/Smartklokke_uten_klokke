#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

// --- PIN DEFINISJONER ---
const int MOTOR_VENSTRE = 2; 
const int MOTOR_MIDT    = 3; 
const int MOTOR_HOYRE   = 4;  
const int POT_PIN       = 1; // Endret fra knapp til analog pin (f.eks. GPIO 1 / A1)

// --- GLOBALE VARIABLER ---
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData;

const int MIN_DIST = 300;
const int MAX_DIST = 1000;

hw_timer_t * timer = NULL;
volatile bool timerFlag = false;

void IRAM_ATTR onTimer() { 
  timerFlag = true; 
}

void setup() {
  Serial.begin(115200);
  
  pinMode(MOTOR_VENSTRE, OUTPUT);
  pinMode(MOTOR_MIDT,    OUTPUT);
  pinMode(MOTOR_HOYRE,   OUTPUT);
  // Potmeter trenger normalt ikke pinMode(INPUT), men greit å merke seg at den er analog

  // ToF Sensor Setup
  Wire.begin(7, 6);
  Wire.setClock(400000); 
  
  if (myImager.begin() == false) {
    Serial.println("ToF feilet - sjekk kabling!");
    while (1) delay(1000);
  }

  myImager.setResolution(8 * 8); 
  myImager.setRangingFrequency(15);
  myImager.startRanging();

  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 66667, true, 0); 
  
  Serial.println("System startet: Potensiometer styrer sensitivitet.");
}

void loop() {
  if (timerFlag) {
    timerFlag = false; 

    // 1. Les sensitivitet fra potmeter (0 - 4095 for ESP32)
    int potVal = analogRead(POT_PIN);
    
    /* LOGIKK FOR SENSITIVITET:
       Vi ønsker å justere "threshold" (hvor mange punkter som trengs for 50% PWM).
       - Ved 0V: 1 punkt = 50% vibrasjon.
       - Ved 3.3V: 50% av sonens punkter = 50% vibrasjon.
    */
    
    if (myImager.isDataReady() && myImager.getRangingData(&measurementData)) {
      int countV = 0, countM = 0, countH = 0;

      for (int i = 0; i < 64; i++) {
        int dist = measurementData.distance_mm[i];
        int status = measurementData.target_status[i];

        if ((status == 5 || status == 9) && dist >= MIN_DIST && dist <= MAX_DIST) {
          int rad = i / 8; 
          if (rad <= 1)      countV++; // Sone venstre (16 punkter totalt)
          else if (rad >= 6) countH++; // Sone høyre (16 punkter totalt)
          else               countM++; // Sone midt (32 punkter totalt)
        }
      }

      // Beregn utgangseffekt basert på potensiometer
      // Vi mapper potVal til en divisor eller multiplikator.
      // Ved 0V (potVal=0) skal 1 treff gi ~127 PWM.
      // Ved 3.3V (potVal=4095) skal f.eks. 8 treff (venstre) gi ~127 PWM.
      
      float sensFactor = map(potVal, 0, 4095, 127, 16); // Omvendt mapping for følsomhet

      int pwmV = constrain(countV * (sensFactor / 1.0), 0, 255);
      int pwmM = constrain(countM * (sensFactor / 2.0), 0, 255); // Dele på 2 fordi sonen er dobbelt så stor
      int pwmH = constrain(countH * (sensFactor / 1.0), 0, 255);

      analogWrite(MOTOR_VENSTRE, pwmV);
      analogWrite(MOTOR_MIDT,    pwmM);
      analogWrite(MOTOR_HOYRE,   pwmH);
    }
  }
}
