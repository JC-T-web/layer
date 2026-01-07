/*
 * IIC Abstract Layer - Hardware Independent I2C Driver
 * Similar design philosophy to MultiButton
 */

#ifndef __IIC_ABSTRACT_H__
#define __IIC_ABSTRACT_H__

#include <stdint.h>
#include <string.h>

// IIC operation result
typedef enum {
    IIC_OK = 0,
    IIC_ERR_NACK,
    IIC_ERR_TIMEOUT,
    IIC_ERR_BUS_BUSY,
    IIC_ERR_INVALID_PARAM
} IIC_Result;

// Hardware abstraction layer operations (similar to MultiButton's hal_button_level)
typedef struct {
    void (*delay_us)(uint32_t us);           // microsecond delay
    void (*delay_ms)(uint32_t ms);           // millisecond delay
    void (*set_sda)(uint8_t level);          // set SDA level
    void (*set_scl)(uint8_t level);          // set SCL level
    uint8_t (*read_sda)(void);               // read SDA level
    void (*sda_mode)(uint8_t is_output);     // set SDA direction (1:output, 0:input)
} IIC_HAL_Ops;

// IIC handle structure
typedef struct {
    const IIC_HAL_Ops* hal_ops;  // hardware operation function set
    uint32_t speed_khz;          // IIC speed (kHz)
    uint16_t timeout_ms;         // timeout (ms)
    uint8_t bus_busy;            // bus busy flag
} IIC_Handle;

#ifdef __cplusplus
extern "C" {
#endif

// API functions
void iic_init(IIC_Handle* handle, const IIC_HAL_Ops* hal_ops);
void iic_set_speed(IIC_Handle* handle, uint32_t speed_khz);
void iic_set_timeout(IIC_Handle* handle, uint16_t timeout_ms);

IIC_Result iic_start(IIC_Handle* handle);
IIC_Result iic_stop(IIC_Handle* handle);
IIC_Result iic_write_byte(IIC_Handle* handle, uint8_t data);
IIC_Result iic_read_byte(IIC_Handle* handle, uint8_t* data, uint8_t ack);

// High-level API
IIC_Result iic_write(IIC_Handle* handle, uint8_t addr, const uint8_t* data, uint16_t len);
IIC_Result iic_read(IIC_Handle* handle, uint8_t addr, uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // __IIC_ABSTRACT_H__
