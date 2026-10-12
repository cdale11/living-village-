# Controller mapping

The game reads SDL game controllers directly, including D-pad and trigger axes. It falls back to raw joystick hats, axes and configurable buttons when SDL does not recognize a device.

`config/controller.cfg` sets raw mappings with keys `confirm`, `back`, `inspect`, `overview`, `previous`, `next`, `zoom_out`, `zoom_in`, `menu` and `speed`. `hat`, `axis_x` and `axis_y` choose direction sources; `-1` disables one. Set `raw_mode=1` for an incorrect SDL mapping. Example raw indices are not claimed to match RG34XX SP.

`mapped_` keys change action-to-SDL-button assignments for recognized devices. D-pad/left-stick navigation and trigger-axis zoom stay fixed. A full SDL mapping database can be supplied as `config/gamecontrollerdb.txt`, or export `SDL_GAMECONTROLLERCONFIG` in `config/platform.env`.

Run the launcher with `--diagnose-input` in a session where EmulationStation has released the display. Logs report name, GUID, mapping mode and raw button/hat presses. Press each button, record its index, then update the config. [KNULLI checks](knulli.md) explain driver diagnostics. Logs flush as events arrive.

Save, load, resume, speed and exit all have controller paths. No touch or mouse input is required. Desktop keyboard equivalents are in the README.

A on the map examines the tile under the reticle. A on reachable empty grass
opens the six construction recipes; D-pad up/down selects, A places the plan,
and B cancels. A on a tree/bush/stone toggles a gathering designation. A on an
existing building panel shows its costs, progress or completed gameplay effect.
In the villager inspector, left/right cycles six pages: needs/reason, skills
and personality, relationships and recent experiences, household and family, personal neural learning, replay and practiced work. Y opens the overview,
where left/right selects village, population history, neural learning, economy/history exports, or sound/vibration. A on village toggles
autonomous new planning; A on population toggles births and arrivals;
A on neural learning freezes/resumes model training. L1/R1 changes the inspected villager.

## Fn+Start exit

Hold Fn and press Start to save and return to EmulationStation from any panel.
Start alone still pauses/resumes. The chord works in either press order and
does not also toggle the pause panel. It uses the normal save-and-exit path;
if saving fails, the game stays open and reports the error.

Recognized controllers use SDL Guide+Start. For a device named
`Anbernic RG34XX-SP Controller` or `Anbernic RG34XX Controller`, the game also
checks raw Fn button 11 plus Start button 10, based on KNULLI's upstream
[controller profile](https://github.com/knulli-cfw/distribution/blob/knulli-main/package/knulli/knulli-emulationstation/controllers/es_input.cfg).
This covers an Fn button that SDL's mapping omits.

Set `hotkey=N` to override the raw Fn button, and `hotkey_start=N` to override
the raw Start button. Defaults of `-1` choose the automatic profile; for other
raw controllers, Start defaults to the configured `menu` button.
`mapped_hotkey=5` selects SDL Guide and can be remapped separately. Existing
configs work without adding these keys. Desktop F10+Space tests the same
save-and-exit request. Use `--diagnose-input` if physical Fn is not recognized.

Start → New village opens name/seed/biome/layout selection. D-pad selects fields;
A edits or confirms, left/right cycles options, X chooses a random seed, B cancels.
In seed/name editors, left/right moves the cursor, up/down changes characters,
and A accepts. Seed supports 0–4294967295 without a keyboard.

Start → Manage saved worlds: up/down selects, A loads, Y renames, X requests
confirmed deletion, and L1 snapshots the open world. The open save cannot be
deleted until another world is opened. Old current/previous saves remain listed.
See [world-management.md](world-management.md) for file layout and behavior.

On Y's economy/history page, A exports CSV files to `saves/exports`.
On Y's sound/vibration page, A cycles volume (including mute) and R1 toggles
optional SDL rumble. These preferences persist with the village.

The overview also has Ecology/weather and Wildlife pages after sound/vibration.
On Wildlife, L1/R1 selects an animal; the page shows actual biology, activity,
motivation and neural learning. A tile inspection shows soil moisture/nutrients,
plant health/age, surface flood water and traffic compaction. Buildings show
condition, delivered maintenance materials and repair work. No extra keyboard
or mouse actions are required. See [ecology](ecology.md).
