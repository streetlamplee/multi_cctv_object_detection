#ifndef MODBUS_HANDLER_H
#define MODBUS_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

// Input Register (Read Only)
#define MODBUS_IREG_SYSTEM_RESERVED                21

// 1110 hj modbus
// #define MODBUS_IREG_CHANNEL1_ALARM_STATUS          21
// #define MODBUS_IREG_CHANNEL1_ALARM_ID              22
// #define MODBUS_IREG_CHANNEL2_ALARM_STATUS          23
// #define MODBUS_IREG_CHANNEL2_ALARM_ID              24
// #define MODBUS_IREG_CHANNEL3_ALARM_STATUS          25
// #define MODBUS_IREG_CHANNEL3_ALARM_ID              26
// #define MODBUS_IREG_CHANNEL4_ALARM_STATUS          27
// #define MODBUS_IREG_CHANNEL4_ALARM_ID              28
// #define MODBUS_IREG_CHANNEL5_ALARM_STATUS          29
// #define MODBUS_IREG_CHANNEL5_ALARM_ID              30
// #define MODBUS_IREG_CHANNEL6_ALARM_STATUS          31
// #define MODBUS_IREG_CHANNEL6_ALARM_ID              32
// #define MODBUS_IREG_CHANNEL7_ALARM_STATUS          33
// #define MODBUS_IREG_CHANNEL7_ALARM_ID              34
// #define MODBUS_IREG_CHANNEL8_ALARM_STATUS          35
// #define MODBUS_IREG_CHANNEL8_ALARM_ID              36
// #define MODBUS_IREG_CHANNEL9_ALARM_STATUS          37
// #define MODBUS_IREG_CHANNEL9_ALARM_ID              38
// #define MODBUS_IREG_CHANNEL10_ALARM_STATUS         39
// #define MODBUS_IREG_CHANNEL10_ALARM_ID             40
// #define MODBUS_IREG_CHANNEL11_ALARM_STATUS         41
// #define MODBUS_IREG_CHANNEL11_ALARM_ID             42
// #define MODBUS_IREG_CHANNEL12_ALARM_STATUS         43
// #define MODBUS_IREG_CHANNEL12_ALARM_ID             44


// Holding Register (Read/Write)
#define MODBUS_HREG_SYSTEM_OPTION_RESERVED         21

// 1110 hj modbus
// #define MODBUS_HREG_CHANNEL1_ALARM_COMPLETE        21
// #define MODBUS_HREG_CHANNEL2_ALARM_COMPLETE        23
// #define MODBUS_HREG_CHANNEL3_ALARM_COMPLETE        25
// #define MODBUS_HREG_CHANNEL4_ALARM_COMPLETE        27
// #define MODBUS_HREG_CHANNEL5_ALARM_COMPLETE        29
// #define MODBUS_HREG_CHANNEL6_ALARM_COMPLETE        31
// #define MODBUS_HREG_CHANNEL7_ALARM_COMPLETE        33
// #define MODBUS_HREG_CHANNEL8_ALARM_COMPLETE        35
// #define MODBUS_HREG_CHANNEL9_ALARM_COMPLETE        37
// #define MODBUS_HREG_CHANNEL10_ALARM_COMPLETE       39
// #define MODBUS_HREG_CHANNEL11_ALARM_COMPLETE       41
// #define MODBUS_HREG_CHANNEL12_ALARM_COMPLETE       43

void modbus_handler_init();
void modbus_handler_release();

void modbus_handler_start();
void modbus_handler_stop();

void modbus_handler_set_ireg(int reg, uint16_t value);
void modbus_handler_set_hreg(int reg, uint16_t value);
uint16_t modbus_handler_get_hreg(int reg);
uint16_t modbus_handler_get_ireg(int reg); // 1106 hj modbus 적용

#ifdef __cplusplus
}
#endif

#endif