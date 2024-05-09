#ifndef BUTTON_H
#define BUTTON_H

#include <cstdint>
#include <functional>

#include "../utils/colorspace.h"

#define HOLD_BUTTON_MIN_MS 500  // press and hold delay (ms)

void init_button();

/**
 * \brief Set up the wake up pin ( must call before going to sleep)
 */
void set_wake_up_signal();

/**
 * \brief handle the button clicked events
 * \param[in] clickSerieCallback A callback for a seri of clicks. Parameter is
 * the number of consequtive clicks detected
 * \param[in] clickHoldSerieCallback A callback for a seri of clicks followed
 * by a long hold. Parameter is the number of consequtive clicks detected and
 * the time of the old event (in milliseconds). Called at until the button is
 * released
 */
void handle_button_events(
    std::function<void(uint8_t)> clickSerieCallback,
    std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback);

/**
 * Display a color on the button
 */
void set_button_color(utils::ColorSpace::RGB color);
void button_blink(const uint offFreq, const uint onFreq,
                  utils::ColorSpace::RGB color);

/**
 * \brief Make the breeze animation on the button
 */
void button_breeze(const uint32_t periodOn, const uint32_t periodOff,
                   const utils::ColorSpace::RGB& color);

#endif
