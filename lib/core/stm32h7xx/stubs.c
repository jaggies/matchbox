#include <stdint.h>
#include "stm32.h"

// Allow applications to override these default implementations
__weak void Error_Handler(void) {
}

__weak void vApplicationIdleHook (void) {
}

__weak void PreSleepHook(uint32_t* ulExpectedIdleTime) {
}

__weak void PostSleepHook(uint32_t *ulExpectedIdleTime) {
}

