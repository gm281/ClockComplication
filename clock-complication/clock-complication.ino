enum Errors {
    timeQueryError,
    timeParsingError,
    sunriseQueryError,
    sunriseParsingError
};

struct Colour {
    int r;
    int g;
    int b;
};

struct ComplicationColours {
    struct Colour colours[24];
};

struct SunriseTimes {
    struct tm sunriseTm;
    struct tm sunsetTm;
    struct tm solarNoonTm;
    struct tm civilTwilightBeginTm;
    struct tm civilTwilightEndTm;
    struct tm nauticalTwilightBeginTm;
    struct tm nauticalTwilightEndTm;
    struct tm astronomicalTwilightBeginTm;
    struct tm astronomicalTwilightEndTm;
};

#include <time.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define CAPITAL "Warsaw"
#define CAPITAL_LATITUDE "52.237049"
#define CAPITAL_LONGNITUDE "21.017532"
const char* ssid = "75-the-meadows";
const char* password = "harrylikesfoxes";
struct Colour nightColour = {2, 0, 5};
struct Colour astronomicalTwilightColour = {5, 0, 10};
struct Colour nauticalTwilightColour = {8, 0, 15};
struct Colour civilTwilightColour = {11, 0, 20};
struct Colour dayColour = {80, 135, 232};
struct Colour sunriseColour = {60, 5, 8};
struct Colour sunsetColour = {60, 15, 8};
#define PIN 26

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);

void setPixel(int pixelID, int sequence) {
    switch ((sequence / 256) % 3) {
        case 0:
            pixels.setPixelColor(pixelID, pixels.Color(sequence % 256, 0, 0));
            break;
        case 1:
            pixels.setPixelColor(pixelID, pixels.Color(0, sequence % 256, 0));
            break;
        case 2:
            pixels.setPixelColor(pixelID, pixels.Color(0, 0, sequence % 256));
            break;
    }
}

void printTM(struct tm tm) {
    Serial.print(tm.tm_hour);
    Serial.print(":");
    Serial.print(tm.tm_min);
    Serial.print(":");
    Serial.print(tm.tm_sec);
}

void printSunriseTime(char *label, struct tm tm) {
    Serial.print(label);
    Serial.print(", ");
    printTM(tm);
    Serial.println();
}

void printSunriseTimes(struct SunriseTimes times) {
    printSunriseTime("sunriseTm", times.sunriseTm);
    printSunriseTime("sunsetTm", times.sunsetTm);
    printSunriseTime("solarNoonTm", times.solarNoonTm);
    printSunriseTime("civilTwilightBeginTm", times.civilTwilightBeginTm);
    printSunriseTime("civilTwilightEndTm", times.civilTwilightEndTm);
    printSunriseTime("nauticalTwilightBeginTm", times.nauticalTwilightBeginTm);
    printSunriseTime("nauticalTwilightEndTm", times.nauticalTwilightEndTm);
    printSunriseTime("astronomicalTwilightBeginTm", times.astronomicalTwilightBeginTm);
    printSunriseTime("astronomicalTwilightEndTm", times.astronomicalTwilightEndTm);
}

void errorCondition(enum Errors errorCode) {
    long totalWaitTimeMS = 1L * 60L * 1000L;
    long cycleDelay = 40L;
    long numberOfIterations = totalWaitTimeMS / cycleDelay / 256;
    for (long i=0; i<numberOfIterations; i++) {
        rainbowCycle(cycleDelay);
    }
}

void parseSunriseDate(char *datestr, int utcOffsetMinutes, struct tm *tm) {
    strptime(datestr, "%Y-%m-%dT%H:%M:%S+00:00", tm);
    time_t time = mktime(tm) + utcOffsetMinutes;
    memcpy(tm, gmtime(&time), sizeof(struct tm));
}

int tmMinutes(struct tm time) {
    int tmMinutes = time.tm_hour * 60 + time.tm_min;
    return tmMinutes;
}

int abs(int input) {
    return input < 0 ? -input : input;
}

int min(int one, int two) {
    return one < two ? one : two;
}

///////////////////////////////////////////////
// Copied from Adafruit Neopixel strand example
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

///////////////////////////////////////////////

struct ComplicationColours calculateComplicationColours(struct tm currentTime, struct SunriseTimes sunriseTimes) {
    bool showingSunrise = currentTime.tm_hour < 12;

    struct ComplicationColours complicationColours = {0};
    for (int i=0; i<24; i++) {
        int minutes = i*30 + (showingSunrise ? 0 : (12*60));
        Serial.print("Index: ");
        Serial.print(i);
        Serial.print(", minutes: ");
        Serial.print(minutes);
        Serial.print(", showingSunrise: ");
        Serial.print(showingSunrise ? "true" : "false");

        struct Colour colour;
        if (minutes < tmMinutes(sunriseTimes.astronomicalTwilightBeginTm)) {
            Serial.print(", picking nightColour");
            colour = nightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.nauticalTwilightBeginTm)) {
            Serial.print(", picking astronomicalTwilightColour");
            colour = astronomicalTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.civilTwilightBeginTm)) {
            Serial.print(", picking nauticalTwilightColour");
            colour = nauticalTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.sunriseTm)) {
            Serial.print(", picking civilTwilightColour");
            colour = civilTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.sunsetTm)) {
            Serial.print(", picking dayColour");
            colour = dayColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.civilTwilightEndTm)) {
            Serial.print(", picking civilTwilightColour");
            colour = civilTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.nauticalTwilightEndTm)) {
            Serial.print(", picking nauticalTwilightColour");
            colour = nauticalTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTimes.astronomicalTwilightEndTm)) {
            Serial.print(", picking astronomicalTwilightColour");
            colour = astronomicalTwilightColour;
        }
        else {
            Serial.print(", picking nightColour");
            colour = nightColour;
        }

        // Exception is sunset and sunrise
        if (showingSunrise) {
            if (abs(tmMinutes(sunriseTimes.sunriseTm) - minutes) <= 15) {
                Serial.print(", overriding with sunriseColour");
                colour = sunriseColour;
            }
        }
        else {
            if (abs(tmMinutes(sunriseTimes.sunsetTm) - minutes) <= 15) {
                Serial.print(", overriding with sunsetColour");
                colour = sunsetColour;
            }
        }
        Serial.println();
        complicationColours.colours[i] = colour;
    }

    return complicationColours;
}

void setup()
{
    Serial.begin(9600);
    pixels.begin();
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      rainbowCycle(20);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/timezone/Europe/"CAPITAL);

    int httpResponseCode = http.GET();

    if (httpResponseCode != 200) {
        Serial.print("Time query failed with: ");
        Serial.println(httpResponseCode);
        errorCondition(timeQueryError);
        return;
    }

    StaticJsonDocument<1024> jsonDocument;
    DeserializationError deserializationError = deserializeJson(jsonDocument, http.getString().c_str());
    if (deserializationError) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(deserializationError.f_str());
        errorCondition(timeParsingError);
        return;
    }
    String datetime = String((const char *)jsonDocument["datetime"]);
    Serial.println(datetime);
    String trimmedDatetime = datetime.substring(0, datetime.indexOf('.'));
    Serial.println(trimmedDatetime);

    struct tm tm = {0};
    strptime((char *)trimmedDatetime.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);

    Serial.print("Hour: ");
    Serial.print(tm.tm_hour);
    Serial.print(", minute: ");
    Serial.print(tm.tm_min);
    Serial.print(", second: ");
    Serial.print(tm.tm_sec);
    Serial.println();

    String utcOffset = String((const char *)jsonDocument["utc_offset"]);
    int utcOffsetMins = utcOffset.substring(1, 3).toInt() * 60 + utcOffset.substring(4, 6).toInt();
    utcOffsetMins = utcOffsetMins * (utcOffset.charAt(0) == '+' ?  1 : -1);

    Serial.print("UTC offset minutes: ");
    Serial.println(utcOffsetMins);

    http.end();

    HTTPClient http2;
    http2.begin("https://api.sunrise-sunset.org/json?lat="CAPITAL_LATITUDE"&lng="CAPITAL_LONGNITUDE"&date=today&formatted=0");
    httpResponseCode = http2.GET();

    if (httpResponseCode != 200) {
        Serial.print("Sunrise query failed with: ");
        Serial.println(httpResponseCode);
        errorCondition(sunriseQueryError);
        return;
    }

    deserializationError = deserializeJson(jsonDocument, http2.getString().c_str());
    http2.end();
    if (deserializationError) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(deserializationError.f_str());
        errorCondition(sunriseParsingError);
        return;
    }
    String sunrise = String((const char *)jsonDocument["results"]["sunrise"]);
    Serial.println(sunrise);

    struct SunriseTimes sunriseTimes = {};
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["sunrise"], utcOffsetMins, &sunriseTimes.sunriseTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["sunset"], utcOffsetMins, &sunriseTimes.sunsetTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["solar_noon"], utcOffsetMins, &sunriseTimes.solarNoonTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["civil_twilight_begin"], utcOffsetMins, &sunriseTimes.civilTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["civil_twilight_end"], utcOffsetMins, &sunriseTimes.civilTwilightEndTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["nautical_twilight_begin"], utcOffsetMins, &sunriseTimes.nauticalTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["nautical_twilight_end"], utcOffsetMins, &sunriseTimes.nauticalTwilightEndTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["astronomical_twilight_begin"], utcOffsetMins, &sunriseTimes.astronomicalTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["astronomical_twilight_end"], utcOffsetMins, &sunriseTimes.astronomicalTwilightEndTm);
    printSunriseTimes(sunriseTimes);

    errorCondition(sunriseParsingError);

    struct ComplicationColours complicationColours = calculateComplicationColours(tm, sunriseTimes);

    //for (int i=0; i<24; i++) {
    //    Serial.print("i=");
    //    Serial.print(i);
    //    Serial.print(", r=");
    //    Serial.print(complicationColours.colours[i].r);
    //    Serial.print(", g=");
    //    Serial.print(complicationColours.colours[i].g);
    //    Serial.print(", b=");
    //    Serial.print(complicationColours.colours[i].b);
    //    Serial.println();
    //}

    int pixelOffset = 0;
    do {
        for (int i=0; i<24; i++) {
            pixels.setPixelColor((i + pixelOffset) % 24, pixels.Color(complicationColours.colours[i].r, complicationColours.colours[i].g, complicationColours.colours[i].b));
        }
        pixels.show();
        pixelOffset++;
        delay(200);
    } while(true);

    //int currentHour = 17;
    //int currentMin = 36;
    //tm.tm_hour = tm.tm_hour - currentHour + 11;
    //tm.tm_min = tm.tm_min - currentMin + 55;
    //if (tm.tm_min > 60) {
    //    tm.tm_min -= 60;
    //    tm.tm_hour++;
    //}

    int switchoverSeconds = ((tm.tm_hour >= 12 ? 24 : 12) - tm.tm_hour) * 3600 - tm.tm_min * 60 - tm.tm_sec;
    int waitTime = min(switchoverSeconds/2, 3600);
    Serial.print("Sleeping for: ");
    Serial.println(waitTime);
    delay(1000UL * waitTime);
}
