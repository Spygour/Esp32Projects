#include "Pwm.h"

void Pwm_TimerInit(timerNum Timer, uint32_t frequency,  timerMode SlewRate, uint8_t Resolution, Pwm_Driver* PwmDrv)
{
    ledc_timer_config_t TimerCfg = 
    {
    .duty_resolution = Resolution,   // resolution of PWM duty
    .freq_hz = frequency,                     // frequency of PWM signal
    .speed_mode = SlewRate,                 // timer mode
    .timer_num = Timer,                      // timer index
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&TimerCfg);

    PwmDrv->TimerNumber = Timer;
    PwmDrv->Frequency = frequency;
    PwmDrv->TimerMode = SlewRate;
    PwmDrv->Resolution = Resolution;
}

void Pwm_TimerStop(Pwm_Driver* PwmDrv)
{

    ledc_timer_pause(PwmDrv->TimerMode, PwmDrv->TimerNumber);
    PwmDrv->Status = false;
}

void Pwm_TimerRun(Pwm_Driver* PwmDrv)
{
    ledc_timer_resume(PwmDrv->TimerMode, PwmDrv->TimerNumber);
    PwmDrv->Status = true;
}

bool Pwm_ChannelInit(gpioNum gpio, uint32_t duty, pwmChannel channel, Pwm_Driver* PwmDrv)
{
    bool result = false;
    if (duty > 0xFFF)
    {

    }
    else
    {
        ledc_channel_config_t PwmChCfg =
        {
            .channel    = channel,
            .duty       = duty,
            .gpio_num   = gpio,
            .speed_mode = PwmDrv->TimerMode,
            .hpoint     = 0,
            .timer_sel  = PwmDrv->TimerNumber,
            .flags.output_invert = 0
        };

        ledc_channel_config(&PwmChCfg);
        result = true;
    }
    PwmDrv->Channel = channel;
    PwmDrv->GpioNum = gpio;
    PwmDrv->DutyCycle = duty;
    PwmDrv->Status = true;
    return result;
}

bool Pwm_SetDuty(Pwm_Driver* PwmDrv, uint32_t dutycycle)
{
    uint8_t max_resolution = PwmDrv->Resolution + 1;
    bool result = false;
    if (dutycycle > (1<<max_resolution))
    {

    }
    else
    {
        ledc_set_duty(PwmDrv->TimerMode, PwmDrv->Channel, dutycycle);
        ledc_update_duty(PwmDrv->TimerMode, PwmDrv->Channel);
        result = true;
    }
    PwmDrv->DutyCycle = dutycycle;
    return result;

}