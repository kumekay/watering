#include <Arduino.h>
// Configuration
#include <credentials.h>
// copy src/credentials_example.h to src/credentials.h and put your data to the file

#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <BlynkSimpleEsp8266.h> // https://github.com/blynkkk/blynk-library
#include <ESP8266httpUpdate.h>  // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266httpUpdate
#include <Ticker.h>             // https://github.com/esp8266/Arduino/blob/master/libraries/Ticker

#define DEBUG_SERIAL Serial

Ticker waterLevelChecker;
Ticker pumpStateChecker;

// Config
const uint16_t blynk_port{8442};
const char device_id[] = "ku_watering";
const char fw_ver[] = "0.0.1";

// Devices

uint8_t pumpPins[2]{D1, D2};
uint8_t pumpDirectionPins[2]{D3, D4};
uint8_t tankEmptyPin{D5};
uint8_t tankLowPin{D6};
WidgetLED tankEmpty{V2};
WidgetLED tankLow{V1};

bool timeToCheckWaterLevel{false};

uint32_t wateringDuration[2]{15000};
uint32_t wateringStartTime[2]{0};
uint8_t wateringState[2]{LOW};

void checkWaterLevel()
{
        timeToCheckWaterLevel = true;
}

void updateWaterLevelState()
{
        if (timeToCheckWaterLevel)
        {
                if (digitalRead(tankLowPin))
                {
                        tankLow.on();
                }
                else
                {
                        tankLow.off();
                }

                if (digitalRead(tankEmptyPin))
                {
                        tankEmpty.on();
                }
                else
                {
                        tankEmpty.off();
                }

                timeToCheckWaterLevel = false;
        }
}

void checkPumpState()
{
        for (uint8_t pumpId = 0; pumpId <= 1; pumpId++)
        {
                if (wateringState[pumpId] == HIGH && wateringStartTime[pumpId] + wateringDuration[pumpId] < millis())
                {
                        wateringState[pumpId] = LOW;
                        DEBUG_SERIAL.println("Turn off pump " + String(pumpId));
                }
                digitalWrite(pumpPins[pumpId], wateringState[pumpId]);
        }
}

void startPump(uint8_t pumpId)
{
        wateringStartTime[pumpId] = millis();
        wateringState[pumpId] = HIGH;
        DEBUG_SERIAL.println("Turn on pump " + String(pumpId));
}

void stopPump(uint8_t pumpId)
{
        wateringState[pumpId] = LOW;
        DEBUG_SERIAL.println("Turn off pump " + String(pumpId));
}

void setup()
{
        DEBUG_SERIAL.begin(115200);

        // Connections
        DEBUG_SERIAL.println("Connecting to WiFI");
        Blynk.connectWiFi(WIFI_SSID, WIFI_PASS);

        DEBUG_SERIAL.println("\nConnecting to Blynk server");
        Blynk.config(BLYNK_AUTH, BLYNK_SERVER, blynk_port);
        while (Blynk.connect() == false)
        {
                delay(500);
                DEBUG_SERIAL.print(".");
        }

        DEBUG_SERIAL.println("\nReady");

        // Devices
        for (uint8 i = 0; i <= 1; i++)
        {
                pinMode(pumpDirectionPins[i], OUTPUT);
                digitalWrite(pumpDirectionPins[i], HIGH);
                pinMode(pumpPins[i], OUTPUT);
                digitalWrite(pumpPins[i], LOW);
        }

        pinMode(tankEmptyPin, INPUT_PULLUP);
        pinMode(tankLowPin, INPUT_PULLUP);

        // Timers
        waterLevelChecker.attach(3.0, checkWaterLevel);
        pumpStateChecker.attach_ms(100, checkPumpState);
}

void loop()
{
        Blynk.run();

        updateWaterLevelState();
}

// Blynk callbacks

// Sync state on reconnect
BLYNK_CONNECTED() {
    Blynk.syncAll();
}

// Watering Duration
BLYNK_WRITE(V9)
{
        wateringDuration[0] = param.asInt()*1000;
        DEBUG_SERIAL.println("Update watering duration: " + String(wateringDuration[0]));
}

BLYNK_WRITE(V10)
{
        wateringDuration[1] = param.asInt()*1000;
        DEBUG_SERIAL.println("Update watering duration: " + String(wateringDuration[1]));
}

// Watering
BLYNK_WRITE(V5)
{
        if (param.asInt() == 1)
        {
                startPump(0);
        }
}

BLYNK_WRITE(V6)
{
        if (param.asInt() == 1)
        {
                stopPump(0);
        }
}

BLYNK_WRITE(V7)
{
        if (param.asInt() == 1)
        {
                startPump(1);
        }
}

BLYNK_WRITE(V8)
{
        if (param.asInt() == 1)
        {
                stopPump(1);
        }
}

// Update FW
BLYNK_WRITE(V22)
{
        if (param.asInt() == 1)
        {
                DEBUG_SERIAL.println("FW update request");

                char full_version[34]{""};
                strcat(full_version, device_id);
                strcat(full_version, "::");
                strcat(full_version, fw_ver);

                t_httpUpdate_return ret = ESPhttpUpdate.update("http://romfrom.space/update", full_version);
                switch (ret)
                {
                case HTTP_UPDATE_FAILED:
                        DEBUG_SERIAL.println("[update] Update failed.");
                        break;
                case HTTP_UPDATE_NO_UPDATES:
                        DEBUG_SERIAL.println("[update] Update no Update.");
                        break;
                case HTTP_UPDATE_OK:
                        DEBUG_SERIAL.println("[update] Update ok.");
                        break;
                }
        }
}
