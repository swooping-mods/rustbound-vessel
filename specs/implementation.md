# Rustbound Vessel: behavior and implementation

Describes the retained payload **0.4.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.4.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.4.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Registers Rustbound Hover Vessel as saved vehicle ID 22, priced at $1,500 in the native garage. The provider supplies its fitted hull, visual model, tuning and game-owned thrust audio; Vehicle Framework owns common hover/flight control. Shift+F9 lifts/lands or boards an eligible nearby owned hover vehicle. W/S fly, A/D turn, Space/Ctrl climb/descend, Z/C strafe, Shift boosts and X brakes.

## State and lifecycle

Ownership and storage identity persist in native vehicle records. Per-instance visual/audio roots and body state are temporary. Shared hover code snapshots/restores constraints/gravity and gates local control by live owner/world/input; remote bodies do not gain a second local simulation. Provider teardown releases its owned presentation.

## Integration and configuration

Runtime dependencies: [vehicle-framework](https://github.com/swooping-mods/vehicle-framework).

The [module metadata](../source/mods/rustbound-vessel/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `vehicle-framework.vehicles` | 1 | at least 48 bytes |

## Implementation sources

- [mod.c](../source/mods/rustbound-vessel/mod.c)
- [hover_profile.h](../source/mods/rustbound-vessel/hover_profile.h)
- [hull_adapter.h](../source/mods/rustbound-vessel/hull_adapter.h)
- [visual.h](../source/mods/rustbound-vessel/visual.h)

## Verification boundary

Matching Vehicle Framework and saved definitions remain required. Private scenario shortcuts are not player controls. Hull collision, obstacles/slopes, reloading/recall and real co-op require matching native-game qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
