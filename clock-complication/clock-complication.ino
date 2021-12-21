enum XErrors {
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
struct Colour astronomicalTwilightColour = {3, 0, 7};
struct Colour nauticalTwilightColour = {4, 0, 10};
struct Colour civilTwilightColour = {5, 0, 13};
struct Colour dayColour = {100, 165, 232};
struct Colour sunriseColour = {50, 5, 13};
struct Colour sunsetColour = {50, 15, 13};
#define PIN 26

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    Serial.begin(9600);
    pixels.begin();
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
}

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

void errorCondition(enum XErrors errorCode) {
    sleep(3600*1000);
}

void parseSunriseDate(char *datestr, int utcOffsetMinutes, struct tm *tm) {
    strptime(datestr, "%Y-%m-%dT%H:%M:%S+00:00", tm);
    time_t time = mktime(tm) + utcOffsetMinutes;
    memcpy(tm, gmtime(&time), sizeof(struct tm));
}

struct ComplicationColours {
    struct Colour colours[24];
};

int tmMinutes(struct tm time) {
    int tmMinutes = time.tm_hour * 60 + time.tm_min;
    return tmMinutes;
}

int abs(int input) {
    return input < 0 ? -input : input;
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
    Serial.println("https://api.sunrise-sunset.org/json?lat="CAPITAL_LATITUDE"&lng="CAPITAL_LATITUDE"&date=today&formatted=0");
    http2.begin("https://api.sunrise-sunset.org/json?lat="CAPITAL_LATITUDE"&lng="CAPITAL_LATITUDE"&date=today&formatted=0");
    httpResponseCode = http2.GET();

    if (httpResponseCode != 200) {
        Serial.print("Sunrise query failed with: ");
        Serial.println(httpResponseCode);
        errorCondition(sunriseQueryError);
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

    struct tm sunriseTm = {0};
    struct tm sunsetTm = {0};
    struct tm solarNoonTm = {0};
    struct tm civilTwilightBeginTm = {0};
    struct tm civilTwilightEndTm = {0};
    struct tm nauticalTwilightBeginTm = {0};
    struct tm nauticalTwilightEndTm = {0};
    struct tm astronomicalTwilightBeginTm = {0};
    struct tm astronomicalTwilightEndTm = {0};
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["sunrise"], utcOffsetMins, &sunriseTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["sunset"], utcOffsetMins, &sunsetTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["solar_noon"], utcOffsetMins, &solarNoonTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["civil_twilight_begin"], utcOffsetMins, &civilTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["civil_twilight_end"], utcOffsetMins, &civilTwilightEndTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["nautical_twilight_begin"], utcOffsetMins, &nauticalTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["nautical_twilight_end"], utcOffsetMins, &nauticalTwilightEndTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["astronomical_twilight_begin"], utcOffsetMins, &astronomicalTwilightBeginTm);
    parseSunriseDate((char *)(const char *)jsonDocument["results"]["astronomical_twilight_end"], utcOffsetMins, &astronomicalTwilightEndTm);
    Serial.print("Hour: ");
    Serial.print(nauticalTwilightBeginTm.tm_hour);
    Serial.print(", minute: ");
    Serial.print(nauticalTwilightBeginTm.tm_min);
    Serial.print(", second: ");
    Serial.print(nauticalTwilightBeginTm.tm_sec);
    Serial.println();

    bool showingSunrise = tm.tm_hour < 12;

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
        if (minutes < tmMinutes(astronomicalTwilightBeginTm)) {
            Serial.print(", picking nightColour");
            colour = nightColour;
        }
        else if (minutes < tmMinutes(nauticalTwilightBeginTm)) {
            Serial.print(", picking astronomicalTwilightColour");
            colour = astronomicalTwilightColour;
        }
        else if (minutes < tmMinutes(civilTwilightBeginTm)) {
            Serial.print(", picking nauticalTwilightColour");
            colour = nauticalTwilightColour;
        }
        else if (minutes < tmMinutes(sunriseTm)) {
            Serial.print(", picking civilTwilightColour");
            colour = civilTwilightColour;
        }
        else if (minutes < tmMinutes(sunsetTm)) {
            Serial.print(", picking dayColour");
            colour = dayColour;
        }
        else if (minutes < tmMinutes(civilTwilightEndTm)) {
            Serial.print(", picking civilTwilightColour");
            colour = civilTwilightColour;
        }
        else if (minutes < tmMinutes(nauticalTwilightEndTm)) {
            Serial.print(", picking nauticalTwilightColour");
            colour = nauticalTwilightColour;
        }
        else if (minutes < tmMinutes(astronomicalTwilightEndTm)) {
            Serial.print(", picking astronomicalTwilightColour");
            colour = astronomicalTwilightColour;
        }
        else {
            Serial.print(", picking nightColour");
            colour = nightColour;
        }

        // Exception is sunset and sunrise
        if (showingSunrise) {
            if (abs(tmMinutes(sunriseTm) - minutes) <= 15) {
                Serial.print(", overriding with sunriseColour");
                colour = sunriseColour;
            }
        }
        else {
            if (abs(tmMinutes(sunsetTm) - minutes) <= 15) {
                Serial.print(", overriding with sunsetColour");
                colour = sunsetColour;
            }
        }
        Serial.println();
        complicationColours.colours[i] = colour;
    }

    int pixelOffset = 0;
    for (int i=0; i<24; i++) {
        Serial.print("i=");
        Serial.print(i);
        Serial.print(", r=");
        Serial.print(complicationColours.colours[i].r);
        Serial.print(", g=");
        Serial.print(complicationColours.colours[i].g);
        Serial.print(", b=");
        Serial.print(complicationColours.colours[i].b);
        Serial.println();
    }
    while(true) {
        for (int i=0; i<24; i++) {
            pixels.setPixelColor((i + pixelOffset) % 24, pixels.Color(complicationColours.colours[i].r, complicationColours.colours[i].g, complicationColours.colours[i].b));
        }
        pixels.show();
        pixelOffset++;
        delay(200);
    }
/*
    for (int i=0; i<256*256; i++) {
        for (int j=0; j<256*256; j++) {
            setPixel(j, i+16*j);
        }
        pixels.show();
        delay(10);
        if (i%256==0) {
            Serial.println("Completed one colour");
        }
    }
    Serial.println("Completed the whole loop");
*/
}
