// ===== parts data, spec derivation, physics =====
typedef struct { const char *name; float knock, energy; int diesel; float cold; const char *tip; } FuelDef;
static const FuelDef FUELS[] = {
 {"Gasoline 87",  11.0f, 1.00f, 0, 1.00f, "Cheap pump gas. Knocks above ~11:1 effective compression."},
 {"Gasoline 93",  12.2f, 1.01f, 0, 1.00f, "Premium pump gas. A bit more knock resistance."},
 {"Race Gas 110", 14.5f, 1.04f, 0, 0.90f, "Leaded race fuel. Lets you run big compression and boost."},
 {"E85",          15.0f, 1.07f, 0, 0.45f, "Ethanol blend. Huge knock resistance, more power. Hard cold starts."},
 {"Methanol",     16.0f, 1.13f, 0, 0.30f, "Alcohol race fuel. Runs cool, makes power. Very hard to cold start."},
 {"Diesel",       99.0f, 1.00f, 1, 1.00f, "Compression ignition: needs 14:1+ compression and injection. Big torque, low rpm."},
 {"Nitromethane",  9.5f, 1.85f, 0, 0.30f, "Carries its own oxygen. Insane power, insane cylinder pressure. Knocks easily."},
};
#define NFUEL 7
static const char *LAYOUTS[] = {"Single","Inline","V 60","V 90","Boxer"};
static const float LAYOUT_HALF[] = {0,0,30,45,90};
static const int CYLOPT[5][8] = {{1},{2,3,4,5,6,8},{2,4,6,8,10,12,16},{2,4,6,8,10,12,16},{2,4,6,8,12}};
static const int CYLOPTN[5] = {1,6,7,7,5};
static const float IMBAL[17] = {0,1.0f,0.7f,0.5f,0.35f,0.25f,0.08f,0,0.15f,0,0.12f,0,0.04f,0,0,0,0.04f};

typedef struct { const char *name; float weight, str, cond, cost; Color col; const char *tip; } BlockDef;
static const BlockDef BLOCKS[] = {
 {"Cast Iron",      1.00f, 1.00f, 0.80f,  300, {78,80,86,255},    "Heavy and tough. Cheap. Holds heat."},
 {"Aluminum",       0.55f, 0.85f, 1.30f,  700, {176,180,186,255}, "Light, sheds heat well, weaker under big cylinder pressure."},
 {"CGI Iron",       0.90f, 1.40f, 0.75f, 1400, {62,68,84,255},    "Compacted graphite iron. Diesel-grade strength."},
 {"Magnesium",      0.40f, 0.65f, 1.20f, 1800, {182,172,138,255}, "Featherweight and fragile. Don't boost it hard."},
 {"Billet Aluminum",0.60f, 1.30f, 1.35f, 6000, {218,222,230,255}, "Machined from solid. Race-team money."},
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
typedef struct { const char *name; float mul, cost; const char *tip; } FlyDef;
static const FlyDef FLY[] = {
 {"Lightweight",0.55f,250,"Revs fast, idles rough, stalls easier."},
 {"Standard",   1.00f,120,"Balanced."},
 {"Heavy",      1.80f,150,"Smooth idle, slow to rev. Easy to pull-start."},
};
typedef struct { const char *name; float flt, rpk, ve, hgt, cost; const char *tip; } VT4Def;
static const VT4Def VT4[] = {
 {"Flathead",    4300, 2600, 0.74f, 0.35f, 100, "Valves in the block. Simple, low-revving, weak breathing."},
 {"OHV Pushrod", 6600, 4300, 0.88f, 0.55f, 300, "Cam in block, pushrods. Compact, torquey, floats valves early."},
 {"SOHC",        8200, 5000, 0.92f, 0.70f, 600, "Single overhead cam. Revs well."},
 {"DOHC",        9800, 5600, 0.96f, 0.85f, 1100,"Twin overhead cams. Best breathing, highest revs."},
};
typedef struct { const char *name; float rpk, ve, cost; const char *tip; } VT2Def;
static const VT2Def VT2[] = {
 {"Piston Port", 6000, 0.80f, 50,  "Piston skirt opens the intake port. Simplest 2-stroke."},
 {"Reed Valve",  6800, 0.86f, 120, "Reed petals stop blow-back. Broad power."},
 {"Rotary Valve",7800, 0.90f, 400, "Rotary disc valve. Peaky, high-revving."},
};
typedef struct { int n; float rpk, ve, flt, cost; } ValveDef;
static const ValveDef VALVES[] = {{2,0.95f,-0.03f,0,0},{3,1.0f,0.0f,200,150},{4,1.06f,0.03f,600,300},{5,1.08f,0.035f,700,500}};
typedef struct { const char *name, *name2; float rpk, width, ve, idle, flt, rough, cost; const char *tip; } CamDef;
static const CamDef CAMS[] = {
 {"Economy","Torque ports",0.75f,1.25f,0.97f,0,  -300,0.00f, 80,"Low-rpm torque, smooth idle."},
 {"Street", "Mild ports",  1.00f,1.00f,1.00f,0,   0,  0.02f,120,"All-rounder."},
 {"Sport",  "Sport ports", 1.18f,0.90f,1.03f,150, 300,0.08f,250,"Pulls harder up top, a little lumpy."},
 {"Race",   "Race ports",  1.40f,0.75f,1.06f,400, 700,0.28f,500,"Peaky top end, loping idle, weak bottom end."},
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
} Build;

static const Build PRESETS[] = {
 {0,0,0,1, 64,50,8.5f, 1,0,2, 1,0,0, 0,2,0,0,0, 0,0,0,0,2,50, "Push Mower 160cc"},
 {1,1,0,1, 66.4f,72,9.0f, 1,2,0, 1,0,3, 0,2,0,0,0, 1,3,0,0,1,40, "Dirt Bike 250 2T"},
 {0,1,1,4, 86,86,9.5f, 1,2,1, 3,2,2, 3,3,1,15,1, 2,1,1,1,1,50, "Hot Hatch 2.0T"},
 {0,1,3,8, 103.25f,92,10.5f, 0,1,1, 1,0,2, 2,1,0,0,0, 2,1,1,1,2,50, "Muscle V8 6.2"},
 {0,5,1,6, 107,124,17.3f, 2,2,2, 1,2,0, 3,1,2,26,1, 2,0,1,1,3,50, "Diesel Truck 6.7"},
 {0,1,2,12, 95,76.4f,12.5f, 1,2,0, 3,2,3, 3,3,0,0,0, 2,2,1,1,0,50, "V12 Exotic 6.5"},
 {0,1,4,4, 85.5f,69,8.5f, 3,0,1, 1,0,1, 0,1,0,0,0, 0,0,1,1,4,50, "Air-cooled Flat-4"},
 {0,6,3,8, 106.4f,114.3f,6.0f, 4,3,0, 1,0,3, 4,0,4,14,0, 2,2,1,0,4,50, "Nitro Dragster"},
};
#define NPRESET 8

typedef struct {
 float vcylL, dispL, disp, rpk, veMax, wLo, wHi, floatRpm, pistonRpm, dieselRpm, redline, inertia;
 float imepPk, fric, pLimit, coolCap, danger, heatC, stall, compDrag, idle, weight, cost, lube;
 int hasBatt, needsElec, limitBy;
 float estHp, estHpRpm, estTq, estTqRpm, estImep;
 int npts; float crpm[160], chp[160], ctq[160];
} Spec;

static float Clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static float Smooth01(float x) { x = Clampf(x, 0, 1); return x * x * (3 - 2 * x); }
static float Lerpf(float a, float b, float t) { return a + (b - a) * t; }

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
 float ch = flow - 2.4f * fd->full;
 if (ch > 0) { float y = ch / fd->full; t /= (1 + y * y); }
 return t;
}

typedef struct { float tind, tfric, tpar, imep, af, knock, dens; } TqOut;

static TqOut EvalTorque(const Build *b, const Spec *s, float rpm, float thr, float boost, float q, float temp, int charging) {
 TqOut o = {0};
 const FuelDef *f = &FUELS[b->fuel];
 float a = Clampf(thr, 0, 1);
 float ref = 0.5f * s->rpk;
 float af = f->diesel ? 1.0f : a / (a + (1 - a) * rpm / ref + 1e-4f);
 float PR = 1 + boost / 14.7f;
 float dens = powf(PR, b->intercooler ? 0.93f : 0.75f);
 float ve = VE(b, s, rpm);
 float load = f->diesel ? (rpm < 450 ? fmaxf(a, 0.4f) : a) : af;
 float imep = s->imepPk * ve * load * dens;
 if (!f->diesel) {
  float eff = b->cr * powf(PR, b->intercooler ? 0.35f : 0.6f) + DELIV[b->delivery].knock + Clampf((temp - s->danger + 25) * 0.03f, 0, 2);
  float ex = eff - f->knock;
  if (ex > 0) { o.knock = ex * powf(af, 1.5f); imep /= (1 + 0.08f * o.knock); }
 }
 imep *= q;
 float k = rpm / 1000.f;
 float fmep = (0.6f + 0.09f * k + 0.025f * k * k) * s->fric * sqrtf(b->stroke / 86.f);
 if (b->cycle == 1) fmep *= 0.7f;
 if (!f->diesel) fmep += (1 - af) * 0.9f;
 float cyc = b->cycle ? 2 * PI : 4 * PI;
 o.tind = imep * 1e5f * s->disp / cyc;
 o.tfric = fmep * 1e5f * s->disp / (4 * PI);
 o.tpar = b->alternator ? (0.8f + (charging ? 1.5f : 0)) : 0;
 if (b->forced == 4) o.tpar += 0.15f * o.tind * (1 - 1 / PR);
 o.imep = imep; o.af = af; o.dens = dens;
 return o;
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
  s->rpk = v->rpk * VALVES[nv].rpk * CAMS[b->cam].rpk * powf(80.f / b->stroke, 0.35f) * powf(0.5f / s->vcylL, 0.12f);
  s->veMax = v->ve + VALVES[nv].ve;
  s->floatRpm = (v->flt + VALVES[nv].flt + CAMS[b->cam].flt) * powf(86.f / b->bore, 0.25f);
 }
 if (f->diesel) s->rpk *= 0.6f;
 s->veMax *= CAMS[b->cam].ve * FILTERS[b->filter].flow * DELIV[b->delivery].flow * (two ? EXH[b->exhaust].flow2 : EXH[b->exhaust].flow4);
 s->wLo = s->rpk * 0.9f * CAMS[b->cam].width; s->wHi = s->rpk * 1.35f * CAMS[b->cam].width;
 if (two) { s->wLo *= 0.8f; s->wHi *= 0.6f; }
 s->pistonRpm = INTERN[b->internals].ps * 60.f / (2.f * b->stroke * 1e-3f);
 s->dieselRpm = f->diesel ? 4600 * powf(90.f / b->bore, 0.35f) : 1e9f;
 float lim = s->pistonRpm; s->limitBy = 0;
 if (s->floatRpm < lim) { lim = s->floatRpm; s->limitBy = 1; }
 if (s->dieselRpm * 1.05f < lim) { lim = s->dieselRpm * 1.05f; s->limitBy = 2; }
 s->redline = floorf(lim * 0.95f / 100.f) * 100.f;
 s->inertia = (0.012f + 0.075f * s->dispL * INTERN[b->internals].mass) * FLY[b->flywheel].mul + 0.004f * b->cyl;
 float th = (1 - powf(b->cr, -0.28f)) / (1 - powf(10, -0.28f));
 s->imepPk = (two ? 10.5f : (f->diesel ? 11.5f : 14.5f)) * th * f->energy;
 s->fric = two ? 0.9f : OILS[b->oil].fric;
 s->pLimit = 27.f * BLOCKS[b->block].str * INTERN[b->internals].str;
 s->coolCap = COOL[b->cooling].cap * powf(s->dispL, 0.85f) * (b->cooling == 0 ? BLOCKS[b->block].cond : 1.f);
 s->danger = COOL[b->cooling].danger + (two ? 0 : OILS[b->oil].hot);
 s->heatC = (9000 * s->dispL + 3000) * (b->cooling ? 1.6f : 1.f);
 s->stall = 30 + s->dispL * b->cr * 5.f;
 s->compDrag = s->dispL * b->cr * 2.0f * (two ? 0.7f : OILS[b->oil].cold);
 s->hasBatt = b->starter == 1 || b->alternator;
 s->needsElec = DELIV[b->delivery].elec;
 s->idle = two ? 1500 : (f->diesel ? 750 : 750 + CAMS[b->cam].idle);
 s->lube = two ? (b->mix < 1 ? 0 : 50.f / b->mix) : 1;
 int nb = (b->layout >= 2) ? 2 : 1;
 float vtw = two ? 0.5f : (b->valvetrain == 0 ? 0.7f : (b->valvetrain == 1 ? 0.9f : (b->valvetrain == 2 ? 1.0f : 1.15f)));
 s->weight = (14 + 32 * s->dispL + 3 * b->cyl) * BLOCKS[b->block].weight
  + (5 + 8 * s->dispL) * vtw * nb * 0.7f + (2 + 7 * s->dispL) * INTERN[b->internals].mass
  + (2 + 5 * powf(s->dispL, 0.7f)) * FLY[b->flywheel].mul
  + (b->forced == 3 ? 18 : (b->forced == 4 ? 20 : (b->forced ? 9 : 0))) + (b->forced && b->intercooler ? 6 : 0)
  + (b->alternator ? 6 : 0) + (b->starter ? 6 : 1.5f) + (b->cooling == 1 ? 12 : (b->cooling == 2 ? 20 : 0))
  + 3 + 4 * s->dispL + (two ? 0 : 3 + 1.5f * s->dispL);
 s->cost = 400 + 250 * s->dispL + 120 * b->cyl;
 s->cost += BLOCKS[b->block].cost * (0.5f + s->dispL * 0.5f) + INTERN[b->internals].cost * (0.4f + b->cyl * 0.15f) + FLY[b->flywheel].cost;
 s->cost += two ? VT2[b->valvetrain].cost : (VT4[b->valvetrain].cost * nb + VALVES[b->valvetrain ? b->valves : 0].cost * b->cyl * 0.25f);
 s->cost += CAMS[b->cam].cost * nb + DELIV[b->delivery].cost * (b->delivery >= 2 ? 0.4f + 0.15f * b->cyl : 1) + FILTERS[b->filter].cost;
 s->cost += FORCED[b->forced].cost + (b->forced && b->intercooler ? 450 : 0) + COOL[b->cooling].cost + EXH[b->exhaust].cost;
 s->cost += (b->starter ? 180 : 40) + (b->alternator ? 160 : 0) + (b->fuel == 6 ? 900 : 0);
 // dyno estimate
 float top = fmaxf(s->redline * 1.0f, 2000);
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
}

static int BuildWarnings(const Build *b, const Spec *s, char out[][96], int *sev, int max) {
 int n = 0;
 const FuelDef *f = &FUELS[b->fuel];
#define W(sv, ...) do { if (n < max) { snprintf(out[n], 96, __VA_ARGS__); sev[n] = sv; n++; } } while (0)
 if (f->diesel && b->cr < 14) W(2, "Diesel needs 14:1+ compression to self-ignite.");
 else if (f->diesel && b->cr < 16) W(1, "Low diesel compression: hard cold starts.");
 if (f->diesel && b->delivery != 3 && b->delivery != 4) W(2, "Diesel needs direct or mechanical injection.");
 if (s->needsElec && !s->hasBatt) W(2, "EFI needs power: add an alternator or electric start.");
 if (b->starter == 1 && !b->alternator) W(1, "No alternator: the battery will drain.");
 if (b->starter == 0 && s->compDrag > 11.f) W(2, "Too much compression to pull-start. Go electric.");
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
 if (b->forced && b->forced != 4 && b->boost > FORCED[b->forced].maxb) W(2, "%.0f psi is past this turbo's limit (%.0f): overspeed.", b->boost, FORCED[b->forced].maxb);
 if (b->forced == 4 && b->boost > FORCED[4].maxb) W(2, "Blower can't make %.0f psi safely.", b->boost);
 if (b->forced && !b->intercooler && b->boost > 8 && !f->diesel) W(1, "No intercooler: hot charge, more knock.");
 if (s->estHp * 745.7f * 0.85f > s->coolCap * 1.1f) W(1, "Cooling is marginal for sustained full load.");
 if (b->filter == 0) W(1, "No air filter: dirt will score the cylinders.");
 if (b->cycle == 0 && b->exhaust == 3) W(1, "Expansion chambers only help 2-strokes.");
 if (b->fuel == 6 && b->delivery < 3) W(1, "Nitro really wants injection.");
 if (b->cycle == 0 && fabsf(b->tilt) > 50) W(1, "Cylinders laid over %.0f deg: wet sump drains badly, oil pressure drops.", fabsf(b->tilt));
 if (b->layout >= 2 && fabsf(b->tilt) > 30) W(1, "Tilting a V/boxer that far puts a bank near the floor.");
#undef W
 return n;
}

// ===== live simulation =====
enum { DMG_KNOCK, DMG_ROD, DMG_VALVE, DMG_PRESS, DMG_HEAT, DMG_OILP, DMG_LUBE, DMG_DIRT, DMG_TURBO, DMG_N };
static const char *DMG_TITLE[] = {"Holed piston","Thrown rod","Dropped valve","Blown head gasket","Seized engine","Spun bearing","Seized piston","Scored cylinders","Turbo grenade"};

typedef struct {
 float omega, angle, rpm;
 float thrCmd, thrAct, idleOpen;
 float spool, boost, temp, oilP, egt, batt, volts;
 float health, dmg[DMG_N], dmgRate[DMG_N];
 int ign, choke, limiter, failed, failCause, cut, running, starterOn, charging;
 float recoilT, q, knock, tind, tbrake, load, rough, roughT, crankTime, runTime, wotTime, prevThr;
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
 m->temp = 20; m->batt = 0.85f; m->health = 100; m->limiter = 1; m->rng = 0x1234567u; m->oilP = 0; m->egt = 20;
 if (keepRuns) { m->npd = old.npd; memcpy(m->pdrpm, old.pdrpm, sizeof m->pdrpm); memcpy(m->pdhp, old.pdhp, sizeof m->pdhp); memcpy(m->pdtq, old.pdtq, sizeof m->pdtq); }
 (void)s;
}

static int gDev = 0;

static void SimStep(Sim *m, const Build *b, const Spec *s, float dt, int starterHeld) {
 const FuelDef *f = &FUELS[b->fuel];
 float rpm = m->omega * 60.f / (2 * PI);
 m->rpm = rpm;
 float warm = Clampf((m->temp - 20) / 30.f, 0, 1);
 // electrics
 m->charging = b->alternator && rpm > 1100;
 m->starterOn = starterHeld && b->starter == 1 && !m->failed;
 float v = 11.6f + 1.0f * m->batt;
 if (m->starterOn) v -= 1.0f + (1 - m->batt) * 2.5f;
 if (m->charging) v = fmaxf(v, 14.2f);
 if (!s->hasBatt) v = 0;
 m->volts = v;
 if (s->hasBatt && !gDev) {
  float d = 0;
  if (m->starterOn) d -= 0.02f;
  if (m->ign && s->needsElec) d -= 0.0012f;
  if (m->ign) d -= 0.0003f;
  if (m->charging) d += 0.006f;
  m->batt = Clampf(m->batt + d * dt, 0, 1);
 }
 // combustion quality
 float q = 1;
 if (!m->ign || m->failed) q = 0;
 if (f->diesel) {
  if (b->delivery != 3 && b->delivery != 4) q = 0;
  if (b->cr < 14) q = 0;
  else { float cold = Clampf((b->cr - 14) / 4.f + (s->hasBatt && m->volts > 10 ? 0.35f : 0), 0, 1); q *= Lerpf(cold, 1, warm); }
 } else {
  if (b->delivery == 0) {
   if (!m->choke) q *= Lerpf(0.18f, 1, warm);
   else q *= Lerpf(1, 0.6f, Clampf((m->temp - 50) / 20.f, 0, 1));
  } else q *= Lerpf(0.85f, 1, warm);
  q *= Lerpf(f->cold, 1, warm);
 }
 if (s->needsElec && (!s->hasBatt || m->volts < 9.f)) q = 0;
 if (b->cycle == 1 && s->lube > 2.2f) q *= Clampf(1 - 0.25f * (s->lube - 2.2f), 0.4f, 1);
 if (rpm < 90) q = 0;
 if (m->limiter) { if (rpm > s->redline) m->cut = 1; else if (rpm < s->redline - 180) m->cut = 0; } else m->cut = 0;
 if (m->cut) q = 0;
 m->q = q;
 m->running = q > 0.05f && rpm > 150;
 // throttle
 float lag = DELIV[b->delivery].lag;
 float tc = m->thrCmd;
 if (m->dyno) tc = 1;
 m->thrAct += (tc - m->thrAct) * Clampf(dt / lag, 0, 1);
 if (m->running) m->idleOpen += (s->idle - rpm) * 0.00004f * dt * (b->delivery == 0 ? 0.6f : 1.0f) * 60.f;
 m->idleOpen = Clampf(m->idleOpen, 0.0f, 0.3f);
 if (!m->running && rpm < 150) m->idleOpen = Clampf(m->idleOpen, 0.06f, 0.3f);
 float thr = fmaxf(m->thrAct, m->idleOpen);
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
 TqOut t = EvalTorque(b, s, rpm, thr, boost, q, m->temp, m->charging);
 if (m->choke && m->temp > 50) t.tind *= 0.8f;
 // idle lope
 m->roughT -= dt;
 if (m->roughT <= 0) { m->roughT = 0.025f + 0.04f * SRand(m); m->rough = SRand(m) * 2 - 1; }
 float rough = CAMS[b->cam].rough * Clampf(1 - rpm / 2800.f, 0, 1) + (FLY[b->flywheel].mul < 0.7f ? 0.04f : 0);
 t.tind *= 1 + rough * m->rough;
 m->tind = t.tind; m->knock = t.knock;
 m->tbrake = t.tind - t.tfric - t.tpar;
 m->load = Clampf(t.tind / fmaxf(s->estTq * 1.1f + 1, 1), 0, 1.5f);
 // starter
 float tst = 0;
 if (m->starterOn && rpm < 450) {
  float vf = Clampf((m->volts - 8.8f) / 2.f, 0, 1);
  tst = s->stall * fmaxf(0, 1 - rpm / 350.f) * vf;
 }
 if (m->recoilT > 0) { m->recoilT -= dt; tst += 12.f * fmaxf(0, 1 - rpm / 700.f); }
 // compression
 int ncomp = b->cycle ? b->cyl : (b->cyl + 1) / 2;
 float cf = Clampf(1 - rpm / 600.f, 0, 1);
 float tcomp = s->compDrag * cf * (0.55f + 0.6f / sqrtf((float)b->cyl) * sinf(m->angle * PI / 180.f * ncomp * 2.f));
 float coldF = b->cycle ? 1 : Lerpf(OILS[b->oil].cold, 1, warm);
 float tf = t.tfric * coldF;
 // dyno absorber
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
  if (m->recoilT > 0 && b->starter == 0) m->recoilStuck = 1;
 } else {
  m->omega += (drive - resist) / s->inertia * dt;
  if (m->omega < 0) m->omega = 0;
 }
 m->angle = fmodf(m->angle + rpm * 6.f * dt, 720.f);
 // thermal
 float Q = (rpm > 100 && q > 0) ? fmaxf(0, t.tind) * m->omega * 0.9f + 1200 * powf(s->dispL, 0.8f) + (m->temp < 75 ? 12000 * powf(s->dispL, 0.8f) : 0) : tf * m->omega * 0.3f;
 float h;
 if (b->cooling == 0) h = s->coolCap / 150.f * (0.4f + 0.6f * Clampf(rpm / 4000.f, 0, 1));
 else h = s->coolCap / 70.f * Clampf((m->temp - 82) / 10.f, 0.06f, 1.f) * (0.5f + 0.5f * Clampf(rpm / 2000.f, 0, 1));
 m->temp += (Q - h * (m->temp - 20)) / s->heatC * dt;
 float egtT = 20 + (m->running ? 250 + 650 * Clampf(t.af * rpm / fmaxf(s->redline, 1), 0, 1.2f) * (0.5f + 0.5f * m->load) : 0);
 m->egt += (egtT - m->egt) * Clampf(dt * 0.8f, 0, 1);
 if (b->cycle == 0) {
  float vf = OILS[b->oil].v * expf(-(m->temp - 90) / 45.f);
  m->oilP = Clampf((8 + rpm * 0.011f) * vf, 0, 80) * (rpm > 50 ? 1 : 0);
  float tl = fabsf(b->tilt);
  if (tl > 50) m->oilP *= 1 - (tl - 50) / 90.f * Clampf(rpm / 6000.f, 0.3f, 1.f);
 }
 // damage
 memset(m->dmgRate, 0, sizeof m->dmgRate);
 if (rpm > 100) {
  m->dmgRate[DMG_KNOCK] = m->knock * 6.f;
  if (rpm > s->pistonRpm) m->dmgRate[DMG_ROD] = (rpm / s->pistonRpm - 1) * 400.f;
  if (b->cycle == 0 && rpm > s->floatRpm * 1.05f) m->dmgRate[DMG_VALVE] = (rpm / (s->floatRpm * 1.05f) - 1) * 600.f;
  if (t.imep > s->pLimit) m->dmgRate[DMG_PRESS] = (t.imep / s->pLimit - 1) * 80.f;
  if (m->temp > s->danger) m->dmgRate[DMG_HEAT] = (m->temp - s->danger) * 0.8f + (m->temp > s->danger + 30 ? 20 : 0);
  if (b->cycle == 0 && m->oilP < 10 && rpm > 2500) m->dmgRate[DMG_OILP] = (10 - m->oilP) * 0.8f;
  if (b->cycle == 1 && s->lube < 0.6f) m->dmgRate[DMG_LUBE] = (0.6f - s->lube) * Clampf(t.af, 0.1f, 1) * rpm / 6000.f * 40.f;
  m->dmgRate[DMG_DIRT] = FILTERS[b->filter].dirt * t.af * rpm / 6000.f * 2.5f;
  if (b->forced && b->boost > FORCED[b->forced].maxb) m->dmgRate[DMG_TURBO] = m->spool * (b->boost / FORCED[b->forced].maxb - 1) * 25.f;
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
 // stats
 if (m->running) m->runTime += dt; else m->runTime = 0;
 if (m->running && m->thrAct > 0.9f) m->wotTime += dt; else m->wotTime = 0;
 if (rpm > m->maxRpm) m->maxRpm = rpm;
 // dyno recording
 if (m->dyno) {
  if (m->dynoTarget < m->dynoStart + 250) { m->dSum = 0; m->dCnt = 0; } else {
  m->dSum += m->tbrake; m->dCnt++;
  float br = m->dSum / m->dCnt;
  if (m->nd < 400 && (m->nd == 0 || rpm - m->drpm[m->nd - 1] >= 50)) {
   m->dSum = 0; m->dCnt = 0;
   m->drpm[m->nd] = rpm; m->dtq[m->nd] = br; m->dhp[m->nd] = br * rpm / 7121.f; m->nd++;
   if (br * rpm / 7121.f > m->dynoPeakHp) m->dynoPeakHp = br * rpm / 7121.f;
   if (br > m->dynoPeakTq) m->dynoPeakTq = br;
  }
  }
  if (m->dynoTarget >= m->dynoEnd || (!m->running && rpm < 200)) m->dyno = 2;
 }
}
