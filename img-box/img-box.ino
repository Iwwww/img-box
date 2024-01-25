#include <FastLED.h>
#include <EncButton.h>
#include "color_temperatures.h"
#include "colors.h"
/* === SETUP === */
#define ROW_COUNT 3
#define COLUMN_COUNT 8
#define NUM_LEDS ROW_COUNT *COLUMN_COUNT
#define DATA_PIN 2
#define SLEEP_TIMER_DELAY (1000 * 1)  // in millis

/* === SETTINGS === */
// color temperature
#define COLOR_STEP 1
#define STARTUP_COLOR_TEMPERATURE 30  // less = warmer

// flickering brightness
#define FLICKERING_BRIGHTNESS_HALF_PERIOD 5000  // in millis
#define FLICKERING_BRIGHTNESS_MAX_DELTA 180
#define FLICKERING_BRIGHTNESS_STEPS FLICKERING_BRIGHTNESS_MAX_DELTA
#define FLICKERING_BRIGHTNESS_DELTA_STEP ((FLICKERING_BRIGHTNESS_MAX_DELTA / FLICKERING_BRIGHTNESS_STEPS) ? (FLICKERING_BRIGHTNESS_MAX_DELTA / FLICKERING_BRIGHTNESS_STEPS) : 1)

// Flickering color temperature
#define FLICKERING_COLOR_TEMPERATURE_HALF_PERIOD 1000 * 60  // in millis
#define FLICKERING_COLOR_TEMPERATURE_MAX_DELTA 15
#define FLICKERING_COLOR_TEMPERATURE_STEPS FLICKERING_COLOR_TEMPERATURE_MAX_DELTA
#define FLICKERING_COLOR_TEMPERATURE_DELTA_STEP ((FLICKERING_COLOR_TEMPERATURE_MAX_DELTA / FLICKERING_COLOR_TEMPERATURE_STEPS) ? (FLICKERING_COLOR_TEMPERATURE_MAX_DELTA / FLICKERING_COLOR_TEMPERATURE_STEPS) : 1)

// brightness
#define BRIGHTNESS_SMALL_STEP 10
#define INITIAL_BRIGHTNESS 255
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 2

// Buttons init
Button btn(3);

int brightness_step = BRIGHTNESS_SMALL_STEP;
int brightness = INITIAL_BRIGHTNESS;
CRGB leds[NUM_LEDS];

int8_t color_step = COLOR_STEP;
int current_temperature = STARTUP_COLOR_TEMPERATURE;

enum MODE {
  DAY,
  NIGHT,
  BRIGHTNESS_NEXT,
  BRIGHTNESS_PREV,
  COLOR_TEMPERATURE_UP,
  COLOR_TEMPERATURE_DOWN,
  COLOR,
  SLEEP_TIMER,
} mode = DAY;

enum INTERACTIVE_MODE {
  NONE,
  FLICKERING,
  COLOR_TEMPERATURE_FLICKERING,
  // BRIGHTNESS_GRADIENT
} interactive_mode = NONE;

/* Timer vars */
unsigned long int prev_millis_flickering_brightness_time = 0;
unsigned long int prev_millis_flickering_color_temperature_time = 0;
unsigned long int prev_sleep_timer = 0;


/* Flickering */
uint8_t delta = 0;
bool delta_flag = 0;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(brightness);
  set_color_temperature(ColorTemperatures[current_temperature]);
  FastLED.show();

  Serial.begin(115200);
  Serial.println("Serial is runngin");
}

void loop() {
  btn.tick();

  flickering_brightness();
  flickering_color_temperature();

  if (mode == SLEEP_TIMER) {
    // wake up mode from sleep only
    if (btn.click()) {
      Serial.println("exit SLEEP_TIMER");
      mode = NIGHT;
    }
  } else {
    if (btn.hasClicks(1)) {
      color_blink(0, NUM_LEDS, Colors[1], 200, 0, 1);
      Serial.println("btn clicks 1");
      switch (mode) {
        case DAY:
          set_color_temperature(ColorTemperatures[COLOR_TEMPERATURE_COUNT]);
          mode = NIGHT;
          Serial.println("set mode DAY");
          break;
        case NIGHT:
          set_color_temperature(ColorTemperatures[0]);
          mode = DAY;
          Serial.println("set mode NIGHT");
          break;
        case BRIGHTNESS_NEXT:
          Serial.println("set BRIGHTNESS_PREV");
          mode = BRIGHTNESS_PREV;
          break;
        case BRIGHTNESS_PREV:
          Serial.println("set BRIGHTNESS_UP");
          mode = BRIGHTNESS_NEXT;
          break;
        case COLOR_TEMPERATURE_UP:
          Serial.println("set COLOR_TEMPERATURE_DOWN");
          mode = COLOR_TEMPERATURE_DOWN;
          break;
        case COLOR_TEMPERATURE_DOWN:
          Serial.println("set COLOR_TEMPERATURE_UP");
          mode = COLOR_TEMPERATURE_UP;
          break;
        default:
          break;
      }
    }
    if (btn.hasClicks(2)) {
      color_blink(0, NUM_LEDS, Colors[2], 200, 100, 2);
      if (mode == DAY) {
        mode = NIGHT;
        Serial.println("set mode NIGHT");
      } else {
        mode = DAY;
        Serial.println("set mode DAY");
      }
    }
    if (btn.hasClicks(3)) {
      color_blink(0, NUM_LEDS, Colors[3], 200, 100, 3);
      mode = BRIGHTNESS_PREV;
      Serial.println("set mode BRIGHTNESS_PREV");
    }
    if (btn.hasClicks(4)) {
      color_blink(0, NUM_LEDS, Colors[3], 200, 100, 4);
      mode = SLEEP_TIMER;
      Serial.println("set mode SLEEP_TIMER");
      prev_sleep_timer = millis();
    }
    if (btn.holdFor(200)) {
      switch (mode) {
        case BRIGHTNESS_NEXT:
          Serial.println("hold BRIGHTNESS_UP");
          brightness_up();
          break;
        case BRIGHTNESS_PREV:
          Serial.println("hold BRIGHTNESS_DOWN");
          brightness_down();
          break;
        case DAY:
          Serial.println("hold color_temperature_next");
          color_temperature_next();
          break;
        case NIGHT:
          Serial.println("hold color_temperature_prev");
          color_temperature_prev();
          break;
        case COLOR_TEMPERATURE_UP:
          Serial.println("hold COLOR_TEMPERATURE_UP");
          color_temperature_next();
          break;
        case COLOR_TEMPERATURE_DOWN:
          Serial.println("hold COLOR_TEMPERATURE_DOWN");
          color_temperature_prev();
          break;
        default:
          break;
      }
      delay(100);
    }
  }


  FastLED.show();
}

// Led functin
void fill_led_strip(int r, int g, int b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(r, g, b);
  }
}

void fill_led_strip(int tmp[3]) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(tmp[0], tmp[1], tmp[2]);
  }
}

// Color temperature
void set_color_temperature(uint8_t color[3]) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(color[0], color[1], color[2]);
  }
}

void color_temperature_next() {
  if (current_temperature + color_step < COLOR_TEMPERATURE_COUNT) {
    current_temperature += color_step;
  } else if (current_temperature + color_step >= COLOR_TEMPERATURE_COUNT) {
    current_temperature = COLOR_TEMPERATURE_COUNT;
  }
  set_color_temperature(ColorTemperatures[current_temperature]);
}

void color_temperature_prev() {
  if (current_temperature - color_step > 0) {
    current_temperature -= color_step;
  } else if (current_temperature - color_step <= 0) {
    current_temperature = 0;
  }
  set_color_temperature(ColorTemperatures[current_temperature]);
}

// Brightness
void brightness_up() {
  if (brightness < MAX_BRIGHTNESS)
    if (brightness + brightness_step <= MAX_BRIGHTNESS) {
      brightness += brightness_step;
    } else {
      brightness = MAX_BRIGHTNESS;
    }
  FastLED.setBrightness(brightness);
}

void brightness_down() {
  if (brightness > MIN_BRIGHTNESS)
    if (brightness - brightness_step >= MIN_BRIGHTNESS) {
      brightness -= brightness_step;
    } else {
      brightness = MIN_BRIGHTNESS;
    }
  FastLED.setBrightness(brightness);
}

void set_color(uint8_t num_leds_start, uint8_t num_leds_end, uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = num_leds_start; i < num_leds_end; i++) {
    leds[i].setRGB(r, g, b);
  }
}

void color_blink(uint8_t num_leds_start, uint8_t num_leds_end, uint8_t color[3], uint8_t delay_color, uint8_t delay_dim, uint8_t repeat) {
  for (uint8_t i = 0; i < repeat; i++) {
    if (delay_dim) {
      set_color(num_leds_start, num_leds_end, 0, 0, 0);
      FastLED.show();
      delay(delay_dim);
    }
    if (delay_color) {
      set_color(num_leds_start, num_leds_end, color[0], color[1], color[2]);
      Serial.println("blink");
      FastLED.show();
      delay(delay_color);
    }
  }
  set_color_temperature(ColorTemperatures[current_temperature]);
}

void blink(uint8_t num_leds_start, uint8_t num_leds_end, uint8_t delay_color, uint8_t delay_dim, uint8_t repeat) {
  color_blink(num_leds_start, num_leds_end, ColorTemperatures[current_temperature], delay_color, delay_dim, repeat);
}

void flickering_brightness() {
  if (millis() - prev_millis_flickering_brightness_time >= FLICKERING_BRIGHTNESS_HALF_PERIOD / FLICKERING_BRIGHTNESS_STEPS) {
    prev_millis_flickering_brightness_time = millis();
    // Serial.print("delta: ");
    // Serial.println(delta);
    if (delta >= FLICKERING_BRIGHTNESS_MAX_DELTA) {
      Serial.print("timer: ");
      Serial.println(millis());
      delta_flag = 1;
    } else if (delta == 0) {
      Serial.print("timer: ");
      Serial.println(millis());
      delta_flag = 0;
    }
    if (delta_flag) {
      delta -= FLICKERING_BRIGHTNESS_DELTA_STEP;
    } else {
      delta += FLICKERING_BRIGHTNESS_DELTA_STEP;
    }
    FastLED.setBrightness(brightness - delta);
  }
}

void flickering_color_temperature() {
  if (millis() - prev_millis_flickering_color_temperature_time >= FLICKERING_COLOR_TEMPERATURE_HALF_PERIOD / FLICKERING_COLOR_TEMPERATURE_STEPS) {
    prev_millis_flickering_color_temperature_time = millis();
    Serial.print("delta: ");
    Serial.println(delta);
    if (delta >= FLICKERING_COLOR_TEMPERATURE_MAX_DELTA) {
      Serial.print("timer: ");
      Serial.println(millis());
      delta_flag = 1;
    } else if (delta == 0) {
      Serial.print("timer: ");
      Serial.println(millis());
      delta_flag = 0;
    }
    if (delta_flag) {
      delta -= FLICKERING_COLOR_TEMPERATURE_DELTA_STEP;
    } else {
      delta += FLICKERING_COLOR_TEMPERATURE_DELTA_STEP;
    }
    set_color_temperature(ColorTemperatures[current_temperature - delta]);
  }
}

void sleep_timer() {
  if (millis() - prev_sleep_timer >= SLEEP_TIMER_DELAY) {
  }
}
