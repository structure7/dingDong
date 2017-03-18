#include <BlynkSimpleEsp8266.h>
//#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//#define BLYNK_DEBUG

#include <SimpleTimer.h>
#include <WidgetRTC.h>          // Blynk's RTC
#include <TimeLib.h>            // Used by WidgetRTC.h

// Required for OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

char auth[] = "fromBlynkApp";

const char* ssid = "ssid";
const char* pw = "pw";

bool notifyFlag;                        // Debounces the push notification send.
bool terminalFlag;                      // Debounces the terminal notification.
bool runOnce;                           // Runs things once then runOnce = 0.
bool testButton;                        // Debounced the test button.
long ringTime;                          // Millis when doorbell last rang.
String currentTime = "(RTC not set)";   // Current time formatted HH:MM:SSam Mon MM/DD.
int bellRelay = 16;                     // WeMos D1 Mini pin D0 - connected to 24VAC relay NO contact.

int currentRingMin, currentRingSec;     // How many min and sec since the last doorbell ring to display "X sec or X min ago."
int ringDisplay;                        // Timer that counts min/sec since last doorbell ring. Turns off after ~5 minutes.
String ringString;                      // Snapshots the currentTime string to diplays the last doorbell ring.

// These are used to give me some startup diagnostics on my app's terminal display
int startMillis = millis();
int preBlynkMillis;                     // Millis when Blynk connection started.
int postBlynkMillis;                    // Millis when Blynk connection established.
int preRtcMillis;                       // Millis when RTC connection started.
int postRtcMillis;                      // Millis when RTC connection established.

int pushNotificationStatus;

SimpleTimer timer;
WidgetRTC rtc;
WidgetTerminal terminal(V0);

void setup()
{
  Serial.begin(9600);

  preBlynkMillis = millis();

  Blynk.begin(auth, ssid, pw);

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  postBlynkMillis = millis();

  // START OTA ROUTINE
  ArduinoOTA.setHostname("dingDong");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  // END OTA ROUTINE

  preRtcMillis = millis();
  rtc.begin();

  timer.setInterval(100, relayWatch);
  timer.setInterval(1000L, setClockTime);

  ringDisplay = timer.setInterval(2000L, ringDisplayFunc);  // Create the timer, then...
  timer.disable(ringDisplay);                               // disable it so it can be started when I need it.
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
}

BLYNK_CONNECTED() {
  {
    Blynk.syncVirtual(V8);
  }
}

// Watched the relay for closure (indicated the doorbell is pressed)
void relayWatch() {
  if (digitalRead(bellRelay) == LOW && notifyFlag == 0) {
    notifyFlag = 1;

    if (pushNotificationStatus == 1) {
      Blynk.notify(String(currentTime) + " Doorbell rang.");
    }

    sendTerminal();
  }
  else if (testButton == 1 && notifyFlag == 0) {
    notifyFlag = 1;

    if (pushNotificationStatus == 1) {
      Blynk.notify("DOORBELL TEST ONLY");
    }

    sendTerminalTest();
  }
  else if (millis() > (ringTime + 5000) && notifyFlag == 1) {
    notifyFlag = 0;
  }
}

void sendTerminal() {
  ringString = currentTime;

  terminal.println(String(currentTime) + " Ding dong!");

  terminal.flush();

  ringTime = millis();
  currentRingSec = (millis() / 1000);
  currentRingMin = (millis() / 1000 / 60);

  timer.enable(ringDisplay);
}

void sendTerminalTest() {
  terminal.println(String(currentTime) + " Ding dong (TEST)!");
  terminal.flush();
  testButton = 0;
}

void ringDisplayFunc() {
  if (millis() < (ringTime + 60000)) {    // Display seconds if under 60 seconds.
    Blynk.virtualWrite(V3, String((millis() / 1000) - currentRingSec) + " seconds ago");
  }
  else if (millis() >= (ringTime + 60000) && millis() < (ringTime + 120000) ) {
    Blynk.virtualWrite(V3, String((millis() / 60000) - currentRingMin) + " minute ago");
  }
  else if (millis() >= (ringTime + 120000) && millis() < (ringTime + 300000) ) {
    Blynk.virtualWrite(V3, String((millis() / 60000) - currentRingMin) + " minutes ago");
  }
  else if (millis() >= (ringTime + 300000) ) {
    Blynk.virtualWrite(V3, ringString);
    timer.disable(ringDisplay);
  }
}

BLYNK_WRITE(V8)
{
  int pinData = param.asInt();

  if (pinData == 0)
  {
    pushNotificationStatus = 0;
  }
  else if (pinData == 1)
  {
    pushNotificationStatus = 1;
  }
}

BLYNK_WRITE(V2)
{
  int pinData = param.asInt();

  if (pinData == 1)
  {
    testButton = 1;
  }
}

BLYNK_WRITE(V4)
{
  int pinData = param.asInt();

  if (pinData == 0)
  {
    timer.disable(ringDisplay);

    terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println("");
    terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println("");
    terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println("");
    terminal.println(String("") + currentTime + " - Log cleared!");
    terminal.print("   IP Address: ");
    terminal.println(WiFi.localIP());
    terminal.println(String("   MAC Address: ") + WiFi.macAddress());
    if (WiFi.RSSI() >= -57) { // -57 or larger ( -23, 0, 23, etc)
      terminal.println(String("   Signal Strength: GOOD (") + WiFi.RSSI() + "dBm)");
    }
    else if (WiFi.RSSI() < -57 && WiFi.RSSI() >= -70) {    // Between -58 through -69
      terminal.println(String("   Signal Strength: OKAY (") + WiFi.RSSI() + "dBm)");
    }
    else if (WiFi.RSSI() < -70) {
      terminal.println(String("   Signal Strength: POOR (") + WiFi.RSSI() + "dBm)");
    }
    terminal.println(""); terminal.println("");
    terminal.flush();

    Blynk.virtualWrite(V3, "No rings yet...");
  }
}

void setClockTime()
{

  if (year() != 1970) // Doesn't start until RTC is set correctly.
  {

    // Below gives me leading zeros on minutes and AM/PM.
    if (minute() > 9 && hour() > 11 && second() > 9) {    // No zeros needed at all, PM.
      currentTime = String(hourFormat12()) + ":" + minute() + ":" + second() + "pm " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() > 9 && hour() > 11 && second() < 10) {    // Zero only for second, PM.
      currentTime = String(hourFormat12()) + ":" + minute() + ":0" + second() + "pm " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() > 11 && second() > 9) {  // Zero only for hour, PM.
      currentTime = String(hourFormat12()) + ":0" + minute() + ":" + second() + "pm " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() > 11 && second() < 10) {  // Zero for hour and second, PM.
      currentTime = String(hourFormat12()) + ":0" + minute() + ":0" + second() + "pm " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }

    if (minute() > 9 && hour() < 12 && second() > 9) {    // No zeros needed at all, AM.
      currentTime = String(hourFormat12()) + ":" + minute() + ":" + second() + "am " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() > 9 && hour() < 12 && second() < 10) {    // Zero only for second, AM.
      currentTime = String(hourFormat12()) + ":" + minute() + ":0" + second() + "am " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() < 12 && second() > 9) {  // Zero only for hour, AM.
      currentTime = String(hourFormat12()) + ":0" + minute() + ":" + second() + "am " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() < 12 && second() < 10) {  // Zero for hour and second, AM.
      currentTime = String(hourFormat12()) + ":0" + minute() + ":0" + second() + "am " + dayShortStr(weekday()) + " " + month() + "/" + day();
    }

    if (runOnce == 0) {
      Blynk.syncVirtual(V3);
      postRtcMillis = millis();
      terminal.println(""); terminal.println(""); terminal.println("");
      terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println(""); terminal.println("");
      terminal.flush();
      terminal.println(String("") + currentTime);
      terminal.println(String("   [") + startMillis + "] Doorbell monitor started.");
      terminal.println(String("   [") + preBlynkMillis + "] Connecting...");
      terminal.println(String("   [") + postBlynkMillis + "] Connected.");
      terminal.println(String("   [") + preRtcMillis + "] Setting RTC.");
      terminal.println(String("   [") + postRtcMillis + "] RTC set.");
      terminal.flush();
      terminal.print("   IP Address: ");
      terminal.println(WiFi.localIP());
      terminal.println(String("   MAC Address: ") + WiFi.macAddress());
      if (WiFi.RSSI() >= -57) { // -57 or larger ( -23, 0, 23, etc)
        terminal.println(String("   Signal Strength: GOOD (") + WiFi.RSSI() + "dBm)");
      }
      else if (WiFi.RSSI() < -57 && WiFi.RSSI() >= -70) {    // Between -58 through -69
        terminal.println(String("   Signal Strength: OKAY (") + WiFi.RSSI() + "dBm)");
      }
      else if (WiFi.RSSI() < -70) {
        terminal.println(String("   Signal Strength: POOR (") + WiFi.RSSI() + "dBm)");
      }
      terminal.println(""); terminal.println("");
      terminal.flush();
      runOnce = 1;
    }
  }
}
