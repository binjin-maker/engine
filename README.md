# Engine Garage

Build an engine from real, rated parts, bolt it to a dyno stand, fire it up and rev it. The engine is simulated live with synthesized sound that follows the actual firing order. Every part changes the physics, and bad combos fail the way they would in real life.

**Download:** [`bin/EngineGarage.ex_`](bin/EngineGarage.ex_). Rename it to `EngineGarage.exe` after downloading. It's a single file with no installer, for Windows 10/11.

## Building an engine
- **Core:** 4- or 2-stroke, 7 fuels, single / inline / V60 / V90 / boxer, 1-16 cylinders, bore, stroke, compression, cylinder tilt (-90 to +90 deg)
- **Materials:** cast iron, cast aluminum, CGI, magnesium, billet. Weak materials lose strength as castings get bigger, and some are unviable past a size. Each has its own heat and compression tolerance.
- **Rated parts catalog.** Every part has ratings and fitment:
  - Flywheels: small-engine magneto vs ring-gear, each with an rpm burst rating
  - Starters: recoil (small/medium/decompressor) and electric (0.4-4.5 kW), each rated by displacement and needing the right flywheel
  - Charging: lighting coil, stator, 70 A / 160 A alternators vs the engine's electrical load
  - Carbs sized in cfm, injectors in lb/hr (too small = lean = melted pistons)
  - Turbos and blowers with boost limits
  - Speed control: governor or rev limiter
  - Parts that don't fit block the build. Under-rated parts fail during the run (snapped recoil spring, burned-out starter, exploded flywheel, dead battery).
- **Oil:** splash dipper, wet sump or dry sump; oil grade, capacity and oil cooler. Tilted cylinders drain oil back slowly, so it pools in the heads and the sump starves. The 3D **Oil Flow view** shows oil moving from the pickup to the pump, galleries, bearings and heads and back.

## The sim
Calibrated against real engines (Predator 212, Honda GCV160, YZ250, EA888, LS3, Cummins 6.7, Lamborghini V12, VW 1600, Honda K20) to within about ±15%.
- **Breathing and power:** volumetric efficiency, small-cylinder heat loss, friction and pumping losses, turbo spool and choke, knock, cylinder pressure, reciprocating-inertia rev limits, breathing-limited redline
- **Temperatures and fluids:** coolant/CHT and oil temperature, oil pressure, oil level and consumption
- **Electrics:** battery load and charging
- **Running behaviour:** cold-start sputter and idle hunting, chokes, governors
- **Smoke:** scales with fuel burned and how dirty the fuel is
- **Costs:** materials + machining + bought parts give a one-off prototype cost, a per-unit cost at 10k units, and an estimated retail price

## Garage
Six tabs of parts, each with fit/rating status dots. View toggles: cutaway, heads, intake, exhaust, cooling, accessories, stand, oil flow. Click the engine name to rename it. Press **O** for the Engine Overview: costs, economy, suggested uses, full parts list. Parts never clip into each other: a layout solver packages everything, and a warning appears if a combo can't be packaged.

## Dyno controls
| Key | Action |
|---|---|
| I | Ignition |
| E (hold) | Electric start / pull cord |
| W / Up / Space | Throttle |
| 1-0 / T | Lock throttle 10-100% / release |
| C | Choke |
| L | Governor / rev limiter on-off |
| D | Dyno pull |
| X | Cutaway |
| V | Oil flow view |
| G / Esc | Back to garage |
| M, N, +/- | Mute, music, volume |

Drag to orbit, wheel to zoom. Builds save to `engine_garage.sav` next to the exe. There are 12 challenges to unlock. Dev code: type `2323`.

## Building from source
`./build_windows.sh` cross-compiles the static exe with mingw-w64 (output: `bin/EngineGarage.ex_`). `./build_linux.sh` builds natively. `tools/calib.c` checks the physics against real engines. Uses [raylib](https://github.com/raysan5/raylib) (zlib). Fonts are DejaVu Sans.
