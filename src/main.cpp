#include <Arduino.h>
#include <FastLED.h>
#include <AudioTools.h>
#include <StarWars30.h>

#define CHANNELS 2
#define SAMPLE_RATE 22050
#define BITS_PER_SAMPLE 16

#define NUM_LEDS 1
#define DATA_PIN GPIO_NUM_21
CRGB leds[NUM_LEDS];

AudioInfo info(22050, 1, 16);
I2SStream i2s; // Output to I2S
MemoryStream music(StarWars30_raw, StarWars30_raw_len);
StreamCopy copier(i2s, music); // copies sound into i2s

int timer = 0;

void setup()
{
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);

  auto config = i2s.defaultConfig(TX_MODE);

  config.copyFrom(info);
  // config.sample_rate = SAMPLE_RATE;
  // config.channels = CHANNELS;
  // config.bits_per_sample = BITS_PER_SAMPLE;

  config.pin_data = GPIO_NUM_4; // Data pin for I2S
  config.pin_bck = GPIO_NUM_5;  // Bit clock pin for I2S
  config.pin_ws = GPIO_NUM_6;   // Word select pin for I2S

  i2s.begin(config);

  music.begin();
}

void loop()
{

  if (!copier.copy())
  {
    i2s.end();
    stop();
  }


  
  // Turn the LED on, then pause
  leds[0] = CRGB::Red;
  FastLED.show();
  // delay(500);
  // Now turn the LED off, then pause
  leds[0] = CRGB::Black;
  FastLED.show();
  // delay(500);
}