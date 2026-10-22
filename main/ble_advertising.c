#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#define BLE_DEVICE_NAME "Robbo Vision"

static const char *TAG = "BLE";

static int on_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (event->type == BLE_GAP_EVENT_ADV_COMPLETE) {
        ESP_LOGW(TAG, "advertising stopped: reason=%d", event->adv_complete.reason);
    }
    return 0;
}

static void start_advertising(uint8_t own_addr_type)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (const uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "setting advertisement fields failed: %d", rc);
        return;
    }

    struct ble_gap_adv_params params = {0};
    params.conn_mode = BLE_GAP_CONN_MODE_NON;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &params, on_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "starting advertisement failed: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE advertising as '%s'", BLE_DEVICE_NAME);
}

static void on_host_sync(void)
{
    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "address inference failed: %d", rc);
        return;
    }

    /* Advertising is intentionally non-connectable; this name beacon exposes
     * no services and does not alter the existing Wi-Fi/CPX data paths. */
    start_advertising(own_addr_type);
}

static void nimble_host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_advertising_init(void)
{
    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
    nimble_port_init();
    ble_hs_cfg.sync_cb = on_host_sync;
    nimble_port_freertos_init(nimble_host_task);
}
