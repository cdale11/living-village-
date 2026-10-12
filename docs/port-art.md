# KNULLI Ports cover art

The illustration was created for this project using OpenAI image generation
on 2026-10-12 and is distributed under the project MIT license.

The archive includes original 600x800 cover art at
`/userdata/roms/ports/livingvillage/images/cover.png`.
The launcher registers `./LivingVillage.sh` and its `<image>` metadata in the
Ports `gamelist.xml` automatically. It does not overwrite a whole game list from
an archive, so existing Ports entries and ratings remain intact.

No registration command is required: extract the launcher and game folder,
then launch Living Village once. Metadata registration runs automatically.

EmulationStation may cache existing artwork until its next **Update Games Lists**
or restart. If the active frontend
writes an older cached gamelist back, run registration with EmulationStation
stopped and start it again. Theme support determines where box art is displayed;
select a game-list view with images enabled. Physical theme/display validation
is still required for this new artwork integration.

Registration is a native C++ command and needs neither Python nor SDL:

```sh
living-village --register-port-art /userdata/roms/ports
```

It scans bounded XML, preserves unrelated entries verbatim, updates only this
launcher's image field, preserves its existing name and other fields, and is
idempotent. Before a changed existing gamelist is replaced atomically and
flushed, the old contents are saved to `gamelist.xml.living-village.bak`.
A lock prevents simultaneous registrations. Malformed, ambiguous, oversized,
symlinked or unsupported XML is left unchanged, with an error in `logs/art.log`.
Artwork registration failure does not prevent gameplay. A crash before rename
can leave `gamelist.xml.living-village.tmp`; after checking the backup and with
registration stopped, remove that temporary file and retry.

All artwork and metadata stay on `/userdata`; system files are untouched.
