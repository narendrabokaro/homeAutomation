/*
* Basic automation requirement - provide one Blynk app switch to turn ON/OFF the corner lamp
* Pending - Time-based automation for the switch
*/

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2dIz_IQKc"
#define BLYNK_TEMPLATE_NAME "XXXXXXXXXXXX"
#define BLYNK_AUTH_TOKEN "ZZZZZZZZZZZ-ydl48xtA0_jhl4jc"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi credentials. [Change the password]
char ssid[] = "Spectrum326";
char pass[] = "<Enter your password>";

// virtual pins for bulb
#define VPIN_BUTTON_1 V0
#define VPIN_BUTTON_2 V1
#define roomLightLedRelaySwitch D1

bool roomLightSwtich = LOW;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncVirtual(VPIN_BUTTON_1);
}

// When App button is pushed - switch the state
BLYNK_WRITE(VPIN_BUTTON_1) {
  roomLightSwtich = param.asInt();
  if(roomLightSwtich == 1){
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
    // light switched off by making relay high
    digitalWrite(roomLightLedRelaySwitch, HIGH);
  } else { 
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on (Note that LOW is the voltage level
    // light switched off by making relay high
    digitalWrite(roomLightLedRelaySwitch, LOW);
  }
}

// When App button is pushed - show the latest time on label
BLYNK_WRITE(VPIN_BUTTON_2) {
  timeClient.update();

  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);
  Blynk.virtualWrite(V2, formattedTime);

  // Hour and Minute data
  // int currentHour = timeClient.getHours();
  // Serial.print("Hour: ");
  // Serial.println(currentHour);

  // int currentMinute = timeClient.getMinutes();
  // Serial.print("Minutes: ");
  // Serial.println(currentMinute);
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(roomLightLedRelaySwitch, OUTPUT);
  digitalWrite(roomLightLedRelaySwitch, HIGH);

  // Blynk configuration
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Connect to internet
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Austin timezone offset = GMT - 6
  timeClient.setTimeOffset(-21600);
}

void loop()
{
  Blynk.run();
}
