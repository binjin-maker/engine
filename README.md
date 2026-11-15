# Engine Garage

Build an engine from parts, bolt it to the dyno stand, fire it up, and rev it. The engine is simulated live with synthesized sound. Every part changes the physics, and bad combos fail the way they would in real life.

**Download:** [`bin/EngineGarage.exe`](bin/EngineGarage.exe). It's a single file with no installer, for Windows 10/11.

## What you can build
- **Cycle:** 4-stroke or 2-stroke
- **Fuel:** 87, 93, race gas, E85, methanol, diesel (compression ignition), nitromethane
- **Layout:** single, inline (2–8), V60/V90 (2–16), boxer (2–12), plus cylinder tilt (-90° to +90°)
- **Bore, stroke, compression ratio**
- **Materials:** cast iron / aluminum / CGI / magnesium / billet block; cast / hypereutectic / forged / billet-titanium internals; flywheel weight
- **Valvetrain:** flathead, OHV pushrod, SOHC, DOHC, 2–5 valves, 4 cams. 2-strokes: piston port / reed / rotary valve
- **Induction:** carb, TBI, multi-port EFI, direct injection, mechanical injection; 5 air filters; small/large/twin turbo or roots blower, boost, intercooler
- **Systems:** air or liquid cooling, muffler / sport / straight pipe / expansion chamber, recoil or electric start, alternator yes/no, oil grade (4T) or oil mix ratio (2T)

## What the sim models
Volumetric efficiency curves, pumping and friction losses, turbo spool and choke, knock vs. effective compression, cylinder pressure limits, piston speed, valve float, coolant/CHT temperature, oil pressure, battery drain and charging, cold starts and choke, and pull-cord compression limits.
Ways to blow it up: holed piston, thrown rod, dropped valve, blown head gasket, seizure, spun bearing, 2-stroke seizure from no oil, scored cylinders, turbo grenade.

## Controls (dyno cell)
| Key | Action |
|---|---|
| I | Ignition |
| E (hold) | Electric start / pull cord |
| W / Up / Space | Throttle |
| 1–0 / T | Lock throttle 10–100% / release |
| C | Choke (carbs) |
| L | Rev limiter |
| D | Dyno pull (WOT sweep, hp + lb-ft graph, previous run overlaid) |
| X | Cutaway view |
| G / Esc | Back to garage |
| M, +/- | Mute, volume |

Drag to orbit, wheel to zoom. Builds save to `engine_garage.sav` next to the exe. There are 12 challenges to unlock. Dev code: type `2323`.

## Building
`./build_windows.sh` cross-compiles the static exe with mingw-w64. `./build_linux.sh` builds a native Linux binary. Uses [raylib](https://github.com/raysan5/raylib) (zlib license). Fonts are DejaVu Sans (Bitstream Vera license).
