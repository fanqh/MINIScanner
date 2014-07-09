#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#define BATTERY_PROBE_READING_SOURCE	VM_ADC_SRC_AIO0			/** defined in adc_if.h **/
#define BATTERY_READING_SOURCE			AIO0					/** defined in battery.h, not the enum defined in adc_if.h (VM_ADC_SRC_AIO0) **/
#define BATTERY_POLLING_PERIOD			(30000)

#define BATTERY_LOW_HYSTERESIS_HIGH_BOUND	3250
#define BATTERY_LOW_HYSTERESIS_LOW_BOUND	3150

#define HAL_ACTIVE_AUTO_SHUTDOWN_DURATION	(D_MIN(10))

#endif /* HAL_CONFIG_H */


