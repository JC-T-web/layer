/*
 * Temperature and Humidity Sensor Abstract Layer
 * Allows easy switching between different sensor types (AHT21, SHT30, DHT11, etc.)
 * Design pattern similar to MultiButton
 */

#ifndef __SENSOR_TEMP_HUMI_H__
#define __SENSOR_TEMP_HUMI_H__

#include <stdint.h>
#include <string.h>

// Sensor type
typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,
    SENSOR_TYPE_AHT21,
    SENSOR_TYPE_SHT30,
    SENSOR_TYPE_DHT11,
    SENSOR_TYPE_DHT22
} SensorType;

// Sensor state
typedef enum {
    SENSOR_STATE_IDLE = 0,
    SENSOR_STATE_MEASURING,
    SENSOR_STATE_READY,
    SENSOR_STATE_ERROR
} SensorState;

// Sensor operation result
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR_INIT,
    SENSOR_ERR_BUSY,
    SENSOR_ERR_TIMEOUT,
    SENSOR_ERR_COMM,
    SENSOR_ERR_INVALID_PARAM,
    SENSOR_ERR_NOT_READY
} SensorResult;

// Forward declaration
typedef struct _TempHumiSensor TempHumiSensor;

// Sensor operation function pointer types (similar to MultiButton's hal_button_level)
typedef SensorResult (*SensorInit)(void* driver_handle);
typedef SensorResult (*SensorReset)(void* driver_handle);
typedef SensorResult (*SensorTrigger)(void* driver_handle);
typedef SensorResult (*SensorRead)(void* driver_handle);
typedef SensorResult (*SensorGetTemp)(void* driver_handle, float* temp);
typedef SensorResult (*SensorGetHumi)(void* driver_handle, float* humi);
typedef SensorState  (*SensorGetState)(void* driver_handle);

// Sensor operation interface (similar to IIC_HAL_Ops design)
typedef struct {
    SensorInit      init;           // initialization
    SensorReset     reset;          // reset
    SensorTrigger   trigger;        // trigger measurement
    SensorRead      read;           // read data
    SensorGetTemp   get_temp;       // get temperature
    SensorGetHumi   get_humi;       // get humidity
    SensorGetState  get_state;      // get state
} SensorOps;

// Unified sensor handle
struct _TempHumiSensor {
    SensorType type;                // sensor type
    const SensorOps* ops;           // operation function set
    void* driver_handle;            // specific driver handle (AHT21_Handle* or SHT30_Handle*)
    
    // Cached data
    float temperature;
    float humidity;
    SensorState state;
    
    // Linked list support (similar to MultiButton's next)
    TempHumiSensor* next;
};

#ifdef __cplusplus
extern "C" {
#endif

// ========== Public API ==========

// Initialize sensor (similar to button_init)
void sensor_init(TempHumiSensor* handle, 
                 SensorType type,
                 const SensorOps* ops, 
                 void* driver_handle);

// Sensor control
SensorResult sensor_reset(TempHumiSensor* handle);
SensorResult sensor_trigger_measure(TempHumiSensor* handle);
SensorResult sensor_read_data(TempHumiSensor* handle);

// Get data
SensorResult sensor_get_temperature(TempHumiSensor* handle, float* temp);
SensorResult sensor_get_humidity(TempHumiSensor* handle, float* humi);
SensorResult sensor_get_both(TempHumiSensor* handle, float* temp, float* humi);
SensorState sensor_get_state(TempHumiSensor* handle);

// Blocking read
SensorResult sensor_read_blocking(TempHumiSensor* handle, float* temp, float* humi);

// State machine - call in timer (similar to button_ticks)
void sensor_ticks(void);

// Add/remove sensor (similar to button_start/stop)
int sensor_start(TempHumiSensor* handle);
void sensor_stop(TempHumiSensor* handle);

#ifdef __cplusplus
}
#endif

#endif // __SENSOR_TEMP_HUMI_H__
