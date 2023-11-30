
//           +-\/-+
// (5) PB5  1|    |8  Vcc
// (3) PB3  2|    |7  PB2 
// (4) PB4  3|    |6  PB1 (1) LED output (PWM)
//     GND  4|    |5  PB0 (0) IR Input
//           +----+
// @Internal 8 MHz

#include "PinDefinitionsAndMore.h"   // Sets input pin to 3
#define IRMP_PROTOCOL_NAMES 1        // Enable protocol number mapping to protocol strings - requires some FLASH.
#define IRMP_SUPPORT_NEC_PROTOCOL 1  // includes APPLE and ONKYO protocols
#include <irmp.hpp>
#include "CandleDefinitions.h"
#include "PowerSaving.h"

#define LED_PIN 1
#define MAX_Brightness 130  // 130 - 100 (slighly flickering), 90 (stronger), 80 well noticable, 60 (strong), 40 (extreme), 30 (dark)
#define STEP_Size 1

IRMP_DATA irmp_data;
int pointer = 0;
int previousIntensity = 130;
int intensitiyLevel = 1;
int intensity = 130;
int state = 2;  // -1 = Sleep, 1 = Steady, 2 = On/Random1 strong flickering (default), 3 = Random2 less flickering, 0 = off
int randomSize = 225; // size of RandLevels

void setup() {
  irmp_init();
  irmp_irsnd_LEDFeedback(false);  // Enable receive signal feedback at ALTERNATIVE_IR_FEEDBACK_LED_PIN
  previousIntensity = pgm_read_byte(&Intensity1[0]);
  intensity = pgm_read_byte(&Intensity1[0]);
  setup_watchdog(4);
}

void loop() {
  while (state == -1)
  {
    // save some energy
    system_sleep();
    for (int i = 0; i < 15; i++)
    {
      delay(10);      
      decodeIRSignal();  // Decode IR-Remote Receiver
    }
  }
  switch (state) {
    case 0:  // Off
      analogWrite(LED_PIN, 0);
      state = -1;
      break;
    case 1:  // Steady
      analogWrite(LED_PIN, MAX_Brightness);
      break;
    case 2:  // Random 1
      randomflickering1(); // stronger flickering
      break;
    case 3:  // Random 2
      randomflickering2(); // less flickering
      break;
    default:
      break;
  }
  decodeIRSignal();  // Decode IR-Remote Receiver
}

void randomflickering1() {
  int8_t level = pgm_read_byte(&RandLevels[random(randomSize)]);
  intensity = pgm_read_byte(&Intensity1[level - 1]);
  int8_t rndStepDelay = random(2, 12);
  fadeinOrOut(intensity, previousIntensity, rndStepDelay);
  previousIntensity = intensity;
}

void randomflickering2() {
  int8_t level = pgm_read_byte(&RandLevels[random(randomSize)]);
  intensity = pgm_read_byte(&Intensity2[level - 1]);
  int8_t rndStepDelay = random(2, 60);
  fadeinOrOut(intensity, previousIntensity, rndStepDelay);
  previousIntensity = intensity;
}

void fadeinOrOut(int current, int previous, int delayTime) {
  if (current >= previous) {
    for (int fadeValue = previous; fadeValue <= current; fadeValue += STEP_Size) {
      analogWrite(LED_PIN, fadeValue);
      delay(delayTime);
    }
  }
  if (current < previous) {
    for (int fadeValue = previous; fadeValue >= current; fadeValue -= STEP_Size) {
      analogWrite(LED_PIN, fadeValue);
      delay(delayTime);
    }
  }
}

void decodeIRSignal() {
  if (irmp_get_data(&irmp_data)) {
    switch (irmp_data.command) {
      case 0x45:  // On-button
      case 0x47:  // LED1-button
        if (state > 0) {
          signalCommandReceived();
        }
        state = 2;  // On/Random1 strong flickering (default)
        pointer = 0;
        break;
      case 0x44:    // Off-button
        state = 0;  // Off
        break;
      case 0x43:    // LED2-button
        state = 1;  // Steady
        signalCommandReceived();
        break;
      case 0x9:     // LED3-button
        state = 3;  // Less flickering
        signalCommandReceived();
        break;
      default:
        break;
    }
  }
}

// show that a command was received by letting the LED dark
void signalCommandReceived() {
  analogWrite(LED_PIN, 0);
  delay(200);
}
