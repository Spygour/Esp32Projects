#pragma once

#include "driver/gptimer.h"


#ifdef __cplusplus
extern "C" {
#endif
/* CODE STARTS HERE */

void GpTimerInit(void);
void GpWaitMs(uint32_t milliseconds);
void GpWaitUs(uint32_t micros);

/* CODE ENDS HERE */
#ifdef __cplusplus
extern "C" {
#endif