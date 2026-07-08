/* Stub: FrontPanel.c uses the new ESP-IDF 6.x RMT API directly; this old-API
   implementation is not called. Stubs provided to satisfy the linker. */
#include "led_strip.h"

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config) { return NULL; }
led_strip_t *led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num) { return NULL; }
esp_err_t led_strip_denit(led_strip_t *strip) { return ESP_OK; }
