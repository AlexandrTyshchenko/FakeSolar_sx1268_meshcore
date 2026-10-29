#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>

#define  PIN_VBAT_READ 17
#ifndef ADC_MULTIPLIER
#define  ADC_MULTIPLIER   (1.815f) // dependent on voltage divider resistors. TODO: more accurate battery tracking
#endif

#ifdef TELEM_INA3221_BAT_CH
  // implemented in EnvironmentSensorManager.cpp; false until the INA3221 is initialised
  bool ina3221_read_battery(uint16_t* millivolts, float* amps);
  #ifndef TELEM_INA3221_CHARGE_THRESHOLD_A
    #define TELEM_INA3221_CHARGE_THRESHOLD_A 0.005f  // ignore noise below 5 mA
  #endif
#endif

class PromicroBoard : public NRF52BoardDCDC {
protected:
  uint8_t btn_prev_state;
  float adc_mult = ADC_MULTIPLIER;

public:
  PromicroBoard() : NRF52Board("ProMicro_OTA") {}
  void begin();

  #define BATTERY_SAMPLES 8

  uint16_t getBattMilliVolts() override {
  #ifdef TELEM_INA3221_BAT_CH
    uint16_t mv;
    if (ina3221_read_battery(&mv, NULL)) return mv;
  #endif
    analogReadResolution(12);
  #ifdef BATT_ADC_SAMPLE_TIME_US
    analogSampleTime(BATT_ADC_SAMPLE_TIME_US);  // longer acquisition for high-impedance dividers
  #endif

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;
    return (adc_mult * raw);
  }

  bool setAdcMultiplier(float multiplier) override {
    if (multiplier == 0.0f) {
      adc_mult = ADC_MULTIPLIER;}
    else {
      adc_mult = multiplier;
    }
    return true;
  }
  float getAdcMultiplier() const override {
    if (adc_mult == 0.0f) {
      return ADC_MULTIPLIER;
    } else {
      return adc_mult;
    }
  }

#ifdef TELEM_INA3221_BAT_CH
  bool isChargerActive() override {
    float amps;
    if (!ina3221_read_battery(NULL, &amps)) return false;
  #ifdef TELEM_INA3221_CHARGING_INVERT
    amps = -amps;
  #endif
    return amps > TELEM_INA3221_CHARGE_THRESHOLD_A;
  }
#endif

  const char* getManufacturerName() const override {
    return "ProMicro DIY";
  }

  int buttonStateChanged() {
    #ifdef BUTTON_PIN
      uint8_t v = digitalRead(BUTTON_PIN);
      if (v != btn_prev_state) {
        btn_prev_state = v;
        return (v == LOW) ? 1 : -1;
      }
    #endif
      return 0;
  }
};
