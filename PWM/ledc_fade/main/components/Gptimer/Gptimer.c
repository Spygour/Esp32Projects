#include "Gptimer.h"

typedef struct  
{
    uint32_t timerCounter;
    bool startIsr;
}Gp_Isr_Struct;

static Gp_Isr_Struct GpIsrStruct =
{
    0,
    false
};

static gptimer_handle_t gptimer = NULL;



static bool GpTimerIsr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void* counterVal)
{
    if ((GpIsrStruct.timerCounter == 0) && (GpIsrStruct.startIsr == true))
    {
        GpIsrStruct.startIsr = false;
        gptimer_stop(gptimer);
        gptimer_disable(gptimer);
    }
    else if ((GpIsrStruct.timerCounter != 0) && (GpIsrStruct.startIsr == true))
    {
        GpIsrStruct.timerCounter--;
    }
    else
    {
        /* Do nothing */
    }
    return true;
}

void GpTimerInit(void)
{

    gptimer_config_t timer_config = 
    {
        .intr_priority = 1,
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    gptimer_new_timer(&timer_config, &gptimer);
    gptimer_alarm_config_t alarm_config = 
    {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = 10, // period 10 us
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);

    gptimer_event_callbacks_t cbs = 
    {
        .on_alarm = GpTimerIsr, // register user callback
    };
    gptimer_register_event_callbacks(gptimer, &cbs, (void*)&GpIsrStruct);
    gptimer_enable(gptimer);
    printf("Etrekse\n");
    gptimer_disable(gptimer);
}

void GpWaitMs(uint32_t milliseconds)
{
    GpIsrStruct.timerCounter = milliseconds * 100;
    GpIsrStruct.startIsr = true;
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
    while (GpIsrStruct.startIsr==true) {}
}

void GpWaitUs(uint32_t micros)
{
    GpIsrStruct.timerCounter = micros/10;
    GpIsrStruct.startIsr = true;
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
    while (GpIsrStruct.startIsr==true) {}
}
