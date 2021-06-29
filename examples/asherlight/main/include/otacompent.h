
#ifndef OTA_COMPENT_H
#define OTA_COMPENT_H

typedef esp_err_t (*ota_callback)();

void ota_check(ota_callback call);

#endif