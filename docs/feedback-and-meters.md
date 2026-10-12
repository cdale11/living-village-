# Audio, vibration and resource meters

## Sound and vibration

Use **Y → Sound / Vibration**. **A** changes saved volume, **R1** toggles saved
vibration, **L1** cycles available audio outputs, and **X** tests the output with a brief 40%-volume tone and a stronger
250 ms motor request. The test does not change your saved settings. Listen and
feel to confirm actual hardware output: software can report a queued buffer or
accepted effect, but cannot confirm speaker/motor operation.

The chosen audio output persists globally in `config/audio-device.txt`; other
volume/vibration settings remain per village. Keep this file when upgrading.

The panel displays the current audio backend, sample rate and channels, and
rumble request status. Startup also plays a short tone and requests a short
pulse when enabled. Confirm/back interactions and major construction milestones
have vibration cues. Original quiet bird, rain and fire sounds accompany running
simulation; other original tones mark village events. Paused menus do not enqueue
new ambient sound.

Audio prefers 48 kHz stereo, negotiates the device's actual format and uses SDL's
audio stream to convert original PCM. It tries the default output and available
named devices. If opening fails, it retries ALSA within the game process.
No system sound configuration or volume is changed. A muted system, wrong
system output or unavailable driver still requires a device-side correction.

Vibration tries mapped SDL controller rumble, raw joystick rumble, SDL haptics,
then Linux evdev `FF_RUMBLE`. The final fallback scans at most 64 event devices,
checks force-feedback capability, uploads one bounded effect, triggers it and
removes it on exit. It never writes system partitions or arbitrary sysfs paths.
Motor effects cap at two seconds. Devices without force-feedback support or
permission to access their event node show an unavailable status and keep playing.

Useful evidence from the handheld:

```sh
grep -E 'Audio|Rumble' /userdata/roms/ports/livingvillage/logs/game.log
cat /proc/bus/input/devices
```

Desktop tests cover real SDL dummy audio negotiation/conversion/queueing and
procedural PCM. An injected simulated kernel interface verifies Linux device discovery, effect
parameters, upload, playback, stop, removal and descriptor cleanup. Physical
speaker and motor validation remains necessary.

## Small CPU/RAM meters

The right side of the top resource bar shows **CPU** and **RAM**. Sampling happens
once per real second, including while simulation is paused. CPU measures game
process CPU time (user + kernel), with 100% representing one fully occupied CPU
core; multithreaded usage can exceed 100%, while its small bar saturates at 100%.
RAM is current process resident memory in MiB, read from `/proc/self/statm`, not
peak RSS or total system usage. Its bar represents a 256 MiB game budget.
`--` means the first CPU interval is still pending or measurement is unavailable.
The meters are display telemetry and do not affect simulation or save files.
