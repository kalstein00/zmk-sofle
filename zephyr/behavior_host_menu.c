#define DT_DRV_COMPAT zmk_behavior_host_menu
#include <drivers/behavior.h>
#include "dongle_menu.h"

static int pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    return dongle_menu_request();
}

static int released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api api = {
    .binding_pressed = pressed,
    .binding_released = released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
