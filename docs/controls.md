# Controller mapping

The game reads SDL game controllers directly, including D-pad and trigger axes. It falls back to raw joystick hats, axes and configurable buttons when SDL does not recognize a device.

`config/controller.cfg` sets raw mappings with keys `confirm`, `back`, `inspect`, `overview`, `previous`, `next`, `zoom_out`, `zoom_in`, `menu` and `speed`. `hat`, `axis_x` and `axis_y` choose direction sources; `-1` disables one. Set `raw_mode=1` for an incorrect SDL mapping. Example raw indices are not claimed to match RG34XX SP.

`mapped_` keys change action-to-SDL-button assignments for recognized devices. D-pad/left-stick navigation and trigger-axis zoom stay fixed. A full SDL mapping database can be supplied as `config/gamecontrollerdb.txt`, or export `SDL_GAMECONTROLLERCONFIG` in `config/platform.env`.

Run the launcher with `--diagnose-input` in a session where EmulationStation has released the display. Logs report name, GUID, mapping mode and raw button/hat presses. Press each button, record its index, then update the config. [KNULLI checks](knulli.md) explain driver diagnostics. Logs flush as events arrive.

Save, load, resume, speed and exit all have controller paths. No touch or mouse input is required. Desktop keyboard equivalents are in the README.
