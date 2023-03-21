/*************************************************************
  Filename - kitchenAutomationProjectV1 - NodeMCU version
  Description - Designed for my wife special requirment for kitchen switch/bulb automation.
  Requirement -
    Active Time frame between - 6AM to 7AM and 6PM to 9PM 
    1. Whenever switch is turned ON btw above time frame, the bulb turns ON for 30 minutes & then it turns OFF. So again if you want to turn it ON then you
       need to press the switch OFF and then ON.
    2. Work normally when switch is turned OFF. Irrespective of time left, the bulb has to be turned OFF.

  version - v1.0.1
  Updates/ Fixes [status]
  > [Fixed] Checking current time fall btw two time slot.
  > [Fixed] - Timer set for 30 minutes, scenario - 11.45 and setting the timer now

  Debug instructions -
  1> Always check the active hours for bathroom lighting. Standard time - 6pm to 7am
  2> Check for bathroom light duration in minute. means how long the bulb will glow once turned ON
 *************************************************************/
#include <ThreeWire.h>  
#include <RtcDS1302.h>

// ThreeWire myWire(D4,D5,D2); // IO, SCLK, CE
ThreeWire myWire(D4,D5,D2);
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime currentTime;

// Basic configuration variables
// Tell whether LED bulb is On/ Off
boolean isKitchenLedOn = false;

// Change this value Accordlingly
int nonActiveHourDuration = 5;    // in minute
int activeHourDuration = 30;    // in minute
int kitchenLightOnDuration = nonActiveHourDuration;    // In minutes

// To maintain the active alarms
struct alarm {
    int alarmId;
    int alarmType;  // 1 - Daily, 2 - Weekly, 3 - Monthly
    int isAlarmSet;
    // Flag indicate whether alarm triggered or not
    int isAlarmTriggered;
    int duration;
    char whomToActivate[25];

    int endTimeHour;
    int endTimeMinute;
    int endTimeSecond;
};

// Time for various comparision
struct TIME {
    int hours;
    int minutes;
};

// Created this struct to maintain more than one alarms i.e. one for kitchen bulb
// 0 - kitchen timer
struct alarm activeAlarm[1];

/* GPIO Pin configuration details */
/* ------------------------------ */
// Sensors/ Relay connection details with GPIOs pins
// D2, D4 and D5 allotted to RTC module DS1302
#define kitchBulbRelay D1
#define kitchenBulbSwitch D6
#define countof(a) (sizeof(a) / sizeof(a[0]))

/*
Active Time frame between - 6AM to 7AM and 6PM to 9PM 
    1. Whenever switch is turned ON btw above time frame, the bulb turns ON for 30 minutes & then it turns OFF. So again if you want to turn it ON then you
       need to press the switch OFF and then ON.
    2. Work normally when switch is turned OFF. Irrespective of time left, the bulb has to be turned OFF.
*/
// Active hours
struct TIME morningActiveStartTime = {6, 0};    // 6.00AM to 7.00AM
struct TIME morningActiveEndTime = {7, 0};
struct TIME eveningActiveStartTime = {18, 0};   // 6.00PM to 10.00PM 
struct TIME eveningActiveEndTime = {22, 0};

// Indicate (boolean) if time if greater/less than given time
bool diffBtwTimePeriod(struct TIME start, struct TIME stop) {
   while (stop.minutes > start.minutes) {
      --start.hours;
      start.minutes += 60;
   }

   return (start.hours - stop.hours) >= 0;
}

// Set light ON duration in case time fall btw active hours
void checkActiveHours() {
    boolean morningActiveHour = diffBtwTimePeriod({currentTime.Hour(), currentTime.Minute()}, morningActiveStartTime) && diffBtwTimePeriod(morningActiveEndTime, {currentTime.Hour(), currentTime.Minute()});
    boolean eveningActiveHour = diffBtwTimePeriod({currentTime.Hour(), currentTime.Minute()}, eveningActiveStartTime) && diffBtwTimePeriod(eveningActiveEndTime, {currentTime.Hour(), currentTime.Minute()});

    if (morningActiveHour || eveningActiveHour) {
        // Keep the bulb On when its a active hour
        kitchenLightOnDuration = activeHourDuration;
        Serial.println("duration : ");
        Serial.print(activeHourDuration);
    } else {
        // Default light duration
        kitchenLightOnDuration = nonActiveHourDuration;
        Serial.println("duration : ");
        Serial.print(nonActiveHourDuration);
    }
}

void printDateTime(const RtcDateTime& dt) {
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

/*
* This function write data into two places - NodeMCU file system (for offline support) and for google sheet API
*/
void actionMessageLogger(String message) {
    Serial.println(message);
    printDateTime(currentTime);
}

// turnBulb("ON", "OutBulb")
void turnBulb(String action, String bulbLocation) {
    if (bulbLocation == "kitchenBulb") {
        // Turn ON the bulb by making relay LOW
        // Turn OFF the bulb by making relay HIGH
        digitalWrite(kitchBulbRelay, action == "ON" ? HIGH : LOW);
        actionMessageLogger(action == "ON" ? "KitchenBulb :: Turn ON the kitchen bulb" : "KitchenBulb :: Turn OFF the kitchen bulb");
    }
}

void unsetAlarm(int alarmId) {
    Serial.println("Removed the alarm");
    activeAlarm[alarmId].isAlarmSet = 0;
    // Set this flag so that next time, it can be set again
    activeAlarm[alarmId].isAlarmTriggered = 1;
}

// duration = for how long you want to set alarm
// alarmType = 1 for hour | 2 for Minute
void setAlarm(int alarmId, int alarmType) {
    checkActiveHours();

    int tempMinute = 0;
    int tempHour = 0;

    int duration = kitchenLightOnDuration;
    // Bathroom Timer duration
    activeAlarm[alarmId].alarmId = alarmId;
    activeAlarm[alarmId].alarmType = alarmType;
    // tell that alarm is set
    activeAlarm[alarmId].isAlarmSet = 1;
    // Set this flag so that next time, it can be set again
    activeAlarm[alarmId].isAlarmTriggered = 0;
    activeAlarm[alarmId].duration = duration;

    // alarmType for Hour
    tempHour = currentTime.Hour() + duration;
    activeAlarm[alarmId].endTimeHour = alarmType == 1 ? (tempHour > 23 ? (tempHour - 24) : tempHour) : currentTime.Hour();

    // alarmType for Minute
    tempMinute = currentTime.Minute() + duration;
    if (alarmType == 2) {
        if (tempMinute > 59) {
            activeAlarm[alarmId].endTimeMinute = tempMinute - 60;
            tempHour = currentTime.Hour() + 1;
            activeAlarm[alarmId].endTimeHour = tempHour > 23 ? (tempHour - 24) : tempHour;
        } else {
            activeAlarm[alarmId].endTimeMinute = tempMinute;
        }
    } else {
        activeAlarm[alarmId].endTimeMinute = currentTime.Minute();
    }

    // activeAlarm[alarmId].endTimeMinute = alarmType == 2 ? (temp > 59 ? (temp - 60) : temp) : currentTime.Minute();

    activeAlarm[alarmId].endTimeSecond = currentTime.Second();
    // alarmId = 1 [window bulb setup] and bulb is not glowing then only setup this
    /* Condition - 
    *   1. Whenever switch is turned ON, the bulb turned ON for 5 minute. So if user want to turn ON the bulb again then he has to turn this ON again.
    *   2. Work normally when switch is turned OFF. Irrespective of time left, the bulb has to be turned OFF.
    */
    if (alarmId == 0) {
        turnBulb("ON", "kitchenBulb");
    }
}

void matchAlarm() {
    // Match condition
    for (int i=0; i < 2; i++) {
        if (currentTime.Hour() == activeAlarm[i].endTimeHour && currentTime.Minute() >= activeAlarm[i].endTimeMinute && currentTime.Second() >= activeAlarm[i].endTimeSecond && !activeAlarm[i].isAlarmTriggered) {
            actionMessageLogger("matchAlarm :: Timer Matched - alarm triggered");
            activeAlarm[i].isAlarmSet = 0;
            activeAlarm[i].isAlarmTriggered = 1;

            // kitchenBulb handler - Check if current time reached the Off Timer and LED still ON
            if (activeAlarm[i].alarmId == 0) {
                turnBulb("OFF", "kitchenBulb");
            }
        }
    }
}

void kitchen_control() {
    // when kitchen switch pressed = ON
    if (digitalRead(kitchenBulbSwitch) == LOW && !isKitchenLedOn) {
        if (activeAlarm[0].isAlarmSet == 0) {
            Serial.println("button pressed ON, setting alarm");
            // int alarmId, int duration, int alarmType
            setAlarm(0, 2);
            isKitchenLedOn = true;
        }
    }

    // when kitchen switch pressed = OFF
    if (digitalRead(kitchenBulbSwitch) == HIGH && isKitchenLedOn) {
        Serial.println("button pressed OFF");
        unsetAlarm(0);
        isKitchenLedOn = false;
        turnBulb("OFF", "kitchenBulb");
        delay(100);
    }

    matchAlarm();
}

void rtcSetup() {
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
}

void setup() {
    
    // Debug console
    Serial.begin(57600);
    // Serial.begin(9600);
    delay(500);

    // Initial setup
    pinMode(kitchBulbRelay, OUTPUT);
    pinMode(kitchenBulbSwitch, INPUT_PULLUP);

    // Setup the RTC mmodule
    rtcSetup();
    Serial.println("Setup :: Setup completed");
}

void loop() {
    currentTime = Rtc.GetDateTime();

    if (!currentTime.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }

    // Serial.println("currentTime");
    // call all manual control i.e. switches, relay
    kitchen_control();

    delay(1000);
}