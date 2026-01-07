/*
 * AHT21 Driver Implementation
 */

#include "aht21.h"

// Internal helper functions
static AHT21_Result aht21_check_status(AHT21_Handle* handle, uint8_t* status);
static void aht21_parse_data(AHT21_Handle* handle);

/**
  * @brief  Initialize AHT21 sensor
  */
AHT21_Result aht21_init(AHT21_Handle* handle, IIC_Handle* iic)
{
    if (!handle || !iic) return AHT21_ERR_INVALID_PARAM;
    
    memset(handle, 0, sizeof(AHT21_Handle));
    handle->iic = iic;
    handle->state = AHT21_STATE_INIT;
    handle->measure_interval = 100;  // default 100ms measurement interval
    
    // Wait for AHT21 power-up stabilization (recommended 40ms)
    if (iic->hal_ops->delay_ms) {
        iic->hal_ops->delay_ms(40);
    }
    
    // Send initialization command
    uint8_t init_cmd[3] = {AHT21_CMD_INIT, 0x08, 0x00};
    if (iic_write(iic, AHT21_ADDR, init_cmd, 3) != IIC_OK) {
        handle->state = AHT21_STATE_ERROR;
        return AHT21_ERR_IIC;
    }
    
    // Wait for initialization to complete
    if (iic->hal_ops->delay_ms) {
        iic->hal_ops->delay_ms(10);
    }
    
    // Check calibration status
    uint8_t status;
    if (aht21_check_status(handle, &status) != AHT21_OK) {
        handle->state = AHT21_STATE_ERROR;
        return AHT21_ERR_IIC;
    }
    
    if (!(status & AHT21_STATUS_CALIBRATED)) {
        handle->state = AHT21_STATE_ERROR;
        return AHT21_ERR_NOT_INIT;
    }
    
    handle->state = AHT21_STATE_IDLE;
    return AHT21_OK;
}

/**
  * @brief  Soft reset AHT21
  */
AHT21_Result aht21_soft_reset(AHT21_Handle* handle)
{
    if (!handle) return AHT21_ERR_INVALID_PARAM;
    
    uint8_t reset_cmd = AHT21_CMD_SOFT_RESET;
    if (iic_write(handle->iic, AHT21_ADDR, &reset_cmd, 1) != IIC_OK) {
        return AHT21_ERR_IIC;
    }
    
    // Wait for reset to complete
    if (handle->iic->hal_ops->delay_ms) {
        handle->iic->hal_ops->delay_ms(20);
    }
    
    handle->state = AHT21_STATE_IDLE;
    return AHT21_OK;
}

/**
  * @brief  Trigger measurement
  */
AHT21_Result aht21_trigger_measure(AHT21_Handle* handle)
{
    if (!handle) return AHT21_ERR_INVALID_PARAM;
    if (handle->state == AHT21_STATE_WAIT_MEASURE) return AHT21_ERR_BUSY;
    
    // Send measurement command
    uint8_t measure_cmd[3] = {AHT21_CMD_TRIGGER, 0x33, 0x00};
    if (iic_write(handle->iic, AHT21_ADDR, measure_cmd, 3) != IIC_OK) {
        return AHT21_ERR_IIC;
    }
    
    handle->state = AHT21_STATE_WAIT_MEASURE;
    handle->measure_ticks = 0;
    
    return AHT21_OK;
}

/**
  * @brief  Read measurement data
  */
AHT21_Result aht21_read_data(AHT21_Handle* handle)
{
    if (!handle) return AHT21_ERR_INVALID_PARAM;
    
    // Check status
    uint8_t status;
    if (aht21_check_status(handle, &status) != AHT21_OK) {
        return AHT21_ERR_IIC;
    }
    
    if (status & AHT21_STATUS_BUSY) {
        return AHT21_ERR_BUSY;
    }
    
    // Read 7 bytes of data
    if (iic_read(handle->iic, AHT21_ADDR, handle->raw_data, 7) != IIC_OK) {
        return AHT21_ERR_IIC;
    }
    
    // Parse data
    aht21_parse_data(handle);
    handle->state = AHT21_STATE_READY;
    
    return AHT21_OK;
}

/**
  * @brief  Get temperature
  */
AHT21_Result aht21_get_temperature(AHT21_Handle* handle, float* temp)
{
    if (!handle || !temp) return AHT21_ERR_INVALID_PARAM;
    if (handle->state != AHT21_STATE_READY) return AHT21_ERR_NOT_INIT;
    
    *temp = handle->temperature;
    return AHT21_OK;
}

/**
  * @brief  Get humidity
  */
AHT21_Result aht21_get_humidity(AHT21_Handle* handle, float* humi)
{
    if (!handle || !humi) return AHT21_ERR_INVALID_PARAM;
    if (handle->state != AHT21_STATE_READY) return AHT21_ERR_NOT_INIT;
    
    *humi = handle->humidity;
    return AHT21_OK;
}

/**
  * @brief  State machine - call periodically in timer (recommended 5-10ms)
  */
void aht21_ticks(AHT21_Handle* handle)
{
    if (!handle) return;
    
    switch (handle->state) {
    case AHT21_STATE_IDLE:
        // Automatically trigger measurement
        aht21_trigger_measure(handle);
        break;
        
    case AHT21_STATE_WAIT_MEASURE:
        handle->measure_ticks++;
        // AHT21 measurement time is about 80ms
        if (handle->measure_ticks >= 80 / 5) {  // assuming 5ms call interval
            if (aht21_read_data(handle) == AHT21_OK) {
                handle->state = AHT21_STATE_READY;
            }
        }
        break;
        
    case AHT21_STATE_READY:
        // Wait for a period before measuring again
        handle->measure_ticks++;
        if (handle->measure_ticks >= handle->measure_interval / 5) {
            handle->state = AHT21_STATE_IDLE;
            handle->measure_ticks = 0;
        }
        break;
        
    case AHT21_STATE_ERROR:
        // Error state - try soft reset
        aht21_soft_reset(handle);
        break;
        
    default:
        handle->state = AHT21_STATE_IDLE;
        break;
    }
}

/**
  * @brief  Blocking read temperature and humidity
  */
AHT21_Result aht21_read_blocking(AHT21_Handle* handle, float* temp, float* humi)
{
    if (!handle || !temp || !humi) return AHT21_ERR_INVALID_PARAM;
    
    AHT21_Result result;
    
    // Trigger measurement
    result = aht21_trigger_measure(handle);
    if (result != AHT21_OK) return result;
    
    // Wait for measurement to complete (80ms)
    if (handle->iic->hal_ops->delay_ms) {
        handle->iic->hal_ops->delay_ms(80);
    }
    
    // Read data
    result = aht21_read_data(handle);
    if (result != AHT21_OK) return result;
    
    *temp = handle->temperature;
    *humi = handle->humidity;
    
    return AHT21_OK;
}

// ========== Internal Functions ==========

/**
  * @brief  Check AHT21 status
  */
static AHT21_Result aht21_check_status(AHT21_Handle* handle, uint8_t* status)
{
    if (iic_read(handle->iic, AHT21_ADDR, status, 1) != IIC_OK) {
        return AHT21_ERR_IIC;
    }
    return AHT21_OK;
}

/**
  * @brief  Parse raw data
  */
static void aht21_parse_data(AHT21_Handle* handle)
{
    uint32_t humidity_raw, temperature_raw;
    
    // Extract humidity data (20 bits)
    humidity_raw = ((uint32_t)handle->raw_data[1] << 12) |
                   ((uint32_t)handle->raw_data[2] << 4) |
                   ((uint32_t)handle->raw_data[3] >> 4);
    
    // Extract temperature data (20 bits)
    temperature_raw = (((uint32_t)handle->raw_data[3] & 0x0F) << 16) |
                      ((uint32_t)handle->raw_data[4] << 8) |
                      ((uint32_t)handle->raw_data[5]);
    
    // Convert to actual values
    handle->humidity = (humidity_raw * 100.0f) / 1048576.0f;
    handle->temperature = (temperature_raw * 200.0f) / 1048576.0f - 50.0f;
}
