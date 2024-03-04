#include "button.h"

#include "../utils/constants.h"
#include "../utils/utils.h"

#define RELEASE_TIMING_MS 200

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt()
{
    wasButtonPressedDetected = true;
}

static volatile bool buttonPressListener = false;
void read_while_pressed()
{
  if(wasButtonPressedDetected and digitalRead(BUTTON_PIN) == HIGH)
  {
    wasButtonPressedDetected = false;
  }
}

void treat_button_pressed(const bool isButtonPressDetected, std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback)
{
  static uint8_t clickedEvents = 0;    // multiple clicks
  static uint32_t buttonHoldStart= 0;  // start of the hold event in millis, valid if clicked event is > 0

  static uint32_t lastButtonPressedTime = 0;  // last time the pressed event was detected
  static bool isButtonPressed = false;
  
  const uint32_t currentMillis = millis();
  const uint32_t sinceLastCall = currentMillis - lastButtonPressedTime;
  const uint32_t buttonPressedDuration = currentMillis - buttonHoldStart;

  // currently in long press status
  const bool isInLongPress = isButtonPressed and buttonPressedDuration > HOLD_BUTTON_MIN_MS;

  // remove button clicked if last call was too long ago
  if(sinceLastCall > RELEASE_TIMING_MS or (isInLongPress && sinceLastCall > RELEASE_TIMING_MS/2))
  {
    // end of button press, change event
    if (isInLongPress)
      // signal end of click and hold
      clickHoldSerieCallback(clickedEvents, 0);
    else
      clickSerieCallback(clickedEvents);

    // reset
    clickedEvents = 0;
    isButtonPressed = false;
    buttonHoldStart = currentMillis;
  }

  // set button high
  if (isButtonPressDetected)
  {
    // small delay since button press
    if (sinceLastCall > 50)
    {
      if (!isInLongPress)
      {
        clickedEvents += 1;
        buttonHoldStart = currentMillis;
      }
    }
    
    lastButtonPressedTime = currentMillis;
    isButtonPressed = true;
  }

  
  if(isInLongPress)
  {
    clickHoldSerieCallback(clickedEvents, buttonPressedDuration);
  }
}


void handle_button_events(std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback)
{
  // keep reading the button value until unpressed
  read_while_pressed();

  // check the button pressed status
  treat_button_pressed(wasButtonPressedDetected, clickSerieCallback, clickHoldSerieCallback);
}

void set_button_color(utils::ColorSpace::RGB color)
{
    const COLOR& col = color.get_rgb();
    analogWrite(BUTTON_RED, col.red);
    analogWrite(BUTTON_GREEN, col.green);
    analogWrite(BUTTON_BLUE, col.blue);
}

float get_battery_level(const bool resetRead)
{
   constexpr float maxVoltage = 16.6;
   constexpr float lowVoltage = 13.0;

   static float lastValue = 0;

   // map the input ADC out to voltage reading.
   constexpr float minInValue = 472.0;
   constexpr float maxInValue = 600.0;
   const float batteryVoltage = utils::mapfloat(analogRead(BATTERY_CHARGE_PIN), minInValue, maxInValue, lowVoltage, maxVoltage);

   // init or reset
   if(lastValue == 0 or resetRead)
   {
     lastValue = batteryVoltage;
   }

   lastValue = batteryVoltage * 0.01 + lastValue * 0.99;
   return map(lastValue, lowVoltage, maxVoltage, 0, 100);
}