#include <LCD_I2C.h>
#include <Wire.h>
#include <AccelStepper.h>

#define MOTOR_INTERFACE_TYPE 4

#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37

#define TRIGGER_PIN 9
#define ECHO_PIN 10


LCD_I2C lcd(0x27, 16, 2);

AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

#define STEPS_PER_REVOLUTION 200
#define QUART_CIRCLE_STEPS (STEPS_PER_REVOLUTION / 2)


#define MIN_ANGLE 10
#define MAX_ANGLE 170
#define QUART_CIRCLE_STEPS_OPEN (STEPS_PER_REVOLUTION * (MAX_ANGLE - MIN_ANGLE) / 360.0)

long duration;
unsigned long distance;
String doorStatus = "Ferme";
unsigned long lastDistanceMeasurement = 0;
unsigned long lastDisplayUpdate = 0;
int currentAngle = MIN_ANGLE;


enum MotorState {
  ATTENTE,
  OUVERTURE,
  FERMETURE,
  MESURE_DISTANCE,
  MISE_A_JOUR_AFFICHAGE,
};

MotorState currentState = ATTENTE;

void setup() {
  lcd.begin();
  Serial.begin(9600);


  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("2419796");
  lcd.setCursor(0, 1);
  lcd.print("Labo 4A");

  delay(2000);



  lcd.clear();
  lcd.backlight();
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  lcd.print(doorStatus);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myStepper.setMaxSpeed(1000);
  myStepper.setAcceleration(500);
  myStepper.setSpeed(200);
}



unsigned long measureDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  return distance;
}
void updateDisplay() {
  lcd.clear();


  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Dist : ");
  lcd.print(distance);
  lcd.print(" cm");

  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print("Porte : ");
  lcd.print(doorStatus);


  if (doorStatus == "Ouverte") {
    currentAngle = MAX_ANGLE;
  } else {
    currentAngle = MIN_ANGLE;
  }

  Serial.print("etd: 2419796, dist: ");
  Serial.print(distance);
  Serial.print("cm");
  Serial.print(", deg: ");
  Serial.println(currentAngle);
}

void openDoor() {
  if (distance < 30) {
    lcd.backlight();
    lcd.setCursor(0, 1);
    doorStatus = "Ouverture";
    lcd.print(doorStatus);

    for (int degree = MIN_ANGLE; degree <= MAX_ANGLE; degree++) {
      int steps = map(degree, MIN_ANGLE, MAX_ANGLE, 0, QUART_CIRCLE_STEPS_OPEN);
      myStepper.moveTo(steps);
      while (myStepper.distanceToGo() != 0) {
        myStepper.run();
      }
      lcd.backlight();
      lcd.setCursor(0, 1);
      lcd.print("Ouverture: ");
      lcd.print(degree);
      lcd.print("dg    ");
      delay(20);
    }

    doorStatus = "Ouverte";
    lcd.backlight();
    lcd.setCursor(0, 1);
    lcd.print(doorStatus);
  }
}

void closeDoor() {
  if (distance > 60) {
    lcd.backlight();
    lcd.setCursor(0, 1);
    doorStatus = "Fermeture";
    lcd.print(doorStatus);

    for (int degree = MAX_ANGLE; degree >= MIN_ANGLE; degree--) {
      int steps = map(degree, MIN_ANGLE, MAX_ANGLE, 0, QUART_CIRCLE_STEPS_OPEN);
      myStepper.moveTo(steps);
      while (myStepper.distanceToGo() != 0) {
        myStepper.run();
      }
      lcd.backlight();
      lcd.setCursor(0, 1);
      lcd.print("Fermeture: ");
      lcd.print(degree);
      lcd.print("dg    ");
      delay(20);
    }
  }

  doorStatus = "Ferme";
  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print(doorStatus);
}


void loop() {
  switch (currentState) {
    case MESURE_DISTANCE:
      measureDistance();
      currentState = MISE_A_JOUR_AFFICHAGE;
      break;

    case MISE_A_JOUR_AFFICHAGE:
      updateDisplay();
      currentState = ATTENTE;
      break;

    case ATTENTE:
      if (distance < 30 && doorStatus == "Ferme") {
        currentState = OUVERTURE;
      } else if (distance > 60 && doorStatus == "Ouverte") {
        currentState = FERMETURE;
      }
      break;

    case OUVERTURE:
      openDoor();
      currentState = MISE_A_JOUR_AFFICHAGE;
      break;

    case FERMETURE:
      closeDoor();
      currentState = MISE_A_JOUR_AFFICHAGE;
      break;
  }


  if (millis() - lastDistanceMeasurement >= 50) {
    lastDistanceMeasurement = millis();
    currentState = MESURE_DISTANCE;
  }
}