/* The pinned ZMK fork shares ZMK_BLE between split scanning and host HID.
 * A dongle central only scans for peripherals; it must never advertise a host HID.
 */
#include <errno.h>
#include <zephyr/bluetooth/bluetooth.h>

int __wrap_bt_le_adv_start(const struct bt_le_adv_param *param,
                           const struct bt_data *ad, size_t ad_len,
                           const struct bt_data *sd, size_t sd_len)
{
    ARG_UNUSED(param);
    ARG_UNUSED(ad);
    ARG_UNUSED(ad_len);
    ARG_UNUSED(sd);
    ARG_UNUSED(sd_len);
    return -ENOTSUP;
}
