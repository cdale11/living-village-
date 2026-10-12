# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 game, Ports launcher,
internal original cover art, configuration, documentation and diagnostics.
Built from source milestone **0e7cddb** using Zig 0.13.0 for Cortex-A53 / AArch64
Linux with a glibc 2.17 baseline. No Python, external image codec, model download
or internet service is required on the handheld.

Download with GitHub's **Download raw file**, extract into `/userdata/roms/ports/`,
preserving the whole `livingvillage/saves/` folder and customized `config/`, and
launch **Ports → LivingVillage**. Replace the launcher, binary, images, scripts
and documentation when upgrading. Keep `config/audio-device.txt` if customized.

The archive has only the launcher and `livingvillage/` at its root. Cover art is
inside `livingvillage/images/cover.png`. Registration runs automatically on launch;
**no manual register_art command is required**. Existing other Ports entries are
preserved and their gamelist is backed up before a changed atomic write.
EmulationStation may retain cached images until its game-list refresh or restart.
The previous external cover file is no longer used; no other Ports images are
removed automatically.

## Added in this update

- **2–20 starting adults**, selectable in **Start → New village**, with independent
  identities, personalities, neural models and persistent world state.
- Larger original pixel sprites for people, trees, homes and all wildlife;
  cached textures, weather/season colors, selected routes, work tools, social
  links, clearer menus and a live ribbon of actual simulation events.
- Nearby wildlife in new worlds and **Y → Wildlife → A** to follow an animal.
  L1/R1 selects animals; D-pad releases the camera. Existing saves retain their
  actual wildlife. Suitable boundary habitats permit conditional regional arrivals.
- Resource discoveries spread through real social contact and cooperation/trust.
  Newly learned locations can change goals; no materials are teleported or created.
- Negotiated/converting audio, original ambient bird/rain/fire cues, startup and
  event tones, controller audio-output selection and a saved device preference.
- Stronger feedback, mapped/raw SDL rumble, initialized SDL haptics, and a bounded
  Linux `FF_RUMBLE` fallback for supported event devices.
- **Y → Sound / Vibration:** A volume, R1 vibration, **X tests sound and motor**,
  **L1 selects output**. Backend, format/output and request status are shown.
- Small header **CPU/RAM meters**, sampled once per second: game CPU time with
  one core at 100%, and current process resident memory in MiB.

Eight biomes, twelve layouts, three food budgets, live neural diagnostics,
persistent named worlds, households, agriculture, society, biology/hydrology,
weathering and maintenance remain available. See [feedback/meters](../docs/feedback-and-meters.md),
[ecology](../docs/ecology.md) and [world management](../docs/world-management.md).

## Verification

- Full native regression suite, SDL rendering/audio, virtual controller/Fn+Start,
  first-run/launcher/world smoke checks and save migration/continuation pass.
- 192 seed/biome/layout cases over ten days: 6–7 villagers surviving.
- 3,500-day generational test: 20 residents, 5 births, 26 arrivals, 17 deaths,
  2,108 repairs and replacement of founders, with bounded histories and valid saves.
- Population/identities/saves, visible wildlife/following, habitat-gated recruitment,
  nearby knowledge exchange, real audio conversion/output selection and meters.
- GCC and SDK libc++ tests for new gameplay, feedback and Linux FF; simulated
  kernel effect upload/start/stop/removal/cleanup and SDL haptic lifecycle tests.
- Address/undefined-behavior sanitizer checks for feedback, FF and texture rendering.
- Short desktop software-rendered benchmark with 20 residents: **30.22 FPS** at
  the 30 FPS cap, 5.79 ms mean frame work, 7.58 ms recent p95, 10,736 KiB peak RSS.
  These are desktop measurements, not handheld performance claims.
- Packaged binary/art matching, ARM64 ELF/glibc baseline and archive checksum.

The provided handheld log confirms ALSA and controller force-feedback capability;
the earlier game loads and Fn+Start exits. **Audible output, felt vibration, new
screens, artwork caching and performance in this update still need physical
confirmation.** Software reports queued audio/effects, not proof of speaker or
motor operation. The output test helps distinguish those cases.

```sh
sha256sum -c packages/SHA256SUMS
```

Version 7 saves and versions 1–6 migration remain supported. Back up saves and
configuration before upgrades. Expanded profiles require a compatible new build.
