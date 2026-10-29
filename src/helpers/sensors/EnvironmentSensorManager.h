#pragma once

#include <Mesh.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>

#ifdef GPS_GUARD
  #ifndef GPS_GUARD_MIN_SATS
    #define GPS_GUARD_MIN_SATS      4
  #endif
  #ifndef GPS_GUARD_MAX_HDOP10
    #define GPS_GUARD_MAX_HDOP10    40      // HDOP 4.0
  #endif
  #ifndef GPS_GUARD_FENCE_M
    #define GPS_GUARD_FENCE_M       1000    // max distance from the reference position
  #endif
  #ifndef GPS_GUARD_STABLE_FIXES
    #define GPS_GUARD_STABLE_FIXES  5       // consecutive consistent fixes before trusting
  #endif
  #ifndef GPS_GUARD_STABLE_M
    #define GPS_GUARD_STABLE_M      100
  #endif
  #ifndef GPS_GUARD_MAX_SPEED_KMH
    #define GPS_GUARD_MAX_SPEED_KMH 250     // jump limit relative to the last trusted fix
  #endif
  #ifndef GPS_GUARD_TIME_MAX_STEP
    #define GPS_GUARD_TIME_MAX_STEP 600     // max GPS-vs-clock correction (s) once the clock is valid
  #endif
#endif

#ifdef GPS_DUTY_CYCLE
  #ifndef GPS_CYCLE_MIN
    #define GPS_CYCLE_MIN   30
  #endif
  #ifndef GPS_ONTIME_SEC
    #define GPS_ONTIME_SEC  120
  #endif
#endif

class EnvironmentSensorManager : public SensorManager {
protected:
  static const int MAX_ACTIVE_SENSORS = 16;

  // Query function pointer + sub-channel index (for multi-channel sensors like INA3221).
  // Sub-channel is 0 for all single-output sensors.
  struct ActiveSensor {
    void    (*query)(uint8_t channel, uint8_t sub_channel, CayenneLPP& telemetry);
    uint8_t   sub_channel;
  };

  ActiveSensor _active_sensors[MAX_ACTIVE_SENSORS];
  int          _active_sensor_count = 0;
  uint8_t      next_available_channel = TELEM_CHANNEL_SELF + 1;

  bool     gps_detected = false;
  bool     gps_active = false;
  uint32_t gps_update_interval_sec = 1;

  #if ENV_INCLUDE_GPS
  LocationProvider* _location;
  void start_gps();
  void stop_gps();
  void initBasicGPS();
  mutable char _setting_buf[48];

  #ifdef GPS_GUARD
  // Anti-spoofing: only plausible, stable fixes update the position, and GPS
  // time is written to the clock only while such a fix is current.
  double   _gps_ref_lat = 0, _gps_ref_lon = 0;   // 0,0 = no reference position
  uint32_t _gps_fence_m = GPS_GUARD_FENCE_M;
  bool     _fix_trusted = false;          // trusted fix seen in the current power window
  bool     _have_trusted_fix = false;     // node_lat/lon hold a trusted position
  unsigned long _fix_trusted_ms = 0;      // first trusted fix in the current window
  unsigned long _last_trusted_ms = 0;
  double   _last_trusted_lat = 0, _last_trusted_lon = 0;
  double   _cand_lat = 0, _cand_lon = 0;
  uint8_t  _stable_count = 0;
  char     _fix_status[24] = "no fix";
  char     _time_status[24] = "-";
  void evaluateGpsFix();
  bool checkGpsTime(uint32_t gps_time, uint32_t clock_time);
  static bool timeGuardCallback(void* ctx, uint32_t gps_time, uint32_t clock_time);
  #endif

  #ifdef GPS_DUTY_CYCLE
  // GPS is powered for at most _gps_ontime_sec every _gps_cycle_sec (0 = always on).
  uint32_t _gps_cycle_sec = GPS_CYCLE_MIN * 60;
  uint32_t _gps_ontime_sec = GPS_ONTIME_SEC;
  bool     _gps_powered = false;
  unsigned long _gps_window_start = 0;
  void gpsPowerOn();
  void gpsPowerOff();
  #endif
  #ifdef RAK_BOARD
  void rakGPSInit();
  bool gpsIsAwake(uint8_t ioPin);
  #endif
  #endif

public:
  #if ENV_INCLUDE_GPS
  EnvironmentSensorManager(LocationProvider &location): _location(&location){};
  LocationProvider* getLocationProvider() { return _location; }
  bool isGPSDetected() const override { return gps_detected; }
  #else
  EnvironmentSensorManager(){};
  #endif
  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
  #if ENV_INCLUDE_GPS || defined(ENV_INCLUDE_BME680_BSEC)
  void loop() override;
  #endif
  int getNumSettings() const override;
  const char* getSettingName(int i) const override;
  const char* getSettingValue(int i) const override;
  bool setSettingValue(const char* name, const char* value) override;
};
