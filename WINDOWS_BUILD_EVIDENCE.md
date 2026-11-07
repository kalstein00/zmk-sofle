# Windows normal-mode build evidence, 2026-11-07

Source baseline: `cad147725279b0635a3d512ac99f6668bfec3704` on `dongle-support`. Candidate: baseline plus logical transform extraction and direct dependency pins. Candidate Git blob IDs: `eyelash_sofle.dtsi` f85d4bf524774ba9fea0826b4d3171fbde210fd0, `eyelash_sofle-logical.dtsi` 9be3cf2a8c6333f70288f3283ba1956946ef9d92, `config/west.yml` 284ac1823dd617f80a753ff59312bd7a95dc64c0. Keymap, board overlays and configuration were unchanged.

Windows native PowerShell, Python 3.12.10, west 1.5.0, CMake 3.27.9, Ninja 1.13.2, Zephyr SDK 0.16.8 ARM GCC 12.2.0. Commands are in [WINDOWS_BUILD.md](WINDOWS_BUILD.md); each clean build used `west build -p always`, separate baseline and final output directories, `nice_nano_v2`, and the listed shields. Left additionally used `studio-rpc-usb-uart`, `CONFIG_ZMK_STUDIO=y`, `CONFIG_ZMK_STUDIO_LOCKING=n`. Final builds and baseline right used `-DCMAKE_C_FLAGS=-ULV_CONF_PATH`; baseline left/reset did not need it. All six completed with exit 0 and generated new UF2 files:

| Target | Baseline and final size | SHA-256, both baseline and final | Flash / RAM |
| --- | ---: | --- | --- |
| left + nice_view | 819712 | 2630410b8588f50d2420fbca176e2a47535c27e7323c24fdfa2ce16af4f1a125 | 409644 / 102706 B |
| right + nice_view | 553984 | d8b24acd7792baf083bfba1ec1671582a5a189670af70ae23af43098aad7ee35 | 276932 / 63660 B |
| settings_reset | 92672 | 10524e682a4566f9b00fdfde372560fa334b23532f51a18b47b99fe1bd2946c1 | 46272 / 11560 B |

Raw logs and outputs remain under `C:\temp\zmk-issue2-baseline-left-clean.log`, `C:\temp\zmk-issue2-baseline-right-clean.log`, `C:\temp\zmk-issue2-baseline-reset.log`, `C:\temp\zmk-issue2-final-{left,right,reset}.log` and `C:\temp\zmk-issue2-build-{baseline,final}-{left,right,reset}\zephyr`. Logs show the board from ZMK, local left/right overlays and keymap, and ZMK nice_view and settings_reset overlays. The left configuration has `CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y`; the right is a split Peripheral. Compared final and baseline `.config` values match for all three. After ignoring comments and whitespace, DTS differences are solely the unchanged transform node moving in declaration order; reset DTS is identical. Both UF2 hashes also match; neither fact verifies hardware behavior.

Actual active dependency commits are recorded in [requirements/west-active-lock.yml](requirements/west-active-lock.yml), generated with `west manifest --freeze --active-only`. ZMK is `4493783ef88ce2e653bf8217c92ee17140df71e3`, Zephyr `dacab4875df72109b96cc8977547a0dc04875bcd`, and nanopb `8c60555d6277a0360c876bd85d491fc4fb0cd74a`. After builds, Zephyr and nanopb were explicitly pinned in the manifest; a subsequent `west update` and active manifest comparison confirmed all 45 project names, URLs, paths, and commits unchanged (only Zephyr clone depth metadata differed). This metadata change does not change the source or generated build inputs. Runtime regression is **not verified**: two keyboards and DYA were unavailable. Follow the physical-device checklist in WINDOWS_BUILD.md.

Initial failed attempts: left setup failed on argument quoting and package discovery; after correction the nanopb generator lacked `pkg_resources`. Installing the recorded setuptools and protobuf tooling resolved it. A build configured while `west update` was still running lacked tinycrypt include paths; clean configuration after update resolved it. Baseline right initially failed because keycode macro `C` expanded the Windows drive letter in LVGL's config path; the documented `-ULV_CONF_PATH` selects the same header via the existing include path. These failures are not counted as successful builds.

## Fresh workspace rerun after review

At source commit `6c1f8e507814d56f8a9c361f26ca5a05d0de9c58`, the documented setup was repeated in a fresh `C:\temp\zmk-normal` workspace and venv. The SDK root alone was changed to `C:\temp\zmk-normal-sdk` to avoid overwriting an existing SDK. Package install, downloads, extraction, `west init`, and `west update` exited 0. The first left build exited 1 because PowerShell's repository path contained backslashes that CMake parsed as escapes (`C:\temp\zmk-normal-left.log`). The guide now normalizes `$repo` to forward slashes; from the same newly initialized workspace all three documented clean builds then exited 0 (`C:\temp\zmk-normal-resume.log`).

| Target | Log | New UF2 SHA-256 | Size | Flash / RAM |
| --- | --- | --- | ---: | --- |
| left | `C:\temp\zmk-normal-left-fixed.log` | e82ff12fb5f1038c5b9a8422551859989197bf1705754e326bdd18e51b6f84e8 | 819712 B | 409664 / 102706 B |
| right | `C:\temp\zmk-normal-right.log` | 88f3b0b602b2e6a30d7e1523e4a7003db59a0e0bac03937bf513ae2bca3d2629 | 553984 B | 276952 / 63660 B |
| reset | `C:\temp\zmk-normal-reset.log` | 283c873fff1d0ee7a6f1d4f72c33bd468db4e5ead68f0224942eb15f34ebc5b0 | 92672 B | 46292 / 11560 B |

The generated UF2 files are under `C:\temp\zmk-normal\build-{left,right,reset}\zephyr\zmk.uf2`. All 45 actual active project URLs, paths and revisions matched the lock snapshot. The logs show the intended local left/right shield, ZMK nice_view and settings_reset, and the nice_nano_v2 board. Left has split Central, Studio and display enabled; right has split and display enabled and no Central role. Compared with the previous final candidate, the three Kconfig assignment sets match and DTS files match after ignoring comments and whitespace. UF2 hashes differ across these separate workspaces; their Zephyr build version strings also differ, so the earlier same-workspace baseline/final byte comparison must not be generalized to fresh clones. Physical operation remains unverified.
