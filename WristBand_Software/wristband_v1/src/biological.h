#include "MAX30105.h"
#include "heartRate.h"


const uint8_t RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
uint8_t rates[RATE_SIZE]; //Array of heart rates
uint8_t rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

char buff[256];

void loop()
{
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true) {
        //We sensed a beat!
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (uint8_t)beatsPerMinute; //Store this reading in the array
            rateSpot %= RATE_SIZE; //Wrap variable

            //Take average of readings
            beatAvg = 0;
            for (uint8_t x = 0 ; x < RATE_SIZE ; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }

    tft.fillScreen(TFT_BLACK);
    snprintf(buff, sizeof(buff), "IR=%lu BPM=%.2f", irValue, beatsPerMinute);
    tft.drawString(buff, 0, 0);
    snprintf(buff, sizeof(buff), "Avg BPM=%d", beatAvg);
    tft.drawString(buff, 0, 16);

    if (irValue < 50000)
        digitalWrite(LED_PIN, 0);
    else
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}
