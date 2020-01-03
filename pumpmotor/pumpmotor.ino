/*
 * Starts a motor every 8 hours for 5 minutes.
 * 
 * Also actives the build-in LED when the motor is active
 * for easier testing and debugging
 */

const bool DEBUG = false;

/** The number of the pin which controls the pump motor. */
const unsigned int MOTOR_PIN = 4;

/** The value of one second in millis. */
const unsigned long SECOND = 1000;

/** The value of one minute in millis. */
const unsigned long MINUTE = 60 * SECOND;

/** The value of one hour in millis. */
const unsigned long HOUR = 60 * MINUTE;

/** How much time should pass between the start of two pumping sequences. */
const unsigned long TIME_BETWEEN_PUMP_STARTS = DEBUG ? 30 * SECOND :  8 * HOUR;

/** How long one pump sequence should last. */
const unsigned long PUMP_DURATION = DEBUG ? 10 * SECOND : 5 * MINUTE;

/** 
 * Point in time when the device was started or reset in millis. 
 * 
 * Since the millis() function always returns the time since the device was
 * started this variable will be zero.
 * 
 * On Arduino long is 4 byte so the millis() function will overflow 
 * after a little over 49 days.
 */
unsigned long referenceTimeMillis;


/** The setup function runs once when you press reset or power the board. */
void setup() {
  // initialize serial communication at 9600 bits per second
  Serial.begin(9600);
  
  // initialize digital pins LED_BUILTIN and MOTOR_PIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  referenceTimeMillis = millis();

  char debugInfo[100];
  sprintf(
      debugInfo,
      "pump duration: %lu, time between pump starts: %lu", 
      PUMP_DURATION,
      TIME_BETWEEN_PUMP_STARTS);
  Serial.println(debugInfo);
}


/** loop() is called repeatedly forever. */
void loop() {
  unsigned long nowMillis = millis();
  unsigned long millisSinceLastPumpStart 
      = (nowMillis - referenceTimeMillis) % TIME_BETWEEN_PUMP_STARTS;

  char debugInfo[100];
  sprintf(
      debugInfo,
      "millis: %lu, minutes: %lu", 
      millisSinceLastPumpStart,
      millisSinceLastPumpStart / MINUTE);
  Serial.println(debugInfo);
  
  if (millisSinceLastPumpStart < PUMP_DURATION) {
    // If we are at the start of an 8 hour period start a pump sequence.
    pumpSequence();
  } else {
    // Wait a short time until the next loop is run.
    // This is mostly to limit the amount of debug out which is generated.
    delay(1000);
  }
}

/**
 * Excecutes one full pump sequence.
 * 
 * Actives the motor and build-in LED, waits for the pump sequence duration
 * and then switches both off again.
 */
void pumpSequence() {
  Serial.println("Starting motor");
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(MOTOR_PIN, LOW);
  delay(PUMP_DURATION);
  
  Serial.println("Stopping motor");
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(MOTOR_PIN, HIGH);

  // A tiny extra delay to avoid starting another pump sequence right away because
  // we are still (just) in the time range where pumping starts.
  delay(10);
}
