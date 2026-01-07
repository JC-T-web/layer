/*
 * AHT21 Temperature and Humidity Sensor Driver
 * Based on hardware-independent IIC abstract layer
 */

#ifndef __AHT21_H__
#define __AHT21_H__

#include "iic_abstract.h"

// AHT21 I2C address
#define AHT21_ADDR              0x38

// AHT21 commands
#define AHT21_CMD_INIT          0xBE
#define AHT21_CMD_TRIGGER       0xAC
#define AHT21_CMD_SOFT_RESET    0xBA

// AHT21 status bits
#define AHT21_STATUS_BUSY       0x80
#define AHT21_STATUS_CALIBRATED 0x08

// AHT21 state enumeration
typedef enum {
    AHT21_STATE_IDLE = 0,       // idle state
    AHT21_STATE_INIT,           // initialization state
    AHT21_STATE_WAIT_MEASURE,   // waiting for measurement
    AHT21_STATE_READY,          // data ready
    AHT21_STATE_ERROR           // error state
} AHT21_State;

// AHT21 operation result
typedef enum {
    AHT21_OK = 0,
    AHT21_ERR_NOT_INIT,
    AHT21_ERR_BUSY,
    AHT21_ERR_TIMEOUT,
    AHT21_ERR_IIC,
    AHT21_ERR_INVALID_PARAM,
    AHT21_ERR_CRC
} AHT21_Result;

// AHT21 handle structure
typedef struct {
    IIC_Handle* iic;            // IIC handle
    AHT21_State state;          // current state
    uint32_t measure_ticks;     // measurement timer
    
    // Raw data
    uint8_t raw_data[7];        // raw read data
    
    // Parsed data
    float temperature;          // temperature (°C)
    float humidity;             // humidity (%)
    
    // Configuration
    uint16_t measure_interval;  // measurement interval (ms)
} AHT21_Handle;

#ifdef __cplusplus
extern "C" {
#endif

// API functions
AHT21_Result aht21_init(AHT21_Handle* handle, IIC_Handle* iic);
AHT21_Result aht21_soft_reset(AHT21_Handle* handle);
AHT21_Result aht21_trigger_measure(AHT21_Handle* handle);
AHT21_Result aht21_read_data(AHT21_Handle* handle);
AHT21_Result aht21_get_temperature(AHT21_Handle* handle, float* temp);
AHT21_Result aht21_get_humidity(AHT21_Handle* handle, float* humi);

// State machine function - call periodically in timer
void aht21_ticks(AHT21_Handle* handle);

// Convenience function - blocking read
AHT21_Result aht21_read_blocking(AHT21_Handle* handle, float* temp, float* humi);

#ifdef __cplusplus
}
#endif

#endif // __AHT21_H__
