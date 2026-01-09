/*
 * AHT21 Adapter for Sensor Abstract Layer
 * Bridges AHT21 driver to generic sensor interface
 */

#include "sensor_temp_humi.h"
#include "aht21.h"

// Adapter functions
static SensorResult aht21_adapter_init(void* handle)
{
    // AHT21 already initialized externally, can do additional checks here
    return SENSOR_OK;
}

static SensorResult aht21_adapter_reset(void* handle)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    return (aht21_soft_reset(aht21) == AHT21_OK) ? SENSOR_OK : SENSOR_ERR_COMM;
}

static SensorResult aht21_adapter_trigger(void* handle)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    AHT21_Result result = aht21_trigger_measure(aht21);
    
    if (result == AHT21_OK) return SENSOR_OK;
    if (result == AHT21_ERR_BUSY) return SENSOR_ERR_BUSY;
    return SENSOR_ERR_COMM;
}

static SensorResult aht21_adapter_read(void* handle)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    AHT21_Result result = aht21_read_data(aht21);
    
    if (result == AHT21_OK) return SENSOR_OK;
    if (result == AHT21_ERR_BUSY) return SENSOR_ERR_BUSY;
    return SENSOR_ERR_COMM;
}

static SensorResult aht21_adapter_get_temp(void* handle, float* temp)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    return (aht21_get_temperature(aht21, temp) == AHT21_OK) ? SENSOR_OK : SENSOR_ERR_NOT_READY;
}

static SensorResult aht21_adapter_get_humi(void* handle, float* humi)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    return (aht21_get_humidity(aht21, humi) == AHT21_OK) ? SENSOR_OK : SENSOR_ERR_NOT_READY;
}

static SensorState aht21_adapter_get_state(void* handle)
{
    AHT21_Handle* aht21 = (AHT21_Handle*)handle;
    
    switch (aht21->state) {
        case AHT21_STATE_IDLE: return SENSOR_STATE_IDLE;
        case AHT21_STATE_WAIT_MEASURE: return SENSOR_STATE_MEASURING;
        case AHT21_STATE_READY: return SENSOR_STATE_READY;
        case AHT21_STATE_ERROR: return SENSOR_STATE_ERROR;
        default: return SENSOR_STATE_IDLE;
    }
}

// AHT21 operation function set (can be defined as const constant)
const SensorOps aht21_ops = {
    .init = aht21_adapter_init,
    .reset = aht21_adapter_reset,
    .trigger = aht21_adapter_trigger,
    .read = aht21_adapter_read,
    .get_temp = aht21_adapter_get_temp,
    .get_humi = aht21_adapter_get_humi,
    .get_state = aht21_adapter_get_state
};
