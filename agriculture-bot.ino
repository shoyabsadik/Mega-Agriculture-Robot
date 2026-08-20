#include <SoftwareSerial.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ===== OBJECTS =====
Servo servo;
SoftwareSerial BTSerial(10, 11);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(2, DHT11);

// ===== PINS =====
#define pump A3
#define buzz 12
#define soil A7

#define trig 3
#define echo 4

#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9

// ===== VARIABLES =====
char cmd = '\0';
unsigned long lastSensorTime = 0;
unsigned long lastServoTime = 0;
unsigned long lastAutoTime = 0;
unsigned long lastBeepTime = 0;

bool servoRunning = false;
bool autoMode = false;
bool servoPos = false;
bool soilBeepFlag = false;

// ===== SETUP =====
void setup()
{
  pinMode(pump, OUTPUT);
  pinMode(buzz, OUTPUT);
  pinMode(soil, INPUT);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  digitalWrite(pump, HIGH); // Pump OFF

  Serial.begin(9600);
  BTSerial.begin(9600);

  dht.begin();
  servo.attach(5);
  servo.write(0);

  lcd.init();
  lcd.backlight();
}

// ===== LOOP =====
void loop()
{
  // ===== BLUETOOTH =====
  if (BTSerial.available())
  {
    cmd = BTSerial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd)
    {
      case 's':
        servoRunning = true;
        autoMode = false;
        break;

      case 'S':
        servoRunning = false;
        autoMode = false;
        stopMotor();
        servo.write(0);
        digitalWrite(pump, HIGH);
        break;

      case 'A':
        autoMode = true;
        servoRunning = false;
        break;

      case 'P':
        digitalWrite(pump, LOW);
        break;

      case 'O':
        digitalWrite(pump, HIGH);
        break;
    }
  }

  // ===== OBSTACLE (SCARECROW) =====
  float dist = getDistance();
  bool obstacle = (dist > 0 && dist < 30);

  if (obstacle)
  {
    // continuous beep (non-blocking style)
    if (millis() - lastBeepTime > 200)
    {
      lastBeepTime = millis();
      digitalWrite(buzz, !digitalRead(buzz));
    }
  }
  else
  {
    digitalWrite(buzz, LOW);
  }

  // ===== SERVO CONTINUOUS =====
  if (servoRunning && millis() - lastServoTime > 500)
  {
    lastServoTime = millis();
    servoPos = !servoPos;
    servo.write(servoPos ? 90 : 0);
  }

  // ===== AUTO MODE =====
  if (autoMode && millis() - lastAutoTime > 1000)
  {
    lastAutoTime = millis();

    forward();
    delay(500);
    stopMotor();

    servo.write(0);
    delay(300);
    servo.write(90);
    delay(300);
    servo.write(0);
  }

  // ===== SENSOR =====
  if (millis() - lastSensorTime > 1500)
  {
    lastSensorTime = millis();

    int soil_raw = analogRead(soil);
    int soil_value = map(soil_raw, 1023, 0, 0, 100);

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    // Soil beep (ONLY if no obstacle)
    if (!obstacle)
    {
      if (soil_value == 0 && !soilBeepFlag)
      {
        beep();
        soilBeepFlag = true;
      }
      else if (soil_value > 0)
      {
        soilBeepFlag = false;
      }
    }

    // ===== LCD =====
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temp);
    lcd.print(" H:");
    lcd.print(humi);

    lcd.setCursor(0, 1);
    lcd.print("Soil:");
    lcd.print(soil_value);
    lcd.print("%");
  }

  // ===== MANUAL CONTROL =====
  if (!autoMode)
    controlCar();
}

// ===== CONTROL =====
void controlCar()
{
  switch (cmd)
  {
    case 'F': forward(); break;
    case 'B': backward(); break;
    case 'L': left(); break;
    case 'R': right(); break;
  }
}

// ===== DISTANCE =====
float getDistance()
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 20000);
  return duration / 58.82;
}

// ===== MOTOR =====
void forward()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void backward()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopMotor()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ===== BUZZER =====
void beep()
{
  digitalWrite(buzz, HIGH);
  delay(100);
  digitalWrite(buzz, LOW);
}
