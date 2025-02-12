#include <stdbool.h>
#include <stddef.h>

#define WIFI_PROV_SEC2_USERNAME "wifiprov"
#define WIFI_PROV_SEC2_PWD "abcd1234"
#define PROV_MGR_MAX_RETRY_CNT 3
#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"

void wifi_init(void);
bool wifi_is_provisioned(void);
void wifi_init_provisioning(char *payload, size_t payload_len);
void wifi_connect(void);
void wifi_reset_provisioning(void);