// Koden leser av ToF-sensor og vibrerer tre motor med ulik intensitet.

#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData;

const int MOTOR_VENSTRE = 12; // Mapper til Topp-radene (0-1)
const int MOTOR_MIDT    = 13; // Mapper til Midt-radene (2-5)
const int MOTOR_HOYRE   = 2; // Mapper til Bunn-radene (6-7)
const int LED = 4;

const int MIN_DIST = 300;
const int MAX_DIST = 1000;

hw_timer_t * timer = NULL;
volatile bool timerFlag = false;

void IRAM_ATTR onTimer() { timerFlag = true; }

void setup() {

  pinMode(MOTOR_VENSTRE, OUTPUT);
  pinMode(MOTOR_MIDT, OUTPUT);
  pinMode(MOTOR_HOYRE, OUTPUT);
  pinMode(LED, OUTPUT);

  Wire.begin(15,14);
  Wire.setClock(100000);
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);
  delay(100);
  if (myImager.begin() == false) {
    digitalWrite(LED, LOW);
    while (1);
  }
  myImager.setResolution(8 * 8);
  myImager.setRangingFrequency(15);
  myImager.setSharpenerPercent(0); 
  myImager.startRanging();

  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 66667, true, 0); 
}

void loop() {
  if (timerFlag) {
    timerFlag = false; 

    if (myImager.isDataReady() && myImager.getRangingData(&measurementData)) {
      
      int countV = 0, countM = 0, countH = 0;
      bool deteksjonsMatrise[64];

      for (int i = 0; i < 64; i++) {
        int dist = measurementData.distance_mm[i];
        int status = measurementData.target_status[i];
        
        digitalWrite(LED, LOW);

        if ((status == 5) && dist >= MIN_DIST && dist <= MAX_DIST) {
          deteksjonsMatrise[i] = true;
          
          int rad = i / 8; // Gir oss radnummer 0 til 7

          if (rad <= 1) {
            countV++; // Topp-rader (bruker motor venstre)
          } else if (rad >= 6) {
            countH++; // Bunn-rader (bruker motor høyre)
          } else {
            countM++; // Midtre rader
          }
        } else {
          deteksjonsMatrise[i] = false;
        }
      }

      // PWM-styring
      int powerV = map(countV, 0, 16, 0, 255);
      int powerH = map(countH, 0, 16, 0, 255);
      int powerM = map(countM, 0, 32, 0, 255);


      analogWrite(MOTOR_VENSTRE, powerV);
      analogWrite(MOTOR_MIDT, powerM);
      analogWrite(MOTOR_HOYRE, powerH);

      static int skip = 0;
      if (++skip >= 10) {
        for (int y = 0; y < 8; y++) {
          for (int x = 0; x < 8; x++) {
            int index = (y * 8) + x;

          }
        }
        skip = 0;
      }
    }
  }
}
