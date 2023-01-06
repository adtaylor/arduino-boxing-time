#include <LiquidCrystal_I2C.h>
#include <YA_FSM.h>

#define redPin 10
#define yellowPin 11
#define greenPin 12
#define startButtonPin 7
#define timeButtonPin 6
#define buzzer 9

LiquidCrystal_I2C lcd(0x3F, 20, 4);

// Create new FSM
YA_FSM stateMachine;

// Logic variables
int setsCount;
long startTime;
long elapsedTime = 0;
int setLengths[] = {120, 180};
int restLength = 60;
int endingLength = 30;
int lastSetLength;
int setLength = setLengths[0];
bool startCallButton = false;
bool timeCallButton = false;

// Output variables
bool redLed = true;
bool greenLed = true;
bool yellowLed = true;
bool hasSetTimerStarted = false;
bool hasRestTimerStarted = false;
bool hasSessionStarted = false;
String displayLn1 = " ";
String displayLn2 = " ";

// State Alias
enum State
{
  OFF_STATE,
  START_CALL,
  WORK_STATE,
  ENDING_STATE,
  REST_STATE
};
// Helper for print labels instead integer when state change
const char *const stateName[] PROGMEM = {"OFF_STATE", "START_CALL", "WORK_STATE", "ENDING_STATE", "REST_STATE"};

/////////////////////////////////////////////////////////////////
// STATE MACHINE CALLBACK FUNCTIONS
/////////////////////////////////////////////////////////////////

void onEnter()
{
  Serial.print(stateMachine.ActiveStateName());
  Serial.println(F(" light ON"));
}

void onOff()
{
  // TODO
}

void onOffTick()
{
  if (timeCallButton)
  {
    delay(500); // time to get off the button
    setLength = (setLength == setLengths[0]) ? setLengths[1] : setLengths[0];
  }

  if (lastSetLength != setLength)
  {
    lastSetLength = setLength;
    playChange();
    printStartMessage();
  }
}

void onStartSet()
{
  Serial.print(stateMachine.ActiveStateName());
  playSetStart();
  startTime = millis();
  setsCount += 1;
  printLine2("Set: " + String(setsCount));
}

void onStartEnding()
{
  playSetEnding();
}

void onStartRest()
{
  Serial.print(stateMachine.ActiveStateName());
  playRest();
  startTime = millis();
  printLine2("Rest...");
}

void onStateSetTick()
{
  printLine1(formattedSeconds(setLength - elapsedTime));
}

void onStateRestTick()
{
  printLine1(formattedSeconds(restLength - elapsedTime));
}

void onExit()
{
  Serial.print(stateMachine.ActiveStateName());
  Serial.println(F(" light OFF\n"));
}

void onEnterStartCall()
{
  Serial.println(F("Call registered, please wait a little time."));
  hasSessionStarted = true;
  hasSetTimerStarted = true;
}

/////////////////////////////////////////////////////////////////
// SETUP THE STATE MACHINE
/////////////////////////////////////////////////////////////////

void setupStateMachine()
{
  stateMachine.AddState(stateName[OFF_STATE], 0, onEnter, onOffTick, onExit);
  stateMachine.AddState(stateName[START_CALL], 0, onEnterStartCall, nullptr, nullptr);
  stateMachine.AddState(stateName[WORK_STATE], 0, onStartSet, onStateSetTick, onExit);
  stateMachine.AddState(stateName[ENDING_STATE], 0, onStartEnding, onStateSetTick, onExit);
  stateMachine.AddState(stateName[REST_STATE], 0, onStartRest, onStateRestTick, onExit);

  stateMachine.AddAction(OFF_STATE, YA_FSM::S, redLed);
  stateMachine.AddAction(OFF_STATE, YA_FSM::S, yellowLed);
  stateMachine.AddAction(OFF_STATE, YA_FSM::S, greenLed);

  stateMachine.AddAction(WORK_STATE, YA_FSM::S, greenLed);
  stateMachine.AddAction(WORK_STATE, YA_FSM::R, redLed);
  stateMachine.AddAction(WORK_STATE, YA_FSM::R, yellowLed);
  stateMachine.AddAction(WORK_STATE, YA_FSM::S, hasSetTimerStarted);
  stateMachine.AddAction(WORK_STATE, YA_FSM::R, hasRestTimerStarted);

  stateMachine.AddAction(ENDING_STATE, YA_FSM::R, greenLed);
  stateMachine.AddAction(ENDING_STATE, YA_FSM::R, redLed);
  stateMachine.AddAction(ENDING_STATE, YA_FSM::S, yellowLed);

  stateMachine.AddAction(REST_STATE, YA_FSM::R, greenLed);
  stateMachine.AddAction(REST_STATE, YA_FSM::S, redLed);
  stateMachine.AddAction(REST_STATE, YA_FSM::R, yellowLed);
  stateMachine.AddAction(REST_STATE, YA_FSM::R, hasSetTimerStarted);
  stateMachine.AddAction(WORK_STATE, YA_FSM::S, hasRestTimerStarted);

  stateMachine.AddTransition(START_CALL, WORK_STATE, []()
                             { return hasSetTimerStarted && hasSessionStarted; });
  stateMachine.AddTransition(WORK_STATE, ENDING_STATE, []()
                             { return elapsedTime > (setLength - endingLength); });
  stateMachine.AddTransition(ENDING_STATE, REST_STATE, []()
                             { return elapsedTime > setLength; });
  stateMachine.AddTransition(REST_STATE, WORK_STATE, []()
                             { return hasRestTimerStarted && elapsedTime > restLength; });
  stateMachine.AddTransition(OFF_STATE, START_CALL, startCallButton);
}

/////////////////////////////////////////////////////////////////
// Sound and Vision helpers
/////////////////////////////////////////////////////////////////

String formattedSeconds(int displayTime)
{
  String minutes = String((displayTime / 60) % 60);
  String seconds = String(displayTime % 60);
  seconds = (seconds.length() == 1) ? "0" + seconds : seconds;
  return minutes + ":" + seconds;
}

void printLine1(String msg)
{
  lcd.setCursor(1, 0);
  lcd.print(msg);
}

void printLine2(String msg)
{
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print(msg);
}

void printStartMessage()
{
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Set len: " + formattedSeconds(setLength));
  lcd.setCursor(1, 1);
  lcd.print("*Press start*");
}

void playChange()
{
  tone(buzzer, 1000);
  delay(100);
  noTone(buzzer);
}

void playSetStart()
{
  tone(buzzer, 2000);
  delay(1000);
  noTone(buzzer);
}

void playSetEnding()
{
  for (int x = 0; x < 3; x++)
  {
    tone(buzzer, 1500);
    delay(100);
    noTone(buzzer);
    delay(100);
  }
}

void playRest()
{
  for (int x = 0; x < 2; x++)
  {
    tone(buzzer, 1000);
    delay(1000);
    noTone(buzzer);
    delay(1000);
  }
}

/////////////////////////////////////////////////////////////////
// Lifecycle
/////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  pinMode(buzzer, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(startButtonPin, INPUT);
  pinMode(timeButtonPin, INPUT_PULLUP);
  digitalWrite(startButtonPin, HIGH);
  digitalWrite(timeButtonPin, HIGH);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  Serial.println(F("Starting the Finite State Machine...\n"));
  setupStateMachine();
}

void loop()
{
  elapsedTime = (millis() - startTime) / 1000;
  // Read inputs
  startCallButton = (digitalRead(startButtonPin) == LOW);
  timeCallButton = (digitalRead(timeButtonPin) == LOW);

  // Update State Machine
  if (stateMachine.Update())
  {
    Serial.print(F("Active state: "));
    Serial.println(stateMachine.ActiveStateName());
  }

  // Set outputs
  digitalWrite(redPin, redLed);
  digitalWrite(yellowPin, yellowLed);
  digitalWrite(greenPin, greenLed);
}
