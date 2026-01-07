/*
 * IIC Abstract Layer Implementation
 */

#include "iic_abstract.h"

// Internal delay calculation
static inline void iic_delay(IIC_Handle* handle)
{
    if (handle->hal_ops->delay_us) {
        uint32_t delay = 500 / handle->speed_khz;  // half period delay
        handle->hal_ops->delay_us(delay > 0 ? delay : 1);
    }
}

/**
  * @brief  Initialize IIC handle
  */
void iic_init(IIC_Handle* handle, const IIC_HAL_Ops* hal_ops)
{
    if (!handle || !hal_ops) return;
    
    memset(handle, 0, sizeof(IIC_Handle));
    handle->hal_ops = hal_ops;
    handle->speed_khz = 100;      // default 100kHz
    handle->timeout_ms = 1000;    // default 1s timeout
    
    // Initialize IIC bus
    if (handle->hal_ops->sda_mode) {
        handle->hal_ops->sda_mode(1);
    }
    handle->hal_ops->set_sda(1);
    handle->hal_ops->set_scl(1);
}

/**
  * @brief  Set IIC speed
  */
void iic_set_speed(IIC_Handle* handle, uint32_t speed_khz)
{
    if (handle) {
        handle->speed_khz = speed_khz;
    }
}

/**
  * @brief  Set timeout
  */
void iic_set_timeout(IIC_Handle* handle, uint16_t timeout_ms)
{
    if (handle) {
        handle->timeout_ms = timeout_ms;
    }
}

/**
  * @brief  IIC start condition
  */
IIC_Result iic_start(IIC_Handle* handle)
{
    if (!handle) return IIC_ERR_INVALID_PARAM;
    
    handle->hal_ops->sda_mode(1);
    handle->hal_ops->set_sda(1);
    handle->hal_ops->set_scl(1);
    iic_delay(handle);
    
    handle->hal_ops->set_sda(0);
    iic_delay(handle);
    handle->hal_ops->set_scl(0);
    
    handle->bus_busy = 1;
    return IIC_OK;
}

/**
  * @brief  IIC stop condition
  */
IIC_Result iic_stop(IIC_Handle* handle)
{
    if (!handle) return IIC_ERR_INVALID_PARAM;
    
    handle->hal_ops->sda_mode(1);
    handle->hal_ops->set_scl(0);
    handle->hal_ops->set_sda(0);
    iic_delay(handle);
    
    handle->hal_ops->set_scl(1);
    iic_delay(handle);
    handle->hal_ops->set_sda(1);
    iic_delay(handle);
    
    handle->bus_busy = 0;
    return IIC_OK;
}

/**
  * @brief  IIC write one byte
  */
IIC_Result iic_write_byte(IIC_Handle* handle, uint8_t data)
{
    if (!handle) return IIC_ERR_INVALID_PARAM;
    
    handle->hal_ops->sda_mode(1);
    handle->hal_ops->set_scl(0);
    
    // Send 8 bits
    for (uint8_t i = 0; i < 8; i++) {
        handle->hal_ops->set_sda((data & 0x80) ? 1 : 0);
        data <<= 1;
        iic_delay(handle);
        handle->hal_ops->set_scl(1);
        iic_delay(handle);
        handle->hal_ops->set_scl(0);
    }
    
    // Wait for ACK
    handle->hal_ops->set_sda(1);
    handle->hal_ops->sda_mode(0);  // SDA switch to input
    iic_delay(handle);
    handle->hal_ops->set_scl(1);
    iic_delay(handle);
    
    uint8_t ack = handle->hal_ops->read_sda();
    handle->hal_ops->set_scl(0);
    
    return (ack == 0) ? IIC_OK : IIC_ERR_NACK;
}

/**
  * @brief  IIC read one byte
  * @param  ack: 1=send ACK, 0=send NACK
  */
IIC_Result iic_read_byte(IIC_Handle* handle, uint8_t* data, uint8_t ack)
{
    if (!handle || !data) return IIC_ERR_INVALID_PARAM;
    
    uint8_t receive = 0;
    handle->hal_ops->sda_mode(0);  // SDA set to input
    
    // Receive 8 bits
    for (uint8_t i = 0; i < 8; i++) {
        handle->hal_ops->set_scl(0);
        iic_delay(handle);
        handle->hal_ops->set_scl(1);
        receive <<= 1;
        if (handle->hal_ops->read_sda()) {
            receive |= 0x01;
        }
        iic_delay(handle);
    }
    
    // Send ACK/NACK
    handle->hal_ops->set_scl(0);
    handle->hal_ops->sda_mode(1);  // SDA switch to output
    handle->hal_ops->set_sda(ack ? 0 : 1);
    iic_delay(handle);
    handle->hal_ops->set_scl(1);
    iic_delay(handle);
    handle->hal_ops->set_scl(0);
    
    *data = receive;
    return IIC_OK;
}

/**
  * @brief  IIC write multiple bytes
  */
IIC_Result iic_write(IIC_Handle* handle, uint8_t addr, const uint8_t* data, uint16_t len)
{
    if (!handle || !data || len == 0) return IIC_ERR_INVALID_PARAM;
    
    IIC_Result result;
    
    result = iic_start(handle);
    if (result != IIC_OK) return result;
    
    result = iic_write_byte(handle, addr << 1);  // write address
    if (result != IIC_OK) {
        iic_stop(handle);
        return result;
    }
    
    for (uint16_t i = 0; i < len; i++) {
        result = iic_write_byte(handle, data[i]);
        if (result != IIC_OK) {
            iic_stop(handle);
            return result;
        }
    }
    
    iic_stop(handle);
    return IIC_OK;
}

/**
  * @brief  IIC read multiple bytes
  */
IIC_Result iic_read(IIC_Handle* handle, uint8_t addr, uint8_t* data, uint16_t len)
{
    if (!handle || !data || len == 0) return IIC_ERR_INVALID_PARAM;
    
    IIC_Result result;
    
    result = iic_start(handle);
    if (result != IIC_OK) return result;
    
    result = iic_write_byte(handle, (addr << 1) | 0x01);  // read address
    if (result != IIC_OK) {
        iic_stop(handle);
        return result;
    }
    
    for (uint16_t i = 0; i < len; i++) {
        result = iic_read_byte(handle, &data[i], (i < len - 1) ? 1 : 0);
        if (result != IIC_OK) {
            iic_stop(handle);
            return result;
        }
    }
    
    iic_stop(handle);
    return IIC_OK;
}
