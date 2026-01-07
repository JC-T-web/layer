/*
 * Temperature and Humidity Sensor Abstract Layer Implementation
 */

#include "sensor_temp_humi.h"

// Sensor linked list head (similar to MultiButton's head_handle)
static TempHumiSensor* head_sensor = NULL;

// Internal state machine handler
static void sensor_handler(TempHumiSensor* handle);

/**
  * @brief  Initialize sensor handle
  */
void sensor_init(TempHumiSensor* handle, 
                 SensorType type,
                 const SensorOps* ops, 
                 void* driver_handle)
{
    if (!handle || !ops || !driver_handle) return;
    
    memset(handle, 0, sizeof(TempHumiSensor));
    handle->type = type;
    handle->ops = ops;
    handle->driver_handle = driver_handle;
    handle->state = SENSOR_STATE_IDLE;
    
    // Call specific driver initialization
    if (handle->ops->init) {
        handle->ops->init(driver_handle);
    }
}

/**
  * @brief  Reset sensor
  */
SensorResult sensor_reset(TempHumiSensor* handle)
{
    if (!handle || !handle->ops->reset) return SENSOR_ERR_INVALID_PARAM;
    
    SensorResult result = handle->ops->reset(handle->driver_handle);
    if (result == SENSOR_OK) {
        handle->state = SENSOR_STATE_IDLE;
    }
    return result;
}

/**
  * @brief  Trigger measurement
  */
SensorResult sensor_trigger_measure(TempHumiSensor* handle)
{
    if (!handle || !handle->ops->trigger) return SENSOR_ERR_INVALID_PARAM;
    
    SensorResult result = handle->ops->trigger(handle->driver_handle);
    if (result == SENSOR_OK) {
        handle->state = SENSOR_STATE_MEASURING;
    }
    return result;
}

/**
  * @brief  Read data
  */
SensorResult sensor_read_data(TempHumiSensor* handle)
{
    if (!handle || !handle->ops->read) return SENSOR_ERR_INVALID_PARAM;
    
    SensorResult result = handle->ops->read(handle->driver_handle);
    if (result == SENSOR_OK) {
        // Update cached data
        handle->ops->get_temp(handle->driver_handle, &handle->temperature);
        handle->ops->get_humi(handle->driver_handle, &handle->humidity);
        handle->state = SENSOR_STATE_READY;
    }
    return result;
}

/**
  * @brief  Get temperature
  */
SensorResult sensor_get_temperature(TempHumiSensor* handle, float* temp)
{
    if (!handle || !temp) return SENSOR_ERR_INVALID_PARAM;
    if (handle->state != SENSOR_STATE_READY) return SENSOR_ERR_NOT_READY;
    
    *temp = handle->temperature;
    return SENSOR_OK;
}

/**
  * @brief  Get humidity
  */
SensorResult sensor_get_humidity(TempHumiSensor* handle, float* humi)
{
    if (!handle || !humi) return SENSOR_ERR_INVALID_PARAM;
    if (handle->state != SENSOR_STATE_READY) return SENSOR_ERR_NOT_READY;
    
    *humi = handle->humidity;
    return SENSOR_OK;
}

/**
  * @brief  Get both temperature and humidity
  */
SensorResult sensor_get_both(TempHumiSensor* handle, float* temp, float* humi)
{
    if (!handle || !temp || !humi) return SENSOR_ERR_INVALID_PARAM;
    if (handle->state != SENSOR_STATE_READY) return SENSOR_ERR_NOT_READY;
    
    *temp = handle->temperature;
    *humi = handle->humidity;
    return SENSOR_OK;
}

/**
  * @brief  Get sensor state
  */
SensorState sensor_get_state(TempHumiSensor* handle)
{
    if (!handle || !handle->ops->get_state) return SENSOR_STATE_ERROR;
    return handle->ops->get_state(handle->driver_handle);
}

/**
  * @brief  Blocking read
  */
SensorResult sensor_read_blocking(TempHumiSensor* handle, float* temp, float* humi)
{
    if (!handle || !temp || !humi) return SENSOR_ERR_INVALID_PARAM;
    
    SensorResult result;
    
    // Trigger measurement
    result = sensor_trigger_measure(handle);
    if (result != SENSOR_OK) return result;
    
    // Wait for measurement to complete (poll status)
    uint16_t timeout = 1000;  // 100ms timeout
    while (timeout--) {
        SensorState state = sensor_get_state(handle);
        if (state == SENSOR_STATE_READY || state != SENSOR_STATE_MEASURING) {
            break;
        }
        // Delay 100us (needs hardware layer support)
    }
    
    // Read data
    result = sensor_read_data(handle);
    if (result != SENSOR_OK) return result;
    
    return sensor_get_both(handle, temp, humi);
}

/**
  * @brief  Add sensor to working list (similar to button_start)
  */
int sensor_start(TempHumiSensor* handle)
{
    if (!handle) return -2;
    
    TempHumiSensor* target = head_sensor;
    while (target) {
        if (target == handle) return -1;  // already exists
        target = target->next;
    }
    
    handle->next = head_sensor;
    head_sensor = handle;
    return 0;
}

/**
  * @brief  Remove sensor from working list (similar to button_stop)
  */
void sensor_stop(TempHumiSensor* handle)
{
    if (!handle) return;
    
    TempHumiSensor** curr;
    for (curr = &head_sensor; *curr; ) {
        TempHumiSensor* entry = *curr;
        if (entry == handle) {
            *curr = entry->next;
            entry->next = NULL;
            return;
        } else {
            curr = &entry->next;
        }
    }
}

/**
  * @brief  Sensor state machine handler (internal function)
  */
static void sensor_handler(TempHumiSensor* handle)
{
    SensorState state = sensor_get_state(handle);
    
    switch (state) {
    case SENSOR_STATE_IDLE:
        // Automatically trigger measurement
        sensor_trigger_measure(handle);
        break;
        
    case SENSOR_STATE_MEASURING:
        // Wait for measurement to complete, try to read
        sensor_read_data(handle);
        break;
        
    case SENSOR_STATE_READY:
        // Data ready, wait for a period before remeasuring
        // Timing logic can be added here
        break;
        
    case SENSOR_STATE_ERROR:
        // Error state, try to reset
        sensor_reset(handle);
        break;
        
    default:
        break;
    }
}

/**
  * @brief  Background timer call (similar to button_ticks)
  */
void sensor_ticks(void)
{
    TempHumiSensor* target;
    for (target = head_sensor; target; target = target->next) {
        sensor_handler(target);
    }
}
