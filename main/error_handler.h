#include <esp_err.h>

void handle_error(esp_err_t err);


// TODO: get rid of this macro
// TODO: handle WiFi errors in wifi file, keep handle_error generic to just restart device

#define CHECK_ERROR(x)                                           \
  do {                                                           \
    esp_err_t __err_rc = (x);                                    \
    if (__err_rc != ESP_OK) {                                    \
      ESP_LOGE("ERROR", "Error: %s", esp_err_to_name(__err_rc)); \
      handle_error(__err_rc);                                    \
    }                                                            \
  } while (0)
