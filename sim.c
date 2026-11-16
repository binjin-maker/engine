// ===== parts data, spec derivation, physics =====
typedef struct { const char *name; float knock, energy; int diesel; float cold; const char *tip; } FuelDef;
static const FuelDef FUELS[] = {
 {"Gasoline 87",  11.0f, 1.00f, 0, 1.00f, "Cheap pump gas. Knocks above ~11:1 effective compression."},
 {"Gasoline 93",  12.2f, 1.01f, 0, 1.00f, "Premium pump gas. A bit more knock resistance."},
 {"Race Gas 110", 14.5f, 1.04f, 0, 0.90f, "Leaded race fuel. Lets you run big compression and boost."},
 {"E85",          15.0f, 1.07f, 0, 0.60f, "Ethanol blend. Huge knock resistance, more power. Hard cold starts."},
 {"Methanol",     16.0f, 1.13f, 0, 0.45f, "Alcohol race fuel. Runs cool, makes power. Very hard to cold start."},
 {"Diesel",       99.0f, 1.00f, 1, 1.00f, "Compression ignition: needs 14:1+ compression and injection. Big torque, low rpm."},
 {"Nitromethane",  9.5f, 1.85f, 0, 0.65f, "Carries its own oxygen. Insane power, insane cylinder pressure. Knocks easily."},
};
#define NFUEL 7
static const char *LAYOUTS[] = {"Single","Inline","V 60","V 90","Boxer","Custom"};
static const float LAYOUT_HALF[] = {0,0,30,45,90,0};
static const int CYLOPT[6][8] = {{1},{2,3,4,5,6,8},{2,4,6,8,10,12,16},{2,4,6,8,10,12,16},{2,4,6,8,12},{1}};
static const int CYLOPTN[6] = {1,6,7,7,5,1};
#define NLAYOUT 6
static const float IMBAL[17] = {0,1.0f,0.7f,0.5f,0.35f,0.25f,0.08f,0,0.15f,0,0.12f,0,0.04f,0,0,0,0.04f};

typedef struct { const char *name; float weight, str, cond, cost; Color col; const char *tip; float maxL, sizeK, heat; } BlockDef;
static const BlockDef BLOCKS[] = {
 {"Cast Iron",      1.00f, 1.00f, 0.80f,  320, {78,80,86,255},    "Heavy and tough. Cheap. Holds heat. Scales to any size.", 40, 0.12f, 0},
 {"Cast Aluminum",  0.55f, 0.85f, 1.30f,  220, {176,180,186,255}, "Light, sheds heat well. Loses strength as engines get big; softens when hot.", 14, 0.22f, -6},
 {"CGI Iron",       0.90f, 1.40f, 0.75f, 1400, {62,68,84,255},    "Compacted graphite iron. Diesel-grade strength that holds up at any size.", 40, 0.02f, 8},
 {"Magnesium",      0.40f, 0.65f, 1.20f, 1800, {182,172,138,255}, "Featherweight and fragile. Creeps when hot. Unviable past ~2.5 L.", 2.5f, 0.38f, -28},
 {"Billet Aluminum",0.60f, 1.30f, 1.35f, 6000, {218,222,230,255}, "Machined from solid. Strong but huge billets get pricey and limited (~10 L).", 10, 0.06f, -2},
};
#define NBLOCK 5
typedef struct { const char *name; float ps, str, mass, cost; const char *tip; } IntDef;
static const IntDef INTERN[] = {
 {"Cast",            19.0f, 0.85f, 1.00f,  150, "Stock cast pistons and rods. Max ~19 m/s piston speed."},
 {"Hypereutectic",   21.0f, 0.95f, 0.95f,  300, "High-silicon cast pistons. ~21 m/s."},
 {"Forged",          25.0f, 1.25f, 0.90f, 1200, "Forged pistons, H-beam rods. ~25 m/s, takes boost."},
 {"Billet / Titanium",28.5f,1.55f, 0.70f, 4500, "Billet crank, titanium rods. ~28.5 m/s. Screams."},
};
#define NINT 4
typedef struct { const char *name; float mul, maxRpm, minL, maxL; int magneto, ring; float cost; const char *tip; } FlyDef;
static const FlyDef FLY[] = {
 {"Die-cast small-engine",  1.5f,  5200, 0.0f, 1.0f, 1, 0,  40, "Stock mower/kart flywheel with magneto. Cast aluminum: bursts past ~5200 rpm. Keep the governor."},
 {"Billet small-engine",    1.0f, 12000, 0.0f, 1.0f, 1, 0, 160, "Billet aluminum with magneto, rated 12,000 rpm. The go-kart upgrade."},
 {"Cast iron, ring gear",   1.0f,  7000, 0.6f, 99,   0, 1, 120, "Car flywheel with starter ring gear. Rated 7000 rpm. No magneto: ignition needs a battery."},
 {"Light steel, ring gear", 0.6f,  9500, 0.6f, 99,   0, 1, 350, "Lightweight billet steel, ring gear, rated 9500 rpm."},
 {"Aluminum race, ring gear",0.45f,12500,0.6f, 99,   0, 1, 900, "Race flywheel, steel insert, rated 12,500 rpm."},
 {"Heavy dual-mass",        1.9f,  5000, 1.5f, 99,   0, 1, 450, "Big, smooth, diesel/truck flywheel. Rated 5000 rpm. Needs 1.5 L+."},
};
#define NFLY 6
typedef struct { const char *name; int elec; float minL, maxL, power; int decomp, ring; float cost; const char *tip; } StarterDef;
static const StarterDef STARTERS[] = {
 {"Recoil, small",          0, 0,    0.20f, 9,    0, 0,  30, "Pull cord rated to 200 cc. Bigger engines snap the spring."},
 {"Recoil, medium",         0, 0,    0.45f, 12,   0, 0,  45, "Pull cord rated to 450 cc."},
 {"Recoil + decompressor",  0, 0,    1.00f, 13,   1, 0,  90, "Auto-decompressor bleeds compression while cranking. Rated to 1.0 L."},
 {"Small electric 0.4 kW",  1, 0,    0.75f, 0.4f, 0, 0, 120, "Engages the small-engine flywheel. Rated to 750 cc. Needs a small-engine flywheel."},
 {"Auto starter 1.2 kW",    1, 0.8f, 4.5f,  1.2f, 0, 1, 160, "Car starter for 0.8-4.5 L. Needs a ring-gear flywheel."},
 {"Gear-reduction 2.0 kW",  1, 2.0f, 8.0f,  2.0f, 0, 1, 260, "High-torque starter for 2-8 L. Needs a ring-gear flywheel."},
 {"Heavy-duty 4.5 kW",      1, 4.0f, 16.f,  4.5f, 0, 1, 480, "Truck/diesel starter for 4-16 L. Needs a ring-gear flywheel."},
 {"Industrial 9 kW",        1, 8.0f, 25.f,  9.0f, 0, 1, 1100, "Marine/industrial starter for 8-25 L. Needs a ring-gear flywheel."},
 {"Air starter 20 kW",      1, 15.f, 40.f, 20.0f, 0, 1, 2600, "Compressed-air turbine starter for 15-40 L monsters. Needs a ring-gear flywheel."},
};
#define NSTART 9
typedef struct { const char *name; float amps, minL; int stator; float cost; const char *tip; } AltDef;
static const AltDef ALTS[] = {
 {"None",                  0,   0,    0,   0, "No charging. Magneto still makes spark on small-engine flywheels."},
 {"Lighting coil 3 A",     3,   0,    1,  25, "Tiny coil under the flywheel. Barely runs a light. Needs a magneto flywheel."},
 {"Charging stator 15 A",  15,  0,    1,  90, "Motorcycle-style stator + regulator. Needs a magneto flywheel."},
 {"Alternator 70 A",       70,  0.6f, 0, 160, "Belt-driven car alternator. Needs 0.6 L+ to mount."},
 {"High-output 160 A",     160, 1.0f, 0, 320, "Big alternator for heavy electrical loads (DI, fans, pumps)."},
};
#define NALT 5
typedef struct { const char *name; float cfm, cost; } CarbDef;
static const CarbDef CARBS[] = {
 {"16 mm small-engine", 10, 25}, {"Mikuni VM22", 20, 60}, {"Mikuni VM34", 85, 120}, {"Solex 34 PICT", 140, 150}, {"Dual Weber 40 DCOE", 250, 700},
 {"Holley 390 cfm", 390, 350}, {"Holley 600 cfm", 600, 450}, {"Holley 850 cfm", 850, 600}, {"Dual-quad 2x850", 1700, 1400},
};
#define NCARB 9
typedef struct { const char *name; float lbhr, cost; } InjDef;
static const InjDef INJ[] = {
 {"19 lb/hr", 19, 30}, {"24 lb/hr", 24, 35}, {"30 lb/hr", 30, 45}, {"42 lb/hr", 42, 60}, {"60 lb/hr", 60, 80},
 {"80 lb/hr", 80, 110}, {"120 lb/hr", 120, 160}, {"220 lb/hr", 220, 260}, {"1500 lb/hr nitro", 1500, 900},
};
#define NINJ 9
static const char *LUBES[] = {"Splash (dipper)", "Wet sump + pump", "Dry sump"};
static const char *LUBE_TIP[] = {
 "A dipper on the rod splashes oil around. Dirt cheap; only feeds small engines, and only while the oil stays under the crank (tilt kills it).",
 "Pump pulls oil from the pan through galleries to bearings and heads, then it drains back by gravity. Laid-over cylinders drain badly.",
 "Scavenge pumps empty the pan into a remote tank; a pressure pump feeds the engine. Works at any angle and under hard g-loads. Pricey."};
static const char *GOVS[] = {"None", "Governor 3600 rpm", "Governor 4500 rpm", "Rev limiter"};
static const float BSFC[] = {0.5f, 0.5f, 0.5f, 0.68f, 1.0f, 0.4f, 2.6f};
typedef struct { const char *name; float flt, rpk, ve, hgt, cost; const char *tip; } VT4Def;
static const VT4Def VT4[] = {
 {"Flathead",    4200, 2400, 0.62f, 0.35f, 100, "Valves in the block. Simple, low-revving, weak breathing."},
 {"OHV Pushrod", 6500, 4300, 0.86f, 0.55f, 300, "Cam in block, pushrods. Compact, torquey, floats valves early."},
 {"SOHC",        7400, 5000, 0.87f, 0.70f, 600, "Single overhead cam. Revs well."},
 {"DOHC",        8400, 5600, 0.96f, 0.85f, 1100,"Twin overhead cams. Best breathing, highest revs."},
};
typedef struct { const char *name; float rpk, ve, cost; const char *tip; } VT2Def;
static const VT2Def VT2[] = {
 {"Piston Port", 6000, 0.80f, 50,  "Piston skirt opens the intake port. Simplest 2-stroke."},
 {"Reed Valve",  6800, 0.86f, 120, "Reed petals stop blow-back. Broad power."},
 {"Rotary Valve",7800, 0.90f, 400, "Rotary disc valve. Peaky, high-revving."},
};
typedef struct { int n; float rpk, ve, flt, cost; } ValveDef;
static const ValveDef VALVES[] = {{2,0.95f,-0.04f,0,0},{3,1.0f,0.0f,200,150},{4,1.06f,0.04f,600,300},{5,1.08f,0.045f,700,500}};
typedef struct { const char *name, *name2; float rpk, width, ve, idle, flt, rough, cost; const char *tip; } CamDef;
static const CamDef CAMS[] = {
 {"Economy","Torque ports",0.70f,1.25f,0.93f,0,  -300,0.00f, 80,"Low-rpm torque, smooth idle."},
 {"Street", "Mild ports",  1.00f,1.00f,1.00f,0,   0,  0.02f,120,"All-rounder."},
 {"Sport",  "Sport ports", 1.18f,0.90f,1.03f,150, 300,0.08f,250,"Pulls harder up top, a little lumpy."},
 {"Race",   "Race ports",  1.40f,0.75f,1.06f,400, 1100,0.28f,500,"Peaky top end, loping idle, weak bottom end."},
};
typedef struct { const char *name; float flow, lag; int elec; float knock, cost; const char *tip; } DelDef;
static const DelDef DELIV[] = {
 {"Carburetor",        0.97f, 0.12f, 0, 0.2f,  120, "No electricity needed. Needs the choke when cold."},
 {"Throttle-body EFI", 0.985f,0.05f, 1, 0.0f,  300, "Simple fuel injection. Needs battery power."},
 {"Multi-port EFI",    1.00f, 0.04f, 1, 0.0f,  600, "Injector per port. Needs battery power."},
 {"Direct Injection",  1.02f, 0.03f, 1,-1.0f, 1500, "Sprays into the cylinder; cools the charge (knock -1). Needs power."},
 {"Mechanical Inj.",   1.00f, 0.03f, 0,-0.3f,  900, "Pump-driven injection. Works for diesel and race fuels, no power needed."},
};
#define NDEL 5
typedef struct { const char *name; float flow, dirt, cost; const char *tip; } FilDef;
static const FilDef FILTERS[] = {
 {"None (stack)",   1.00f, 1.00f,  0, "Open velocity stack. Max flow, eats dirt. Cylinders WILL wear."},
 {"Paper",          0.97f, 0.02f, 15, "Classic pleated paper. Good filtering."},
 {"Foam",           0.975f,0.06f, 12, "Oiled foam. Small-engine and dirt bike staple."},
 {"Oiled Cotton",   0.99f, 0.05f, 60, "High-flow cone. Loud intake."},
 {"Oil Bath",       0.93f, 0.01f, 40, "Old-school tractor filter. Restrictive, filters great."},
};
#define NFIL 5
typedef struct { const char *name; float full, maxb, tau, cost; const char *tip; } ForcedDef;
static const ForcedDef FORCED[] = {
 {"None",         0, 0,  0,    0, "Naturally aspirated."},
 {"Small Turbo",  5, 22, 0.35f,900, "Spools early, runs out of breath at high flow."},
 {"Large Turbo", 13, 42, 1.00f,1600,"Laggy, then a wall of boost."},
 {"Twin Turbo",   8, 34, 0.55f,2800,"Two mid turbos. Quick spool, big top end."},
 {"Supercharger", 0, 35, 0.0f, 2200,"Belt-driven roots blower. Instant boost, eats some crank power."},
};
#define NFORCED 5
typedef struct { const char *name; float cap, danger, cost; const char *tip; } CoolDef;
static const CoolDef COOL[] = {
 {"Air-cooled fins",   25000, 175, 0,   "Finned cylinders + shroud fan. Simple, light, runs hot."},
 {"Liquid, small rad", 40000, 118, 250, "Water jacket + compact radiator."},
 {"Liquid, big rad",   85000, 118, 450, "Big radiator for big power."},
};
typedef struct { const char *name; float flow4, flow2, vol, cost; const char *tip; } ExhDef;
static const ExhDef EXH[] = {
 {"Muffler",           0.96f,0.92f,0.55f, 120, "Quiet and a bit restrictive."},
 {"Sport Exhaust",     1.00f,0.97f,0.80f, 400, "Free-flowing, nice growl."},
 {"Straight Pipe",     1.02f,0.98f,1.00f,  60, "Loud. Pops on decel."},
 {"Expansion Chamber", 0.97f,1.00f,0.90f, 350, "2-stroke tuned pipe: big power boost near the pipe's rpm."},
};
typedef struct { const char *name; float fric, v, cold, hot; const char *tip; } OilDef;
static const OilDef OILS[] = {
 {"0W-20", 0.94f,0.80f,1.00f,-8, "Thin. Low friction, weak protection when hot."},
 {"5W-30", 0.97f,0.95f,1.05f, 0, "Modern all-rounder."},
 {"10W-40",1.00f,1.10f,1.15f, 5, "Thicker. Good for older or high-mileage engines."},
 {"15W-40",1.02f,1.15f,1.25f, 8, "Heavy-duty diesel oil."},
 {"20W-50",1.05f,1.30f,1.40f,12, "Very thick. Hot-climate, air-cooled, racing."},
};

typedef struct {
 int cycle, fuel, layout, cyl;
 float bore, stroke, cr;
 int block, internals, flywheel;
 int valvetrain, valves, cam;
 int delivery, filter, forced; float boost; int intercooler;
 int cooling, exhaust, starter, alternator, oil; float mix;
 char name[32];
 float tilt;
 int carb, inj, gov;
 int lube; float oilL; int oilCooler;
 // block design
 float wall, spacing; int deck, skirt, caps, balShaft, crankFlat;
 int ignition, camDrive;
 // custom cylinder layout
 int ncust; float cAng[16]; int cSlot[16], cFlip[16];
 // part placement
 int placeMan; float altAng, stAng; int filtPos, turboSide;
} Build;
static float WallMM(const Build *b) { return b->wall > 0.5f ? b->wall : 8.f; }
static float SpacingMM(const Build *b) { return b->spacing > 0.5f ? b->spacing : 10.f; }
static const char *DECKS[] = {"Semi-closed", "Open deck", "Closed deck"};
static const char *DECK_TIP[] = {"Bridges between bores and block. Good balance of strength and cooling.", "Water jacket open at the top. Best cooling, weakest under boost.", "Solid deck around the bores. Strongest for big boost, coolant flow suffers."};
static const float DECK_STR[] = {1.0f, 0.85f, 1.22f}, DECK_COOL[] = {1.0f, 1.08f, 0.9f}, DECK_COST[] = {0, -40, 180};
static const char *SKIRTS[] = {"Standard skirt", "Shallow skirt", "Deep skirt", "Bedplate / girdle"};
static const char *SKIRT_TIP[] = {"Block sides end near the crank centerline.", "Light and cheap; the crank flexes at high rpm.", "Block walls wrap below the crank: stiffer bottom end, heavier.", "Separate one-piece main bearing girdle. Stiffest bottom end there is."};
static const float SKIRT_STR[] = {1.0f, 0.92f, 1.08f, 1.2f}, SKIRT_RPM[] = {1.0f, 0.95f, 1.03f, 1.07f}, SKIRT_W[] = {1.0f, 0.92f, 1.08f, 1.12f}, SKIRT_COST[] = {0, -30, 120, 450};
static const char *CAPS[] = {"2-bolt mains", "4-bolt mains", "Cross-bolted mains"};
static const char *CAPS_TIP[] = {"Two bolts per main cap. Fine for stock power.", "Four bolts per cap: holds the crank down under big power and vibration.", "Caps bolted from the sides too. Race and diesel spec."};
static const float CAPS_STR[] = {1.0f, 1.08f, 1.15f}, CAPS_VIB[] = {1.0f, 1.35f, 1.7f}, CAPS_COST[] = {0, 90, 260};
static const char *IGNS[] = {"Stock (auto)", "Points + coil", "Distributor + coil", "CDI", "Coil-on-plug"};
static const char *IGN_TIP[] = {"Magneto/points on small-engine flywheels, a distributor otherwise.", "Mechanical points. Cheap; weak spark above ~6500 rpm and under boost.", "Distributor with one coil. Good to ~9000 rpm.", "Capacitive discharge. Hot spark, high rpm, works off a magneto or battery.", "Coil per cylinder, computer-timed. Strongest spark; needs a battery."};
static const float IGN_RPM[] = {0, 6500, 9000, 13000, 16000}, IGN_COST[] = {0, 15, 60, 90, 60};
static const char *CAMDRV[] = {"Timing chain", "Gear drive", "Timing belt"};
static const char *CAMDRV_TIP[] = {"Durable roller chain.", "Straight-cut gears. Bulletproof and precise, heavier.", "Rubber belt. Quiet, light, cheap; can snap at very high rpm."};
static const float CAMDRV_FRIC[] = {1.0f, 1.01f, 0.99f}, CAMDRV_COST[] = {0, 220, -20};

static const Build PRESETS[] = {
 {0,0,0,1, 64,50,8.5f, 1,0,0, 1,0,0, 0,2,0,0,0, 0,0,0,0,2,50, "Push Mower 160cc", 0, 0,0,1, 0,0.6f,0},
 {1,1,0,1, 66.4f,72,9.0f, 1,2,1, 1,0,3, 0,2,0,0,0, 1,3,1,0,1,40, "Dirt Bike 250 2T", 0, 2,0,3, 0,0,0},
 {0,1,1,4, 86,86,9.5f, 1,2,3, 3,2,2, 3,3,1,15,1, 2,1,4,3,1,50, "Hot Hatch 2.0T", 0, 6,4,3, 1,5.0f,1},
 {0,1,3,8, 103.25f,92,10.5f, 0,1,2, 1,0,2, 2,1,0,0,0, 2,1,5,3,2,50, "Muscle V8 6.2", 0, 6,3,3, 1,5.7f,0},
 {0,5,1,6, 107,124,17.3f, 2,2,5, 1,2,0, 3,1,2,26,1, 2,0,6,4,3,50, "Diesel Truck 6.7", 0, 6,4,3, 1,11.5f,1},
 {0,1,2,12, 95,76.4f,12.5f, 1,2,4, 3,2,3, 3,3,0,0,0, 2,2,5,4,0,50, "V12 Exotic 6.5", 0, 6,4,3, 2,10.f,1},
 {0,1,4,4, 85.5f,69,8.5f, 3,0,2, 1,0,1, 0,1,0,0,0, 0,0,4,3,4,50, "Air-cooled Flat-4", 0, 3,0,3, 1,2.5f,1},
 {0,6,3,8, 106.4f,114.3f,6.0f, 4,3,4, 1,0,3, 4,3,4,14,1, 2,2,6,3,4,50, "Nitro Dragster", 0, 6,8,3, 2,14.f,1},
 {0,0,0,1, 70,55,8.5f, 1,0,1, 1,0,0, 0,1,0,0,0, 0,0,1,0,2,50, "Buggy 212cc", 15, 1,0,3, 0,0.6f,0},
};
#define NPRESET 9

typedef struct {
 float prim, sec, rock, vib, vibForce, vibLimit;
 float vcylL, dispL, disp, rpk, veMax, wLo, wHi, floatRpm, pistonRpm, dieselRpm, redline, inertia;
 float imepPk, fric, pLimit, coolCap, danger, heatC, stall, compDrag, idle, weight, cost, lube;
 int hasBatt, needsElec, limitBy, magneto, ninj;
 float oilNeed, oilMax, oilHeadCap, drainK;
 float blockEff, intEff, govRpm, topRpm, injHp, needCfm, demandA, mfgOne, mfgK, decomp, uncappedHp;
 float estHp, estHpRpm, estTq, estTqRpm, estImep;
 int npts; float crpm[160], chp[160], ctq[160];
} Spec;

static float Clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static float Smooth01(float x) { x = Clampf(x, 0, 1); return x * x * (3 - 2 * x); }
static float Lerpf(float a, float b, float t) { return a + (b - a) * t; }

// shared cylinder geometry (mm, crank axis = x): position along crank, bank angle (rad, incl. tilt), bank index
static float CylPitch(const Build *b) {
 float p = b->bore * 1.18f + SpacingMM(b);
 if (b->layout == 5) {
  int cnt[16] = {0}, mx = 1;
  for (int i = 0; i < b->ncust; i++) { int sl = b->cSlot[i] & 15; cnt[sl]++; if (cnt[sl] > mx) mx = cnt[sl]; }
  p *= 1 + 0.24f * (mx - 1);
 }
 return p;
}
static int CylGeom(const Build *b, float *x, float *a, int *bank) {
 int n = b->layout == 5 ? b->ncust : b->cyl;
 float pitch = CylPitch(b), tlt = b->tilt * DEG2RAD, half = LAYOUT_HALF[b->layout] * DEG2RAD;
 if (b->layout == 5) {
  int used[16] = {0}, order[16], ns = 0;
  for (int i = 0; i < n; i++) used[b->cSlot[i] & 15] = 1;
  for (int s2 = 0; s2 < 16; s2++) order[s2] = used[s2] ? ns++ : -1;
  int cnt[16] = {0}, idx[16];
  for (int i = 0; i < n; i++) idx[i] = cnt[b->cSlot[i] & 15]++;
  for (int i = 0; i < n; i++) {
   int sl = b->cSlot[i] & 15, k = order[sl], m = cnt[sl];
   x[i] = (k - (ns - 1) * 0.5f) * pitch + (idx[i] - (m - 1) * 0.5f) * pitch * 0.24f / (1 + 0.24f * 0) ;
   a[i] = b->cAng[i] * DEG2RAD + tlt;
   bank[i] = sinf(a[i]) >= -0.01f ? 0 : 1;
  }
  return n;
 }
 int nb = b->layout >= 2 ? 2 : 1, m = nb == 2 ? (n + 1) / 2 : n;
 for (int i = 0; i < n; i++) {
  if (nb == 2) { int bk = i & 1, j = i >> 1; bank[i] = bk; a[i] = (bk ? -half : half) + tlt; x[i] = (j - (m - 1) * 0.5f) * pitch + (bk ? 0.12f : -0.12f) * pitch; }
  else { bank[i] = 0; a[i] = tlt; x[i] = (i - (n - 1) * 0.5f) * pitch; }
 }
 return n;
}
static int NumCyl(const Build *b) { return b->layout == 5 ? (b->ncust < 1 ? 1 : b->ncust) : b->cyl; }
// custom crank: each throw gets an angle; a cylinder's TDC = throw + its bank angle.
// throws are searched for the smoothest, most even-firing crank and cached.
static void CustFireFrom(const Build *b, const float *thr, float *fire) {
 int n = b->ncust; float cyc = b->cycle ? 360.f : 720.f;
 int idx[16]; float t[16];
 for (int i = 0; i < n; i++) { t[i] = fmodf(thr[b->cSlot[i] & 7] + b->cAng[i] + 720.f, 360.f); idx[i] = i; }
 for (int i = 1; i < n; i++) for (int k = i; k > 0 && t[idx[k]] < t[idx[k - 1]]; k--) { int q = idx[k]; idx[k] = idx[k - 1]; idx[k - 1] = q; }
 for (int k = 0; k < n; k++) fire[idx[k]] = t[idx[k]] + ((!b->cycle && (k & 1)) ? 360.f : 0.f);
 (void)cyc;
}
static float CustCost(const Build *b, const float *thr) {
 int n = b->ncust; float fire[16], cyc = b->cycle ? 360.f : 720.f, lam = 1.f / 3.5f;
 CustFireFrom(b, thr, fire);
 float x[16], a[16]; int bk[16]; CylGeom(b, x, a, bk);
 float pitch = CylPitch(b), f1x = 0, f1n = 1e9f, f2x = 0, m1x = 0, m1n = 1e9f;
 for (int t = 0; t < 360; t += 6) {
  float th = t * DEG2RAD, f1y = 0, f1z = 0, f2y = 0, f2z = 0, my = 0, mz = 0;
  for (int i = 0; i < n; i++) { float ph = th - fire[i] * DEG2RAD, c1 = cosf(ph), c2 = lam * cosf(2 * ph), uy = cosf(a[i]), uz = sinf(a[i]), xr = x[i] / pitch;
   f1y += c1 * uy; f1z += c1 * uz; f2y += c2 * uy; f2z += c2 * uz; my += xr * c1 * uy; mz += xr * c1 * uz; }
  float f1 = sqrtf(f1y * f1y + f1z * f1z), m1 = sqrtf(my * my + mz * mz);
  f1x = fmaxf(f1x, f1); f1n = fminf(f1n, f1); f2x = fmaxf(f2x, sqrtf(f2y * f2y + f2z * f2z)); m1x = fmaxf(m1x, m1); m1n = fminf(m1n, m1);
 }
 float s[16]; for (int i = 0; i < n; i++) s[i] = fmodf(fire[i], cyc);
 for (int i = 1; i < n; i++) for (int k = i; k > 0 && s[k] < s[k - 1]; k--) { float q = s[k]; s[k] = s[k - 1]; s[k - 1] = q; }
 float gmax = s[0] + cyc - s[n - 1], gmin = gmax;
 for (int i = 1; i < n; i++) { gmax = fmaxf(gmax, s[i] - s[i - 1]); gmin = fminf(gmin, s[i] - s[i - 1]); }
 float uneven = n > 1 ? (gmax - gmin) / (cyc / n) : 0;
 return ((f1x - f1n) * 0.5f + f2x + (m1x - m1n) * 0.3f) / (0.79f * n) + 0.35f * uneven;
}
static void CustomFire(const Build *b, float *fire) {
 static Build key; static float best[8]; static int have = 0;
 int n = b->ncust < 1 ? 1 : b->ncust;
 if (b->ncust < 1) { fire[0] = 0; return; }
 if (!have || key.ncust != b->ncust || key.cycle != b->cycle || key.bore != b->bore || key.spacing != b->spacing ||
     memcmp(key.cAng, b->cAng, sizeof key.cAng) || memcmp(key.cSlot, b->cSlot, sizeof key.cSlot)) {
  int used[8] = {0}, ns = 0;
  for (int i = 0; i < n; i++) used[b->cSlot[i] & 7] = 1;
  for (int k = 0; k < 8; k++) ns += used[k];
  float cur[8] = {0}, bc;
  int step = ns <= 2 ? 180 : (ns <= 4 ? 90 : 45);
  for (int k = 0, j = 0; k < 8; k++) if (used[k]) { cur[k] = (float)((j * 360 / ns) % 360); j++; }
  memcpy(best, cur, sizeof cur); bc = CustCost(b, cur);
  unsigned r = 99;
  for (int it = 0; it < 160; it++) {
   for (int k = 0; k < 8; k++) if (used[k]) { r = r * 1103515245u + 12345u; cur[k] = (float)(((r >> 16) % (360 / step)) * step); }
   for (int pass = 0; pass < 2; pass++) for (int k = 1; k < 8; k++) if (used[k]) {
    float keep = cur[k], kc = CustCost(b, cur), bk = keep;
    for (int v = 0; v < 360; v += step) { cur[k] = (float)v; float c = CustCost(b, cur); if (c < kc - 1e-4f) { kc = c; bk = cur[k]; } }
    cur[k] = bk;
   }
   float c = CustCost(b, cur);
   if (c < bc - 1e-4f) { bc = c; memcpy(best, cur, sizeof cur); }
   if (ns <= 1) break;
  }
  key = *b; have = 1;
 }
 CustFireFrom(b, best, fire);
}
static void FireAngles(const Build *b, float *fire) {
 int n = NumCyl(b);
 float cyc = b->cycle ? 360.f : 720.f;
 static const int FO3[] = {1,2,3}, FO4[] = {1,3,4,2}, FO5[] = {1,2,4,5,3}, FO6[] = {1,5,3,6,2,4}, FO8[] = {1,6,2,5,8,3,7,4};
 static const int FOV4[] = {1,3,2,4}, FOV6[] = {1,4,2,5,3,6}, FOV8[] = {1,8,4,3,6,5,7,2}, FOV10[] = {1,6,5,10,2,7,3,8,4,9}, FOV12[] = {1,7,5,11,3,9,6,12,2,8,4,10};
 static const int FOB4[] = {1,3,2,4}, FOB6[] = {1,6,2,4,3,5};
 const int *fo = NULL;
 for (int i = 0; i < n; i++) fire[i] = cyc * i / n;
 if (b->layout == 5) { CustomFire(b, fire); return; }
 int nb = b->layout >= 2 ? 2 : 1;
 if (!b->cycle) {
  if (nb == 1) fo = n == 3 ? FO3 : n == 4 ? FO4 : n == 5 ? FO5 : n == 6 ? FO6 : n == 8 ? FO8 : NULL;
  else if (b->layout == 4) fo = n == 4 ? FOB4 : n == 6 ? FOB6 : NULL;
  else fo = n == 4 ? FOV4 : n == 6 ? FOV6 : n == 8 ? FOV8 : n == 10 ? FOV10 : n == 12 ? FOV12 : NULL;
 }
 if (fo) for (int slot = 0; slot < n; slot++) fire[fo[slot] - 1] = cyc * slot / n;
 if (!b->cycle && (b->layout == 2 || b->layout == 3) && n >= 4 && n != 8) {
  int m = n / 2; float bf[8];
  const int *io = m == 3 ? FO3 : m == 4 ? FO4 : m == 5 ? FO5 : m == 6 ? FO6 : m == 8 ? FO8 : NULL;
  for (int j = 0; j < m; j++) bf[j] = 720.f * j / m;
  if (io) for (int k = 0; k < m; k++) bf[io[k] - 1] = 720.f * k / m;
  if (m == 2) { bf[0] = 0; bf[1] = 360; }
  for (int j = 0; j < m; j++) { fire[2 * j] = bf[j]; fire[2 * j + 1] = fmodf(bf[j] + 2 * LAYOUT_HALF[b->layout], 720.f); }
 }
 if (!b->cycle && n == 8 && nb == 2 && b->layout != 4 && b->crankFlat) {
  static const int i4[4] = {0, 2, 3, 1};
  for (int k = 0; k < 4; k++) { fire[2 * i4[k]] = 180.f * k; fire[2 * i4[k] + 1] = 180.f * k + 90.f; }
 }
 if (n == 2 && b->layout >= 2 && b->layout <= 3) fire[1] = b->cycle ? 180.f : 360.f - 2 * LAYOUT_HALF[b->layout];
}
// balance: residual shaking forces/couples after crank counterweights (rotating primary is cancellable)
static void Balance(const Build *b, float *prim, float *sec, float *rock, float *vib) {
 float x[16], a[16], fire[16]; int bank[16];
 int n = CylGeom(b, x, a, bank);
 FireAngles(b, fire);
 float lam = 1.f / 3.5f, pitch = CylPitch(b);
 float f1max = 0, f1min = 1e9f, f2max = 0, m1max = 0, m1min = 1e9f, m2max = 0;
 for (int t = 0; t < 360; t += 2) {
  float th = t * DEG2RAD, f1y = 0, f1z = 0, f2y = 0, f2z = 0, m1y = 0, m1z = 0, m2y = 0, m2z = 0;
  for (int i = 0; i < n; i++) {
   float ph = th - fire[i] * DEG2RAD;
   float c1 = cosf(ph), c2 = lam * cosf(2 * ph), uy = cosf(a[i]), uz = sinf(a[i]), xr = x[i] / pitch;
   f1y += c1 * uy; f1z += c1 * uz; f2y += c2 * uy; f2z += c2 * uz;
   m1y += xr * c1 * uy; m1z += xr * c1 * uz; m2y += xr * c2 * uy; m2z += xr * c2 * uz;
  }
  float f1 = sqrtf(f1y * f1y + f1z * f1z), f2 = sqrtf(f2y * f2y + f2z * f2z), m1 = sqrtf(m1y * m1y + m1z * m1z), m2 = sqrtf(m2y * m2y + m2z * m2z);
  f1max = fmaxf(f1max, f1); f1min = fminf(f1min, f1); f2max = fmaxf(f2max, f2); m1max = fmaxf(m1max, m1); m1min = fminf(m1min, m1); m2max = fmaxf(m2max, m2);
 }
 float rp = (f1max - f1min) * 0.5f, rs = f2max, rm = (m1max - m1min) * 0.5f + m2max;
 if (b->balShaft) { rp *= 0.4f; rs *= 0.2f; rm *= 0.6f; }
 *prim = rp; *sec = rs; *rock = rm;
 *vib = (rp + rs + rm * 0.6f) / (0.79f * n);
}

static float VE(const Build *b, const Spec *s, float rpm) {
 float x = rpm - s->rpk, w = x < 0 ? s->wLo : s->wHi;
 float ve = s->veMax * (0.30f + 0.70f * expf(-(x * x) / (w * w)));
 if (b->cycle == 1 && b->exhaust == 3) { float y = (rpm - 1.08f * s->rpk) / (0.22f * s->rpk); ve *= 1 + 0.18f * expf(-y * y); }
 if (rpm > s->floatRpm) { float y = (rpm - s->floatRpm) / (0.05f * s->floatRpm); ve /= (1 + y * y); }
 if (rpm > s->dieselRpm) { float y = (rpm - s->dieselRpm) / 500.f; ve *= expf(-y * y); }
 return ve;
}

static float SpoolTarget(const Build *b, const Spec *s, float rpm, float af) {
 if (b->forced == 0) return 0;
 if (b->forced == 4) { float x = Clampf(rpm / (0.45f * s->redline), 0, 1); return powf(x, 0.8f) * (0.25f + 0.75f * sqrtf(af)); }
 const ForcedDef *fd = &FORCED[b->forced];
 float flow = s->dispL * rpm / 1000.f * (b->cycle ? 2.f : 1.f) * (0.2f + 0.8f * af);
 float t = Smooth01((flow - 0.35f * fd->full) / (0.65f * fd->full));
 float ch = flow - 2.0f * fd->full;
 if (ch > 0) { float y = ch / fd->full; t /= (1 + y * y); }
 return t;
}

static float CfmNeed(const Build *b, const Spec *s, float rpm, float ve, float dens) {
 return s->dispL * rpm * ve * (b->cycle ? 1.8f : 0.5f) * 0.0353f * dens;
}

typedef struct { float tind, tfric, tpar, imep, af, knock, dens, lean; } TqOut;

static TqOut EvalTorque(const Build *b, const Spec *s, float rpm, float thr, float boost, float q, float temp, int charging) {
 TqOut o = {0};
 const FuelDef *f = &FUELS[b->fuel];
 float a = Clampf(thr, 0, 1);
 float ref = 0.5f * s->rpk;
 float af = f->diesel ? 1.0f : a / (a + (1 - a) * rpm / ref + 1e-4f);
 float PR = 1 + boost / 14.7f;
 float dens = powf(PR, b->intercooler ? 0.84f : 0.68f);
 float ve = VE(b, s, rpm);
 if (b->delivery == 0 && !f->diesel) {
  float need = CfmNeed(b, s, rpm, ve * af, dens), cfm = CARBS[b->carb].cfm;
  ve *= 1.f / powf(1 + powf(need / cfm, 4), 0.25f) * (need > 1e-3f ? 1 : 1);
 }
 float load = f->diesel ? (rpm < 450 ? fmaxf(a, 0.4f) : a) : af;
 float imep = s->imepPk * ve * load * dens;
 if (!f->diesel) {
  float eff = b->cr * powf(PR, b->intercooler ? 0.35f : 0.6f) + DELIV[b->delivery].knock + Clampf((temp - s->danger + 25) * 0.03f, 0, 2);
  float ex = eff - f->knock;
  if (ex > 0) { o.knock = ex * powf(af, 1.5f); imep /= (1 + 0.08f * o.knock); }
 }
 float cyc = b->cycle ? 2 * PI : 4 * PI;
 if (b->delivery > 0 && s->injHp > 0) {
  float hpReq = imep * 1e5f * s->disp / cyc * rpm / 7121.f;
  if (hpReq > s->injHp) { o.lean = hpReq / s->injHp; imep /= o.lean; }
 }
 imep *= q;
 float k = rpm / 1000.f;
 float fmep = (0.6f + 0.09f * k + 0.025f * k * k) * s->fric * sqrtf(b->stroke / 86.f);
 if (b->cycle == 1) fmep *= 0.7f;
 if (!f->diesel) fmep += (1 - af) * 0.9f;
 o.tind = imep * 1e5f * s->disp / cyc;
 o.tfric = fmep * 1e5f * s->disp / (4 * PI);
 o.tpar = b->alternator >= 3 ? (0.8f + (charging ? 1.5f : 0)) * (b->alternator == 4 ? 1.6f : 1.f) : (b->alternator ? 0.15f : 0);
 if (b->forced == 4) o.tpar += 0.15f * o.tind * (1 - 1 / PR);
 o.imep = imep; o.af = af; o.dens = dens;
 return o;
}

// part status: 0 ok, 1 marginal, 2 under-rated (fails in use), 3 doesn't fit
enum { P_BLOCK, P_FLY, P_START, P_ALT, P_CARB, P_INJ, P_TURBO, P_LUBE, P_N };
static int PartStatus(const Build *b, const Spec *s, int cat, char *why, int n) {
 why[0] = 0;
 switch (cat) {
 case P_BLOCK: {
  const BlockDef *bd = &BLOCKS[b->block];
  if (b->layout == 5) {
   if (b->ncust < 1) { snprintf(why, n, "No cylinders placed. Add some in BLOCK > CYLINDERS."); return 3; }
   float xx[16], aa[16]; int bb[16]; int nc = CylGeom(b, xx, aa, bb);
   float rr = b->stroke * 0.5f, v0 = rr * 1.25f + 10, deck = rr + 1.75f * b->stroke + 0.25f * b->bore + 1, mid = (v0 + deck) * 0.5f;
   float dmin = 2 * asinf(Clampf(0.55f * b->bore / mid, 0, 1));
   for (int i = 0; i < nc; i++) for (int j = i + 1; j < nc; j++) {
    if (b->cSlot[i] != b->cSlot[j]) continue;
    float d = fabsf(remainderf(aa[i] - aa[j], 2 * PI));
    if (d < dmin) { snprintf(why, n, "Cylinders %d and %d collide: need %.0f+ deg apart on the same throw.", i + 1, j + 1, dmin * RAD2DEG); return 3; }
   }
   if (!b->cycle && b->lube != 2) for (int i = 0; i < nc; i++) if (cosf(aa[i]) < -0.15f) { snprintf(why, n, "Cylinder %d points down into the oil pan: needs a dry sump.", i + 1); return 3; }
  }
  if (s->dispL > bd->maxL) { snprintf(why, n, "%s is unviable at %.1f L (max ~%.1f L): too weak at that size.", bd->name, s->dispL, bd->maxL); return 3; }
  if (s->blockEff < bd->str * 0.65f) { snprintf(why, n, "Big casting: strength down to %.0f%% of a small one.", 100 * s->blockEff / bd->str); return 1; }
  return 0; }
 case P_FLY: {
  const FlyDef *f = &FLY[b->flywheel];
  if (s->dispL > f->maxL) { snprintf(why, n, "Small-engine flywheel won't fit a %.1f L crank.", s->dispL); return 3; }
  if (s->dispL < f->minL) { snprintf(why, n, "Too big for a %.2f L engine; needs %.1f L+.", s->dispL, f->minL); return 3; }
  if (s->topRpm > f->maxRpm) { snprintf(why, n, "Rated %.0f rpm, engine can spin %.0f. It will burst.", f->maxRpm, s->topRpm); return 2; }
  if (s->topRpm > f->maxRpm * 0.92f) { snprintf(why, n, "Rated %.0f rpm: close to the edge.", f->maxRpm); return 1; }
  snprintf(why, n, "Rated %.0f rpm.", f->maxRpm); return 0; }
 case P_START: {
  const StarterDef *st = &STARTERS[b->starter];
  if (st->ring && !FLY[b->flywheel].ring) { snprintf(why, n, "Needs a ring-gear flywheel to engage."); return 3; }
  if (st->elec && !st->ring && !FLY[b->flywheel].magneto) { snprintf(why, n, "Engages small-engine flywheels only."); return 3; }
  if (st->minL > 0 && s->dispL < st->minL * 0.5f) { snprintf(why, n, "Too big to mount on a %.2f L engine.", s->dispL); return 3; }
  if (s->dispL > st->maxL) { snprintf(why, n, st->elec ? "Rated %.2f L. On %.2f L it'll overheat and burn out." : "Rated %.2f L. On %.2f L the spring/rope will snap.", st->maxL, s->dispL); return 2; }
  if (s->dispL > st->maxL * 0.85f) { snprintf(why, n, "Near its %.2f L rating.", st->maxL); return 1; }
  snprintf(why, n, "Rated to %.2f L.", st->maxL); return 0; }
 case P_ALT: {
  const AltDef *al = &ALTS[b->alternator];
  if (al->stator && !FLY[b->flywheel].magneto) { snprintf(why, n, "Stators live under a small-engine flywheel."); return 3; }
  if (s->dispL < al->minL) { snprintf(why, n, "No room to mount on a %.2f L engine.", s->dispL); return 3; }
  if (s->demandA > al->amps + 0.01f && (s->hasBatt || s->needsElec)) { snprintf(why, n, "Load is %.0f A, makes %.0f A. Battery will drain.", s->demandA, al->amps); return 2; }
  snprintf(why, n, "Load %.0f A of %.0f A.", s->demandA, al->amps); return 0; }
 case P_CARB: {
  if (b->delivery != 0) return 0;
  float cfm = CARBS[b->carb].cfm;
  if (s->needCfm > cfm * 1.15f) { snprintf(why, n, "Engine wants %.0f cfm, carb flows %.0f: chokes the top end.", s->needCfm, cfm); return 1; }
  if (cfm > s->needCfm * 2.2f) { snprintf(why, n, "Too big (%.0f cfm vs %.0f needed): stumbles and floods at low rpm.", cfm, s->needCfm); return 1; }
  snprintf(why, n, "%.0f cfm for a %.0f cfm engine.", cfm, s->needCfm); return 0; }
 case P_INJ: {
  if (b->delivery == 0) return 0;
  float uncapped = s->uncappedHp;
  if (FUELS[b->fuel].diesel) { if (s->injHp < uncapped * 0.98f) { snprintf(why, n, "Fuel-limited to ~%.0f hp.", s->injHp); return 1; } }
  else if (s->injHp < uncapped * 0.98f) { snprintf(why, n, "Supports ~%.0f hp on %s: will run LEAN and melt pistons.", s->injHp, FUELS[b->fuel].name); return 2; }
  snprintf(why, n, "Supports ~%.0f hp.", s->injHp); return 0; }
 case P_LUBE: {
  if (b->cycle) { snprintf(why, n, "2-strokes are oiled by the fuel mix."); return 0; }
  float tl = fabsf(b->tilt);
  if (b->lube == 0 && (s->dispL > 0.8f || b->cyl > 2)) { snprintf(why, n, "Splash can't feed a %.1f L %d-cyl: bearings starve at speed.", s->dispL, b->cyl); return 2; }
  if (b->lube == 0 && tl > 25) { snprintf(why, n, "Tilted %.0f deg: the dipper can't reach the oil.", tl); return 2; }
  if (b->lube == 1 && tl > 50) { snprintf(why, n, "Tilted %.0f deg: oil pools in the heads, pan runs dry. Dry sump it.", tl); return 2; }
  if (b->oilL < s->oilNeed) { snprintf(why, n, "%.1f L is too little (needs %.1f L): pickup sucks air.", b->oilL, s->oilNeed); return 2; }
  if (b->oilL > s->oilMax) { snprintf(why, n, "Overfilled: the crank whips the oil into froth."); return 1; }
  if (b->lube == 1 && tl > 35) { snprintf(why, n, "Tilted %.0f deg: slow drain-back, keep it topped up.", tl); return 1; }
  snprintf(why, n, "Needs %.1f L, has %.1f L.", s->oilNeed, b->oilL); return 0; }
 case P_TURBO:
  if (b->forced && b->boost > FORCED[b->forced].maxb) { snprintf(why, n, "%.0f psi is past its %.0f psi limit.", b->boost, FORCED[b->forced].maxb); return 2; }
  return 0;
 }
 return 0;
}

static void ComputeSpec(const Build *b, Spec *s) {
 memset(s, 0, sizeof *s);
 int two = b->cycle == 1;
 const FuelDef *f = &FUELS[b->fuel];
 s->vcylL = PI / 4 * b->bore * b->bore * b->stroke * 1e-6f;
 s->dispL = s->vcylL * b->cyl; s->disp = s->dispL * 1e-3f;
 if (two) {
  const VT2Def *v = &VT2[b->valvetrain];
  s->rpk = v->rpk * CAMS[b->cam].rpk * powf(55.f / b->stroke, 0.45f) * powf(0.25f / s->vcylL, 0.1f);
  s->veMax = v->ve; s->floatRpm = 1e9f;
 } else {
  const VT4Def *v = &VT4[b->valvetrain];
  int nv = b->valvetrain == 0 ? 0 : b->valves;
  s->rpk = v->rpk * VALVES[nv].rpk * CAMS[b->cam].rpk * powf(80.f / b->stroke, 0.35f) * powf(0.5f / s->vcylL, 0.05f);
  s->veMax = v->ve + VALVES[nv].ve;
  s->floatRpm = (v->flt + VALVES[nv].flt + CAMS[b->cam].flt) * powf(86.f / b->bore, 0.25f);
 }
 if (f->diesel) s->rpk *= 0.6f;
 s->veMax *= Clampf(powf(s->vcylL / (two ? 0.3f : 0.6f), 0.17f), 0.5f, 1.04f);
 s->veMax *= CAMS[b->cam].ve * FILTERS[b->filter].flow * DELIV[b->delivery].flow * (two ? EXH[b->exhaust].flow2 : EXH[b->exhaust].flow4);
 s->wLo = s->rpk * 0.9f * CAMS[b->cam].width; s->wHi = s->rpk * 1.35f * CAMS[b->cam].width;
 if (two) { s->wLo *= 0.8f; s->wHi *= 0.6f; }
 {
  float mp = powf(b->bore / 86.f, 3.f) * INTERN[b->internals].mass;
  s->pistonRpm = 9400.f * sqrtf(INTERN[b->internals].str / (mp * (b->stroke / 86.f))) * SKIRT_RPM[b->skirt] * (b->caps ? 1.02f : 1.f);
 }
 s->dieselRpm = f->diesel ? 4600 * powf(90.f / b->bore, 0.35f) : 1e9f;
 float lim = s->pistonRpm; s->limitBy = 0;
 if (s->floatRpm < lim) { lim = s->floatRpm; s->limitBy = 1; }
 if (s->dieselRpm * 1.05f < lim) { lim = s->dieselRpm * 1.05f; s->limitBy = 2; }
 s->redline = floorf(lim * 0.95f / 100.f) * 100.f;
 s->govRpm = b->gov == 1 ? 3600 : (b->gov == 2 ? 4500 : 0);
 s->topRpm = s->govRpm > 0 ? s->govRpm * 1.06f : (b->gov == 3 ? s->redline * 1.01f : s->redline * 1.12f);
 s->inertia = (0.012f + 0.075f * s->dispL * INTERN[b->internals].mass) * FLY[b->flywheel].mul + 0.004f * b->cyl;
 float th = (1 - powf(b->cr, -0.28f)) / (1 - powf(10, -0.28f));
 s->imepPk = (two ? 12.0f : (f->diesel ? 9.6f : 14.0f)) * th * f->energy;
 s->fric = (two ? 0.9f : OILS[b->oil].fric) * (two ? 1 : CAMDRV_FRIC[b->camDrive]) * (b->balShaft ? 1.03f : 1.f);
 {
  const BlockDef *bd = &BLOCKS[b->block];
  s->blockEff = bd->str * Clampf(powf(fmaxf(s->dispL, 0.05f) / 1.5f, -bd->sizeK), 0.2f, 1.25f);
  if (s->dispL > bd->maxL) s->blockEff *= 0.4f;
  float ik = 0.12f * (1.6f - INTERN[b->internals].str);
  s->intEff = INTERN[b->internals].str * Clampf(powf(fmaxf(s->vcylL, 0.02f) / 0.5f, -ik), 0.5f, 1.2f);
  float crPen = Clampf((b->cr - (bd->str < 0.9f ? 11.f : 14.f)) * 0.04f, 0, 0.4f);
  s->blockEff *= sqrtf(WallMM(b) / 8.f) * DECK_STR[b->deck] * SKIRT_STR[b->skirt] * CAPS_STR[b->caps];
  s->pLimit = 27.f * s->blockEff * s->intEff * (1 - crPen);
 }
 s->coolCap = COOL[b->cooling].cap * powf(s->dispL, 0.85f) * (b->cooling == 0 ? BLOCKS[b->block].cond : 1.f);
 s->danger = COOL[b->cooling].danger + (two ? 0 : OILS[b->oil].hot) + BLOCKS[b->block].heat * (b->cooling == 0 ? 1.6f : 1.f) - (SpacingMM(b) < 8 ? (8 - SpacingMM(b)) * 2.f : 0);
 s->coolCap *= DECK_COOL[b->deck] * powf(8.f / WallMM(b), 0.12f);
 s->heatC = (9000 * s->dispL + 3000) * (b->cooling ? 1.6f : 1.f);
 const StarterDef *st = &STARTERS[b->starter];
 s->stall = st->elec ? 150.f * st->power : st->power;
 s->decomp = st->decomp ? 0.45f : 1.f;
 s->compDrag = s->dispL * b->cr * 2.0f * (two ? 0.7f : OILS[b->oil].cold);
 s->magneto = FLY[b->flywheel].magneto;
 s->hasBatt = st->elec || b->alternator >= 2;
 s->needsElec = DELIV[b->delivery].elec;
 s->ninj = b->delivery == 1 ? 2 : b->cyl;
 s->demandA = (s->magneto ? 0 : 1 + 0.4f * b->cyl) + (b->cooling == 1 ? 8 : (b->cooling == 2 ? 15 : 0));
 if (s->needsElec) s->demandA += 6 + (b->delivery == 3 ? (f->diesel ? 15 : 10) + 2 * b->cyl : (b->delivery == 1 ? 4 : 3 + 1.f * b->cyl));
 s->injHp = b->delivery > 0 ? s->ninj * INJ[b->inj].lbhr * 0.85f / BSFC[b->fuel] : 0;
 s->oilNeed = (0.25f + 0.55f * powf(s->dispL, 0.9f)) * (b->lube == 2 ? 1.3f : 1.f);
 s->oilMax = s->oilNeed * 4.f + 1.f;
 s->oilHeadCap = 0.15f + 0.3f * s->dispL;
 {
  float tl = fabsf(b->tilt);
  s->drainK = b->lube == 2 ? 3.0f : 1.6f * (1 - Smooth01((tl - 25) / 60.f)) + 0.04f;
 }
 s->idle = two ? 1500 : (f->diesel ? 750 : 750 + CAMS[b->cam].idle);
 s->lube = two ? (b->mix < 1 ? 0 : 50.f / b->mix) : 1;
 {
  float topNeed = s->redline;
  float spT = SpoolTarget(b, s, topNeed, 1);
  float dens = powf(1 + b->boost * spT / 14.7f, b->intercooler ? 0.84f : 0.68f);
  s->needCfm = CfmNeed(b, s, topNeed, VE(b, s, topNeed), dens);
 }
 int nb = (b->layout >= 2 && b->layout != 5) ? 2 : 1;
 if (b->layout == 5) { float xx[16], aa[16]; int bb[16]; int n2 = CylGeom(b, xx, aa, bb), seen0 = 0, seen1 = 0; for (int i = 0; i < n2; i++) { if (bb[i]) seen1 = 1; else seen0 = 1; } nb = seen0 + seen1; if (nb < 1) nb = 1; }
 Balance(b, &s->prim, &s->sec, &s->rock, &s->vib);
 s->vibForce = (s->prim + s->sec + s->rock * 0.6f) * s->vcylL * (b->stroke / 86.f) * INTERN[b->internals].mass;
 s->vibLimit = 60.f * CAPS_VIB[b->caps] * SKIRT_STR[b->skirt] * (b->internals >= 2 ? 1.25f : 1.f);
 float L = s->dispL;
 float vtw = two ? 0.5f : (b->valvetrain == 0 ? 0.7f : (b->valvetrain == 1 ? 0.9f : (b->valvetrain == 2 ? 1.0f : 1.15f)));
 float blockKg = (3 + 22 * powf(L, 0.85f) + 1.2f * b->cyl) * BLOCKS[b->block].weight * (0.55f + 0.45f * WallMM(b) / 8.f) * SKIRT_W[b->skirt] * (b->deck == 2 ? 1.04f : (b->deck == 1 ? 0.97f : 1.f)) * (0.85f + 0.15f * SpacingMM(b) / 10.f);
 float headKg = (1.5f + 4 * L) * vtw * nb * 0.7f * (BLOCKS[b->block].weight < 0.7f ? 0.8f : 1.f);
 float intKg = (0.8f + 5 * L) * INTERN[b->internals].mass;
 float flyKg = (1 + 5 * powf(L, 0.7f)) * FLY[b->flywheel].mul * 0.8f;
 float accKg = (b->forced == 3 ? 18 : (b->forced == 4 ? 20 : (b->forced ? 9 : 0))) + (b->forced && b->intercooler ? 6 : 0)
  + (b->alternator >= 3 ? 6 : (b->alternator ? 1 : 0)) + (st->elec ? 1 + 2.5f * st->power : 0.8f) + (b->cooling == 1 ? 8 + 4 * L : (b->cooling == 2 ? 12 + 6 * L : 0))
  + 1 + 3 * L + (two ? 0 : 0.5f + 1.5f * L);
 s->weight = blockKg + headKg + intKg + flyKg + accKg + (b->balShaft ? 2 + 2.5f * L : 0) + (b->camDrive == 1 ? 1 + L : 0);
 // cost: materials + machining + bought parts (catalog prices are retail; volume buys at ~35%)
 static const float MATKG[] = {1.3f, 2.5f, 2.0f, 8.f, 6.f};
 static const float MACH[] = {1.0f, 0.75f, 1.3f, 1.4f, 5.0f};
 static const float INTC[] = {30, 70, 300, 1100};
 static const float CAMC[] = {0, 0, 150, 400};
 static const float VTC[] = {0, 15, 80, 200};
 static const float ECU[] = {0, 250, 400, 1200, 600};
 float matl = blockKg * MATKG[b->block] + headKg * (BLOCKS[b->block].weight < 0.7f ? 3.0f : 1.5f) + (s->weight - blockKg - headKg) * 1.8f;
 float hours = (0.4f + 0.25f * b->cyl + 0.4f * nb * vtw + 0.6f * L) * MACH[b->block] + (b->block == 4 ? 4 + 3 * L : 0);
 float sz = Clampf(sqrtf(L / 2.f), 0.15f, 1.6f);
 float bought = INTC[b->internals] * (0.2f + s->vcylL * 1.6f) * b->cyl
  + (two ? VT2[b->valvetrain].cost * 0.3f : VTC[b->valvetrain] * nb * (0.3f + 0.3f * L) + VALVES[b->valvetrain ? b->valves : 0].cost * b->cyl * 0.1f)
  + CAMC[b->cam] * nb * (0.3f + 0.15f * L)
  + (b->delivery == 0 ? CARBS[b->carb].cost : INJ[b->inj].cost * s->ninj + ECU[b->delivery] * (0.5f + 0.1f * b->cyl))
  + FILTERS[b->filter].cost * (0.3f + 0.35f * sqrtf(L)) + FORCED[b->forced].cost * Clampf(sqrtf(L / 2.f), 0.4f, 1.6f) + (b->forced && b->intercooler ? 450 * sz : 0)
  + COOL[b->cooling].cost * (0.3f + 0.35f * L) + EXH[b->exhaust].cost * (0.05f + 0.6f * sz)
  + st->cost + ALTS[b->alternator].cost + FLY[b->flywheel].cost + (b->gov == 3 ? 60 : (b->gov ? 12 : 0)) + (b->fuel == 6 ? 300 : 0)
  + (two ? 0 : (b->lube == 0 ? 1 : (b->lube == 1 ? 25 + 15 * L : 900 + 200 * L))) + (b->oilCooler ? 80 + 40 * L : 0)
  + (DECK_COST[b->deck] + SKIRT_COST[b->skirt]) * (0.4f + 0.3f * L) + CAPS_COST[b->caps] * (0.3f + 0.1f * b->cyl) + (b->balShaft ? 120 + 60 * L : 0)
  + IGN_COST[b->ignition] * (b->ignition == 4 ? b->cyl * 0.5f : 1) + (two ? 0 : CAMDRV_COST[b->camDrive] * (0.5f + 0.25f * L));
 s->mfgOne = matl * 1.6f + hours * 95.f + bought + 200;
 s->mfgK = matl * 0.85f + hours * 0.12f * 40.f + bought * 0.25f + 4 + 3 * b->cyl;
 s->cost = s->mfgK * 1.7f;
 // breathing: redline sits a bit past where power falls off, never above the mechanical limit
 {
  float pk = 0, pkR = 0, mech = s->redline;
  for (int i = 0; i < 120; i++) {
   float rpm = 1000 + (mech - 1000) * i / 119.f;
   TqOut t = EvalTorque(b, s, rpm, 1, b->boost * SpoolTarget(b, s, rpm, 1), 1, 90, 1);
   float hp = (t.tind - t.tfric - t.tpar) * rpm;
   if (hp > pk) { pk = hp; pkR = rpm; }
  }
  float fall = mech;
  for (int i = 0; i < 120; i++) {
   float rpm = pkR + (mech - pkR) * i / 119.f;
   TqOut t = EvalTorque(b, s, rpm, 1, b->boost * SpoolTarget(b, s, rpm, 1), 1, 90, 1);
   if ((t.tind - t.tfric - t.tpar) * rpm < pk * 0.82f) { fall = rpm; break; }
  }
  float br = floorf(fminf(fmaxf(fall, pkR * 1.05f), pkR * 1.14f) / 100.f) * 100.f;
  if (br < s->redline) { s->redline = br; s->limitBy = 3; }
  s->topRpm = s->govRpm > 0 ? s->govRpm * 1.06f : (b->gov == 3 ? s->redline * 1.01f : s->redline * 1.12f);
 }
 // dyno estimate
 float top = fmaxf(s->govRpm > 0 ? s->govRpm : s->redline, 2000);
 float lo = fminf(s->idle, 800);
 s->npts = 0;
 for (int i = 0; i < 160; i++) {
  float rpm = lo + (top - lo) * i / 159.f;
  float boost = b->boost * SpoolTarget(b, s, rpm, 1);
  TqOut t = EvalTorque(b, s, rpm, 1, boost, 1, 90, 1);
  float br = t.tind - t.tfric - t.tpar;
  s->crpm[i] = rpm; s->ctq[i] = br; s->chp[i] = br * rpm / 7121.f;
  if (s->chp[i] > s->estHp) { s->estHp = s->chp[i]; s->estHpRpm = rpm; }
  if (br > s->estTq) { s->estTq = br; s->estTqRpm = rpm; }
  if (t.imep > s->estImep) s->estImep = t.imep;
  s->npts++;
 }
 s->uncappedHp = s->estHp;
 if (b->delivery > 0) {
  float save = s->injHp, peak = 0;
  s->injHp = 0;
  for (int i = 0; i < 40; i++) {
   float rpm = lo + (top - lo) * i / 39.f;
   TqOut t = EvalTorque(b, s, rpm, 1, b->boost * SpoolTarget(b, s, rpm, 1), 1, 90, 1);
   float hp = (t.tind - t.tfric - t.tpar) * rpm / 7121.f;
   if (hp > peak) peak = hp;
  }
  s->injHp = save; s->uncappedHp = peak;
 }
}

static const char *PART_NAME[] = {"Block", "Flywheel", "Starter", "Charging", "Carb", "Injectors", "Turbo", "Oiling"};

static int BuildWarnings(const Build *b, const Spec *s, char out[][96], int *sev, int max) {
 int n = 0;
 const FuelDef *f = &FUELS[b->fuel];
#define W(sv, ...) do { if (n < max) { snprintf(out[n], 96, __VA_ARGS__); sev[n] = sv; n++; } } while (0)
 char why[96];
 for (int p = 0; p < P_N; p++) {
  int st = PartStatus(b, s, p, why, 96);
  if (st == 3) W(3, "%s DOESN'T FIT: %s", PART_NAME[p], why);
  else if (st == 2) W(2, "%s: %s", PART_NAME[p], why);
  else if (st == 1) W(1, "%s: %s", PART_NAME[p], why);
 }
 if (f->diesel && b->cr < 14) W(2, "Diesel needs 14:1+ compression to self-ignite.");
 else if (f->diesel && b->cr < 16) W(1, "Low diesel compression: hard cold starts.");
 if (f->diesel && b->delivery != 3 && b->delivery != 4) W(2, "Diesel needs direct or mechanical injection.");
 if (s->needsElec && !s->hasBatt) W(2, "EFI needs power: add a charging system or electric start.");
 if (!f->diesel && !s->magneto && !s->hasBatt) W(2, "No spark: car flywheels have no magneto. Add a battery.");
 if (STARTERS[b->starter].elec && b->alternator == 0) W(1, "Nothing charges the battery: it will drain.");
 if (!STARTERS[b->starter].elec && s->compDrag * s->decomp > STARTERS[b->starter].power * 0.92f) W(2, "Too much compression to pull through. Decompressor or electric.");
 if (b->cycle == 1) {
  if (b->mix < 1) W(2, "NO OIL in the fuel. A 2-stroke will seize.");
  else if (b->mix > 75) W(1, "Lean oil mix: high wear under load.");
  else if (b->mix < 24) W(1, "Very rich mix: smoke and fouled plugs.");
 }
 if (!f->diesel) {
  float PR = 1 + b->boost * (b->forced ? 1 : 0) / 14.7f;
  float eff = b->cr * powf(PR, b->intercooler ? 0.35f : 0.6f) + DELIV[b->delivery].knock;
  if (eff > f->knock + 0.3f) W(2, "Will detonate on %s (eff. CR %.1f > %.1f).", f->name, eff, f->knock);
  else if (eff > f->knock - 0.4f) W(1, "Close to the knock limit for %s.", f->name);
 }
 if (s->estImep > s->pLimit) W(2, "Cylinder pressure %.0f bar > block/internals limit %.0f.", s->estImep, s->pLimit);
 else if (s->estImep > s->pLimit * 0.9f) W(1, "Cylinder pressure near the %.0f bar limit.", s->pLimit);
 if (b->forced && !b->intercooler && b->boost > 8 && !f->diesel) W(1, "No intercooler: hot charge, more knock.");
 if (s->estHp * 745.7f * 0.4f > s->coolCap * 1.1f) W(1, "Cooling is marginal for sustained full load.");
 if (b->filter == 0) W(1, "No air filter: dirt will score the cylinders.");
 if (b->cycle == 0 && b->exhaust == 3) W(1, "Expansion chambers only help 2-strokes.");
 if (b->cycle == 0 && fabsf(b->tilt) > 50) W(1, "Cylinders laid over %.0f deg: wet sump drains badly.", fabsf(b->tilt));
 if (b->layout >= 2 && fabsf(b->tilt) > 30) W(1, "Tilting a V/boxer that far puts a bank near the floor.");
 if (b->gov == 0) W(1, "No governor or limiter: nothing stops it over-revving.");
 if (!b->cycle && b->camDrive == 2 && s->topRpm > 9000) W(2, "Timing belt above ~9000 rpm: it will jump teeth or snap.");
 if (b->ignition == 4 && !s->hasBatt) W(3, "Coil-on-plug needs a battery. Add electric start or a charging system.");
 if (b->ignition == 2 && !s->hasBatt) W(3, "A distributor coil needs a battery. Use points/CDI on a magneto.");
 { int ig = b->ignition ? b->ignition : (s->magneto ? 3 : 2); if (s->topRpm > IGN_RPM[ig] * 1.05f && !FUELS[b->fuel].diesel) W(1, "%s ignition weakens above ~%.0f rpm: misfires up top.", IGNS[ig], IGN_RPM[ig]); }
 if (s->vibForce * powf(s->topRpm / 1000.f, 2) > s->vibLimit) W(2, "Shakes too hard at %.0f rpm: the crank will crack. Balance it or rev less.", s->topRpm);
 else if (s->vib > 0.6f && NumCyl(b) > 2) W(1, "Rough-running layout (vibration %.0f%%). Balance shafts help.", s->vib * 100);
#undef W
 return n;
}

// ===== live simulation =====
enum { DMG_KNOCK, DMG_ROD, DMG_VALVE, DMG_PRESS, DMG_HEAT, DMG_OILP, DMG_LUBE, DMG_DIRT, DMG_TURBO, DMG_FLY, DMG_LEAN, DMG_VIB, DMG_BELT, DMG_N };
static const char *DMG_TITLE[] = {"Holed piston","Thrown rod","Dropped valve","Blown head gasket","Seized engine","Spun bearing","Seized piston","Scored cylinders","Turbo grenade","Flywheel exploded","Melted piston (lean)","Cracked crankshaft","Snapped timing belt"};

typedef struct {
 float omega, angle, rpm;
 float thrCmd, thrAct, idleOpen, idleCmd, idleI, govOpen;
 float spool, boost, temp, oilT, oilP, egt, batt, volts;
 float oilSump, oilHead, oilSupply, oilFlow, oilBurn, oilSplash;
 float health, dmg[DMG_N], dmgRate[DMG_N];
 int ign, choke, limiter, failed, failCause, cut, running, starterOn, charging;
 float recoilT, q, knock, lean, tind, tbrake, tgross, load, rough, roughT, crankTime, runTime, wotTime, prevThr;
 float misT; int fireOk;
 float starterHeat; int starterDead, recoilDead, partMsgPending; char partMsg[128];
 int bovCount, recoilStuck;
 int dyno; float dynoStart, dynoTarget, dynoInt, dynoLoad, dynoEnd;
 int nd; float drpm[400], dhp[400], dtq[400];
 float dSum; int dCnt;
 int npd; float pdrpm[400], pdhp[400], pdtq[400];
 float dynoPeakHp, dynoPeakTq, maxRpm;
 unsigned rng;
} Sim;

static float SRand(Sim *m) { m->rng ^= m->rng << 13; m->rng ^= m->rng >> 17; m->rng ^= m->rng << 5; return (m->rng & 0xffffff) / 16777216.f; }

static void SimReset(Sim *m, const Spec *s, int keepRuns) {
 Sim old = *m;
 memset(m, 0, sizeof *m);
 m->temp = 20; m->oilT = 20; m->batt = 0.85f; m->oilSupply = 1; m->health = 100; m->limiter = 1; m->rng = 0x1234567u; m->egt = 20; m->govOpen = 1; m->fireOk = 1;
 m->oilSump = -1;
 if (keepRuns) { m->npd = old.npd; memcpy(m->pdrpm, old.pdrpm, sizeof m->pdrpm); memcpy(m->pdhp, old.pdhp, sizeof m->pdhp); memcpy(m->pdtq, old.pdtq, sizeof m->pdtq); }
 (void)s;
}

static int gDev = 0;

static void PartFail(Sim *m, const char *msg) { snprintf(m->partMsg, 128, "%s", msg); m->partMsgPending = 1; }

static void SimStep(Sim *m, const Build *b, const Spec *s, float dt, int starterHeld) {
 const FuelDef *f = &FUELS[b->fuel];
 const StarterDef *st = &STARTERS[b->starter];
 float rpm = m->omega * 60.f / (2 * PI);
 m->rpm = rpm;
 float warm = Clampf((m->temp - 20) / 30.f, 0, 1);
 float warm2 = Clampf((m->temp - 20) / 45.f, 0, 1);
 // electrics
 float altA = ALTS[b->alternator].amps * Clampf((rpm - 900) / 1500.f, 0, 1);
 m->charging = altA > s->demandA * 0.5f && altA > 0.5f;
 m->starterOn = starterHeld && st->elec && !m->failed && !m->starterDead;
 float v = 11.6f + 1.0f * m->batt;
 if (m->starterOn) v -= 1.0f + (1 - m->batt) * 2.5f;
 if (altA > s->demandA + 1 && s->hasBatt) v = fmaxf(v, 13.2f + Clampf((altA - s->demandA) / 20.f, 0, 1));
 if (!s->hasBatt) v = 0;
 m->volts = v;
 if (s->hasBatt && !gDev) {
  float amps = 0;
  if (m->starterOn) amps -= 150 * st->power;
  if (m->ign) amps -= s->demandA * (m->running ? 1.f : 0.6f);
  if (m->running || rpm > 900) amps += altA;
  m->batt = Clampf(m->batt + amps / 35.f / 3600.f * 20.f * dt, 0, 1);
 }
 // starter heat (undersized burns out)
 if (m->starterOn) {
  float over = s->dispL / st->maxL;
  m->starterHeat += dt * (0.45f + (over > 1 ? (over - 1) * 3.f : 0));
  if (m->starterHeat > 6.f && !gDev) { m->starterDead = 1; PartFail(m, over > 1 ? "Starter motor burned out: under-rated for this engine." : "Starter overheated from cranking too long. Let it cool (rebuild)."); }
 } else m->starterHeat = fmaxf(0, m->starterHeat - dt * 0.25f);
 // combustion quality
 float q = 1;
 int ig = b->ignition ? b->ignition : (s->magneto ? 3 : 2);
 int battOk = s->hasBatt && m->volts > 8.5f;
 int spark = f->diesel || ((ig == 1 || ig == 3) ? (s->magneto || battOk) : battOk);
 if (!m->ign || m->failed || !spark) q = 0;
 if (f->diesel) {
  if (b->delivery != 3 && b->delivery != 4) q = 0;
  if (b->cr < 14) q = 0;
  else { float cold = Clampf((b->cr - 14) / 4.f + (s->hasBatt && m->volts > 10 ? 0.35f : 0), 0, 1); q *= Lerpf(cold, 1, warm); }
 } else {
  if (b->delivery == 0) {
   if (!m->choke) q *= Lerpf(0.18f, 1, warm);
   else q *= Lerpf(1, 0.6f, Clampf((m->temp - 50) / 20.f, 0, 1));
   q *= Lerpf(m->choke ? 0.82f : 0.55f, 1, powf(warm2, 0.7f));
   float big = CARBS[b->carb].cfm / fmaxf(s->needCfm, 1);
   if (big > 2.2f) q *= 1 - Clampf((big - 2.2f) * 0.2f, 0, 0.45f) * Clampf(1 - rpm / fmaxf(s->redline, 1), 0, 1);
  } else q *= Lerpf(0.9f, 1, warm2);
  q *= Lerpf(f->cold, 1, warm);
 }
 if (s->needsElec && (!s->hasBatt || m->volts < 9.f)) q = 0;
 if (b->cycle == 1 && s->lube > 2.2f) q *= Clampf(1 - 0.25f * (s->lube - 2.2f), 0.4f, 1);
 if (!f->diesel) {
  q *= 1 - Clampf((rpm - IGN_RPM[ig]) / 3000.f, 0, 0.6f);
  if (ig <= 2) q *= 1 - Clampf((m->boost - 15) / 30.f, 0, 0.4f);
 }
 if (rpm < 90) q = 0;
 if (b->gov == 3 && m->limiter) { if (rpm > s->redline) m->cut = 1; else if (rpm < s->redline - 180) m->cut = 0; } else m->cut = 0;
 if (m->cut) q = 0;
 m->q = q;
 // per-firing misfire gating: sputter is violent, not averaged
 m->misT -= dt;
 if (m->misT <= 0) {
  float cyc = b->cycle ? 360.f : 720.f;
  m->misT = (cyc / b->cyl) / (fmaxf(rpm, 60) * 6.f);
  m->fireOk = SRand(m) < q;
 }
 m->running = q > 0.05f && rpm > fmaxf(150, s->idle * 0.45f);
 // throttle, idle control (hunts when cold), governor
 float lag = DELIV[b->delivery].lag;
 float tc = m->thrCmd;
 if (m->dyno) tc = 1;
 m->thrAct += (tc - m->thrAct) * Clampf(dt / lag, 0, 1);
 float coldHunt = (b->delivery == 0 ? 1.f : 0.25f) * (1 - warm2);
 float ierr = s->idle - rpm;
 if (m->running) m->idleI += ierr * 0.00045f * (1 + 2.2f * coldHunt) * dt;
 m->idleI = Clampf(m->idleI, 0.0f, 0.3f);
 if (!m->running && rpm < 150) m->idleI = Clampf(m->idleI, 0.06f, 0.3f);
 m->idleCmd = Clampf(m->idleI + ierr * 0.00028f * (1 - 0.55f * coldHunt), 0, 0.35f);
 float itau = 0.05f + 0.4f * coldHunt;
 m->idleOpen += (m->idleCmd - m->idleOpen) * Clampf(dt / itau, 0, 1);
 float thr = fmaxf(m->thrAct, m->idleOpen);
 if (s->govRpm > 0 && m->limiter) {
  float tgt = Clampf(0.55f + (s->govRpm - rpm) * 0.0016f * (1 + 1.5f * (1 - warm2)), 0, 1);
  float gtau = 0.15f + 0.35f * (1 - warm2);
  m->govOpen += (tgt - m->govOpen) * Clampf(dt / gtau, 0, 1);
  thr = fminf(thr, fmaxf(m->govOpen, m->idleOpen * 0.8f));
 }
 // boost
 if (b->forced == 4) { m->spool = SpoolTarget(b, s, rpm, thr); }
 else if (b->forced) {
  float af0 = thr / (thr + (1 - thr) * rpm / (0.5f * s->rpk) + 1e-4f);
  float tgt = SpoolTarget(b, s, rpm, m->running ? af0 : 0);
  m->spool += (tgt - m->spool) * Clampf(dt / FORCED[b->forced].tau, 0, 1);
 } else m->spool = 0;
 float boost = b->boost * m->spool;
 if (m->prevThr > 0.5f && thr < 0.15f && m->boost > 5 && b->forced && b->forced != 4) m->bovCount++;
 m->prevThr = thr;
 m->boost = boost;
 TqOut t = EvalTorque(b, s, rpm, thr, boost, q > 0 ? 1 : 0, m->temp, m->charging);
 if (m->choke && m->temp > 50) t.tind *= 0.8f;
 m->tgross = t.tind * (q > 0 ? q : 0);
 if (!m->fireOk) t.tind = 0;
 m->roughT -= dt;
 if (m->roughT <= 0) { m->roughT = 0.025f + 0.04f * SRand(m); m->rough = SRand(m) * 2 - 1; }
 float rough = CAMS[b->cam].rough * Clampf(1 - rpm / 2800.f, 0, 1) + (FLY[b->flywheel].mul < 0.7f ? 0.04f : 0);
 t.tind *= 1 + rough * m->rough;
 m->tind = t.tind; m->knock = t.knock; m->lean = t.lean;
 m->tbrake = t.tind - t.tfric - t.tpar;
 m->load = Clampf(t.tind / fmaxf(s->estTq * 1.1f + 1, 1), 0, 1.5f);
 // starter
 float tst = 0;
 if (m->starterOn && rpm < 450) {
  float vf = Clampf((m->volts - 8.8f) / 2.f, 0, 1);
  tst = s->stall * fmaxf(0, 1 - rpm / 350.f) * vf;
 }
 if (m->recoilT > 0) { m->recoilT -= dt; tst += st->power * fmaxf(0, 1 - rpm / 700.f); }
 int ncomp = b->cycle ? b->cyl : (b->cyl + 1) / 2;
 float cf = Clampf(1 - rpm / 600.f, 0, 1);
 float decomp = (rpm < 400 && !st->elec) ? s->decomp : 1.f;
 float tcomp = (1 - 0.8f * (m->fireOk ? q : 0)) * s->compDrag * decomp * cf * (0.55f + 0.6f / sqrtf((float)b->cyl) * sinf(m->angle * PI / 180.f * ncomp * 2.f));
 float coldF = b->cycle ? 1 : Lerpf(OILS[b->oil].cold, 1, warm);
 float tf = t.tfric * coldF;
 float tload = 0;
 if (m->dyno) {
  m->dynoTarget += 700.f * dt;
  float e = (m->omega - m->dynoTarget * 2 * PI / 60.f);
  float kp = s->inertia * 60.f;
  m->dynoInt += e * kp * 6.f * dt;
  if (m->dynoInt < 0) m->dynoInt = 0;
  tload = fmaxf(0, kp * e + m->dynoInt);
  m->dynoLoad = tload;
 }
 float drive = t.tind + tst;
 float resist = tf + t.tpar + tcomp + tload;
 if (m->failed) resist += 400 * s->dispL + 20;
 if (m->omega < 0.5f && drive < resist) {
  m->omega = 0;
  if (m->recoilT > 0 && !st->elec) m->recoilStuck = 1;
 } else {
  m->omega += (drive - resist) / s->inertia * dt;
  if (m->omega < 0) m->omega = 0;
 }
 m->angle = fmodf(m->angle + rpm * 6.f * dt, 720.f);
 // thermal
 float Q = (rpm > 100 && q > 0) ? fmaxf(0, m->tgross) * m->omega * 0.9f + 1200 * powf(s->dispL, 0.8f) + (m->temp < 75 ? 12000 * powf(s->dispL, 0.8f) : 0) : tf * m->omega * 0.3f;
 float h;
 if (b->cooling == 0) h = s->coolCap / 150.f * (0.4f + 0.6f * Clampf(rpm / 4000.f, 0, 1));
 else h = s->coolCap / 70.f * Clampf((m->temp - 82) / 10.f, 0.06f, 1.f) * (0.5f + 0.5f * Clampf(rpm / 2000.f, 0, 1));
 m->temp += (Q - h * (m->temp - 20)) / s->heatC * dt;
 float egtT = 20 + (m->running ? 250 + 650 * Clampf(t.af * rpm / fmaxf(s->redline, 1), 0, 1.2f) * (0.5f + 0.5f * m->load) + (m->lean > 1 ? (m->lean - 1) * 400 : 0) : 0);
 m->egt += (egtT - m->egt) * Clampf(dt * 0.8f, 0, 1);
 {
  float tgt = m->temp + (rpm > 50 ? 4 + rpm * 0.0022f + 18 * Clampf(m->load, 0, 1.2f) : -5) - (b->cycle ? 0 : 6) - (b->oilCooler ? 8 + 14 * Clampf(m->load, 0, 1) : 0);
  m->oilT += (tgt - m->oilT) * Clampf(dt / 22.f, 0, 1);
 }
 if (b->cycle == 0) {
  if (m->oilSump < 0) m->oilSump = b->oilL;
  float tl = fabsf(b->tilt);
  float need = s->oilNeed;
  if (b->lube == 0) {
   float reach = Smooth01((m->oilSump - need * 0.5f) / (need * 0.5f)) * (1 - Smooth01((tl - 15) / 20.f));
   float sp = Clampf(1 - (rpm - 6800) / 3500.f, 0.25f, 1) * (b->cyl > 2 || s->dispL > 0.8f ? 0.45f : 1.f);
   m->oilSplash = reach * sp;
   m->oilSupply = m->oilSplash;
   m->oilFlow = rpm > 50 ? 0.4f * reach * Clampf(rpm / 3000.f, 0.2f, 1.5f) : 0;
   m->oilP = 0;
  } else {
   float supply = b->lube == 2 ? Smooth01((m->oilSump - need * 0.25f) / (need * 0.2f)) : Smooth01((m->oilSump - need * 0.55f) / (need * 0.4f));
   if (b->lube == 1 && tl > 45) supply *= 1 - 0.5f * Smooth01((tl - 45) / 40.f) * Clampf(rpm / 5000.f, 0.3f, 1.f);
   if (m->oilSump > s->oilMax) supply *= 0.85f;
   if (supply < 0.98f && rpm > 50) supply *= 0.8f + 0.2f * SRand(m);
   m->oilSupply = supply;
   float vf = OILS[b->oil].v * expf(-(m->oilT - 90) / 45.f);
   m->oilP = Clampf((8 + rpm * 0.011f) * vf, 0, 80) * (rpm > 50 ? 1 : 0) * supply * (b->lube == 2 ? 1.1f : 1.f);
   m->oilFlow = rpm > 50 ? 0.03f * powf(s->dispL, 0.6f) * rpm / 1000.f * supply : 0;
  }
  // circulation: pump lifts oil to the heads, gravity drains it back; tilt slows drain-back
  float up = b->lube == 0 ? m->oilFlow * 0.05f : m->oilFlow;
  float take = fminf(up * dt, fmaxf(m->oilSump - 0.02f, 0));
  m->oilSump -= take; m->oilHead += take;
  float drain = m->oilHead * s->drainK * dt;
  m->oilHead -= drain; m->oilSump += drain;
  if (m->oilHead > s->oilHeadCap) { float ex = (m->oilHead - s->oilHeadCap) * 0.5f * dt; m->oilHead -= ex; m->oilBurn += ex; }
  float burn = (m->running ? 2e-6f * rpm * s->dispL * (1 + (m->health < 60 ? 4 : 0) + m->boost / 12.f) : 0) * dt;
  if (b->tilt < -75 || b->tilt > 75) burn *= 3;
  m->oilSump = fmaxf(0, m->oilSump - burn); m->oilBurn += burn;
  if (m->oilSump > s->oilMax && rpm > 1500) m->tbrake -= (m->oilSump / s->oilMax - 1) * s->dispL * 25 * (rpm / 6000.f) * (rpm / 6000.f);
 }
 // damage
 memset(m->dmgRate, 0, sizeof m->dmgRate);
 if (rpm > 100) {
  m->dmgRate[DMG_KNOCK] = m->knock * 6.f;
  if (rpm > s->pistonRpm) m->dmgRate[DMG_ROD] = (rpm / s->pistonRpm - 1) * 400.f;
  if (b->cycle == 0 && rpm > s->floatRpm * 1.05f) m->dmgRate[DMG_VALVE] = (rpm / (s->floatRpm * 1.05f) - 1) * 600.f;
  if (t.imep > s->pLimit) m->dmgRate[DMG_PRESS] = (t.imep / s->pLimit - 1) * 80.f;
  if (m->temp > s->danger) m->dmgRate[DMG_HEAT] = (m->temp - s->danger) * 0.8f + (m->temp > s->danger + 30 ? 20 : 0);
  if (b->cycle == 0 && b->lube > 0 && m->oilP < 10 && rpm > 2500) m->dmgRate[DMG_OILP] = (10 - m->oilP) * 0.8f;
  if (b->cycle == 0 && b->lube == 0 && m->oilSplash < 0.6f && rpm > 2000) m->dmgRate[DMG_OILP] = (0.6f - m->oilSplash) * 12.f * rpm / 4000.f;
  if (b->cycle == 0 && m->oilT > 150) m->dmgRate[DMG_OILP] += (m->oilT - 150) * 0.5f;
  if (b->cycle == 1 && s->lube < 0.6f) m->dmgRate[DMG_LUBE] = (0.6f - s->lube) * Clampf(t.af, 0.1f, 1) * rpm / 6000.f * 40.f;
  m->dmgRate[DMG_DIRT] = FILTERS[b->filter].dirt * t.af * rpm / 6000.f * (b->filter == 0 ? 2.5f : 0.06f);
  if (b->forced && b->boost > FORCED[b->forced].maxb) m->dmgRate[DMG_TURBO] = m->spool * (b->boost / FORCED[b->forced].maxb - 1) * 25.f;
  if (rpm > FLY[b->flywheel].maxRpm) m->dmgRate[DMG_FLY] = (rpm / FLY[b->flywheel].maxRpm - 1) * 700.f + 3.f;
  if (m->lean > 1.02f && !f->diesel) m->dmgRate[DMG_LEAN] = (m->lean - 1) * 70.f;
  { float vf = s->vibForce * (rpm / 1000.f) * (rpm / 1000.f); if (vf > s->vibLimit) m->dmgRate[DMG_VIB] = (vf / s->vibLimit - 1) * 40.f; }
  if (!b->cycle && b->camDrive == 2 && rpm > 9000) m->dmgRate[DMG_BELT] = (rpm / 9000.f - 1) * 300.f;
 }
 if (!gDev && !m->failed) {
  float tot = 0;
  for (int i = 0; i < DMG_N; i++) { m->dmg[i] += m->dmgRate[i] * dt; tot += m->dmgRate[i]; }
  m->health -= tot * dt;
  if (m->health <= 0) {
   m->health = 0; m->failed = 1; m->dyno = 0;
   int best = 0; for (int i = 1; i < DMG_N; i++) if (m->dmg[i] > m->dmg[best]) best = i;
   m->failCause = best;
  }
 }
 if (m->running) m->runTime += dt; else m->runTime = 0;
 if (m->running && m->thrAct > 0.9f) m->wotTime += dt; else m->wotTime = 0;
 if (rpm > m->maxRpm) m->maxRpm = rpm;
 if (m->dyno) {
  if (m->dynoTarget < m->dynoStart + 250) { m->dSum = m->tbrake; m->dCnt = 0; } else {
   m->dSum += (m->tbrake - m->dSum) * Clampf(dt / (0.03f + 0.25f / b->cyl), 0, 1); m->dCnt++;
   float br = m->dSum;
   if (m->nd < 400 && (m->nd == 0 || rpm - m->drpm[m->nd - 1] >= 50) && m->dCnt > 60) {
    m->drpm[m->nd] = rpm; m->dtq[m->nd] = br; m->dhp[m->nd] = br * rpm / 7121.f; m->nd++;
    if (br * rpm / 7121.f > m->dynoPeakHp) m->dynoPeakHp = br * rpm / 7121.f;
    if (br > m->dynoPeakTq) m->dynoPeakTq = br;
   }
  }
  if (m->dynoTarget >= m->dynoEnd || (!m->running && rpm < 200)) m->dyno = 2;
 }
}
