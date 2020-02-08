/*
   Starts a pump motor every 8 hours for 4 minutes.
   Actives a light for 16 hours and deactivates for 8 hours.
   Button input can stop a pump cycle or start a manual one.
   Button input can reset the sync time point.

   Also actives the build-in LED when the pump motor is active
   for easier testing and debugging
*/

const bool DEBUG = true;

/** The number of the pin which controls the pump motor. */
const unsigned int PUMP_PIN = 4;

/** The number of the pin which controls the light. */
const unsigned int LIGHT_PIN = 5;

/** The number of the input pin for the pump override button. */
const unsigned int BUTTON_PUMP_PIN = 8;

/** The number of the input pin for the time reset button. */
const unsigned int BUTTON_TIME_PIN = 9;

const unsigned int RELAY_OFF = HIGH;

const unsigned int RELAY_ON = LOW;

/** The value of one second in millis. */
const unsigned long SECOND = 1000;

/** The value of one minute in millis. */
const unsigned long MINUTE = 60 * SECOND;

/** The value of one hour in millis. */
const unsigned long HOUR = 60 * MINUTE;

/** How much time should pass between the start of two pumping sequences. */
const unsigned long TIME_BETWEEN_PUMP_STARTS = DEBUG ? 30 * SECOND :  8 * HOUR;

/** How long one pump sequence should last. */
const unsigned long PUMP_DURATION = DEBUG ? 10 * SECOND : 4 * MINUTE;

const unsigned long LIGHT_OFF_DURATION = DEBUG ? 30 * SECOND : 8 * HOUR;

const unsigned long LIGHT_ON_DURATION = DEBUG ? 60 * SECOND : 16 * HOUR;


unsigned long lightRefMillis;
unsigned long pumpRefMillis;
bool lightActive;
bool pumpActive;

bool pumpOverrideActive;
/** true if the override switched the pump on. false if the override switched it off. */
bool pumpOverrideMode;
unsigned long pumpOverrideRefMillis = 0;

/*
   TODO:
   finish comments
   prod debug option
*/


/** The setup function runs once when you press reset or power the board. */
void setup() {
  // initialize serial communication at 9600 bits per second
  Serial.begin(9600);

  // initialize all relevant digital pins as output or input
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PUMP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TIME_PIN, INPUT_PULLUP);

  initState(millis());

  char debugInfo[100];
  sprintf(
    debugInfo,
    "pump duration: %lu, time between pump starts: %lu, light: %lu / %lu",
    PUMP_DURATION,
    TIME_BETWEEN_PUMP_STARTS,
    LIGHT_OFF_DURATION,
    LIGHT_ON_DURATION);
  Serial.println(debugInfo);
}

void initState(unsigned long nowMillis) {
  pumpOverrideActive = false;
  setLightOff(nowMillis);
  setPumpOn(nowMillis);
}


/** loop() is called repeatedly forever. */
void loop() {
  unsigned long nowMillis = millis();

  unsigned long lightPassedTimeMillis = light(nowMillis);
  unsigned long pumpPassedTimeMillis = pump(nowMillis);

  int timeButtonState = digitalRead(BUTTON_TIME_PIN);
  if (timeButtonState == LOW) {
    Serial.print("Resetting refTimes to ");
    Serial.println(nowMillis);

    // briefly set the light on to give feedback on the button press
    setLightOn(nowMillis);
    delay(3000);

    // do the actual reset which also turns the light off
    initState(nowMillis);
  }

  int pumpButtonState = digitalRead(BUTTON_PUMP_PIN);
  if (pumpButtonState == LOW) {
    pumpOverride(nowMillis);
  }

  if (pumpOverrideActive && pumpOverrideMode) {
    unsigned long passedTimeMillis = nowMillis - pumpOverrideRefMillis;
    if (passedTimeMillis > PUMP_DURATION) {
      Serial.println("Pump override OFF (expired)");
      directPumpAccess(false);
      pumpOverrideActive = false;
    }
  }

  if (DEBUG) {
    char debugInfo[100];
    sprintf(
      debugInfo,
      "pump: %s%s %lu ms %lu m, light: %s %lu ms %lu m",
      pumpActive ? "ON" : "OFF",
      pumpOverrideActive ? (pumpOverrideMode ? "(*ON*)" : "(*OFF*)") : "",
      pumpPassedTimeMillis,
      pumpPassedTimeMillis / MINUTE,
      lightActive ? "ON" : "OFF",
      lightPassedTimeMillis,
      lightPassedTimeMillis / MINUTE);
    Serial.println(debugInfo);
  }

  // Wait a short time until the next loop is run.
  // This is mostly to limit the amount of debug out which is generated.
  delay(DEBUG ? 1000 : 10);
}

/** Start with OFF period, then ON period, then repeat. */
unsigned long light(unsigned long nowMillis) {
  unsigned long passedTimeMillis = nowMillis - lightRefMillis;

  if (lightActive) {
    if (passedTimeMillis > LIGHT_ON_DURATION) {
      setLightOff(nowMillis);
    }
  } else {
    if (passedTimeMillis > LIGHT_OFF_DURATION) {
      setLightOn(nowMillis);
    }
  }

  return passedTimeMillis;
}

void setLightOff(unsigned long nowMillis) {
  Serial.println("Light OFF");
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  lightActive = false;
  lightRefMillis = nowMillis;
}

void setLightOn(unsigned long nowMillis) {
  Serial.println("Light ON");
  digitalWrite(LIGHT_PIN, RELAY_ON);
  lightActive = true;
  lightRefMillis = nowMillis;
}

unsigned long pump(unsigned long nowMillis) {
  unsigned long passedTimeMillis = nowMillis - pumpRefMillis;

  if (pumpActive) {
    if (passedTimeMillis > PUMP_DURATION) {
      setPumpOff();
    }
  } else {
    if (passedTimeMillis > TIME_BETWEEN_PUMP_STARTS) {
      setPumpOn(nowMillis);
    }
  }

  return passedTimeMillis;
}

void setPumpOff() {
  directPumpAccess(false);
  pumpActive = false;

  if (pumpOverrideActive) {
    Serial.println("Pump override OFF (state change)");
    pumpOverrideActive = false;
  }
}

void setPumpOn(unsigned long nowMillis) {
  directPumpAccess(true);
  pumpActive = true;
  pumpRefMillis = nowMillis;

  if (pumpOverrideActive) {
    Serial.println("Pump override OFF (state change)");
    pumpOverrideActive = false;
  }
}

void pumpOverride(unsigned long nowMillis) {
  unsigned long passedTimeMillis = nowMillis - pumpOverrideRefMillis;
  if (passedTimeMillis < 3000) {
    Serial.print("Ignoring pump button press since the last one was only ");
    Serial.print(passedTimeMillis);
    Serial.println(" ms ago.");
    return;
  }
  
  pumpOverrideRefMillis = nowMillis;

  if (pumpOverrideActive) {
    Serial.println("Pump override OFF (button)");
    pumpOverrideActive = false;
    
    if (pumpOverrideMode) {
      directPumpAccess(false);
    } else {
      directPumpAccess(true);
    }
  } else {
    Serial.println("Pump override ON");
    pumpOverrideActive = true;

    if (pumpActive) {
      directPumpAccess(false);
      pumpOverrideMode = false;
    } else {
      directPumpAccess(true);
      pumpOverrideMode = true;
    }
  }
}

void directPumpAccess(bool on) {
  if (on) {
    Serial.println("Pump ON");
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(PUMP_PIN, RELAY_ON);
  } else {
    Serial.println("Pump OFF");
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(PUMP_PIN, RELAY_OFF);
  }
}
