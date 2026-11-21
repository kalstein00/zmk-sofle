"""Adapt the pinned settings and Runtime Combo modules to the Sofle dongle."""

import hashlib
from pathlib import Path
import sys


def replace(source, old, new):
    if source.count(old) != 1:
        raise SystemExit(f"Unexpected Runtime Combo source at {old[:100]!r}")
    return source.replace(old, new)


def fix_settings(source):
    # Backport cormoran/zmk-feature-custom-settings f30a1b6. ZMK's settings-table
    # IDs may not be loaded when Zephyr calls a custom setting's load handler.
    source = replace(
        source,
        "const struct zmk_custom_setting_behavior_value *behavior) {\n    if (behavior->behavior_id",
        "const struct zmk_custom_setting_behavior_value *behavior, bool resolve_behavior) {\n"
        "    if (behavior->behavior_id",
    )
    source = replace(
        source,
        "#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)\n    const char *behavior_dev_name",
        "    if (!resolve_behavior) {\n        return 0;\n    }\n\n"
        "#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)\n    const char *behavior_dev_name",
    )
    source = replace(
        source,
        "const struct zmk_custom_setting_value *value) {\n    if (setting->value_type != value->type)",
        "const struct zmk_custom_setting_value *value, bool resolve_behaviors) {\n"
        "    if (setting->value_type != value->type)",
    )
    source = replace(
        source,
        "validate_behavior_value(&value->behavior_value)",
        "validate_behavior_value(&value->behavior_value, resolve_behaviors)",
    )
    source = replace(
        source,
        "static int validate_behavior_id_constraint(const struct zmk_custom_setting_value *value) {",
        "static int validate_behavior_id_constraint(const struct zmk_custom_setting_value *value,\n"
        "                                           bool resolve_behavior) {",
    )
    source = replace(
        source,
        "#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)\n    return zmk_behavior_find_behavior_name",
        "    if (!resolve_behavior) {\n        return 0;\n    }\n\n"
        "#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LOCAL_IDS)\n    return zmk_behavior_find_behavior_name",
    )
    source = replace(
        source,
        "int zmk_custom_setting_validate(const struct zmk_custom_setting *setting,\n"
        "                                const struct zmk_custom_setting_value *value) {",
        "static int validate_value(const struct zmk_custom_setting *setting,\n"
        "                          const struct zmk_custom_setting_value *value, bool resolve_behaviors) {",
    )
    source = replace(source, "value_type_validate(setting, value)",
                     "value_type_validate(setting, value, resolve_behaviors)")
    source = replace(source, "validate_behavior_id_constraint(value)",
                     "validate_behavior_id_constraint(value, resolve_behaviors)")
    source = replace(
        source,
        "int zmk_custom_setting_set_default(",
        """int zmk_custom_setting_validate(const struct zmk_custom_setting *setting,
                                const struct zmk_custom_setting_value *value) {
    return validate_value(setting, value, true);
}

int zmk_custom_setting_set_default(""",
    )
    return replace(
        source,
        "int ret = zmk_custom_setting_validate(setting, &value);\n    if (ret < 0)",
        "/* Validate stored types and ranges before the behavior ID table is available. */\n"
        "    int ret = validate_value(setting, &value, false);\n    if (ret < 0)",
    )


def fix_delete(source):
    source = replace(
        source,
        """            *combo = def->config;
            return 0;
        }
    }
    ret = packed_to_combo(&value, combo);""",
        """            *combo = def->config;
            return 0;
        }
        return -ENOENT;
    }
    ret = packed_to_combo(&value, combo);""",
    )
    source = replace(
        source,
        "int zmk_runtime_combo_delete(uint32_t index, bool persist) {",
        "static int runtime_combo_clear(uint32_t index, bool persist);\n\n"
        "int zmk_runtime_combo_delete(uint32_t index, bool persist) {",
    )
    source = replace(
        source,
        "    combo.enabled = false;\n    return zmk_runtime_combo_write(index, &combo, persist);",
        """    if (!zmk_runtime_combo_has_default(index)) {
        return runtime_combo_clear(index, persist);
    }
    combo.enabled = false;
    return zmk_runtime_combo_write(index, &combo, persist);""",
    )
    source = replace(
        source,
        "int zmk_runtime_combo_reset(uint32_t index) {",
        "static int runtime_combo_clear(uint32_t index, bool persist) {",
    )
    source = replace(
        source,
        """    int ret = zmk_custom_setting_write_array_element(
        setting, &RUNTIME_COMBO_EMPTY_BYTES, array_size, ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);

    const struct zmk_custom_setting *name = name_setting(index);
    if (name) {
        uint32_t name_array_size = MAX(zmk_custom_setting_array_size(name), index + 1);
        zmk_custom_setting_write_array_element(name, &ZMK_CUSTOM_SETTING_VALUE_STRING(""),
                                               name_array_size,
                                               ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);
    }
    /* The in-memory value updates even if persisting to flash failed (see
     * zmk_runtime_combo_write()), so the cache must refresh regardless of
     * the write's outcome. */
    runtime_combo_cache_rebuild_slot(index);
    return ret;
}""",
        """    enum zmk_custom_setting_write_mode mode =
        persist ? ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST : ZMK_CUSTOM_SETTING_WRITE_MODE_MEMORY;
    int ret = zmk_custom_setting_write_array_element(
        setting, &RUNTIME_COMBO_EMPTY_BYTES, array_size, mode);
    runtime_combo_cache_rebuild_slot(index);
    if (ret < 0) {
        return ret;
    }

    return zmk_runtime_combo_write_name(index, "", persist);
}

int zmk_runtime_combo_reset(uint32_t index) {
    return runtime_combo_clear(index, true);
}""",
    )
    # An active chord must release the binding it pressed even if Studio changes its slot.
    source = replace(
        source,
        "struct runtime_combo_active {\n    uint16_t combo_idx;",
        """struct runtime_combo_active {
    uint16_t combo_idx;
    struct zmk_behavior_binding behavior;
    bool slow_release;
    bool released;""",
    )
    source = replace(
        source,
        "static int release_combo_behavior(uint32_t combo_idx, const struct zmk_runtime_combo_config *combo,",
        "static int release_combo_behavior(uint32_t combo_idx, const struct zmk_behavior_binding *behavior,",
    )
    source = replace(
        source, "zmk_behavior_invoke_binding(&combo->behavior, event, false)",
        "zmk_behavior_invoke_binding(behavior, event, false)",
    )
    source = replace(
        source,
        "static struct runtime_combo_active *store_active_combo(uint32_t combo_idx, uint8_t count) {",
        """static struct runtime_combo_active *store_active_combo(
    uint32_t combo_idx, uint8_t count, const struct zmk_runtime_combo_config *combo) {""",
    )
    source = replace(
        source,
        "*active = (struct runtime_combo_active){.combo_idx = combo_idx};",
        """struct zmk_runtime_combo_global_settings global = runtime_combo_cached_global_settings();
    *active = (struct runtime_combo_active){
        .combo_idx = combo_idx,
        .behavior = combo->behavior,
        .slow_release = effective_slow_release(combo, &global),
    };""",
    )
    source = replace(source, "store_active_combo(combo_idx, pending_count)",
                     "store_active_combo(combo_idx, pending_count, combo)")
    source = replace(source, "store_active_combo(combo_idx, combo_len)",
                     "store_active_combo(combo_idx, combo_len, combo)")
    source = replace(
        source,
        """        struct zmk_runtime_combo_config combo;
        if (runtime_combo_cache_read(active->combo_idx, &combo) < 0) {
            deactivate_combo(active_idx);
            return true;
        }

        bool key_released = false;
        bool all_keys_pressed = active->key_positions_pressed_count == combo.key_position_len;""",
        """        bool key_released = false;""",
    )
    source = replace(
        source,
        """        struct zmk_runtime_combo_global_settings global = runtime_combo_cached_global_settings();
        bool slow_release = effective_slow_release(&combo, &global);
        if ((slow_release && all_keys_released) || (!slow_release && all_keys_pressed)) {
            release_combo_behavior(active->combo_idx, &combo, timestamp);
        }""",
        """        if (!active->released && (!active->slow_release || all_keys_released)) {
            active->released = true;
            release_combo_behavior(active->combo_idx, &active->behavior, timestamp);
        }""",
    )
    source = replace(
        source, "static int32_t fully_pressed_combo_idx = -1;",
        """static int32_t fully_pressed_combo_idx = -1;
static struct zmk_runtime_combo_config fully_pressed_combo;""",
    )
    source = replace(
        source, "            fully_pressed_combo_idx = complete_combo;",
        """            fully_pressed_combo = combo;
            fully_pressed_combo_idx = complete_combo;""",
    )
    return replace(
        source,
        """    if (runtime_combo_cache_read(combo_idx, &combo) < 0 || combo.key_position_len > pending_count) {
        return false;
    }""",
        """    const struct zmk_runtime_combo_config *matched = &fully_pressed_combo;
    if (runtime_combo_cache_read(combo_idx, &combo) < 0 || !combo.enabled ||
        combo.key_position_len > pending_count || combo.key_position_len != matched->key_position_len ||
        combo.layer_mask != matched->layer_mask || combo.timeout_ms != matched->timeout_ms ||
        combo.require_prior_idle_ms != matched->require_prior_idle_ms ||
        combo.slow_release_override != matched->slow_release_override ||
        combo.behavior.behavior_dev != matched->behavior.behavior_dev ||
        combo.behavior.param1 != matched->behavior.param1 ||
        combo.behavior.param2 != matched->behavior.param2 ||
        memcmp(combo.key_positions, matched->key_positions,
               combo.key_position_len * sizeof(combo.key_positions[0])) != 0) {
        // Replay the buffered keys if the matched slot changed before it fired.
        return false;
    }""",
    )


PATCHES = {
    "custom_settings": (
        "62f60e7265715fc0b04acbe11358ca8597db497e0556a87c5922e6edf37bcb06", fix_settings
    ),
    "runtime_combo": (
        "767107190f0b3d8da1e3a4f0236a5506131a211ad7974b17002d958e40e8efda", fix_delete
    ),
}

name, source_path, output_path = sys.argv[1:]
source = Path(source_path).read_text(encoding="utf-8")
digest, patch = PATCHES[name]
if hashlib.sha256(source.encode("utf-8")).hexdigest() != digest:
    raise SystemExit(f"Unexpected {name} source; review the Runtime Combo compatibility changes")
Path(output_path).write_text(patch(source), encoding="utf-8")
