#pragma once

#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef ledc_timer_t timerNum;
typedef ledc_mode_t  timerMode;
typedef int          gpioNum;
typedef ledc_channel_t pwmChannel;

typedef struct 
{
    timerNum    TimerNumber;
    timerMode   TimerMode;
    uint32_t    Frequency;
    uint32_t    DutyCycle;
    uint8_t     Resolution;
    int         GpioNum;
    pwmChannel  Channel;
    bool        Status;
}Pwm_Driver;

extern void Pwm_TimerInit(timerNum Timer, uint32_t frequency,  timerMode SlewRate, uint8_t Resolution, Pwm_Driver* PwmDrv);
extern void Pwm_TimerStop(Pwm_Driver* PwmDrv);
extern void Pwm_TimerRun(Pwm_Driver* PwmDrv);
extern bool Pwm_ChannelInit(gpioNum gpio, uint32_t duty, pwmChannel channel, Pwm_Driver* PwmDrv);
extern bool Pwm_SetDuty(Pwm_Driver* PwmDrv, uint32_t dutycycle);


#ifdef __cplusplus
}
#endif


