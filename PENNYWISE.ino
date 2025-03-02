#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

Servo myservo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int  SS_PIN = 9;
const int RST_PIN = 10;

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

#define COIN_PIN 2
const int btn1 = 5;
const int btn2 = 6;
const int btn3 = 7;
const int pinServo = 3;
const int relay = 4;

bool btn1Pressed = false;
bool btn2Pressed = false;

float moneyCOM1 = 0.0;
float moneyCOM2 = 0.0;
float currentBalance = 0.0; // Temporary balance before allocation

volatile int pulses = 0;
volatile long timeLastPulse = 0;

const int interval = 30000;
int remainingTime = 30;
unsigned long lastUpdate = 0;
unsigned long prevTime = 0;
bool isActivated = false;
int servoTargetAngle = 90;
bool com1Selected = false;
bool com2Selected = false;

String validRFIDs[] = {
  "D3 4F 37 E4", "E4 17 81 DF", "24 4C 88 DF", 
  "D7 91 0B 65"
};

void servo1() {
  myservo.write(0);
}

void servo2() {
  myservo.write(180);
}

void servoReset() {
  myservo.write(90);
}

void scanRFIDPrompt() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Scan Your RFID");
}

bool validateRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return false;
  }
  Serial.println("[DEBUG] pass here!");
  String scannedRFID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    scannedRFID += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) scannedRFID += " ";
  }
  scannedRFID.toUpperCase();

 for (int i = 0; i < 3; i++) {  // Check only indices 0 to 2
      if (scannedRFID == validRFIDs[i]) {
        Serial.println("RFID Accepted: " + scannedRFID);
        switch (i) {
          case 0:
            servo1();
            break;
          case 1:
            servo2();
            break;
          case 2:
            servoReset();
            break;
          case 3:
            digitalWrite(relay, HIGH);
            delay(5000);
            digitalWrite(relay, LOW);
            break;
        }
        return true;
      }
  }


  Serial.println("RFID Denied: " + scannedRFID);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Access Denied!");
  delay(2000);
  scanRFIDPrompt();
  return false;
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("C1 Bal: ");
  lcd.print(moneyCOM1);
  lcd.setCursor(0, 1);
  lcd.print("C2 Bal: ");
  lcd.print(moneyCOM2);
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Money Deposited:");
  lcd.setCursor(0, 1);
  lcd.print("$");
  lcd.print(currentBalance, 2);
}

void updateTimerDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time Left: ");
  lcd.setCursor(0, 1);
  lcd.print(remainingTime);
  lcd.print(" sec");
}

void setup() {
  Serial.begin(9600);
  Serial.println("Ready...");

  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);

  //scanRFIDPrompt();
  showMenu();
  pinMode(COIN_PIN, INPUT_PULLUP);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(relay, OUTPUT);

  digitalWrite(relay, LOW);

  myservo.attach(pinServo);
  myservo.write(90);

  attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinISR, FALLING);
}

void coinISR() {
  pulses++;
  timeLastPulse = millis();
}

void loop() {

  // if (!validateRFID()) {
  //   return;  // Keep scanning for a valid card
  // }

  if(Serial.available()) {
    String command = Serial.readStringUntil('\n');
      command.trim();
    if(command == "S1")
      myservo.write(30);
    else if(command == "S2")
      myservo.write(150);
    else if(command == "A"){
        digitalWrite(relay, HIGH);
      delay(5000);
      digitalWrite(relay, LOW);
    }

  }
 
  unsigned long currentTime = millis();
  long timeFromLastPulse = currentTime - timeLastPulse;

  if (pulses > 0 && timeFromLastPulse > 200) {
    if (pulses == 1) currentBalance += 1;
    else if (pulses == 2) currentBalance += 1;
    else if (pulses == 5) currentBalance += 5;
    else if (pulses == 10) currentBalance += 10;
    else if (pulses == 20) currentBalance += 20;

    updateLCD();
    pulses = 0;
    remainingTime = 30;
    isActivated = true;
    lastUpdate = currentTime;
  }

  if (digitalRead(btn1) == LOW && !btn1Pressed) {
    btn1Pressed = true;
    com1Selected = true;
    com2Selected = false;
    moneyCOM1 += currentBalance;
    currentBalance = 0;
    Serial.println("Compartment 1 Activated");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected");
    lcd.setCursor(0, 1);
    lcd.print("COM 1");
    prevTime = millis();
    isActivated = true;
  } else if (digitalRead(btn1) == HIGH) {
    btn1Pressed = false;
  }

  if (digitalRead(btn2) == LOW && !btn2Pressed) {
    btn2Pressed = true;
    com1Selected = false;
    com2Selected = true;
    moneyCOM2 += currentBalance;
    currentBalance = 0;
    Serial.println("Compartment 2 Activated");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected");
    lcd.setCursor(0, 1);
    lcd.print("COM 2");
    prevTime = millis();
    isActivated = true;
  } else if (digitalRead(btn2) == HIGH) {
    btn2Pressed = false;
  }

  if (isActivated && currentTime - lastUpdate >= 1000) {
    lastUpdate = currentTime;
    if (remainingTime > 0) {
      remainingTime--;
      updateTimerDisplay();
      Serial.print("Remaining Time: ");
      Serial.println(remainingTime);
    } else {
      isActivated = false;
      Serial.println("Timer Stopped.");
      showMenu();
      servoReset();
      // delay(7000);
      // scanRFIDPrompt();
    }
  }
}
