// M0: EPD driver bring-up. Static full-refresh screen to verify the
// copied driver + pin map are correct before any navigation/network
// code is layered on. See docs/plans in the parent repo history for
// the full milestone plan.
#include "src/epd_driver/EPD.h"
#include "src/epd_driver/EPD_Init.h"

// Panel power enable — must be driven HIGH before EPD_GPIOInit()/any
// SPI traffic, confirmed against vendor-reference main.ino setup().
#define PANEL_POWER_PIN 7

uint8_t ImageBW[27200];

void setup() {
  Serial.begin(115200);

  pinMode(PANEL_POWER_PIN, OUTPUT);
  digitalWrite(PANEL_POWER_PIN, HIGH);

  EPD_GPIOInit();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);

  EPD_ShowString(20, 20, "Hello Entrance HMI", 48, BLACK);
  EPD_ShowString(20, 90, "M0: driver bring-up OK", 24, BLACK);

  EPD_Init();
  EPD_Display(ImageBW);
  EPD_Update();
  EPD_DeepSleep();

  Serial.println("M0 boot screen shown.");
}

void loop() {
  // Static screen for M0 — nothing to do yet.
}
