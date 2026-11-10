---
name: sofle-release
description: Prepare or publish a GitHub firmware release for zmk-sofle with the matched dongle, left keyboard, and right keyboard UF2 files. Use whenever a firmware release or release asset set is requested in this repository.
---

# Sofle firmware release

Release a matched **dongle-mode set of three** UF2 files from one source revision. A request to prepare a release authorizes preparation; publish or change an existing release only when the user requests that action. Follow an explicit user instruction if it changes the release scope.

1. Identify the release repository, version, target commit, and applicable changes. Reuse the existing version convention; if none exists, choose a suitable initial version. Make sure the three artifacts will be built from that exact commit, including its pinned dependencies. Do not include unrelated working-tree changes; if they affect build inputs, use a clean checkout of the intended commit or report the boundary.
2. Build the three `nice_nano_v2` dongle-mode targets defined in `build.yaml`: `sofle_dongle`, `eyelash_sofle_dongle_left nice_view`, and `eyelash_sofle_dongle_right nice_view`. Follow `DONGLE_BUILD.md` and `WINDOWS_BUILD.md` for environment, flags, and shield selection. Build each in a separate clean output directory and check each exit status and resulting overlay/target. Do not reuse a stale UF2 after a failed build.
3. Prepare three distinct, clearly named direct release assets: `sofle-dongle-nice-nano-<version>.uf2`, `sofle-dongle-left-<version>.uf2`, and `sofle-dongle-right-<version>.uf2`. Compute SHA-256 for each and include a `SHA256SUMS.txt` covering all three. Clearly identify each device, board target, dongle mode, source revision, installation guidance, and the scope of actual testing in the release notes. A settings-reset UF2 and normal-mode firmware do not substitute for any of these three.
4. Before publishing or updating a release, verify that all three files are fresh successful builds for the same target commit and that their hashes match the checksum file. If any one is missing, invalid, or built from a different revision, stop the release operation and report what failed.
5. When publication is authorized, create the release targeting the verified commit and upload all three UF2s and the checksum file. Verify the published tag resolves to that commit and each uploaded filename, size, and SHA-256 matches the local artifact. Report the release URL and any verification limitations; build success alone is not physical-device testing.
