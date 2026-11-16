#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include "font_reg.h"
#include "font_bold.h"
#include "sim.c"
#include "audio.c"
#include "render.c"

static Build gB; static Spec gS; static Sim gM;
static int gMode = 0, gTab = 0, gPreset = 3, gSlot = 0, gShowAch = 0;
static float camYaw = 0.75f, camPitch = 0.32f, camDist = 8, camDistT = 8;
static int gDragging = 0, gUiHover = 0, gUiHoverPrev = 0;
static DrawState gDS;
static float gVisAng = 0, gTime = 0, gRoll = 0, gCrankT = 0, gPullAnim = 0;
static char gWarn[16][96]; static int gWarnSev[16], gNWarn = 0;
static int gAutotest = 0; static char gShotDir[256] = ".";
static int gDiagShown = 0, gWasRunning = 0, gStartedOnce = 0;
static float gVolume = 0.85f, gMusicVol = 0.6f; static int gMuted = 0, gMusicOn = 1;
static char gDevBuf[8] = {0};

static Font fR, fB;
static Vector2 MP; static int MPress, MDown, MRel, gActive = -1;
static const char *gTip = NULL;
static const Color COL_BG = {17, 19, 23, 255}, COL_PANEL = {27, 30, 36, 255}, COL_PANEL2 = {38, 42, 50, 255}, COL_LINE = {54, 59, 68, 255};
static const Color COL_TXT = {232, 234, 238, 255}, COL_DIM = {140, 146, 158, 255}, COL_ACC = {255, 138, 28, 255}, COL_RED = {235, 70, 55, 255};
static const Color COL_GRN = {90, 205, 115, 255}, COL_YEL = {240, 196, 60, 255}, COL_BLUE = {80, 160, 255, 255};

// ===== toasts =====
static int gToastQuiet = 0;
typedef struct { int magic, invertX, invertY, blur, fps; float sens, zoom, master, eng, music, sfx, smoke; } Settings;
static Settings gSet = {0x53455431, 0, 0, 1, 0, 1.f, 1.f, 0.85f, 1.f, 0.6f, 1.f, 10.f};
static int gShowSettings = 0;
typedef struct { char s[128]; float t; Color c; } Toast;
static Toast gToast[6];
static void ToastAdd(const char *s, Color c) {
 for (int i = 5; i > 0; i--) gToast[i] = gToast[i - 1];
 snprintf(gToast[0].s, 128, "%s", s); gToast[0].t = 4.5f; gToast[0].c = c;
 if (!gToastQuiet) SfxPush(c.r == 235 && c.g == 70 ? SFX_WARN : SFX_POP);
 gToastQuiet = 0;
}

// ===== saves + challenges =====
typedef struct { int magic, ver; Build slot[4]; int used[4]; unsigned ach; } SaveFile;
static SaveFile gSave;
static const char *ACH_NAME[] = {"Lawnmower Man","Ring Ding Ding","Breathing Easy","Torque Monster","Thousand Horse Club","Screamer","Oil Burner","Kaboom","Popeye Arms","Marathon","Featherweight","Bulletproof"};
static const char *ACH_DESC[] = {
 "Run a single-cylinder with recoil start and no alternator",
 "Spin a 2-stroke past 10,000 rpm",
 "100+ hp per liter on the dyno, no turbo or blower",
 "800+ lb-ft on the dyno",
 "1000+ hp on the dyno",
 "Spin any engine past 12,000 rpm",
 "A diesel making 400+ hp on the dyno",
 "Destroy an engine",
 "Pull-start an engine of 0.8 L or more",
 "Hold 90%+ throttle for 60 seconds without breaking",
 "1+ hp per kg of engine weight on the dyno",
 "500+ hp dyno pull finished with 99%+ health",
};
#define NACH 12
static const char *SavePath(void) { return TextFormat("%sengine_garage.sav", GetApplicationDirectory()); }
static void SaveWrite(void) { gSave.magic = 0x454E4731; gSave.ver = 1; SaveFileData(SavePath(), &gSave, sizeof gSave); }
static void SaveRead(void) {
 int sz = 0; unsigned char *d = LoadFileData(SavePath(), &sz);
 memset(&gSave, 0, sizeof gSave);
 if (d && sz >= 12 && *(int *)d == 0x454E4731) {
  int bs = (sz - 8 - 16 - 4) / 4;
  if (bs > 0 && bs <= (int)sizeof(Build) && 8 + 4 * bs + 16 + 4 == sz) {
   for (int i = 0; i < 4; i++) memcpy(&gSave.slot[i], d + 8 + i * bs, bs);
   memcpy(gSave.used, d + 8 + 4 * bs, 16); memcpy(&gSave.ach, d + 8 + 4 * bs + 16, 4);
   gSave.magic = 0x454E4731;
   if (bs <= (int)offsetof(Build, lube)) for (int i = 0; i < 4; i++) if (!gSave.slot[i].cycle) { gSave.slot[i].lube = 1; gSave.slot[i].oilL = 1.f + 0.6f * gSave.slot[i].bore * gSave.slot[i].bore * gSave.slot[i].stroke * gSave.slot[i].cyl * 7.85e-7f; }
  }
 }
 if (d) UnloadFileData(d);
}
static const char *CfgPath(void) { return TextFormat("%sengine_garage.cfg", GetApplicationDirectory()); }
static void SettingsSave(void) { SaveFileData(CfgPath(), &gSet, sizeof gSet); }
static void SettingsLoad(void) { int sz = 0; unsigned char *d = LoadFileData(CfgPath(), &sz); if (d && sz == (int)sizeof gSet && ((Settings *)d)->magic == gSet.magic) memcpy(&gSet, d, sizeof gSet); if (d) UnloadFileData(d); }
static void Ach(int id) {
 if (gSave.ach & (1u << id)) return;
 gSave.ach |= 1u << id; SaveWrite();
 gToastQuiet = 1; ToastAdd(TextFormat("CHALLENGE COMPLETE: %s", ACH_NAME[id]), COL_YEL); SfxPush(SFX_ACH);
}
static int AchCount(void) { int n = 0; for (int i = 0; i < NACH; i++) if (gSave.ach & (1u << i)) n++; return n; }

// ===== text + widgets =====
static void Tx(const char *s, float x, float y, float sz, Color c) { DrawTextEx(fR, s, (Vector2){roundf(x), roundf(y)}, sz, 0, c); }
static void Tb(const char *s, float x, float y, float sz, Color c) { DrawTextEx(fB, s, (Vector2){roundf(x), roundf(y)}, sz, 0, c); }
static float Tw(const char *s, float sz, int bold) { return MeasureTextEx(bold ? fB : fR, s, sz, 0).x; }
static void TxC(const char *s, float cx, float y, float sz, Color c, int bold) { float w = Tw(s, sz, bold); (bold ? Tb : Tx)(s, cx - w * 0.5f, y, sz, c); }
static void TxR(const char *s, float rx, float y, float sz, Color c, int bold) { float w = Tw(s, sz, bold); (bold ? Tb : Tx)(s, rx - w, y, sz, c); }
static void Panel(Rectangle r, Color c) { DrawRectangleRounded(r, 0.04f, 8, c); if (CheckCollisionPointRec(MP, r)) gUiHover = 1; }
static void PanelR(Rectangle r, float rd, Color c) { DrawRectangleRounded(r, rd, 8, c); if (CheckCollisionPointRec(MP, r)) gUiHover = 1; }

static int UiButton(Rectangle r, const char *l, int style, int enabled) {
 int hov = CheckCollisionPointRec(MP, r);
 if (hov) gUiHover = 1;
 hov = hov && enabled;
 Color bg;
 if (style == 1) bg = hov ? (Color){255, 166, 70, 255} : COL_ACC;
 else if (style == 2) bg = hov ? (Color){110, 220, 140, 255} : COL_GRN;
 else if (style == 3) bg = hov ? (Color){250, 100, 85, 255} : COL_RED;
 else bg = hov ? COL_LINE : COL_PANEL2;
 if (!enabled) bg = (Color){36, 39, 45, 255};
 DrawRectangleRounded(r, 0.25f, 8, bg);
 Color tc = (style && enabled) ? (Color){18, 18, 20, 255} : (enabled ? COL_TXT : (Color){90, 95, 105, 255});
 float sz = r.height > 40 ? 19 : 16;
 TxC(l, r.x + r.width * 0.5f, r.y + (r.height - sz) * 0.5f - 1, sz, tc, 1);
 if (hov && MPress) SfxPush(SFX_CLICK);
 return hov && MPress;
}

static int gSelStatus = -1;
static int gEditName = 0, gShowOverview = 0;
static int UiSelect(Rectangle r, const char *label, const char *val, const char *tip, int enabled) {
 int hov = CheckCollisionPointRec(MP, r);
 if (hov) { gUiHover = 1; if (tip) gTip = tip; }
 DrawRectangleRounded(r, 0.18f, 8, hov && enabled ? COL_PANEL2 : COL_PANEL);
 Tx(label, r.x + 12, r.y + (r.height - 17) * 0.5f, 17, enabled ? COL_DIM : (Color){80, 85, 95, 255});
 if (gSelStatus >= 0 && enabled) { Color sc = gSelStatus == 0 ? COL_GRN : (gSelStatus == 1 ? COL_YEL : COL_RED); DrawCircleV((Vector2){r.x + r.width * 0.40f - 12, r.y + r.height * 0.5f}, 5, sc); }
 gSelStatus = -1;
 Rectangle vb = {r.x + r.width * 0.40f, r.y + 5, r.width * 0.60f - 6, r.height - 10};
 DrawRectangleRounded(vb, 0.3f, 8, (Color){19, 21, 26, 255});
 Rectangle lA = {vb.x, vb.y, 30, vb.height}, rA = {vb.x + vb.width - 30, vb.y, 30, vb.height};
 int hl = enabled && CheckCollisionPointRec(MP, lA), hr = enabled && CheckCollisionPointRec(MP, rA);
 Color ac = enabled ? COL_ACC : (Color){70, 74, 82, 255};
 Tb("<", lA.x + 10, vb.y + (vb.height - 17) * 0.5f, 17, hl ? COL_TXT : ac);
 Tb(">", rA.x + 11, vb.y + (vb.height - 17) * 0.5f, 17, hr ? COL_TXT : ac);
 float sz = 16;
 while (Tw(val, sz, 1) > vb.width - 64 && sz > 11) sz -= 1;
 TxC(val, vb.x + vb.width * 0.5f, vb.y + (vb.height - sz) * 0.5f - 1, sz, enabled ? COL_TXT : (Color){90, 95, 105, 255}, 1);
 int ch = 0;
 if (enabled && MPress && hov) { if (hl) ch = -1; else if (hr || CheckCollisionPointRec(MP, vb)) ch = 1; }
 if (enabled && hov) { float w = GetMouseWheelMove(); if (w > 0) ch = -1; else if (w < 0) ch = 1; }
 if (ch) SfxPush(SFX_SELECT);
 return ch;
}

static int UiSlider(int id, Rectangle r, const char *label, float *v, float lo, float hi, float step, const char *valtxt, const char *tip, int enabled) {
 int hov = CheckCollisionPointRec(MP, r);
 if (hov) { gUiHover = 1; if (tip) gTip = tip; }
 DrawRectangleRounded(r, 0.18f, 8, hov && enabled ? COL_PANEL2 : COL_PANEL);
 Tx(label, r.x + 12, r.y + (r.height - 17) * 0.5f, 17, enabled ? COL_DIM : (Color){80, 85, 95, 255});
 Rectangle bar = {r.x + r.width * 0.40f + 4, r.y + r.height * 0.5f - 3, r.width * 0.60f - 104, 6};
 float t = Clampf((*v - lo) / (hi - lo), 0, 1);
 DrawRectangleRounded(bar, 1, 6, (Color){19, 21, 26, 255});
 DrawRectangleRounded((Rectangle){bar.x, bar.y, bar.width * t, bar.height}, 1, 6, enabled ? COL_ACC : (Color){70, 74, 82, 255});
 DrawCircleV((Vector2){bar.x + bar.width * t, bar.y + 3}, 8, enabled ? COL_TXT : (Color){90, 95, 105, 255});
 TxR(valtxt, r.x + r.width - 10, r.y + (r.height - 16) * 0.5f - 1, 16, enabled ? COL_TXT : (Color){90, 95, 105, 255}, 1);
 int changed = 0;
 Rectangle hit = {bar.x - 10, r.y, bar.width + 20, r.height};
 if (enabled && MPress && CheckCollisionPointRec(MP, hit)) gActive = id;
 float old = *v;
 if (gActive == id && MDown) {
  float tt = Clampf((MP.x - bar.x) / bar.width, 0, 1);
  *v = lo + tt * (hi - lo);
 }
 if (enabled && hov && gActive < 0) { float w = GetMouseWheelMove(); if (w != 0) *v += (w > 0 ? step : -step) * (IsKeyDown(KEY_LEFT_SHIFT) ? 10 : 1); }
 *v = Clampf(roundf(*v / step) * step, lo, hi);
 if (*v != old) { changed = 1; static double lt = 0; if (GetTime() - lt > 0.04) { SfxPush(SFX_TICK); lt = GetTime(); } }
 return changed;
}

static char gPartTip[4][256]; static int gPartTipI = 0;
static const char *PartTip(int cat, const char *base) {
 char why[160]; int st = PartStatus(&gB, &gS, cat, why, 160);
 gSelStatus = st;
 char *o = gPartTip[gPartTipI++ & 3];
 snprintf(o, 256, "%s %s%s", base, st == 3 ? "DOESN'T FIT: " : (st == 2 ? "UNDER-RATED: " : (st == 1 ? "Heads up: " : "")), why);
 return o;
}
static int Cyc(int v, int d, int n) { v += d; if (v < 0) v = n - 1; if (v >= n) v = 0; return v; }

static const char *CustomKind(const Build *b, int shortForm) {
 float ang[16]; int na = 0, slots = 0, used[16] = {0};
 for (int i = 0; i < b->ncust; i++) {
  int f = 0; for (int k = 0; k < na; k++) if (fabsf(remainderf(ang[k] - b->cAng[i], 360)) < 2) f = 1;
  if (!f) ang[na++] = b->cAng[i];
  if (!used[b->cSlot[i] & 15]) { used[b->cSlot[i] & 15] = 1; slots++; }
 }
 if (b->ncust <= 1) return "Single";
 if (na == 1) return fabsf(ang[0]) > 120 ? (shortForm ? "Inv-I" : "Inverted inline") : (shortForm ? "I" : "Inline");
 if (slots == 1 && na >= 3) return "Radial";
 if (na == 2) return fabsf(fabsf(remainderf(ang[0] - ang[1], 360)) - 180) < 2 ? (shortForm ? "Flat-" : "Boxer") : "V";
 if (na == 3) return "W";
 if (na == 4) return "X";
 return shortForm ? "Cust" : "Custom";
}
// ===== build helpers =====
static void BuildName(const Build *b, char *out, int n) {
 if (b->name[0]) { snprintf(out, n, "%s", b->name); return; }
 for (int i = 0; i < NPRESET; i++) {
  Build p = PRESETS[i];
  memcpy(p.name, b->name, sizeof p.name);
  if (!memcmp(&p, b, sizeof p)) { snprintf(out, n, "%s", PRESETS[i].name); return; }
 }
 Spec s; ComputeSpec(b, &s);
 const char *lay = b->layout == 5 ? CustomKind(b, 1) : (b->layout == 0 ? "Single" : (b->layout == 1 ? "I" : (b->layout == 4 ? "Flat-" : "V")));
 char cyl[16]; if (b->layout == 0 || (b->layout == 5 && b->ncust <= 1)) snprintf(cyl, 16, "Single"); else snprintf(cyl, 16, lay[strlen(lay) - 1] >= 'a' ? "%s-%d" : "%s%d", lay, b->cyl);
 snprintf(out, n, "Custom %s %.1fL%s%s", cyl, s.dispL, b->forced ? (b->forced == 4 ? " Blown" : " Turbo") : "", b->cycle ? " 2T" : (FUELS[b->fuel].diesel ? " Diesel" : ""));
}
static void FixBuild(Build *b) {
 if (b->layout == 5) { if (b->ncust > 16) b->ncust = 16; b->cyl = b->ncust < 1 ? 1 : b->ncust; if (b->forced == 0) b->intercooler = 0; if (b->cycle == 1 && b->valvetrain > 2) b->valvetrain = 2; return; }
 if (b->cycle == 1) { if (b->valvetrain > 2) b->valvetrain = 2; }
 else if (b->exhaust == 3) b->exhaust = 1;
 int ok = 0;
 if (b->layout > 5) b->layout = 0;
 for (int i = 0; i < CYLOPTN[b->layout]; i++) if (CYLOPT[b->layout][i] == b->cyl) ok = 1;
 if (!ok) {
  int best = CYLOPT[b->layout][0];
  for (int i = 0; i < CYLOPTN[b->layout]; i++) if (abs(CYLOPT[b->layout][i] - b->cyl) < abs(best - b->cyl)) best = CYLOPT[b->layout][i];
  b->cyl = best;
 }
 if (b->forced == 0) b->intercooler = 0;
}
static float gFocus = 0;
static void OnBuildChanged(void) {
 FixBuild(&gB);
 ComputeSpec(&gB, &gS);
 AudioSetupEngine(&gB, &gS);
 GeoCompute(&gB, &gS);
 OilPathsBuild(&gB);
 gNWarn = BuildWarnings(&gB, &gS, gWarn, gWarnSev, 16);
 if (G.attachErr[0] && gNWarn < 16) {
  for (int i = gNWarn; i > 0; i--) { memcpy(gWarn[i], gWarn[i - 1], 96); gWarnSev[i] = gWarnSev[i - 1]; }
  snprintf(gWarn[0], 96, "DOESN'T ATTACH: %s", G.attachErr); gWarnSev[0] = 3; gNWarn++;
 }
 if (G.solveLeft) {
  int ba = -1, bb = -1, bv = 0;
  for (int a = 0; a < G_N; a++) for (int c = 0; c < G_N; c++) if (gConf[a][c] + gConf[c][a] > bv) { bv = gConf[a][c] + gConf[c][a]; ba = a; bb = c; }
  if (ba >= 0 && gNWarn < 16) {
   for (int i = gNWarn; i > 0; i--) { memcpy(gWarn[i], gWarn[i - 1], 96); gWarnSev[i] = gWarnSev[i - 1]; }
   snprintf(gWarn[0], 96, "%s: the %s runs into the %s. %s", gB.placeMan ? "CAN'T MOUNT" : "Packaging", GROUP_NAME[ba], GROUP_NAME[bb], gB.placeMan ? "Move it." : "Try another layout or part.");
   gWarnSev[0] = gB.placeMan ? 3 : 1; gNWarn++;
  }
 }
}

// ===== gauges =====
static void Gauge(float cx, float cy, float R, float v, float lo, float hi, float red, const char *label, const char *vtxt0, int majors, float labDiv, Color needle) {
 char vtxt[32]; snprintf(vtxt, 32, "%s", vtxt0);
 DrawCircleV((Vector2){cx, cy}, R + 8, (Color){12, 13, 16, 235});
 DrawRing((Vector2){cx, cy}, R + 6, R + 8, 0, 360, 72, COL_LINE);
 float a0 = 135, a1 = 405;
 if (red < hi) { float ar = a0 + Clampf((red - lo) / (hi - lo), 0, 1) * (a1 - a0); DrawRing((Vector2){cx, cy}, R - 9, R - 2, ar, a1, 48, (Color){200, 50, 40, 255}); }
 for (int i = 0; i <= majors * 5; i++) {
  float f = (float)i / (majors * 5), an = (a0 + f * (a1 - a0)) * DEG2RAD;
  float l = (i % 5 == 0) ? R * 0.14f : R * 0.07f;
  Vector2 p0 = {cx + cosf(an) * (R - 2), cy + sinf(an) * (R - 2)}, p1 = {cx + cosf(an) * (R - 2 - l), cy + sinf(an) * (R - 2 - l)};
  DrawLineEx(p0, p1, i % 5 == 0 ? 2.5f : 1.2f, i % 5 == 0 ? COL_TXT : COL_DIM);
  if (i % 5 == 0) {
   float val = lo + f * (hi - lo);
   const char *t = TextFormat("%.0f", val / labDiv);
   float sz = R > 100 ? 17 : 13;
   Vector2 tp = {cx + cosf(an) * (R * 0.70f), cy + sinf(an) * (R * 0.70f)};
   TxC(t, tp.x, tp.y - sz * 0.5f, sz, COL_TXT, 0);
  }
 }
 float vf = Clampf((v - lo) / (hi - lo), -0.02f, 1.04f);
 float an = (a0 + vf * (a1 - a0)) * DEG2RAD;
 DrawLineEx((Vector2){cx - cosf(an) * R * 0.12f, cy - sinf(an) * R * 0.12f}, (Vector2){cx + cosf(an) * (R - 10), cy + sinf(an) * (R - 10)}, R > 100 ? 4 : 3, needle);
 DrawCircleV((Vector2){cx, cy}, R > 100 ? 10 : 7, COL_PANEL2);
 TxC(label, cx, cy - R * 0.40f, R > 100 ? 14 : 11, COL_DIM, 0);
 TxC(vtxt, cx, cy + R * (R > 100 ? 0.42f : 0.36f), R > 100 ? 28 : 18, COL_TXT, 1);
}

static void DynoGraph(Rectangle r, int n, const float *rpm, const float *hp, const float *tq, int pn, const float *prpm, const float *php, const float *ptq, float redline, int small) {
 float xmax = fmaxf(1000, ceilf(redline * 1.08f / 1000) * 1000);
 float ymax = 10;
 for (int i = 0; i < n; i++) { ymax = fmaxf(ymax, hp[i]); ymax = fmaxf(ymax, tq[i] * 0.7376f); xmax = fmaxf(xmax, ceilf(rpm[i] / 1000) * 1000); }
 for (int i = 0; i < pn; i++) { ymax = fmaxf(ymax, php[i]); ymax = fmaxf(ymax, ptq[i] * 0.7376f); }
 float stepY = ymax > 1500 ? 500 : (ymax > 600 ? 200 : (ymax > 250 ? 100 : (ymax > 100 ? 50 : (ymax > 40 ? 10 : 5))));
 ymax = ceilf(ymax * 1.1f / stepY) * stepY;
 float L = r.x + (small ? 34 : 46), T = r.y + (small ? 8 : 34), Rr = r.x + r.width - 12, B = r.y + r.height - (small ? 22 : 30);
 for (float y = 0; y <= ymax + 0.1f; y += stepY) {
  float py = B - (B - T) * y / ymax;
  DrawLineEx((Vector2){L, py}, (Vector2){Rr, py}, 1, (Color){45, 49, 57, 255});
  TxR(TextFormat("%.0f", y), L - 5, py - 7, small ? 11 : 13, COL_DIM, 0);
 }
 float sx = xmax > 10000 ? 2000 : 1000;
 for (float x = 0; x <= xmax + 1; x += sx) {
  float px = L + (Rr - L) * x / xmax;
  DrawLineEx((Vector2){px, T}, (Vector2){px, B}, 1, (Color){38, 42, 49, 255});
  TxC(TextFormat("%.0f", x / 1000), px, B + 5, small ? 11 : 13, COL_DIM, 0);
 }
 if (redline > 0) { float px = L + (Rr - L) * redline / xmax; DrawLineEx((Vector2){px, T}, (Vector2){px, B}, 1.5f, (Color){170, 50, 40, 200}); }
#define GX(v) (L + (Rr - L) * (v) / xmax)
#define GY(v) (B - (B - T) * Clampf((v), 0, ymax) / ymax)
 for (int i = 1; i < pn; i++) {
  DrawLineEx((Vector2){GX(prpm[i - 1]), GY(php[i - 1])}, (Vector2){GX(prpm[i]), GY(php[i])}, 1.5f, (Color){255, 138, 28, 90});
  DrawLineEx((Vector2){GX(prpm[i - 1]), GY(ptq[i - 1] * 0.7376f)}, (Vector2){GX(prpm[i]), GY(ptq[i] * 0.7376f)}, 1.5f, (Color){80, 160, 255, 90});
 }
 for (int i = 1; i < n; i++) {
  DrawLineEx((Vector2){GX(rpm[i - 1]), GY(hp[i - 1])}, (Vector2){GX(rpm[i]), GY(hp[i])}, small ? 2 : 2.6f, COL_ACC);
  DrawLineEx((Vector2){GX(rpm[i - 1]), GY(tq[i - 1] * 0.7376f)}, (Vector2){GX(rpm[i]), GY(tq[i] * 0.7376f)}, small ? 2 : 2.6f, COL_BLUE);
 }
#undef GX
#undef GY
 if (!small) {
  Tb("DYNO", r.x + 12, r.y + 9, 16, COL_TXT);
  Tx("hp", r.x + 70, r.y + 10, 14, COL_ACC); Tx("lb-ft", r.x + 96, r.y + 10, 14, COL_BLUE);
  Tx("x1000 rpm", Rr - 70, B + 5, 12, COL_DIM);
  if (pn) Tx("faded = previous run", r.x + 150, r.y + 10, 13, COL_DIM);
 }
}

// ===== camera =====
static void UpdateCamera3D(Camera3D *cam, int allowDrag) {
 if (!gDragging && allowDrag && !gUiHoverPrev && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) gDragging = 1;
 if (gDragging) {
  Vector2 d = GetMouseDelta();
  camYaw += d.x * 0.006f * gSet.sens * (gSet.invertX ? -1 : 1); camPitch += d.y * 0.005f * gSet.sens * (gSet.invertY ? -1 : 1);
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) gDragging = 0;
 }
 if (!gUiHoverPrev && allowDrag) { float w = GetMouseWheelMove(); camDistT *= powf(1 - 0.12f * gSet.zoom, w); }
 camPitch = Clampf(camPitch, 0.02f, 1.35f);
 float base = fmaxf(7.0f, (G.L + G.ccW + 900) * 0.01f * 1.15f);
 camDistT = Clampf(camDistT, base * 0.4f, base * 3.f);
 camDist += (camDistT - camDist) * 0.2f;
 Vector3 tgt = {0.6f, G.H * 0.8f, 0};
 if (gMode == 0) { Vector3 r = Vector3Scale((Vector3){-sinf(camYaw), 0, cosf(camYaw)}, camDist * 0.04f); tgt = Vector3Add(tgt, r); }
 cam->target = tgt;
 cam->position = (Vector3){tgt.x + cosf(camPitch) * cosf(camYaw) * camDist, tgt.y + sinf(camPitch) * camDist, tgt.z + cosf(camPitch) * sinf(camYaw) * camDist};
 if (cam->position.y < 0.4f) cam->position.y = 0.4f;
}
static void ResetCamDist(void) { float base = fmaxf(7.0f, (G.L + G.ccW + 900) * 0.01f * 1.15f); camDistT = camDist = base; }

// ===== 3D scene =====
static void Scene3D(Camera3D cam) {
 BeginMode3D(cam);
 Vector3 vp = cam.position;
 SetShaderValue(gSh, locView, &vp, SHADER_UNIFORM_VEC3);
 Vector3 bg = {COL_BG.r / 255.f, COL_BG.g / 255.f, COL_BG.b / 255.f};
 DrawFloor(bg, (Vector3){0, 0, 0}, fmaxf(G.L, G.ccW + 300) * 0.01f * 0.75f);
 DrawStand(&G);
 float imb = Clampf(gS.vib, 0, 1.5f) + 0.03f;
 float act = gM.running ? 1.f : (gM.rpm > 30 ? 0.4f : 0);
 imb *= 1 - 0.35f * fabsf(sinf(gB.tilt * DEG2RAD)) * (gB.cyl <= 2 ? 1 : 0.4f);
 float amp = (imb * (1.2f + 2.5f * gM.load) + CAMS[gB.cam].rough * 4 * Clampf(1 - gM.rpm / 2500.f, 0, 1)) * act;
 if (gMode == 0) amp = 0;
 float ph = gTime * 2 * PI * 9.f;
 float rollT = gMode ? Clampf(gM.tind / fmaxf(gS.estTq, 1), -0.3f, 1.5f) * 0.018f : 0;
 gRoll += (rollT - gRoll) * 0.08f;
 Matrix shake = MatrixMultiply(MatrixRotateX(gRoll + sinf(ph * 0.7f) * amp * 0.0008f), MatrixTranslate(0, sinf(ph) * amp * 0.004f, cosf(ph * 1.3f) * amp * 0.0025f));
 gBase = MatrixMultiply(MatrixMultiply(MatrixScale(0.01f, 0.01f, 0.01f), shake), MatrixTranslate(0, G.H, 0));
 gDS.glow = gMode ? gM.egt : 20;
 unsigned saveHide = gHide; int saveCut = gDS.cut;
 if (gDS.oilView) { gDS.cut = 1; gHide |= (1u << G_BLOCK) | (1u << G_HEAD) | (1u << G_PLUG) | (1u << G_INTAKE) | (1u << G_FILTER) | (1u << G_SC) | (1u << G_CHARGE) | (1u << G_EXH) | (1u << G_TURBO) | (1u << G_COOL) | (1u << G_STARTER) | (1u << G_ALT); }
 gDS.oilSump = gMode ? (gM.oilSump < 0 ? gB.oilL : gM.oilSump) : gB.oilL; gDS.oilHead = gMode ? gM.oilHead : 0; gDS.oilNeed = gS.oilNeed;
 gDS.oilSupply = gMode ? gM.oilSupply : 1; gDS.oilFlow = gMode ? gM.oilFlow : 0.25f; G_drainVis = gS.drainK / 1.6f;
 gDS.oilT = gMode ? gM.oilT : 60;
 {
  float cm, ch, cw, ct;
  if (gB.cycle) { float l = Clampf(gS.lube / 0.8f, 0, 1); cm = cw = l; ch = 1; ct = l; if (gMode && gM.rpm < 50) cm = cw = fminf(l, 0.5f); }
  else if (gMode && gM.rpm > 50) {
   if (gB.lube == 0) { cm = gM.oilSplash; cw = gM.oilSplash * Clampf(gM.rpm / 1500.f, 0.3f, 1); ch = gM.oilSplash * 0.6f; ct = 0; }
   else { float p = Clampf(gM.oilP / 14.f, 0, 1) * gM.oilSupply; cm = p; cw = p * Clampf(gM.rpm / 1200.f, 0.4f, 1); ch = fmaxf(p * Clampf(gM.oilP / 22.f, 0, 1), Clampf(gM.oilHead / fmaxf(gS.oilHeadCap, 0.05f), 0, 1)); ct = Clampf(gM.oilP / 28.f, 0, 1) * gM.oilSupply; }
  } else if (gMode) { cm = cw = 0.5f; ch = 0.35f; ct = 0.35f; }
  else {
   char why[8]; int st = PartStatus(&gB, &gS, P_LUBE, why, 8);
   float base = st == 2 ? 0.3f : (st == 1 ? 0.6f : 1.f);
   cm = base; cw = base; ct = base; ch = gB.lube == 0 ? base * 0.6f : (gS.drainK < 0.6f ? 1 : base);
   if (gB.lube == 1 && fabsf(gB.tilt) > 50) { cm = cw = ct = 0.3f; ch = 1; }
  }
  gDS.covMain = cm; gDS.covRod = cm * 0.95f; gDS.covWall = cw; gDS.covHead = ch; gDS.covTurbo = ct;
 }
 if (gDS.oilView) OilUpdate(&gB, &gS, &gDS, gMode ? gM.rpm : 600, gVisAng, GetFrameTime() > 0.1f ? 0.016f : GetFrameTime());
 DrawEngine(&gB, &gS, gVisAng, &gDS);
 if (gDS.oilView) {
  // see-through block and heads on top so the oil is visible inside
  gHide = 0; gOnly = (1u << G_BLOCK) | (1u << G_HEAD); gAlpha = 46; gDS.cut = 0; gDS.oilView = 0;
  rlDisableDepthMask();
  DrawEngine(&gB, &gS, gVisAng, &gDS);
  rlEnableDepthMask();
  gOnly = 0; gAlpha = 0; gDS.oilView = 1;
 }
 gHide = saveHide; gDS.cut = saveCut;
 PuffDraw();
 EndMode3D();
}
static Vector3 LocalToWorld(Vector3 p) { return Vector3Transform(p, gBase); }

// ===== garage UI =====
static int gBlockSub = 0, gCustSel = 0, gCustSlot = 0;
static void MakeCustom(Build *b) {
 if (b->layout == 5) return;
 float x[16], a[16]; int bk[16];
 int n = CylGeom(b, x, a, bk);
 Build t = *b; t.layout = 5;
 float pitch = CylPitch(b);
 float xmin = 1e9f; for (int i = 0; i < n; i++) xmin = fminf(xmin, x[i]);
 for (int i = 0; i < n && i < 16; i++) {
  t.cAng[i] = (a[i] - b->tilt * DEG2RAD) * RAD2DEG;
  t.cSlot[i] = (b->layout >= 2) ? (i >> 1) : i;
  t.cFlip[i] = 0;
 }
 (void)pitch; (void)xmin;
 t.ncust = n; t.cyl = n;
 *b = t;
}
static void BlockTab(Build *b, float x, float y, float w, int *ch) {
 const char *subs[] = {"SHAPE", "CYLINDERS", "PLACEMENT"};
 float sw3 = (w - 8) / 3;
 for (int i = 0; i < 3; i++) {
  Rectangle r = {x + i * (sw3 + 4), y, sw3, 30};
  int hov = CheckCollisionPointRec(MP, r);
  DrawRectangleRounded(r, 0.3f, 8, gBlockSub == i ? COL_LINE : (hov ? COL_PANEL2 : COL_PANEL));
  TxC(subs[i], r.x + r.width * 0.5f, r.y + 8, 13, gBlockSub == i ? COL_ACC : COL_TXT, 1);
  if (hov && MPress) { gBlockSub = i; SfxPush(SFX_SELECT); }
 }
 y += 40;
 float rh = 40, gap = 6; int d; char vb[64];
#define ROWB() (Rectangle){x, y, w, rh}
#define NEXTB() y += rh + gap
 if (gBlockSub == 0) {
  float wl = WallMM(b), sp = SpacingMM(b);
  snprintf(vb, 64, "%.0f mm", wl);
  if (UiSlider(30, ROWB(), "Wall thickness", &wl, 4, 16, 1, vb, "Thicker walls: stronger block that takes more boost, but heavier and it sheds heat a little slower.", 1)) { b->wall = wl; *ch = 1; } NEXTB();
  snprintf(vb, 64, "%.0f mm", sp);
  if (UiSlider(31, ROWB(), "Bore spacing", &sp, 4, 40, 1, vb, "Metal between bores. Under ~8 mm the bores are siamesed: compact and light, but hot spots between cylinders.", 1)) { b->spacing = sp; *ch = 1; } NEXTB();
  if ((d = UiSelect(ROWB(), "Deck", DECKS[b->deck], DECK_TIP[b->deck], 1))) { b->deck = Cyc(b->deck, d, 3); *ch = 1; } NEXTB();
  if ((d = UiSelect(ROWB(), "Skirt", SKIRTS[b->skirt], SKIRT_TIP[b->skirt], 1))) { b->skirt = Cyc(b->skirt, d, 4); *ch = 1; } NEXTB();
  if ((d = UiSelect(ROWB(), "Main caps", CAPS[b->caps], CAPS_TIP[b->caps], 1))) { b->caps = Cyc(b->caps, d, 3); *ch = 1; } NEXTB();
  if ((d = UiSelect(ROWB(), "Balance shafts", b->balShaft ? "Yes" : "No", "Counter-rotating weights that cancel most of the shaking a layout can't balance itself. Costs a little power.", 1))) { b->balShaft ^= 1; *ch = 1; } NEXTB();
  int v8 = (b->layout == 2 || b->layout == 3) && b->cyl == 8 && !b->cycle;
  if ((d = UiSelect(ROWB(), "Crank (V8)", b->crankFlat ? "Flat-plane" : "Cross-plane", "Cross-plane: smooth, burbly. Flat-plane: revs and screams like two I4s, shakes more.", v8))) { b->crankFlat ^= 1; *ch = 1; } NEXTB();
  y += 4;
  Tb("BALANCE", x + 4, y, 14, COL_DIM); y += 20;
  const char *nm[4] = {"Primary", "Secondary", "Rocking", "Overall"};
  float val[4] = {gS.prim / fmaxf(gS.vib > 0 ? 1.f : 1.f, 1), gS.sec, gS.rock, gS.vib};
  for (int i = 0; i < 4; i++) {
   float v = Clampf(i == 3 ? val[i] : val[i] / (0.8f * b->cyl), 0, 1);
   Tx(nm[i], x + 4, y, 14, COL_DIM);
   DrawRectangleRounded((Rectangle){x + 110, y + 3, w - 180, 10}, 1, 6, (Color){14, 15, 18, 255});
   DrawRectangleRounded((Rectangle){x + 110, y + 3, (w - 180) * v + 2, 10}, 1, 6, v < 0.25f ? COL_GRN : (v < 0.6f ? COL_YEL : COL_RED));
   TxR(i == 3 ? (v < 0.05f ? "smooth" : (v < 0.3f ? "fine" : (v < 0.6f ? "buzzy" : "shaker"))) : TextFormat("%.2f", val[i]), x + w - 4, y, 14, COL_TXT, 1);
   y += 20;
  }
 } else if (gBlockSub == 1) {
  if (b->layout != 5) {
   Tx("This engine uses a preset layout.", x + 4, y + 4, 16, COL_TXT); y += 28;
   Tx("Convert it to place every cylinder yourself:", x + 4, y, 15, COL_DIM); y += 26;
   if (UiButton((Rectangle){x, y, w, 40}, "MAKE IT A CUSTOM LAYOUT", 1, 1)) { MakeCustom(b); *ch = 1; gCustSel = 0; }
   return;
  }
  float cs = 220, cx = x + cs * 0.5f + 4, cy = y + cs * 0.5f;
  Rectangle cv = {x, y, cs + 8, cs};
  PanelR(cv, 0.06f, (Color){14, 15, 18, 255});
  DrawCircleLinesV((Vector2){cx, cy}, 26, COL_LINE);
  DrawCircleV((Vector2){cx, cy}, 8, COL_DIM);
  Tx("front view", x + 6, y + 4, 11, COL_DIM);
  static const Color slotC[8] = {{255,138,28,255},{80,160,255,255},{90,205,115,255},{240,196,60,255},{200,110,255,255},{255,100,120,255},{80,220,220,255},{200,200,200,255}};
  for (int i = 0; i < b->ncust; i++) {
   float an = b->cAng[i] * DEG2RAD;
   Vector2 dir = {sinf(an), -cosf(an)};
   Vector2 p0 = {cx + dir.x * 34, cy + dir.y * 34}, p1 = {cx + dir.x * 100, cy + dir.y * 100};
   Color c = slotC[b->cSlot[i] & 7];
   DrawLineEx(p0, p1, i == gCustSel ? 22 : 18, i == gCustSel ? COL_TXT : c);
   if (i == gCustSel) DrawLineEx(p0, p1, 14, c);
   TxC(TextFormat("%d", i + 1), p1.x + dir.x * 10, p1.y + dir.y * 10 - 7, 13, COL_TXT, 1);
   Vector2 ip = {p1.x - dir.y * (b->cFlip[i] ? -12 : 12) * ((b->cAng[i] > 0.5f) ? -1 : 1), p1.y + dir.x * (b->cFlip[i] ? -12 : 12) * ((b->cAng[i] > 0.5f) ? -1 : 1)};
   DrawCircleV(ip, 3, (Color){120, 200, 255, 255});
  }
  if (CheckCollisionPointRec(MP, cv) && MPress) {
   float dx = MP.x - cx, dy = MP.y - cy;
   if (dx * dx + dy * dy > 30 * 30) {
    float ang = atan2f(dx, -dy) * RAD2DEG;
    int hit = -1; float best = 9;
    for (int i = 0; i < b->ncust; i++) { float dd = fabsf(remainderf(b->cAng[i] - ang, 360)); if (dd < best) { best = dd; hit = i; } }
    if (hit >= 0) { gCustSel = hit; SfxPush(SFX_SELECT); }
    else if (b->ncust < 16) { int i = b->ncust++; b->cAng[i] = roundf(ang / 5) * 5; b->cSlot[i] = gCustSlot; b->cFlip[i] = 0; gCustSel = i; *ch = 1; SfxPush(SFX_RATCHET); }
   }
  }
  float rx2 = x + cs + 16, rw2 = w - cs - 16;
  Tx("Click the circle to", rx2, y + 6, 13, COL_DIM); Tx("add a cylinder at", rx2, y + 22, 13, COL_DIM); Tx("that angle, or click", rx2, y + 38, 13, COL_DIM); Tx("one to select it.", rx2, y + 54, 13, COL_DIM);
  Tx("Color = crank throw.", rx2, y + 80, 13, COL_DIM); Tx("Blue dot = intake side.", rx2, y + 96, 13, COL_DIM);
  Tx(TextFormat("%d cylinder%s", b->ncust, b->ncust == 1 ? "" : "s"), rx2, y + 124, 16, COL_TXT);
  Tx(TextFormat("%.0f cc", gS.dispL * 1000), rx2, y + 146, 16, COL_TXT);
  y += cs + 8;
  if (gCustSel >= b->ncust) gCustSel = b->ncust - 1;
  int has = gCustSel >= 0 && b->ncust > 0;
  snprintf(vb, 64, "Throw %d", (has ? b->cSlot[gCustSel] : gCustSlot) + 1);
  if ((d = UiSelect(ROWB(), "Crank throw", vb, "Which crankpin the cylinder's rod rides on. Cylinders sharing a throw must be spread apart by angle.", 1))) { if (has) { b->cSlot[gCustSel] = Cyc(b->cSlot[gCustSel], d, 8); gCustSlot = b->cSlot[gCustSel]; } else gCustSlot = Cyc(gCustSlot, d, 8); *ch = 1; } NEXTB();
  if (has) {
   float an = b->cAng[gCustSel];
   snprintf(vb, 64, "%+.0f deg", an);
   if (UiSlider(32, ROWB(), "Angle", &an, -180, 180, 5, vb, "Where this cylinder points around the crank. 0 = straight up.", 1)) { b->cAng[gCustSel] = an; *ch = 1; } NEXTB();
   if ((d = UiSelect(ROWB(), "Ports", b->cFlip[gCustSel] ? "Flipped" : "Normal", "Swap which side the intake and exhaust ports face.", 1))) { b->cFlip[gCustSel] ^= 1; *ch = 1; } NEXTB();
   if (UiButton((Rectangle){x, y, w * 0.5f - 3, 38}, "DELETE CYLINDER", 3, 1)) {
    for (int i = gCustSel; i + 1 < b->ncust; i++) { b->cAng[i] = b->cAng[i + 1]; b->cSlot[i] = b->cSlot[i + 1]; b->cFlip[i] = b->cFlip[i + 1]; }
    b->ncust--; if (gCustSel >= b->ncust) gCustSel = b->ncust - 1; *ch = 1;
   }
   if (UiButton((Rectangle){x + w * 0.5f + 3, y, w * 0.5f - 3, 38}, "CLEAR ALL", 0, 1)) { b->ncust = 0; gCustSel = -1; *ch = 1; }
  }
 } else {
  if ((d = UiSelect(ROWB(), "Placement", b->placeMan ? "Manual" : "Auto", "Auto packs parts for you. Manual: you place them, and each must bolt to what it works with without hitting anything.", 1))) {
   b->placeMan ^= 1; if (b->placeMan && b->altAng == 0 && b->stAng == 0) { b->altAng = 70; b->stAng = 225; } *ch = 1;
  } NEXTB();
  float aa = b->altAng, sa = b->stAng;
  snprintf(vb, 64, "%+.0f deg", aa);
  if (UiSlider(33, ROWB(), "Alternator", &aa, -180, 180, 5, vb, "Angle around the crank pulley. It bolts to the block/head on a bracket and the belt must reach.", b->placeMan && b->alternator >= 3)) { b->altAng = aa; *ch = 1; } NEXTB();
  snprintf(vb, 64, "%+.0f deg", sa);
  if (UiSlider(34, ROWB(), "Starter", &sa, -180, 180, 5, vb, "Angle around the flywheel. Its gear has to mesh with the ring gear.", b->placeMan && STARTERS[b->starter].elec)) { b->stAng = sa; *ch = 1; } NEXTB();
  static const char *fpn[] = {"Front", "Left side", "Right side"};
  if ((d = UiSelect(ROWB(), "Oil filter", fpn[b->filtPos], "Spin-on filter location. It threads onto the block's oil gallery.", !b->cycle && b->lube > 0))) { b->filtPos = Cyc(b->filtPos, d, 3); *ch = 1; } NEXTB();
  static const char *tsn[] = {"Auto", "Left side", "Right side", "Rear"};
  if ((d = UiSelect(ROWB(), "Turbo", tsn[b->turboSide], "The turbine bolts to the exhaust manifold, so it has to be on an exhaust side (or rear-mounted with long pipes).", b->forced >= 1 && b->forced <= 3))) { b->turboSide = Cyc(b->turboSide, d, 4); *ch = 1; } NEXTB();
  y += 6;
  int nbad = 0;
  for (int i = 0; i < gNWarn; i++) if (gWarnSev[i] == 3) {
   float s2 = 14; while (Tw(gWarn[i], s2, 0) > w - 8 && s2 > 10) s2 -= 0.5f;
   Tx(gWarn[i], x + 4, y, s2, (Color){255, 140, 125, 255}); y += 20; nbad++;
  }
  if (!nbad) Tx("Everything attaches and nothing collides.", x + 4, y, 15, COL_GRN);
 }
#undef ROWB
#undef NEXTB
}

static void BuildPanel(int sw, int sh) {
 float pw = 470;
 Panel((Rectangle){12, 64, pw, sh - 76}, (Color){22, 25, 30, 245});
 const char *tabs[] = {"ENGINE", "METAL", "VALVES", "INTAKE", "SYSTEM", "OIL", "BLOCK"};
 static const int ord[7] = {0, 6, 1, 2, 3, 4, 5};
 float tw = (pw - 20) / 7;
 for (int k = 0; k < 7; k++) {
  int i = ord[k];
  Rectangle tr = {22 + k * tw, 74, tw - 3, 34};
  int hov = CheckCollisionPointRec(MP, tr);
  DrawRectangleRounded(tr, 0.3f, 8, gTab == i ? COL_ACC : (hov ? COL_PANEL2 : COL_PANEL));
  TxC(tabs[i], tr.x + tr.width * 0.5f, tr.y + 10, 12.f, gTab == i ? (Color){18, 18, 20, 255} : COL_TXT, 1);
  if (hov && MPress && gTab != i) { gTab = i; SfxPush(SFX_SELECT); }
 }
 float x = 22, y = 122, w = pw - 20, rh = 42, gap = 6;
 int ch = 0;
#define ROW() (Rectangle){x, y, w, rh}
#define NEXT() y += rh + gap
 Build *b = &gB;
 char vb[64];
 if (gTab == 0) {
  int d;
  if ((d = UiSelect(ROW(), "Cycle", b->cycle ? "2-stroke" : "4-stroke", b->cycle ? "2-stroke: fires every revolution. Ports instead of valves, oil mixed into the fuel." : "4-stroke: intake, compression, power, exhaust. Valves, oil sump.", 1))) { b->cycle ^= 1; ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Fuel", FUELS[b->fuel].name, FUELS[b->fuel].tip, 1))) { b->fuel = Cyc(b->fuel, d, NFUEL); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Layout", LAYOUTS[b->layout], "Cylinder arrangement. Custom lets you place every cylinder yourself in BLOCK > CYLINDERS.", 1))) { int nl = Cyc(b->layout, d, NLAYOUT); if (nl == 5) MakeCustom(b); else b->layout = nl; ch = 1; } NEXT();
  snprintf(vb, 64, "%d", b->cyl);
  if ((d = UiSelect(ROW(), "Cylinders", vb, b->layout == 5 ? "Custom layout: add and remove cylinders in BLOCK > CYLINDERS." : "More cylinders: smoother, revs higher for the same displacement, heavier.", b->layout != 0 && b->layout != 5))) {
   int idx = 0; for (int i = 0; i < CYLOPTN[b->layout]; i++) if (CYLOPT[b->layout][i] == b->cyl) idx = i;
   b->cyl = CYLOPT[b->layout][Cyc(idx, d, CYLOPTN[b->layout])]; ch = 1;
  } NEXT();
  snprintf(vb, 64, "%.1f mm", b->bore);
  ch |= UiSlider(1, ROW(), "Bore", &b->bore, 30, 135, 0.5f, vb, "Cylinder diameter. Bigger bore = bigger valves, more displacement. Shift+wheel for big steps.", 1); NEXT();
  snprintf(vb, 64, "%.1f mm", b->stroke);
  ch |= UiSlider(2, ROW(), "Stroke", &b->stroke, 30, 140, 0.5f, vb, "Piston travel. Long stroke = torque; short stroke = revs (lower piston speed).", 1); NEXT();
  snprintf(vb, 64, "%.1f : 1", b->cr);
  ch |= UiSlider(3, ROW(), "Compression", &b->cr, 5, 25, 0.1f, vb, "Higher = more efficient and powerful, but knocks on weak fuel. Diesel needs 14+.", 1); NEXT();
  snprintf(vb, 64, "%+.0f deg", b->tilt);
  ch |= UiSlider(6, ROW(), "Cylinder tilt", &b->tilt, -90, 90, 1, vb, "Lean the cylinders over (slant-six, laid-down singles). Lowers the engine and shakes less up-and-down, but past ~50 deg a wet sump drains badly: low oil pressure, burns oil.", 1); NEXT();
  y += 6;
  Tx(TextFormat("Displacement  %.0f cc  (%.2f L)", gS.dispL * 1000, gS.dispL), x + 4, y, 17, COL_TXT); y += 24;
  Tx(TextFormat("Per cylinder  %.0f cc    Bore/stroke %.2f", gS.vcylL * 1000, b->bore / b->stroke), x + 4, y, 15, COL_DIM); y += 22;
  Tx(TextFormat("Mean piston speed at redline  %.1f m/s", 2 * b->stroke * 1e-3f * gS.redline / 60), x + 4, y, 15, COL_DIM);
 } else if (gTab == 1) {
  int d;
  if ((d = UiSelect(ROW(), "Block", BLOCKS[b->block].name, PartTip(P_BLOCK, BLOCKS[b->block].tip), 1))) { b->block = Cyc(b->block, d, NBLOCK); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Pistons & rods", INTERN[b->internals].name, INTERN[b->internals].tip, 1))) { b->internals = Cyc(b->internals, d, NINT); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Flywheel", FLY[b->flywheel].name, PartTip(P_FLY, FLY[b->flywheel].tip), 1))) { b->flywheel = Cyc(b->flywheel, d, NFLY); ch = 1; } NEXT();
  y += 6;
  Tx(TextFormat("Cylinder pressure limit  %.0f bar", gS.pLimit), x + 4, y, 17, COL_TXT); y += 24;
  Tx(TextFormat("Block strength at this size  %.0f%%   internals %.0f%%", gS.blockEff / BLOCKS[b->block].str * 100, gS.intEff / INTERN[b->internals].str * 100), x + 4, y, 15, COL_DIM); y += 22;
  Tx(TextFormat("Peak pressure (est.)  %.0f bar", gS.estImep), x + 4, y, 15, gS.estImep > gS.pLimit ? COL_RED : COL_DIM); y += 22;
  Tx(TextFormat("Rod/piston inertia limit  %.0f rpm  (heavier pistons = lower)", gS.pistonRpm), x + 4, y, 15, COL_DIM); y += 22;
  Tx(TextFormat("Rotating inertia  %.3f kg*m2", gS.inertia), x + 4, y, 15, COL_DIM);
 } else if (gTab == 2) {
  int d;
  if (b->cycle == 0) {
   if ((d = UiSelect(ROW(), "Valvetrain", VT4[b->valvetrain].name, VT4[b->valvetrain].tip, 1))) { b->valvetrain = Cyc(b->valvetrain, d, 4); ch = 1; } NEXT();
   snprintf(vb, 64, b->valvetrain == 0 ? "2 (fixed)" : "%d per cyl", VALVES[b->valves].n);
   if ((d = UiSelect(ROW(), "Valves", vb, "More valves: better breathing and lighter valves (higher float rpm). 4 is the sweet spot.", b->valvetrain != 0))) { b->valves = Cyc(b->valves, d, 4); ch = 1; } NEXT();
   if ((d = UiSelect(ROW(), "Camshaft", CAMS[b->cam].name, CAMS[b->cam].tip, 1))) { b->cam = Cyc(b->cam, d, 4); ch = 1; } NEXT();
   y += 6;
   Tx(TextFormat("Valve float  %.0f rpm", gS.floatRpm), x + 4, y, 17, COL_TXT); y += 24;
  } else {
   if ((d = UiSelect(ROW(), "Intake", VT2[b->valvetrain].name, VT2[b->valvetrain].tip, 1))) { b->valvetrain = Cyc(b->valvetrain, d, 3); ch = 1; } NEXT();
   if ((d = UiSelect(ROW(), "Port timing", CAMS[b->cam].name2, CAMS[b->cam].tip, 1))) { b->cam = Cyc(b->cam, d, 4); ch = 1; } NEXT();
   y += 6;
   Tx("2-strokes have no valves to float.", x + 4, y, 17, COL_TXT); y += 24;
  }
  Tx(TextFormat("Torque peak (NA)  ~%.0f rpm", gS.rpk), x + 4, y, 15, COL_DIM); y += 22;
  const char *lb[] = {"rod/piston inertia", "valve float", "diesel combustion", "breathing (power falls off)"};
  Tx(TextFormat("Redline  %.0f rpm  (limited by %s)", gS.redline, lb[gS.limitBy]), x + 4, y, 15, COL_DIM);
 } else if (gTab == 3) {
  int d;
  if ((d = UiSelect(ROW(), "Fuel delivery", DELIV[b->delivery].name, DELIV[b->delivery].tip, 1))) { b->delivery = Cyc(b->delivery, d, NDEL); ch = 1; } NEXT();
  if (b->delivery == 0) { if ((d = UiSelect(ROW(), "Carburetor", CARBS[b->carb].name, PartTip(P_CARB, "Carb size in cfm must match the engine's airflow."), 1))) { b->carb = Cyc(b->carb, d, NCARB); ch = 1; } NEXT(); }
  else { if ((d = UiSelect(ROW(), b->delivery == 4 ? "Inj. nozzles" : "Injectors", INJ[b->inj].name, PartTip(P_INJ, "Injector flow caps how much power you can fuel. Too small = lean = melted pistons."), 1))) { b->inj = Cyc(b->inj, d, NINJ); ch = 1; } NEXT(); }
  if ((d = UiSelect(ROW(), "Air filter", FILTERS[b->filter].name, FILTERS[b->filter].tip, 1))) { b->filter = Cyc(b->filter, d, NFIL); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Forced induction", FORCED[b->forced].name, b->forced ? PartTip(P_TURBO, FORCED[b->forced].tip) : FORCED[0].tip, 1))) { b->forced = Cyc(b->forced, d, NFORCED); if (b->forced && b->boost < 1) b->boost = 10; ch = 1; } NEXT();
  snprintf(vb, 64, "%.0f psi", b->boost);
  ch |= UiSlider(4, ROW(), "Boost", &b->boost, 0, 50, 1, vb, "Wastegate / pulley setting. More boost = more air = more power, heat, knock and pressure.", b->forced != 0); NEXT();
  if ((d = UiSelect(ROW(), "Intercooler", b->intercooler ? "Yes" : "No", "Cools compressed air: denser charge and much less knock.", b->forced != 0))) { b->intercooler ^= 1; ch = 1; } NEXT();
  y += 6;
  if (b->forced) {
   const ForcedDef *fd = &FORCED[b->forced];
   if (b->forced == 4) Tx(TextFormat("Blower safe max  %.0f psi", fd->maxb), x + 4, y, 17, COL_TXT);
   else Tx(TextFormat("Turbo max  %.0f psi    full spool ~%.0f rpm", fd->maxb, fd->full / fmaxf(gS.dispL * (b->cycle ? 2 : 1), 0.01f) * 1000), x + 4, y, 16, COL_TXT);
  } else Tx("Naturally aspirated.", x + 4, y, 17, COL_TXT);
 } else if (gTab == 6) {
  BlockTab(b, x, y - 4, w, &ch);
 } else if (gTab == 5) {
  int d;
  if (b->cycle == 0) {
   if ((d = UiSelect(ROW(), "Lubrication", LUBES[b->lube], PartTip(P_LUBE, LUBE_TIP[b->lube]), 1))) { b->lube = Cyc(b->lube, d, 3); ch = 1; } NEXT();
   if ((d = UiSelect(ROW(), "Engine oil", OILS[b->oil].name, OILS[b->oil].tip, 1))) { b->oil = Cyc(b->oil, d, 5); ch = 1; } NEXT();
   snprintf(vb, 64, "%.1f L", b->oilL);
   ch |= UiSlider(7, ROW(), "Oil capacity", &b->oilL, 0.2f, 20, 0.1f, vb, "How much oil goes in. Too little and the pickup sucks air under load; too much and the crank whips it into froth (power loss).", 1); NEXT();
   if ((d = UiSelect(ROW(), "Oil cooler", b->oilCooler ? "Yes" : "No", "Small radiator for the oil. Keeps it from thinning out under sustained load.", 1))) { b->oilCooler ^= 1; ch = 1; } NEXT();
   y += 6;
   Tx(TextFormat("Needs %.1f L    drain-back %.0f%%", gS.oilNeed, Clampf(gS.drainK / 1.6f, 0, 1) * 100), x + 4, y, 16, COL_TXT); y += 24;
   Tx(fabsf(b->tilt) > 35 && b->lube == 1 ? "Tilted cylinders hold oil in the heads." : "Oil drains back to the pan by gravity.", x + 4, y, 15, COL_DIM); y += 26;
  } else {
   if (b->mix < 1) snprintf(vb, 64, "NO OIL"); else snprintf(vb, 64, "%.0f : 1", b->mix);
   ch |= UiSlider(5, ROW(), "Oil mix", &b->mix, 0, 100, 1, vb, "Gas-to-oil ratio. 32-50:1 is typical. Too lean = seize. Too rich = smoke, fouled plugs.", 1); NEXT();
   Tx("2-strokes are oiled by the fuel mix:", x + 4, y + 6, 16, COL_TXT); y += 28;
   Tx("oil mist coats the crank and cylinder on the way in.", x + 4, y, 15, COL_DIM); y += 26;
  }
  if (UiButton((Rectangle){x, y, w, 40}, gDS.oilView ? "OIL FLOW VIEW: ON   [V]" : "OIL FLOW VIEW: OFF   [V]", gDS.oilView ? 2 : 0, 1)) gDS.oilView ^= 1;
 } else {
  int d;
  if ((d = UiSelect(ROW(), "Cooling", COOL[b->cooling].name, COOL[b->cooling].tip, 1))) { b->cooling = Cyc(b->cooling, d, 3); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Exhaust", EXH[b->exhaust].name, EXH[b->exhaust].tip, 1))) { b->exhaust = Cyc(b->exhaust, d, b->cycle ? 4 : 3); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Starter", STARTERS[b->starter].name, PartTip(P_START, STARTERS[b->starter].tip), 1))) { b->starter = Cyc(b->starter, d, NSTART); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Charging", ALTS[b->alternator].name, PartTip(P_ALT, ALTS[b->alternator].tip), 1))) { b->alternator = Cyc(b->alternator, d, NALT); ch = 1; } NEXT();
  { int ig = b->ignition; if ((d = UiSelect(ROW(), "Ignition", IGNS[ig], IGN_TIP[ig], !FUELS[b->fuel].diesel))) { b->ignition = Cyc(b->ignition, d, 5); ch = 1; } NEXT(); }
  if ((d = UiSelect(ROW(), "Cam drive", CAMDRV[b->camDrive], CAMDRV_TIP[b->camDrive], !b->cycle && b->valvetrain > 0))) { b->camDrive = Cyc(b->camDrive, d, 3); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Speed control", GOVS[b->gov], "Cheap engines use a mechanical governor (holds a set rpm, hunts when cold). Performance engines use a rev limiter. None = nothing stops over-revving.", 1))) { b->gov = Cyc(b->gov, d, NGOV); if (b->gov == 4 && b->govSet < 1000) b->govSet = 3000; ch = 1; } NEXT();
  if (b->gov == 4) {
   float gs = GovSet(b); char gv[32]; snprintf(gv, 32, "%.0f rpm  $%.0f", gs, GovCost(b));
   if (UiSlider(35, ROW(), "Governor set", &gs, 1000, 6000, 100, gv, "Where the governor holds the engine. Higher settings need stiffer springs and heavier flyweights, so they cost more.", 1)) { b->govSet = gs; ch = 1; } NEXT();
  }
  y += 6;
  Tx(TextFormat("Battery: %s   Spark: %s", gS.hasBatt ? "yes" : "none", gS.magneto ? "magneto" : (gS.hasBatt ? "battery coil" : "NONE")), x + 4, y, 16, COL_TXT); y += 24;
  Tx(TextFormat("Electrical load %.0f A   charging %.0f A", gS.demandA, ALTS[b->alternator].amps), x + 4, y, 15, COL_DIM); y += 22;
  Tx(TextFormat("Cooling capacity  ~%.0f kW", gS.coolCap / 1000), x + 4, y, 15, COL_DIM);
 }
#undef ROW
#undef NEXT
 // preset + save area
 float by = sh - 286;
 DrawLineEx((Vector2){22, by - 10}, (Vector2){22 + w, by - 10}, 1, COL_LINE);
 int d;
 if ((d = UiSelect((Rectangle){x, by, w - 110, 40}, "Preset", PRESETS[gPreset].name, "Start from a known engine, then mess with it.", 1))) gPreset = Cyc(gPreset, d, NPRESET);
 if (UiButton((Rectangle){x + w - 102, by, 102, 40}, "LOAD", 0, 1)) { gB = PRESETS[gPreset]; ch = 1; SfxPush(SFX_RATCHET); ResetCamDist(); ToastAdd(TextFormat("Loaded preset: %s", PRESETS[gPreset].name), COL_TXT); }
 by += 48;
 char sl[64];
 if (gSave.used[gSlot]) { char nm[64]; BuildName(&gSave.slot[gSlot], nm, 64); snprintf(sl, 64, "%d: %s", gSlot + 1, nm); } else snprintf(sl, 64, "%d: (empty)", gSlot + 1);
 if ((d = UiSelect((Rectangle){x, by, w - 190, 40}, "Slot", sl, "Save your builds. Stored next to the exe.", 1))) gSlot = Cyc(gSlot, d, 4);
 if (UiButton((Rectangle){x + w - 182, by, 88, 40}, "SAVE", 0, 1)) { gSave.slot[gSlot] = gB; gSave.used[gSlot] = 1; SaveWrite(); ToastAdd(TextFormat("Saved to slot %d", gSlot + 1), COL_GRN); }
 if (UiButton((Rectangle){x + w - 88, by, 88, 40}, "LOAD", 0, gSave.used[gSlot])) { gB = gSave.slot[gSlot]; ch = 1; ResetCamDist(); ToastAdd(TextFormat("Loaded slot %d", gSlot + 1), COL_TXT); }
 by += 54;
 PanelR((Rectangle){x, by, w, sh - by - 22}, 0.08f, (Color){16, 18, 22, 255});
 const char *tip = gTip ? gTip : "Hover a part for details. Drag the 3D view to orbit, wheel to zoom.";
 // wrap tip
 {
  char line[160]; int li = 0; float ty = by + 12; const char *p = tip;
  while (*p && ty < sh - 40) {
   int last = -1; li = 0;
   while (p[li] && li < 150) { line[li] = p[li]; line[li + 1] = 0; if (p[li] == ' ') last = li; if (Tw(line, 16, 0) > w - 24) break; li++; }
   if (p[li] && last > 0) li = last + 1;
   memcpy(line, p, li); line[li] = 0;
   Tx(line, x + 12, ty, 16, COL_DIM); ty += 21; p += li;
  }
 }
 if (ch) OnBuildChanged();
}

#include "shop.c"

static void SpecCard(int sw, int sh) {
 float w = 400, x = sw - w - 12, y = 64;
 int nw = gNWarn > 6 ? 6 : gNWarn;
 float h = 454 + nw * 22 + (nw ? 26 : 0);
 Panel((Rectangle){x, y, w, h}, (Color){22, 25, 30, 240});
 char nm[64]; BuildName(&gB, nm, 64);
 Rectangle nr = {x + 8, y + 6, w - 16, 34};
 int nh = CheckCollisionPointRec(MP, nr);
 if (nh && MPress && !gEditName) { gEditName = 1; snprintf(gB.name, 32, "%s", nm); }
 if (gEditName) {
  DrawRectangleRounded(nr, 0.2f, 6, (Color){14, 15, 18, 255});
  DrawRectangleRoundedLinesEx(nr, 0.2f, 6, 1.5f, COL_ACC);
  int c;
  while ((c = GetCharPressed())) { int l = (int)strlen(gB.name); if (c >= 32 && c < 127 && l < 30) { gB.name[l] = (char)c; gB.name[l + 1] = 0; } }
  if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && strlen(gB.name)) gB.name[strlen(gB.name) - 1] = 0;
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || (MPress && !nh)) { gEditName = 0; SfxPush(SFX_SELECT); }
  int blinkOn = ((int)(GetTime() * 2)) & 1;
  Tb(TextFormat("%s%s", gB.name, blinkOn ? "_" : ""), x + 14, y + 12, 20, COL_TXT);
 } else {
  float sz = 22; while (Tw(nm, sz, 1) > w - 60 && sz > 14) sz--;
  Tb(nm, x + 14, y + 12, sz, nh ? COL_ACC : COL_TXT);
  if (nh) { TxR("rename", x + w - 14, y + 16, 13, COL_DIM, 0); gTip = "Click to name your engine."; }
 }
 const char *lay = gB.layout == 5 ? CustomKind(&gB, 0) : gB.layout == 0 ? "Single" : (gB.layout == 1 ? "Inline" : (gB.layout == 4 ? "Boxer" : "V"));
 Tx(TextFormat("%s-%d  %s  %s  %s", lay, gB.cyl, gB.cycle ? "2-stroke" : "4-stroke", gB.cycle ? VT2[gB.valvetrain].name : VT4[gB.valvetrain].name, FUELS[gB.fuel].name), x + 14, y + 42, 14, COL_DIM);
 float yy = y + 70;
 Tb(TextFormat("%.0f", gS.estHp), x + 14, yy, 40, COL_ACC);
 float hw = Tw(TextFormat("%.0f", gS.estHp), 40, 1);
 Tx(TextFormat("hp @ %.0f", gS.estHpRpm), x + 20 + hw, yy + 18, 16, COL_DIM);
 Tb(TextFormat("%.0f", gS.estTq * 0.7376f), x + 210, yy, 40, COL_BLUE);
 float tw2 = Tw(TextFormat("%.0f", gS.estTq * 0.7376f), 40, 1);
 Tx(TextFormat("lb-ft @ %.0f", gS.estTqRpm), x + 216 + tw2, yy + 18, 16, COL_DIM);
 yy += 50;
 Tx("Estimated at the flywheel. Prove it on the dyno.", x + 14, yy, 13, COL_DIM); yy += 22;
 DynoGraph((Rectangle){x + 6, yy, w - 12, 140}, gS.npts, gS.crpm, gS.chp, gS.ctq, 0, 0, 0, 0, gS.redline, 1);
 yy += 148;
 struct { const char *k; char v[48]; } rows[6];
 rows[0].k = "Displacement"; snprintf(rows[0].v, 48, "%.2f L  (%.0f cc)", gS.dispL, gS.dispL * 1000);
 rows[1].k = "Redline"; snprintf(rows[1].v, 48, "%.0f rpm", gS.redline);
 rows[2].k = "Torque"; snprintf(rows[2].v, 48, "%.0f Nm", gS.estTq);
 rows[3].k = "Weight"; snprintf(rows[3].v, 48, "%.0f kg  (%.0f lb)", gS.weight, gS.weight * 2.2046f);
 rows[4].k = "Power/weight"; snprintf(rows[4].v, 48, "%.2f hp/kg   %.0f hp/L", gS.estHp / gS.weight, gS.estHp / fmaxf(gS.dispL, 0.01f));
 rows[5].k = "Est. retail price"; snprintf(rows[5].v, 48, "$%.0f", gS.cost);
 for (int i = 0; i < 6; i++) {
  Tx(rows[i].k, x + 14, yy, 15, COL_DIM); TxR(rows[i].v, x + w - 14, yy, 15, COL_TXT, 1); yy += 21;
 }
 Tx("Build cost: one-off / @10k units", x + 14, yy, 15, COL_DIM); TxR(TextFormat("$%.0f / $%.0f", gS.mfgOne, gS.mfgK), x + w - 14, yy, 15, COL_TXT, 1); yy += 21;
 if (nw) {
  yy += 6;
  Tb("WARNINGS", x + 14, yy, 14, COL_YEL); yy += 20;
  for (int i = 0; i < nw; i++) {
   DrawCircleV((Vector2){x + 20, yy + 8}, 4, gWarnSev[i] >= 2 ? COL_RED : COL_YEL);
   float s2 = 14; while (Tw(gWarn[i], s2, 0) > w - 46 && s2 > 10) s2 -= 0.5f;
   Tx(gWarn[i], x + 30, yy, s2, gWarnSev[i] >= 2 ? (Color){255, 140, 125, 255} : COL_TXT); yy += 22;
  }
 }
 if (UiButton((Rectangle){x, sh - 128, w * 0.5f - 4, 42}, "OVERVIEW  [O]", 0, 1)) gShowOverview ^= 1;
 if (UiButton((Rectangle){x + w * 0.5f + 4, sh - 128, w * 0.5f - 4, 42}, "BUILD TO SELL  >", 2, 1)) { gShop = 1; gShopTab = 1; }
 int blocked = -1;
 for (int i = 0; i < gNWarn; i++) if (gWarnSev[i] == 3) { blocked = i; break; }
 if (UiButton((Rectangle){x, sh - 78, w, 64}, blocked >= 0 ? "CAN'T MOUNT A PART  (see warnings)" : "FIRE IT UP ON THE DYNO  >", blocked >= 0 ? 3 : 1, 1)) {
  if (blocked >= 0) { ToastAdd(gWarn[blocked], COL_RED); return; }
  gMode = 1; SimReset(&gM, &gS, 1); gDiagShown = 0; gWasRunning = 0; gStartedOnce = 0; gCrankT = 0;
  for (int i = 0; i < NPUFF; i++) gPuff[i].life = 0;
  ToastAdd(STARTERS[gB.starter].elec ? "Hold E (or START) to crank. I = ignition." : "Press E (or PULL) to yank the cord. I = ignition.", COL_TXT);
 }
}

static const char *Diagnose(void) {
 const FuelDef *f = &FUELS[gB.fuel];
 if (!gM.ign) return "Ignition is off. Press I.";
 if (f->diesel && gB.delivery != 3 && gB.delivery != 4) return "Diesel can't run on a carb or port injection. Use direct or mechanical injection.";
 if (f->diesel && gB.cr < 14) return "Diesel won't self-ignite below 14:1 compression.";
 if (gS.needsElec && !gS.hasBatt) return "Fuel injection has no power. Add an alternator or electric start (battery).";
 if (gS.needsElec && gM.volts < 9) return "Battery too low to run the injectors.";
 if (gM.starterDead) return "The starter is burned out. Rebuild (R) and fit a properly rated starter.";
 if (gM.recoilDead) return "The recoil is broken. Rebuild (R) and fit a properly rated one.";
 if (!f->diesel && !gS.magneto && !gS.hasBatt) return "No spark: this flywheel has no magneto and there's no battery.";
 if (STARTERS[gB.starter].elec && gM.volts < 9.6f) return "Battery's dead: the starter just clicks.";
 if (gB.delivery == 0 && !gM.choke && gM.temp < 40) return "Cold carbureted engine: turn on the CHOKE (C).";
 if (f->cold < 0.5f && gM.temp < 35) return "Alcohol fuels hate cold starts. Keep trying, or add a choke/DI.";
 if (f->diesel && gB.cr < 16) return "Low compression diesel: cold start is hard. Keep cranking.";
 return "Keep cranking...";
}

static void TestUpdate(float dt, int startHeld, int startPress) {
 const Spec *s = &gS;
 if (startPress && !gM.ign && !gM.failed) { gM.ign = 1; SfxPush(SFX_KEY); ToastAdd("Ignition ON", COL_DIM); }
 int crank = 0;
 const StarterDef *sd = &STARTERS[gB.starter];
 if (sd->elec) {
  crank = startHeld && !gM.failed;
  if (startPress && (gM.starterDead || (s->hasBatt && gM.volts < 9.6f))) AU.click++;
 } else if (startPress && gM.recoilT <= 0 && !gM.failed && gM.rpm < 400) {
  if (gM.recoilDead) { ToastAdd("The recoil is broken. Rebuild (R).", COL_RED); }
  else {
   gM.recoilT = 0.35f; gM.recoilStuck = 0; gPullAnim = 0.001f;
   float over = s->dispL / sd->maxL;
   if (over > 1 && !gDev && SRand(&gM) < Clampf((over - 1) * 0.8f, 0.2f, 0.95f)) { gM.recoilDead = 1; gM.recoilT = 0.12f; PartFail(&gM, "SNAP! The recoil spring broke: it's under-rated for this engine."); }
  }
 }
 if (gPullAnim > 0) { gPullAnim += dt / 0.6f; if (gPullAnim >= 1) gPullAnim = 0; }
 gDS.pullT = gPullAnim;
 int steps = (int)(dt * 1000 + 0.5f); if (steps < 1) steps = 1; if (steps > 50) steps = 50;
 int wasFailed = gM.failed;
 for (int i = 0; i < steps; i++) SimStep(&gM, &gB, s, 0.001f, crank);
 if (gM.recoilStuck) { gM.recoilStuck = 0; ToastAdd("Can't pull it through compression! Too big for a pull cord.", COL_RED); }
 if (gM.partMsgPending) { gM.partMsgPending = 0; ToastAdd(gM.partMsg, COL_RED); AU.bang += 0; SfxPush(SFX_WARN); }
 if (gM.dyno == 2) {
  gM.dyno = 0; gM.thrCmd = 0;
  if (gM.nd > 5) {
   ToastAdd(TextFormat("Dyno: %.0f hp, %.0f lb-ft", gM.dynoPeakHp, gM.dynoPeakTq * 0.7376f), COL_ACC); SfxPush(SFX_DONE);
   if (gB.forced == 0 && gM.dynoPeakHp / gS.dispL >= 100) Ach(2);
   if (gM.dynoPeakTq * 0.7376f >= 800) Ach(3);
   if (gM.dynoPeakHp >= 1000) Ach(4);
   if (FUELS[gB.fuel].diesel && gM.dynoPeakHp >= 400) Ach(6);
   if (gM.dynoPeakHp / gS.weight >= 1) Ach(10);
   if (gM.health >= 99 && gM.dynoPeakHp >= 500) Ach(11);
  }
 }
 if ((crank || gM.recoilT > 0) && !gM.running) gCrankT += dt;
 if (gCrankT > 4.f && !gDiagShown) { gDiagShown = 1; ToastAdd(Diagnose(), COL_YEL); }
 if (gM.running && gM.rpm > s->idle * 0.6f && !gWasRunning) {
  gWasRunning = 1; gCrankT = 0; gDiagShown = 0;
  if (!gStartedOnce) ToastAdd("IT'S ALIVE!", COL_GRN);
  gStartedOnce = 1;
 }
 if (gWasRunning && gM.rpm < 120 && !crank) { gWasRunning = 0; if (!gM.failed && gM.ign) { gToastQuiet = 1; ToastAdd("Stalled.", COL_YEL); SfxPush(SFX_STALL); } }
 if (gM.failed && !wasFailed) {
  AU.bang++; Ach(7);
  Vector3 c = LocalToWorld((Vector3){0, G.deck, 0});
  for (int i = 0; i < 40; i++) PuffAdd(c, (Vector3){(SRand(&gM) - 0.5f) * 3, SRand(&gM) * 2.5f, (SRand(&gM) - 0.5f) * 3}, 2.5f + SRand(&gM) * 2, 0.25f + SRand(&gM) * 0.3f, (Color){90, 90, 95, 200});
 }
 // challenges
 if (gM.running && gM.runTime > 3) {
  if (gB.cyl == 1 && !STARTERS[gB.starter].elec && !gB.alternator) Ach(0);
  if (!STARTERS[gB.starter].elec && gS.dispL >= 0.8f) Ach(8);
 }
 if (gM.running && gB.cycle == 1 && gM.rpm > 10000) Ach(1);
 if (gM.running && gM.rpm > 12000) Ach(5);
 if (gM.wotTime >= 60) Ach(9);
 // smoke: volume follows fuel burned, color/density follows how dirty the burn is
 static float puffT = 0;
 puffT -= dt;
 if (puffT <= 0 && (gM.running || gM.failed)) {
  puffT = 0.035f;
  const FuelDef *f = &FUELS[gB.fuel];
  float flow = Clampf(gS.dispL * Clampf(gM.rpm / 3000.f, 0.15f, 3.f) * (0.25f + 0.75f * Clampf(gM.load, 0, 1.3f)) * (gB.cycle ? 2.f : 1.f) * (1 + gM.boost / 14.7f), 0.02f, 40.f);
  static const float DIRT[] = {0.30f, 0.26f, 0.35f, 0.10f, 0.06f, 0.85f, 0.70f};
  float dirt = DIRT[gB.fuel];
  if (gB.cycle == 1) dirt += 0.35f * gS.lube;
  if (gM.choke && gM.temp > 50) dirt += 0.5f;
  if (gB.delivery == 0) dirt += 0.12f * (1 - Clampf((gM.temp - 20) / 45.f, 0, 1));
  float rich = f->diesel ? Clampf(gM.load - gM.spool * 0.6f, 0, 1) : 0;
  if (gM.health < 45) dirt += 0.4f;
  float amount = flow * dirt;
  float sm = gSet.smoke;
  int n = (int)Clampf(amount * 0.6f * (0.3f + 0.25f * sm) + SRand(&gM), 0, 4 + (int)(sm * 2));
  float size = 0.035f * sqrtf(fmaxf(gS.dispL, 0.1f)) * (0.7f + 0.6f * Clampf(amount / 6.f, 0, 2)) * (0.6f + 0.12f * sm);
  for (int oi = 0; oi < G.nOut; oi++) {
   Vector3 o = LocalToWorld(G.exhOut[oi]);
   for (int k = 0; k < n; k++) {
    Vector3 v = {1.0f + gM.rpm / 3500.f * (0.6f + 0.4f * SRand(&gM)), 0.15f + 0.2f * SRand(&gM), (SRand(&gM) - 0.5f) * 0.35f};
    Color c;
    if (gB.cycle == 1) c = (Color){175, 185, 205, (unsigned char)Clampf(50 + 40 * gS.lube, 0, 170)};
    else if (f->diesel) c = (Color){(unsigned char)Lerpf(120, 25, rich), (unsigned char)Lerpf(122, 25, rich), (unsigned char)Lerpf(126, 28, rich), (unsigned char)Clampf(70 + 120 * rich, 0, 200)};
    else if (gB.fuel == 6) c = (Color){190, 175, 140, 120};
    else if (gM.choke && gM.temp > 50) c = (Color){45, 45, 48, 140};
    else if (gM.health < 45) c = (Color){150, 165, 190, 130};
    else c = (Color){150, 152, 158, (unsigned char)Clampf(30 + 25 * dirt, 0, 110)};
    c.a = (unsigned char)Clampf(c.a * (0.7f + 0.09f * sm), 0, 235);
    if (sm > 0) PuffAdd(o, v, (1.4f + 1.2f * Clampf(amount / 8.f, 0, 1)) * (0.8f + 0.06f * sm), size * (0.8f + 0.4f * SRand(&gM)), c);
   }
   if (gM.temp < 40 && gM.running && SRand(&gM) < 0.5f) PuffAdd(o, (Vector3){0.8f, 0.3f, 0}, 1.4f, size * 1.2f, (Color){232, 232, 236, 70});
   if (!gB.cycle && fabsf(gB.tilt) > 65 && gM.running) PuffAdd(o, (Vector3){0.9f, 0.2f, 0}, 1.6f, size, (Color){150, 165, 190, 90});
  }
  if (gM.temp > gS.danger) PuffAdd(LocalToWorld((Vector3){0, G.deck + G.headH, 0}), (Vector3){0, 1.2f, 0}, 1.5f, 0.12f * sqrtf(gS.dispL), (Color){235, 235, 240, 110});
  if (gM.failed && gM.rpm > 10) PuffAdd(LocalToWorld((Vector3){0, G.deck, 0}), (Vector3){0, 0.8f, 0}, 2.5f, 0.15f * sqrtf(gS.dispL), (Color){80, 80, 85, 150});
 }
 PuffUpdate(dt);
}

static void TestHUD(int sw, int sh) {
 const Spec *s = &gS;
 char nm[64]; BuildName(&gB, nm, 64);
 Panel((Rectangle){12, 64, 360, 96}, (Color){22, 25, 30, 220});
 Tb(nm, 26, 74, 19, COL_TXT);
 Tx(TextFormat("%.2f L   %d cyl   redline %.0f", s->dispL, gB.cyl, s->redline), 26, 100, 14, COL_DIM);
 Tx(TextFormat("%s   %s", FUELS[gB.fuel].name, FORCED[gB.forced].name), 26, 120, 14, COL_DIM);
 // health + status
 float hw = 420, hx = sw * 0.5f - hw * 0.5f;
 PanelR((Rectangle){hx, 64, hw, 46}, 0.2f, (Color){22, 25, 30, 220});
 Tx("ENGINE HEALTH", hx + 12, 70, 13, COL_DIM);
 Color hc = gM.health > 60 ? COL_GRN : (gM.health > 30 ? COL_YEL : COL_RED);
 DrawRectangleRounded((Rectangle){hx + 12, 88, hw - 90, 12}, 1, 6, (Color){14, 15, 18, 255});
 DrawRectangleRounded((Rectangle){hx + 12, 88, (hw - 90) * gM.health / 100.f, 12}, 1, 6, hc);
 TxR(TextFormat("%.0f%%", gM.health), hx + hw - 12, 80, 20, hc, 1);
 // live warnings
 const char *wl[8]; Color wc[8]; int nwl = 0;
 int blink = ((int)(gTime * 4)) & 1;
#define WL(cond, txt, col) if ((cond) && nwl < 8) { wl[nwl] = txt; wc[nwl] = col; nwl++; }
 WL(gM.dmgRate[DMG_KNOCK] > 0.5f, "KNOCK!", COL_RED);
 WL(gM.dmgRate[DMG_ROD] > 0, "OVER-REV: ROD STRESS", COL_RED);
 WL(gB.cycle == 0 && gM.rpm > s->floatRpm && gM.running, "VALVE FLOAT", COL_YEL);
 WL(gM.dmgRate[DMG_PRESS] > 0, "CYLINDER PRESSURE!", COL_RED);
 WL(gM.temp > s->danger, "OVERHEATING", COL_RED);
 WL(gM.dmgRate[DMG_OILP] > 0 && (gB.lube > 0 || gM.oilT > 150), gM.oilT > 150 ? "OIL OVERHEATING" : "LOW OIL PRESSURE", COL_RED);
 WL(gM.dmgRate[DMG_LUBE] > 0.3f, "NOT ENOUGH OIL IN FUEL", COL_RED);
 WL(gM.dmgRate[DMG_DIRT] > 0.3f, "SUCKING IN DIRT", COL_YEL);
 WL(!gB.cycle && gM.running && gM.oilSupply < 0.6f, "OIL STARVATION", COL_RED);
 WL(!gB.cycle && gB.lube > 0 && gM.oilHead > s->oilHeadCap * 0.7f, "OIL POOLING IN HEADS", COL_YEL);
 WL(gM.dmgRate[DMG_TURBO] > 0.2f, "TURBO OVERSPEED", COL_RED);
 WL(s->hasBatt && gM.batt < 0.2f, "BATTERY LOW", COL_YEL);
 WL(gM.cut && gM.running, "REV LIMITER", COL_ACC);
#undef WL
 float wx = sw * 0.5f;
 float tot = 0; for (int i = 0; i < nwl; i++) tot += Tw(wl[i], 15, 1) + 26;
 float cx = wx - tot * 0.5f;
 for (int i = 0; i < nwl; i++) {
  float ww = Tw(wl[i], 15, 1) + 18;
  Color bg = wc[i]; if (!blink && (wc[i].r == COL_RED.r)) bg = Shade(bg, 0.7f);
  DrawRectangleRounded((Rectangle){cx, 118, ww, 26}, 0.4f, 6, bg);
  TxC(wl[i], cx + ww * 0.5f, 123, 15, (Color){18, 18, 20, 255}, 1);
  cx += ww + 8;
 }
 // gauges
 float gy = sh - 160;
 float tmax = fmaxf(3000, ceilf(s->redline * 1.2f / 1000) * 1000);
 int maj = (int)(tmax / 1000); float div = 1000;
 if (maj > 12) { maj = (int)ceilf(tmax / 2000); tmax = maj * 2000.f; }
 Gauge(sw * 0.5f, gy, 132, gM.rpm, 0, tmax, s->redline, "x1000 rpm", TextFormat("%.0f", gM.rpm), maj, div, COL_ACC);
 float hp = fmaxf(0, gM.tbrake) * gM.rpm / 7121.f;
 float sp = Clampf((sw * 0.5f - 330) / 2.6f, 125, 175), gr = fminf(72, sp * 0.44f);
 float L1 = sw * 0.5f - 125 - sp * 0.85f, L2 = L1 - sp, R1 = sw * 0.5f + 125 + sp * 0.85f, R2 = R1 + sp, R3 = R2 + sp;
 if (gB.cooling == 0) Gauge(L1, gy + 30, gr, gM.temp, 20, 240, s->danger, "BLOCK (CHT) C", TextFormat("%.0f", gM.temp), 4, 1, gM.temp > s->danger ? COL_RED : COL_TXT);
 else Gauge(L1, gy + 30, gr, gM.temp, 20, 140, s->danger, "BLOCK (WATER) C", TextFormat("%.0f", gM.temp), 4, 1, gM.temp > s->danger ? COL_RED : COL_TXT);
 if (gB.cycle == 0 && gB.lube > 0) Gauge(L2, gy + 30, gr, gM.oilP, 0, 80, 999, "OIL  psi", TextFormat("%.0f", gM.oilP), 4, 1, gM.oilP < 10 && gM.rpm > 2000 ? COL_RED : COL_TXT);
 else if (gB.cycle == 0) Gauge(L2, gy + 30, gr, gM.oilSplash * 100, 0, 100, 999, "SPLASH  %", TextFormat("%.0f", gM.oilSplash * 100), 5, 1, gM.oilSplash < 0.6f && gM.rpm > 1500 ? COL_RED : COL_TXT);
 if (gB.cycle == 0) {
  float dx = L2 + gr + 10, dy = gy - 30, dh = 120, cap = fmaxf(s->oilMax, 0.1f);
  float lv = Clampf((gM.oilSump < 0 ? gB.oilL : gM.oilSump) / cap, 0, 1);
  DrawRectangleRounded((Rectangle){dx, dy, 14, dh}, 0.5f, 6, (Color){12, 13, 16, 235});
  DrawRectangleRounded((Rectangle){dx + 2, dy + dh * (1 - lv), 10, dh * lv}, 0.5f, 6, (Color){214, 150, 32, 255});
  float mn = dy + dh * (1 - s->oilNeed / cap), mx = dy + dh * (1 - s->oilMax / cap);
  DrawLineEx((Vector2){dx - 4, mn}, (Vector2){dx + 18, mn}, 2, COL_RED); DrawLineEx((Vector2){dx - 4, mx}, (Vector2){dx + 18, mx}, 2, COL_GRN);
  TxC("OIL", dx + 7, dy + dh + 4, 11, COL_DIM, 1);
 }
 else {
  DrawCircleV((Vector2){L2, gy + 30}, gr + 8, (Color){12, 13, 16, 235});
  TxC("OIL MIX", L2, gy, 13, COL_DIM, 0);
  TxC(gB.mix < 1 ? "NONE" : TextFormat("%.0f:1", gB.mix), L2, gy + 22, 24, s->lube < 0.6f ? COL_RED : COL_TXT, 1);
 }
 if (gB.cycle == 0) Gauge(R1, gy + 30, gr, gM.oilT, 20, 180, 150, "OIL TEMP C", TextFormat("%.0f", gM.oilT), 4, 1, gM.oilT > 150 ? COL_RED : COL_TXT);
 else Gauge(R1, gy + 30, gr, gM.egt, 0, 1000, 850, "EGT C", TextFormat("%.0f", gM.egt), 5, 1, COL_TXT);
 if (gB.forced) Gauge(R2, gy + 30, gr, gM.boost, 0, fmaxf(10, ceilf(gB.boost * 1.25f / 10) * 10), gB.forced == 4 ? FORCED[4].maxb : FORCED[gB.forced].maxb, "BOOST  psi", TextFormat("%.1f", gM.boost), 5, 1, COL_ACC);
 else Gauge(R2, gy + 30, gr, fmaxf(gM.thrAct, gM.idleOpen) * 100, 0, 100, 999, "THROTTLE  %", TextFormat("%.0f", fmaxf(gM.thrAct, gM.idleOpen) * 100), 5, 1, COL_TXT);
 Gauge(R3, gy + 30, gr, s->hasBatt ? gM.volts : 0, 8, 16, 999, s->hasBatt ? "VOLTS" : "NO BATTERY", s->hasBatt ? TextFormat("%.1f", gM.volts) : "--", 4, 1, s->hasBatt && gM.volts < 11.5f ? COL_YEL : COL_TXT);
 // readout
 PanelR((Rectangle){sw * 0.5f - 170, gy - 224, 340, 60}, 0.2f, (Color){12, 13, 16, 220});
 float ghp = fmaxf(0, gM.tgross) * gM.rpm / 7121.f;
 TxC(TextFormat("%.0f hp combustion", ghp), sw * 0.5f, gy - 216, 19, COL_TXT, 1);
 TxC(gM.dyno ? TextFormat("DYNO: %.0f hp / %.0f lb-ft at the crank", hp, fmaxf(0, gM.tbrake) * 0.7376f) : (gM.running ? (gM.thrAct > 0.05f ? TextFormat("net %.0f hp (free-revving: no load)", hp) : "idling, no load") : (gM.rpm > 30 ? "cranking" : "stopped")), sw * 0.5f, gy - 192, 14, gM.dyno ? COL_ACC : COL_DIM, 0);
 // throttle bar
 Rectangle tb = {sw - 64, sh - 330, 40, 300};
 PanelR((Rectangle){tb.x - 8, tb.y - 34, tb.width + 16, tb.height + 46}, 0.2f, (Color){22, 25, 30, 220});
 TxC("THR", tb.x + tb.width * 0.5f, tb.y - 26, 13, COL_DIM, 1);
 DrawRectangleRounded(tb, 0.3f, 6, (Color){14, 15, 18, 255});
 float tf = Clampf(fmaxf(gM.thrCmd, gM.dyno ? 1 : 0), 0, 1);
 DrawRectangleRounded((Rectangle){tb.x + 4, tb.y + 4 + (tb.height - 8) * (1 - tf), tb.width - 8, (tb.height - 8) * tf}, 0.3f, 6, COL_ACC);
 // controls
 float bx = 12, by = sh - 430, bw = 220, bh = 40;
 Panel((Rectangle){bx, by - 10, bw + 20, 428}, (Color){22, 25, 30, 220});
 bx += 10;
 extern int gBtnIgn, gBtnStart, gBtnChoke, gBtnLim, gBtnDyno, gBtnCut, gBtnBack, gStartHeldUI;
 gBtnIgn = UiButton((Rectangle){bx, by, bw, bh}, gM.ign ? "IGNITION: ON   [I]" : "IGNITION: OFF   [I]", gM.ign ? 2 : 0, !gM.failed); by += bh + 8;
 Rectangle sr = {bx, by, bw, bh};
 gBtnStart = UiButton(sr, gB.starter ? "START (hold)   [E]" : "PULL CORD   [E]", 1, !gM.failed);
 gStartHeldUI = CheckCollisionPointRec(MP, sr) && MDown && !gM.failed;
 by += bh + 8;
 gBtnChoke = UiButton((Rectangle){bx, by, bw, bh}, gB.delivery == 0 ? (gM.choke ? "CHOKE: ON   [C]" : "CHOKE: OFF   [C]") : "CHOKE: n/a (EFI)", gM.choke ? 2 : 0, gB.delivery == 0); by += bh + 8;
 {
  const char *nmS = gB.gov == 3 ? "LIMITER" : (gB.gov ? "GOVERNOR" : "SPEED CTRL");
  gBtnLim = UiButton((Rectangle){bx, by, bw, bh}, gB.gov ? TextFormat("%s: %s   [L]", nmS, gM.limiter ? "ON" : "OFF") : "NO SPEED CONTROL", gM.limiter || !gB.gov ? 0 : 3, gB.gov != 0); by += bh + 8;
 }
 gBtnDyno = UiButton((Rectangle){bx, by, bw, bh}, gM.dyno ? "ABORT PULL   [D]" : "DYNO PULL   [D]", gM.dyno ? 3 : 0, gM.running || gM.dyno); by += bh + 8;
 gBtnCut = UiButton((Rectangle){bx, by, bw, bh}, gDS.cut ? "CUTAWAY: ON   [X]" : "CUTAWAY: OFF   [X]", gDS.cut ? 2 : 0, 1); by += bh + 8;
 if (UiButton((Rectangle){bx, by, bw, bh}, gDS.oilView ? "OIL FLOW: ON   [V]" : "OIL FLOW: OFF   [V]", gDS.oilView ? 2 : 0, 1)) gDS.oilView ^= 1; by += bh + 8;
 gBtnBack = UiButton((Rectangle){bx, by, bw, bh}, "< BACK TO GARAGE   [G]", 0, 1); by += bh + 8;
 Tx("Rev: hold W / Up / Space.  1-0 lock throttle", bx, by + 2, 13, COL_DIM);
 Tx("T = release lock.  M mute, N music, +/- vol", bx, by + 18, 13, COL_DIM);
 // dyno graph
 if (gM.nd > 1 || gM.npd > 1) {
  Rectangle gr = {sw - 492, 64, 480, 300};
  Panel(gr, (Color){22, 25, 30, 230});
  DynoGraph(gr, gM.nd, gM.drpm, gM.dhp, gM.dtq, gM.npd, gM.pdrpm, gM.pdhp, gM.pdtq, s->redline, 0);
  if (gM.nd > 1) TxR(TextFormat("peak %.0f hp  /  %.0f lb-ft", gM.dynoPeakHp, gM.dynoPeakTq * 0.7376f), gr.x + gr.width - 12, gr.y + 10, 14, COL_TXT, 1);
 }
 // failure modal
 if (gM.failed) {
  float mw = 640, mh = 250, mx = sw * 0.5f - mw * 0.5f, my = sh * 0.5f - mh * 0.5f - 60;
  Panel((Rectangle){mx, my, mw, mh}, (Color){30, 18, 18, 245});
  DrawRectangleRoundedLinesEx((Rectangle){mx, my, mw, mh}, 0.04f, 8, 2, COL_RED);
  TxC("ENGINE DESTROYED", sw * 0.5f, my + 20, 30, COL_RED, 1);
  TxC(DMG_TITLE[gM.failCause], sw * 0.5f, my + 62, 22, COL_TXT, 1);
  const char *why = "";
  switch (gM.failCause) {
  case DMG_KNOCK: why = TextFormat("Detonation burned through a piston. Lower compression or boost, add an intercooler, or run better fuel than %s.", FUELS[gB.fuel].name); break;
  case DMG_ROD: why = TextFormat("Reciprocating forces beat the rods past %.0f rpm. Lighter pistons, smaller bore, shorter stroke, stronger rods, or keep the limiter on.", s->pistonRpm); break;
  case DMG_VALVE: why = TextFormat("The valves floated past %.0f rpm and met the pistons. Lighter valvetrain, more valves per cylinder, or respect the redline.", s->floatRpm); break;
  case DMG_PRESS: why = TextFormat("Cylinder pressure beat the %.0f bar the block and internals can hold. Stronger block, forged/billet parts, or less boost.", s->pLimit); break;
  case DMG_HEAT: why = "It cooked itself. Bigger radiator, liquid cooling, or less sustained load."; break;
  case DMG_OILP: why = "Oil pressure dropped and a rod bearing spun. Thicker oil and keep temps under control."; break;
  case DMG_LUBE: why = "A 2-stroke gets ALL its lubrication from the fuel. Mix in oil: 32:1 to 50:1."; break;
  case DMG_DIRT: why = "Unfiltered air sandblasted the cylinder walls. Put a filter on it."; break;
  case DMG_TURBO: why = "The turbo spun past its limit and came apart. Lower the boost or use a bigger turbo."; break;
  case DMG_FLY: why = TextFormat("The flywheel spun past its %.0f rpm rating and burst. Fit a stronger one or keep the governor/limiter on.", FLY[gB.flywheel].maxRpm); break;
  case DMG_LEAN: why = "The injectors couldn't keep up: it ran lean and melted a piston. Bigger injectors or less boost."; break;
  case DMG_VIB: why = "Unbalanced shaking fatigued the crank until it cracked. Better-balanced layout, balance shafts, lighter pistons, stronger caps, or fewer rpm."; break;
  case DMG_BELT: why = "The timing belt jumped teeth and let the valves hit the pistons. Use a chain or gear drive for high rpm."; break;
  }
  char line[160]; float ty = my + 104; const char *p = why;
  while (*p) {
   int li = 0, last = -1;
   while (p[li] && li < 150) { line[li] = p[li]; line[li + 1] = 0; if (p[li] == ' ') last = li; if (Tw(line, 17, 0) > mw - 60) break; li++; }
   if (p[li] && last > 0) li = last + 1;
   memcpy(line, p, li); line[li] = 0;
   TxC(line, sw * 0.5f, ty, 17, COL_TXT, 0); ty += 23; p += li;
  }
  extern int gBtnRebuild;
  gBtnRebuild = UiButton((Rectangle){sw * 0.5f - 230, my + mh - 62, 220, 46}, "REBUILD   [R]", 1, 1);
  if (UiButton((Rectangle){sw * 0.5f + 10, my + mh - 62, 220, 46}, "BACK TO GARAGE", 0, 1)) gBtnBack = 1;
 }
}
int gBtnIgn, gBtnStart, gBtnChoke, gBtnLim, gBtnDyno, gBtnCut, gBtnBack, gStartHeldUI, gBtnRebuild;

static void ViewBar(int sw) {
 static const char *nm[] = {"CUTAWAY", "HEADS", "INTAKE", "EXHAUST", "COOLING", "ACCESS.", "STAND", "OIL FLOW"};
 static const unsigned masks[] = {0, (1u << G_HEAD) | (1u << G_PLUG), (1u << G_INTAKE) | (1u << G_FILTER) | (1u << G_SC) | (1u << G_CHARGE), (1u << G_EXH) | (1u << G_TURBO), 1u << G_COOL, (1u << G_ALT) | (1u << G_STARTER) | (1u << G_FRONT) | (1u << G_OILSYS), 1u << G_STAND};
 float x0 = 494, x1 = sw - 424, w = (x1 - x0) / 8.f;
 if (w > 116) { x0 += (w - 116) * 4.f; w = 116; }
 for (int i = 0; i < 8; i++) {
  int on = i == 0 ? gDS.cut : (i == 7 ? gDS.oilView : !((gHide & masks[i]) == masks[i]));
  if (UiButton((Rectangle){x0 + i * w, 64, w - 6, 30}, nm[i], on ? (i == 0 ? 2 : 0) : 0, 1)) {
   if (i == 0) gDS.cut ^= 1; else if (i == 7) gDS.oilView ^= 1; else gHide ^= masks[i];
  }
  if (!on && i && i < 7) DrawLineEx((Vector2){x0 + i * w + 8, 79}, (Vector2){x0 + i * w + w - 14, 79}, 2, (Color){235, 70, 55, 200});
 }
}

static void Overview(int sw, int sh) {
 const Build *b = &gB; const Spec *s = &gS; const FuelDef *f = &FUELS[b->fuel];
 float w = fminf(1180, sw - 60), h = fminf(720, sh - 90), x = sw * 0.5f - w * 0.5f, y = 70;
 Panel((Rectangle){x, y, w, h}, (Color){18, 20, 25, 252});
 DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, 0.02f, 8, 1.5f, COL_LINE);
 char nm[64]; BuildName(b, nm, 64);
 Tb(nm, x + 24, y + 18, 28, COL_TXT);
 Tx("ENGINE OVERVIEW", x + 26, y + 52, 14, COL_ACC);
 if (UiButton((Rectangle){x + w - 110, y + 16, 92, 34}, "CLOSE", 0, 1)) gShowOverview = 0;
 float cw = (w - 72) / 3.f, c1 = x + 24, c2 = c1 + cw + 12, c3 = c2 + cw + 12, yy;
 // column 1: numbers + cost
 yy = y + 86;
 Tb("PERFORMANCE", c1, yy, 15, COL_DIM); yy += 24;
 const char *lay = b->layout == 5 ? CustomKind(b, 0) : b->layout == 0 ? "Single" : (b->layout == 1 ? "Inline" : (b->layout == 4 ? "Boxer" : "V"));
 struct { const char *k; char v[64]; } r[30]; int n = 0;
#define ROWV(K, ...) do { r[n].k = K; snprintf(r[n].v, 64, __VA_ARGS__); n++; } while (0)
 ROWV("Layout", "%s%s%d %s, %.0f deg tilt", lay, lay[strlen(lay) - 1] == '-' ? "" : "-", NumCyl(b), b->cycle ? "2-stroke" : "4-stroke", b->tilt);
 ROWV("Displacement", "%.2f L  (%.0f cc)", s->dispL, s->dispL * 1000);
 ROWV("Bore x stroke", "%.1f x %.1f mm", b->bore, b->stroke);
 ROWV("Compression", "%.1f : 1", b->cr);
 ROWV("Peak power", "%.0f hp @ %.0f rpm", s->estHp, s->estHpRpm);
 ROWV("Peak torque", "%.0f lb-ft (%.0f Nm) @ %.0f", s->estTq * 0.7376f, s->estTq, s->estTqRpm);
 ROWV("Redline", "%.0f rpm%s", s->redline, s->govRpm > 0 ? TextFormat(" (governed %.0f)", s->govRpm) : "");
 ROWV("Weight", "%.0f kg  (%.0f lb)", s->weight, s->weight * 2.2046f);
 ROWV("Specific output", "%.0f hp/L   %.2f hp/kg", s->estHp / fmaxf(s->dispL, 0.01f), s->estHp / s->weight);
 for (int i = 0; i < n; i++) { Tx(r[i].k, c1, yy, 15, COL_DIM); TxR(r[i].v, c1 + cw, yy, 15, COL_TXT, 1); yy += 21; }
 yy += 12; Tb("COST", c1, yy, 15, COL_DIM); yy += 24; n = 0;
 ROWV("Manufacture, one-off prototype", "$%.0f", s->mfgOne);
 ROWV("Manufacture @ 10,000 units", "$%.0f each", s->mfgK);
 ROWV("Est. retail price", "$%.0f", s->cost);
 ROWV("Margin per unit", "$%.0f", s->cost - s->mfgK);
 for (int i = 0; i < n; i++) { Tx(r[i].k, c1, yy, 15, COL_DIM); TxR(r[i].v, c1 + cw, yy, 15, COL_TXT, 1); yy += 21; }
 // economy
 static const float LHV[] = {32.f, 32.f, 32.f, 24.f, 15.8f, 36.f, 11.f};
 float th = (1 - powf(b->cr, -0.28f));
 float eff = th * 0.62f * (f->diesel ? 1.18f : 1.f) * (b->delivery == 3 ? 1.05f : 1.f) * (b->cycle ? 0.72f : 1.f) * (b->delivery == 0 ? 0.95f : 1.f);
 eff = Clampf(eff, 0.08f, 0.48f);
 float cruiseKw = s->estHp * 0.7457f * 0.25f, fullKw = s->estHp * 0.7457f;
 float cruiseLh = cruiseKw / eff * 3.6f / LHV[b->fuel], fullLh = fullKw / eff * 3.6f / LHV[b->fuel];
 yy += 12; Tb("ECONOMY", c1, yy, 15, COL_DIM); yy += 24; n = 0;
 ROWV("Peak thermal efficiency", "%.0f%%", eff * 100);
 ROWV("Cruise (25% power)", "%.1f L/h  (%.2f gal/h)", cruiseLh, cruiseLh * 0.264f);
 ROWV("Wide open", "%.1f L/h  (%.1f gal/h)", fullLh, fullLh * 0.264f);
 ROWV("Range on a 50 L tank", "~%.1f h cruising", 50.f / fmaxf(cruiseLh, 0.01f));
 for (int i = 0; i < n; i++) { Tx(r[i].k, c1, yy, 15, COL_DIM); TxR(r[i].v, c1 + cw, yy, 15, COL_TXT, 1); yy += 21; }
 // column 2: uses
 yy = y + 86;
 Tb("GOOD FOR", c2, yy, 15, COL_DIM); yy += 26;
 const char *use[12]; const char *why[12]; int nu = 0;
#define USE(a, bb) do { if (nu < 12) { use[nu] = a; why[nu] = bb; nu++; } } while (0)
 float hp = s->estHp, lbft = s->estTq * 0.7376f, pw = hp / s->weight;
 if (b->fuel == 6) USE("Top Fuel drag racing", "Absurd power for a few seconds, then a rebuild.");
 if (hp < 15 && s->govRpm > 0) USE("Lawn & garden / generators", "Governed, low power, simple.");
 if (hp >= 3 && hp < 25 && s->dispL < 0.5f) USE("Go-karts & minibikes", "Small, light, enough grunt for one rider.");
 if (b->cycle && pw > 0.8f) USE("Dirt bikes / motocross", "Light 2-stroke punch, high rpm.");
 if (hp >= 20 && hp < 220 && s->weight < 90 && b->cyl <= 6) USE("Motorcycles", "Compact and light for its power.");
 if (s->redline > 10000 && hp > 60) USE("Superbikes / race bikes", "Screams past 10k rpm.");
 if (hp >= 60 && hp < 220 && s->weight >= 60 && !f->diesel) USE("Compact & economy cars", "Reasonable power, decent efficiency.");
 if (hp >= 200 && hp < 700 && !f->diesel && b->fuel != 6) USE("Sports & muscle cars", "Big power with a usable rev range.");
 if (hp >= 700 && b->fuel != 6) USE("Hypercars / drag cars", "Serious horsepower.");
 if (lbft > 450 && s->redline < 5000) USE("Trucks & towing", "Lots of low-rpm torque.");
 if (f->diesel && lbft > 300) USE("Tractors & heavy equipment", "Diesel torque all day.");
 if (lbft > 250 && s->redline < 6500 && b->cooling) USE("Boats / marine", "Steady load and big torque, liquid cooled.");
 if (b->cooling == 0 && (b->layout == 4 || b->layout == 1) && hp > 60 && hp < 450 && pw > 0.6f && s->redline < 7000) USE("Light aircraft", "Air-cooled, balanced, good power-to-weight.");
 if (hp > 15 && s->govRpm > 0) USE("Generator sets / pumps", "Holds a steady rpm under load.");
 if (nu == 0) USE("Science fair", "Doesn't fit an obvious job yet. Keep tuning.");
 for (int i = 0; i < nu && yy < y + h - 60; i++) {
  DrawCircleV((Vector2){c2 + 6, yy + 9}, 4, COL_ACC);
  Tb(use[i], c2 + 18, yy, 16, COL_TXT); yy += 20;
  Tx(why[i], c2 + 18, yy, 13, COL_DIM); yy += 24;
 }
 int bad = 0; for (int i = 0; i < gNWarn; i++) if (gWarnSev[i] >= 2) bad++;
 if (bad) { yy += 8; Tb(TextFormat("%d serious issue%s: fix before production.", bad, bad > 1 ? "s" : ""), c2, yy, 15, COL_RED); yy += 22; }
 // column 3: parts + settings
 yy = y + 86;
 Tb("PARTS & SETTINGS", c3, yy, 15, COL_DIM); yy += 24; n = 0;
 ROWV("Fuel", "%s", f->name);
 ROWV("Block", "%s", BLOCKS[b->block].name); ROWV("Block design", "%.0f mm walls, %.0f mm between bores, %s", WallMM(b), SpacingMM(b), DECKS[b->deck]);
 ROWV("Bottom end", "%s, %s%s%s", SKIRTS[b->skirt], CAPS[b->caps], b->balShaft ? ", balance shafts" : "", b->crankFlat ? ", flat-plane" : "");
 ROWV("Balance", "primary %.2f, secondary %.2f, rocking %.2f", s->prim, s->sec, s->rock);
 ROWV("Ignition", "%s", IGNS[b->ignition ? b->ignition : (s->magneto ? 3 : 2)]);
 if (!b->cycle && b->valvetrain > 0) ROWV("Cam drive", "%s", CAMDRV[b->camDrive]);
 if (b->placeMan) ROWV("Accessories", "alternator %+.0f deg, starter %+.0f deg (hand-placed)", b->altAng, b->stAng);
 ROWV("Pistons & rods", "%s", INTERN[b->internals].name);
 ROWV("Flywheel", "%s", FLY[b->flywheel].name);
 ROWV("Valvetrain", "%s", b->cycle ? VT2[b->valvetrain].name : TextFormat("%s, %d valves/cyl", VT4[b->valvetrain].name, b->valvetrain ? VALVES[b->valves].n : 2));
 ROWV("Cam / ports", "%s", b->cycle ? CAMS[b->cam].name2 : CAMS[b->cam].name);
 ROWV("Fuel delivery", "%s", DELIV[b->delivery].name);
 ROWV(b->delivery == 0 ? "Carburetor" : "Injectors", "%s", b->delivery == 0 ? CARBS[b->carb].name : TextFormat("%d x %s", s->ninj, INJ[b->inj].name));
 ROWV("Air filter", "%s", FILTERS[b->filter].name);
 ROWV("Induction", "%s%s", FORCED[b->forced].name, b->forced ? TextFormat(", %.0f psi%s", b->boost, b->intercooler ? ", intercooled" : "") : "");
 ROWV("Cooling", "%s", COOL[b->cooling].name);
 ROWV("Exhaust", "%s", EXH[b->exhaust].name);
 ROWV("Starter", "%s", STARTERS[b->starter].name);
 ROWV("Charging", "%s", ALTS[b->alternator].name);
 ROWV("Speed control", "%s", b->gov == 4 ? TextFormat("Adjustable governor @ %.0f rpm", GovSet(b)) : GOVS[b->gov]);
 if (!b->cycle) ROWV("Lubrication", "%s, %.1f L%s", LUBES[b->lube], b->oilL, b->oilCooler ? ", cooler" : "");
 ROWV(b->cycle ? "Oil mix" : "Oil", "%s", b->cycle ? (b->mix < 1 ? "NONE" : TextFormat("%.0f : 1", b->mix)) : OILS[b->oil].name);
 for (int i = 0; i < n && yy < y + h - 24; i++) {
  Tx(r[i].k, c3, yy, 14, COL_DIM);
  float sz = 14; while (Tw(r[i].v, sz, 1) > cw * 0.62f && sz > 10) sz -= 0.5f;
  TxR(r[i].v, c3 + cw, yy, sz, COL_TXT, 1); yy += 21;
 }
#undef ROWV
#undef USE
}

static void SettingsPanel(int sw, int sh) {
 float w = 520, h = 628, x = sw * 0.5f - w * 0.5f, y = 66;
 Panel((Rectangle){x, y, w, h}, (Color){18, 20, 25, 252});
 DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, 0.03f, 8, 1.5f, COL_LINE);
 Tb("SETTINGS", x + 20, y + 16, 24, COL_TXT);
 if (UiButton((Rectangle){x + w - 110, y + 14, 92, 34}, "CLOSE", 0, 1)) { gShowSettings = 0; SettingsSave(); }
 float yy = y + 66, rw = w - 40, rx = x + 20; int d; char vb[32];
 Tb("CAMERA", rx, yy, 14, COL_DIM); yy += 22;
 if ((d = UiSelect((Rectangle){rx, yy, rw, 40}, "Invert X (orbit)", gSet.invertX ? "On" : "Off", "Flip left/right orbit direction.", 1))) gSet.invertX ^= 1; yy += 46;
 if ((d = UiSelect((Rectangle){rx, yy, rw, 40}, "Invert Y (tilt)", gSet.invertY ? "On" : "Off", "Flip up/down tilt direction.", 1))) gSet.invertY ^= 1; yy += 46;
 snprintf(vb, 32, "%.2fx", gSet.sens); UiSlider(20, (Rectangle){rx, yy, rw, 40}, "Mouse sensitivity", &gSet.sens, 0.2f, 3.f, 0.05f, vb, NULL, 1); yy += 46;
 snprintf(vb, 32, "%.2fx", gSet.zoom); UiSlider(21, (Rectangle){rx, yy, rw, 40}, "Zoom speed", &gSet.zoom, 0.2f, 3.f, 0.05f, vb, NULL, 1); yy += 54;
 Tb("AUDIO", rx, yy, 14, COL_DIM); yy += 22;
 snprintf(vb, 32, "%.0f%%", gSet.master * 100); UiSlider(22, (Rectangle){rx, yy, rw, 40}, "Master", &gSet.master, 0, 1, 0.05f, vb, NULL, 1); yy += 46;
 snprintf(vb, 32, "%.0f%%", gSet.eng * 100); UiSlider(23, (Rectangle){rx, yy, rw, 40}, "Engine", &gSet.eng, 0, 2, 0.05f, vb, NULL, 1); yy += 46;
 snprintf(vb, 32, "%.0f%%", gSet.music * 100); UiSlider(24, (Rectangle){rx, yy, rw, 40}, "Music", &gSet.music, 0, 1, 0.05f, vb, NULL, 1); yy += 46;
 snprintf(vb, 32, "%.0f%%", gSet.sfx * 100); UiSlider(25, (Rectangle){rx, yy, rw, 40}, "Effects", &gSet.sfx, 0, 2, 0.05f, vb, NULL, 1); yy += 54;
 Tb("GRAPHICS", rx, yy, 14, COL_DIM); yy += 22;
 snprintf(vb, 32, "%.0fx", gSet.smoke); UiSlider(26, (Rectangle){rx, yy, rw, 40}, "Smoke amount", &gSet.smoke, 0, 20, 1, vb, NULL, 1); yy += 46;
 if ((d = UiSelect((Rectangle){rx, yy, rw, 40}, "Motion blur", gSet.blur ? "On" : "Off", "Blurs pistons, flywheel and fan at real rpm.", 1))) gSet.blur ^= 1; yy += 46;
 (void)sh;
}

static void TopBar(int sw) {
 DrawRectangle(0, 0, sw, 52, (Color){13, 14, 17, 255});
 DrawRectangle(0, 52, sw, 1, COL_LINE);
 Tb("ENGINE", 18, 12, 26, COL_ACC);
 Tb("GARAGE", 18 + Tw("ENGINE ", 26, 1), 12, 26, COL_TXT);
 Tx(gShop ? "Shop office" : (gMode ? "Dyno cell" : "Build bench"), 26 + Tw("ENGINE GARAGE", 26, 1), 19, 16, COL_DIM);
 if (gDev) { DrawRectangleRounded((Rectangle){340, 13, 120, 26}, 0.4f, 6, COL_RED); TxC("DEV 2323", 400, 17, 15, COL_TXT, 1); }
 if (UiButton((Rectangle){sw - 222, 9, 210, 34}, TextFormat("CHALLENGES  %d/%d", AchCount(), NACH), gShowAch ? 1 : 0, 1)) gShowAch ^= 1;
 if (UiButton((Rectangle){sw - 400, 9, 170, 34}, gMusicOn ? "MUSIC: ON  [N]" : "MUSIC: OFF  [N]", 0, 1)) gMusicOn ^= 1;
 if (UiButton((Rectangle){470, 9, 250, 34}, TextFormat("SHOP  $%.0f  [H]", C.money), gShop ? 1 : 2, 1)) { gShop ^= 1; if (gShop) gMode = 0; }
 if (UiButton((Rectangle){sw - 530, 9, 122, 34}, "SETTINGS", gShowSettings ? 1 : 0, 1)) { gShowSettings ^= 1; if (!gShowSettings) SettingsSave(); }
 TxR(gMuted ? "SOUND MUTED [M]" : TextFormat("VOL %.0f%%  (+/-)", gSet.master * 100), sw - 542, 18, 14, COL_DIM, 0);
}

static void AchPanel(int sw, int sh) {
 float w = 520, h = 70 + NACH * 44, x = sw - w - 12, y = 60;
 Panel((Rectangle){x, y, w, h}, (Color){20, 22, 27, 250});
 Tb("CHALLENGES", x + 16, y + 14, 20, COL_TXT);
 for (int i = 0; i < NACH; i++) {
  int done = (gSave.ach >> i) & 1;
  float yy = y + 52 + i * 44;
  DrawRectangleRounded((Rectangle){x + 12, yy, w - 24, 38}, 0.2f, 6, done ? (Color){34, 52, 38, 255} : COL_PANEL);
  DrawCircleV((Vector2){x + 32, yy + 19}, 8, done ? COL_GRN : COL_LINE);
  Tb(ACH_NAME[i], x + 50, yy + 3, 15, done ? COL_GRN : COL_TXT);
  Tx(ACH_DESC[i], x + 50, yy + 20, 13, COL_DIM);
 }
 (void)sh;
}

static void Toasts(int sw, float dt) {
 float y = gMode ? 154 : 66;
 if (gMode == 0) y = 70;
 if (gShop) y = GetScreenHeight() - 60;
 for (int i = 0; i < 6; i++) {
  if (gToast[i].t <= 0) continue;
  gToast[i].t -= dt;
  float a = Clampf(gToast[i].t, 0, 1);
  float sz = 18, w = Tw(gToast[i].s, sz, 1) + 32;
  float x = sw * 0.5f - w * 0.5f;
  if (gMode == 0) x = 500;
  if (gShop) x = sw - w - 30;
  DrawRectangleRounded((Rectangle){x, y, w, 34}, 0.4f, 6, Fade((Color){12, 13, 16, 255}, 0.9f * a));
  Tb(gToast[i].s, x + 16, y + 7, sz, Fade(gToast[i].c, a));
  y += gShop ? -40 : 40;
 }
}

// ===== autotest =====
static void Shot(const char *name) { TakeScreenshot(TextFormat("%s/%s.png", gShotDir, name)); }
static int gAtFrame = 0, gAtStart = 0, gAtThr = 0;

static void AutoAudioTest(void) {
 float buf[4096 * 2];
 FILE *f = fopen(TextFormat("%s/audio.raw", gShotDir), "wb");
 double sum = 0; float peak = 0; int nan = 0;
 AU.running = 1; AU.q = 1; AU.volume = 0.7f; AU.engVol = 1; AU.sfxVol = 1;
 for (int blk = 0; blk < 60; blk++) {
  AU.rpm = 800 + blk * 100.f; AU.load = blk < 30 ? 0.3f : 1.0f; AU.ncyl = 4; AU.vcylL = 0.5f; AU.fire[0] = 0; AU.fire[1] = 180; AU.fire[2] = 360; AU.fire[3] = 540; AU.gain[0] = AU.gain[1] = AU.gain[2] = AU.gain[3] = 1; AU.thr = AU.load; AU.spool = blk / 60.f;
  AudioCB(buf, 4096);
  for (int i = 0; i < 8192; i++) { if (isnan(buf[i])) nan++; sum += buf[i] * buf[i]; if (fabsf(buf[i]) > peak) peak = fabsf(buf[i]); }
  if (f) fwrite(buf, sizeof(float), 8192, f);
 }
 if (f) fclose(f);
 printf("AUDIO rms=%.4f peak=%.3f nan=%d\n", sqrt(sum / (60 * 8192)), peak, nan);
}

static void WavOut(const char *path, const short *d, int frames) {
 FILE *f = fopen(path, "wb"); if (!f) return;
 int bytes = frames * 4, v;
 fwrite("RIFF", 1, 4, f); v = 36 + bytes; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
 v = 16; fwrite(&v, 4, 1, f); short s2 = 1; fwrite(&s2, 2, 1, f); s2 = 2; fwrite(&s2, 2, 1, f);
 v = 44100; fwrite(&v, 4, 1, f); v = 44100 * 4; fwrite(&v, 4, 1, f); s2 = 4; fwrite(&s2, 2, 1, f); s2 = 16; fwrite(&s2, 2, 1, f);
 fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f); fwrite(d, 2, frames * 2, f); fclose(f);
}
// offline: start each preset cold, idle, blip, full pull to redline, lift off; write a wav
static void RenderSounds(const char *dir, int only) {
 static short pcm[44100 * 2 * 16];
 for (int p = 0; p < NPRESET; p++) {
  if (only >= 0 && p != only) continue;
  Build b = PRESETS[p]; Spec s; ComputeSpec(&b, &s);
  Sim m; memset(&m, 0, sizeof m); SimReset(&m, &s, 0); m.ign = 1; m.choke = b.delivery == 0; m.temp = 35; m.oilT = 35;
  memset(&SY, 0, sizeof SY); SY.rng = 1234;
  AudioSetupEngine(&b, &s);
  AU.volume = 0.8f; AU.engVol = 1; AU.sfxVol = 0; AU.musicVol = 0;
  int fr = 0, tot = 44100 * 14; float buf[441 * 2]; double sum = 0; float peak = 0; int nan = 0, bov = 0;
  for (int c = 0; c * 441 < tot; c++) {
   float t = c * 0.01f;
   int crank = !m.running && t < 3.f && STARTERS[b.starter].elec;
   if (!STARTERS[b.starter].elec && !m.running && fmodf(t, 0.8f) < 0.01f && t < 3) m.recoilT = 0.35f;
   m.thrCmd = t < 4 ? 0 : (t < 4.4f ? 0.6f : (t < 6 ? 0 : (t < 11 ? 1 : 0)));
   if (m.temp > 45) m.choke = 0;
   if (t > 5) m.temp = fmaxf(m.temp, 80);
   for (int k = 0; k < 10; k++) SimStep(&m, &b, &s, 0.001f, crank);
   AU.rpm = m.rpm; AU.temp = m.temp; AU.load = m.load; AU.q = m.q; AU.knock = m.knock; AU.spool = m.spool; AU.boost = m.boost;
   AU.thr = fmaxf(m.thrAct, m.idleOpen); AU.volts = m.volts; AU.running = m.running; AU.cut = m.cut;
   AU.starter = m.starterOn && m.volts > 9.5f; AU.recoil = m.recoilT > 0; if (m.bovCount != bov) { bov = m.bovCount; AU.bov = bov; }
   AudioCB(buf, 441);
   for (int i = 0; i < 882 && fr * 2 + i < 44100 * 2 * 16; i++) { float v = buf[i]; if (isnan(v)) { nan++; v = 0; } sum += v * v; peak = fmaxf(peak, fabsf(v)); pcm[fr * 2 + i] = (short)(Clampf(v, -1, 1) * 32000); }
   fr += 441;
  }
  printf("SOUND %-22s rms %.3f peak %.3f nan %d maxrpm %.0f\n", PRESETS[p].name, sqrt(sum / (fr * 2.0)), peak, nan, m.maxRpm);
  char nm[64]; snprintf(nm, 64, "%s", PRESETS[p].name); for (char *q = nm; *q; q++) if (!isalnum((unsigned char)*q)) *q = '_';
  WavOut(TextFormat("%s/%s.wav", dir, nm), pcm, fr);
 }
}

int main(int argc, char **argv) {
 if (getenv("SHOPSIM")) {
  gAutotest = 1; CareerNew(); C.rng = 42;
  for (int p = 0; p < NPRESET; p++) {
   C.equip[0] = 1; C.equip[2] = 1; C.equip[3] = 1;
   Quote q = ShopQuote(&PRESETS[p], 0); Item it; ItemFromBuild(&it, &PRESETS[p]); int bs; float v = BestValue(&it, 1, &bs);
   C.equip[2] = C.equip[3] = 0; Quote q0 = ShopQuote(&PRESETS[p], 0);
   printf("QUOTE %-20s cost $%6.0f (full shop $%6.0f) %4.1f days  value $%6.0f (%s) risk %d ok %d %s\n", PRESETS[p].name, QuoteTotal(&q0, 1), QuoteTotal(&q, 1), q0.days, v, SEG_NAME[bs], it.risk, q0.ok, q0.why);
  }
  CareerNew(); C.rng = 7;
  C.money -= 3500; C.equip[0] = 1;
  int pick = atoi(getenv("SHOPSIM"));
  for (int round = 0; round < 6; round++) {
   if (!StartBuild(&PRESETS[pick], 1)) { printf("cant build: money %.0f\n", C.money); break; }
   int d0 = C.day; while (C.job[0].used && C.day < d0 + 30) ShopDay();
   for (int i = 0; i < NITEM; i++) if (C.inv[i].used && C.inv[i].site < 0) { C.inv[i].site = 0; C.inv[i].price = roundf(BestValue(&C.inv[i], 0, NULL) * (getenv("PK") ? atof(getenv("PK")) : 1.f) / 25) * 25; }
   int s0 = C.sales; d0 = C.day;
   while (C.sales == s0 && C.day < d0 + 60) { ShopDay(); for (int o = 0; o < NOFFER; o++) if (C.off[o].used && !C.off[o].scam) { SellItem(C.off[o].item, C.off[o].amount, 0, "x"); C.off[o].used = 0; } }
   printf("round %d day %d money %.0f rep %.1f sales %d  %s\n", round, C.day, C.money, C.rep, C.sales, C.log[C.nlog-1].s);
  }
  return 0;
 }
 if (getenv("SOUNDS")) { RenderSounds(getenv("SOUNDS"), getenv("ONLY") ? atoi(getenv("ONLY")) : -1); return 0; }
 for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--autotest") && i + 1 < argc) { gAutotest = 1; snprintf(gShotDir, 256, "%s", argv[i + 1]); }
 SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
 InitWindow(1600, 900, "Engine Garage - DOOG");
 SetWindowMinSize(1440, 820);
 SetExitKey(0);
 SetTargetFPS(gAutotest ? 0 : 120);
 int cps[100]; int nc = 0; for (int c = 32; c < 127; c++) cps[nc++] = c; cps[nc++] = 176;
 fR = LoadFontFromMemory(".ttf", reg_ttf, reg_ttf_len, 48, cps, nc);
 fB = LoadFontFromMemory(".ttf", bold_ttf, bold_ttf_len, 48, cps, nc);
 GenTextureMipmaps(&fR.texture); SetTextureFilter(fR.texture, TEXTURE_FILTER_TRILINEAR);
 GenTextureMipmaps(&fB.texture); SetTextureFilter(fB.texture, TEXTURE_FILTER_TRILINEAR);
 rlSetClipPlanes(0.2, 250);
 RenderInit();
 SaveRead(); SettingsLoad(); CareerLoad();
 gB = PRESETS[gPreset];
 OnBuildChanged(); ResetCamDist();
 SimReset(&gM, &gS, 0);
 AudioStream stream = {0}; int audioOk = 0;
 if (!gAutotest) {
  InitAudioDevice();
  if (IsAudioDeviceReady()) {
   SetAudioStreamBufferSizeDefault(1024);
   stream = LoadAudioStream(SR, 32, 2);
   SetAudioStreamCallback(stream, AudioCB);
   PlayAudioStream(stream); audioOk = 1;
  }
 } else AutoAudioTest();
 if (getenv("CUSTSHOT")) {
  for (int k = 0; k < 4; k++) {
   Build c = PRESETS[6]; c.layout = 5; c.lube = 2; c.oilL = 6; c.bore = 90; c.stroke = 80; c.block = 1;
   if (k == 0) { c.ncust = 5; for (int i = 0; i < 5; i++) { c.cAng[i] = i * 72.f; if (c.cAng[i] > 180) c.cAng[i] -= 360; c.cSlot[i] = 0; c.cFlip[i] = 0; } }
   if (k == 1) { c.ncust = 12; c.cooling = 2; for (int i = 0; i < 12; i++) { c.cAng[i] = (i % 3 - 1) * 60.f; c.cSlot[i] = i / 3; c.cFlip[i] = 0; } c.starter = 5; c.flywheel = 2; }
   if (k == 2) { c.ncust = 4; c.cooling = 1; for (int i = 0; i < 4; i++) { c.cAng[i] = 180; c.cSlot[i] = i; c.cFlip[i] = 0; } }
   if (k == 3) { c = PRESETS[2]; c.placeMan = 1; c.altAng = 70; c.stAng = 225; c.filtPos = 1; c.turboSide = 2; }
   c.cyl = c.layout == 5 ? c.ncust : c.cyl; c.name[0] = 0;
   gB = c; OnBuildChanged();
   char nm[64]; BuildName(&gB, nm, 64);
   printf("CUST %d %s: vib %.2f left %d attach '%s'\n", k, nm, gS.vib, G.solveLeft, G.attachErr);
   for (int i = 0; i < gNWarn; i++) printf("   W%d %s\n", gWarnSev[i], gWarn[i]);
   for (int ga = 0; ga < G_N; ga++) for (int gb = 0; gb < G_N; gb++) if (gConf[ga][gb]) printf("   CONFLICT %s in %s: %d\n", GROUP_NAME[ga], GROUP_NAME[gb], gConf[ga][gb]);
   Camera3D c2 = {0}; c2.up = (Vector3){0, 1, 0}; c2.fovy = 38; c2.projection = CAMERA_PERSPECTIVE;
   for (int v = 0; v < 2; v++) { camYaw = v ? 3.9f : 0.75f; camPitch = 0.3f; ResetCamDist(); camDist = camDistT = camDist * 0.8f; UpdateCamera3D(&c2, 0); BeginDrawing(); ClearBackground(COL_BG); Scene3D(c2); EndDrawing(); Shot(TextFormat("cu_%d_%d", k, v)); }
  }
  CloseWindow(); return 0;
 }
 if (getenv("FUZZ")) {
  int tot[G_N][G_N] = {{0}}, bad = 0, nb = atoi(getenv("FUZZ"));
  unsigned r = 12345;
#define RN(n) ((int)((r = r * 1103515245u + 12345u) >> 8) % (n))
  for (int k = 0; k < nb; k++) {
   Build b = PRESETS[RN(NPRESET)];
   b.layout = RN(5); b.cyl = CYLOPT[b.layout][RN(CYLOPTN[b.layout])]; b.cycle = RN(5) == 0;
   b.bore = 45 + RN(80); b.stroke = 40 + RN(90); b.cr = 7 + RN(12); b.tilt = RN(3) ? 0 : (float)(RN(121) - 60);
   b.valvetrain = b.cycle ? RN(3) : RN(4); b.valves = RN(4); b.cam = RN(4); b.delivery = RN(NDEL); b.filter = RN(NFIL);
   b.forced = RN(NFORCED); b.boost = 5 + RN(20); b.intercooler = RN(2); b.cooling = RN(3); b.exhaust = RN(b.cycle ? 4 : 3);
   b.starter = RN(NSTART); b.alternator = RN(NALT); b.flywheel = RN(NFLY); b.carb = RN(NCARB); b.inj = RN(NINJ); b.gov = RN(NGOV); b.govSet = 1000 + RN(51) * 100.f; b.name[0] = 0;
   if (RN(4) == 0) { b.layout = 5; b.lube = 2; b.ncust = 1 + RN(10); for (int i = 0; i < b.ncust; i++) { b.cAng[i] = (float)(RN(73) * 5 - 180); b.cSlot[i] = RN(5); b.cFlip[i] = RN(4) == 0; } b.cyl = b.ncust; }
   b.placeMan = RN(5) == 0; b.altAng = (float)(RN(73) * 5 - 180); b.stAng = (float)(RN(73) * 5 - 180); b.filtPos = RN(3); b.turboSide = RN(4);
   b.skirt = RN(4); b.deck = RN(3); b.caps = RN(3); b.wall = 4 + RN(13); b.ignition = RN(5); b.camDrive = RN(3);
   { const char *fp = getenv("FUZZPLAIN"); if (fp) { int f = atoi(fp);
     if (f & 1) { if (b.layout == 5) { b.layout = 1; b.cyl = 4; } } if (f & 2) { b.skirt = 0; b.wall = 0; } if (f & 4) { b.filtPos = 0; b.turboSide = 0; } if (f & 8) b.placeMan = 0; } }
   gB = b; OnBuildChanged();
   if (G.solveLeft && bad < 4 && getenv("FUZZSHOT")) {
    Camera3D c2 = {0}; c2.up = (Vector3){0, 1, 0}; c2.fovy = 38; c2.projection = CAMERA_PERSPECTIVE;
    for (int v = 0; v < 2; v++) { camYaw = v ? 3.9f : 0.75f; camPitch = 0.3f; ResetCamDist(); camDist = camDistT = camDist * 0.7f; UpdateCamera3D(&c2, 0); BeginDrawing(); ClearBackground(COL_BG); Scene3D(c2); EndDrawing(); Shot(TextFormat("fz_%d_%d", k, v)); }
   }
   if (G.solveLeft && !b.placeMan) { bad++; for (int a = 0; a < G_N; a++) for (int c = 0; c < G_N; c++) tot[a][c] += gConf[a][c] ? 1 : 0;
    if (bad <= 12) { for (int a = 0; a < G_N; a++) for (int c = 0; c < G_N; c++) if (gConf[a][c]) printf("    [%s in %s %d]", GROUP_NAME[a], GROUP_NAME[c], gConf[a][c]); printf("\n"); }
    if (bad <= 12) printf("BAD %d man%d skirt%d: layout %d cyl %d bore %.0f stroke %.0f tilt %.0f forced %d cool %d start %d alt %d ex %d 2T %d left %d\n", k, b.placeMan, b.skirt, b.layout, b.cyl, b.bore, b.stroke, b.tilt, b.forced, b.cooling, b.starter, b.alternator, b.exhaust, b.cycle, G.solveLeft); }
  }
  printf("FUZZ %d builds, %d with leftover overlaps\n", nb, bad);
  for (int a = 0; a < G_N; a++) for (int c = 0; c < G_N; c++) if (tot[a][c]) printf("  %s in %s: %d builds\n", GROUP_NAME[a], GROUP_NAME[c], tot[a][c]);
  CloseWindow(); return 0;
 }
 Camera3D cam = {0}; cam.up = (Vector3){0, 1, 0}; cam.fovy = 38; cam.projection = CAMERA_PERSPECTIVE;
 int uiShot = getenv("UISHOT") ? 1 : 0, uiF = 0;
 if (uiShot) { gB = PRESETS[8]; gB.gov = 4; gB.govSet = 4200; OnBuildChanged(); gTab = 4; }
 int shopShot = getenv("SHOPSHOT") ? 1 : 0, shF = 0, shT = 0;
 if (shopShot) {
  gNoCareerSave = 1; CareerNew(); C.rng = 5; C.money = 30000; C.equip[0] = 1; C.equip[1] = 1;
  StartBuild(&PRESETS[3], 1); for (int i = 0; i < 6; i++) ShopDay();
  StartBuild(&PRESETS[2], 1); for (int i = 0; i < 2; i++) ShopDay();
  C.inv[0].site = 0; C.rep = 12; NewContract(); NewContract();
  for (int i = 0; i < 4; i++) ShopDay();
  C.stock[5] = 120; C.stockCost[5] = 270; C.ad[1] = 2;
  gB = PRESETS[3]; OnBuildChanged(); gShop = 1; gShopTab = 0; gInvSel = -1;
 }
 while (!WindowShouldClose()) {
  float dt = gAutotest ? 1.f / 60.f : fminf(GetFrameTime(), 0.05f);
  gTime += dt;
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  MP = GetMousePosition(); MPress = IsMouseButtonPressed(MOUSE_BUTTON_LEFT); MDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT); MRel = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
  if (!MDown) gActive = -1;
  gTip = NULL;
  gUiHoverPrev = gUiHover; gUiHover = 0;
  // dev code
  int chr;
  while (!gEditName && (chr = GetCharPressed())) {
   if (chr >= '0' && chr <= '9') { memmove(gDevBuf, gDevBuf + 1, 3); gDevBuf[3] = (char)chr; gDevBuf[4] = 0; if (!strcmp(gDevBuf, "2323")) { gDev ^= 1; ToastAdd(gDev ? "DEV MODE: unbreakable engine, infinite battery" : "Dev mode off", COL_RED); memset(gDevBuf, 0, sizeof gDevBuf); } }
  }
  if (gEditName) goto skipKeys;
  if (IsKeyPressed(KEY_M)) gMuted ^= 1;
  if (IsKeyPressed(KEY_O) && gMode == 0 && !gShop) gShowOverview ^= 1;
  if (IsKeyPressed(KEY_H) && !gEditName && !gPriceEdit) { gShop ^= 1; if (gShop) gMode = 0; }
  if (gShop && IsKeyPressed(KEY_ESCAPE) && !gPriceEdit) gShop = 0;
  if (IsKeyPressed(KEY_V)) gDS.oilView ^= 1;
  if (IsKeyPressed(KEY_N)) gMusicOn ^= 1;
  if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) gSet.master = Clampf(gSet.master + 0.1f, 0, 1);
  if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) gSet.master = Clampf(gSet.master - 0.1f, 0, 1);
  skipKeys:;
  int startHeld = 0, startPress = 0;
  // autotest script
  if (gAutotest) {
   int f = gAtFrame++;
   static int at_preset = 0, at_phase = 0, at_t = 0;
   if (f == 2) { gTab = 0; }
   if (at_phase == 0) {
    if (f % 4 == 1) { gB = PRESETS[at_preset]; OnBuildChanged(); ResetCamDist(); gTab = at_preset % 5; }
    if (f % 4 == 2 && getenv("ANGLES")) {
     float ys[4] = {0.75f, 2.3f, 3.9f, 5.4f};
     for (int v = 0; v < 4; v++) {
      camYaw = ys[v]; camPitch = v == 2 ? 0.9f : 0.3f; camDist = camDistT = (G.L + G.ccW + 900) * 0.01f * 0.75f;
      UpdateCamera3D(&cam, 0);
      BeginDrawing(); ClearBackground(COL_BG); gDS.cut = (v == 3); Scene3D(cam); gDS.cut = 0; EndDrawing();
      Shot(TextFormat("ang_%d_%d", at_preset, v));
     }
     camYaw = 0.75f; camPitch = 0.32f; ResetCamDist();
    }
    if (f % 4 == 3) {
     printf("SOLVE %-20s iters %d leftover %d\n", PRESETS[at_preset].name, G.solveIters, G.solveLeft);
     for (int ga = 0; ga < G_N; ga++) for (int gb = 0; gb < G_N; gb++) if (gConf[ga][gb]) printf("   CONFLICT %s in %s: %d\n", GROUP_NAME[ga], GROUP_NAME[gb], gConf[ga][gb]);
     printf("PRESET %-20s est %.0f hp @%.0f  %.0f lbft @%.0f  red %.0f  w %.0fkg  imep %.1f/%.1f  warn %d\n", PRESETS[at_preset].name, gS.estHp, gS.estHpRpm, gS.estTq * 0.7376f, gS.estTqRpm, gS.redline, gS.weight, gS.estImep, gS.pLimit, gNWarn);
     for (int i = 0; i < gNWarn; i++) printf("   W: %s\n", gWarn[i]);
     Shot(TextFormat("build_%d", at_preset));
     if (at_preset == 2) { gB.tilt = 30; OnBuildChanged(); Shot("build_tilt"); }
     if (++at_preset >= NPRESET) { at_phase = 1; at_preset = 0; at_t = 0; }
    }
   } else if (at_phase == 1) {
    // per preset dyno test
    static const int order[] = {3, 0, 1, 2, 4, 5, 6, 7, 8};
    if (at_t == 0) { gB = PRESETS[order[at_preset]]; OnBuildChanged(); ResetCamDist(); gMode = 1; SimReset(&gM, &gS, 0); gDiagShown = 0; gWasRunning = 0; gStartedOnce = 0; gCrankT = 0; gM.choke = gB.delivery == 0; gM.ign = 1; }
    at_t++;
    if (STARTERS[gB.starter].elec) startHeld = at_t < 150 && !gM.running;
    else if (at_t % 50 == 5 && !gM.running) startPress = 1;
    if (gM.choke && gM.temp > 45) gM.choke = 0;
    if (at_t == 400) { gM.thrCmd = 1; }
    if (at_t == 430) { Shot(TextFormat("test_rev_%d", order[at_preset])); gM.thrCmd = 0; }
    if (at_t == 600 && gM.running) { gM.dyno = 1; gM.nd = 0; gM.dSum = 0; gM.dCnt = 0; gM.dynoPeakHp = gM.dynoPeakTq = 0; gM.dynoTarget = gM.dynoStart = fmaxf(gM.rpm, gS.idle + 300); gM.dynoInt = 0; gM.dynoEnd = gS.govRpm > 0 ? gS.govRpm * 0.97f : gS.redline * 0.99f; }
    if (at_t > 600 && at_t < 2400 && gM.dyno == 0 && gM.nd > 5 && at_t % 1 == 0 && !gM.failed) {
     static int shot = -1;
     if (shot != at_preset) { shot = at_preset; Shot(TextFormat("test_dyno_%d", order[at_preset])); gDS.cut = 1; at_t = 2400; }
    }
    if (at_t == 2405) { Shot(TextFormat("test_cut_%d", order[at_preset])); gDS.cut = 0; gDS.oilView = 1; gM.thrCmd = 0.5f; }
    if (at_t == 2408) { Shot(TextFormat("test_oil_%d", order[at_preset])); gDS.oilView = 0; gM.thrCmd = 0; }
    if (at_t == 2410) {
     printf("DYNO %-20s running=%d rpm=%.0f peak %.0f hp %.0f lbft health %.1f temp %.0f batt %.2f failed=%d cause=%s pts=%d crankT=%.1f\n", gB.name, gM.running, gM.rpm, gM.dynoPeakHp, gM.dynoPeakTq * 0.7376f, gM.health, gM.temp, gM.batt, gM.failed, gM.failed ? DMG_TITLE[gM.failCause] : "-", gM.nd, gCrankT);
     at_t = 0; if (++at_preset >= NPRESET) { at_phase = 2; at_t = 0; }
    }
   } else if (at_phase == 2) {
    if (at_t == 0) { gB = PRESETS[2]; gB.cr = 13; OnBuildChanged(); gMode = 1; SimReset(&gM, &gS, 0); gM.ign = 1; gM.limiter = 0; }
    at_t++;
    startHeld = at_t < 120 && !gM.running;
    if (at_t > 200) gM.thrCmd = 1;
    if (gM.failed || at_t > 2000) { static int d2 = 0; if (!d2++) { printf("BREAK test: failed=%d cause=%s t=%d\n", gM.failed, DMG_TITLE[gM.failCause], at_t); } }
    if (at_t == 2010 || (gM.failed && at_t % 1000 == 999)) {}
    if (gM.failed) { static int k = 0; if (++k == 40) { Shot("fail"); at_phase = 3; } }
    if (at_t > 2100) at_phase = 3;
   } else if (at_phase == 3) {
    static int k3 = 0; k3++;
    if (k3 == 1) { gMode = 0; gShowAch = 0; gB = PRESETS[3]; OnBuildChanged(); ResetCamDist(); gShowOverview = 1; }
    if (k3 == 4) Shot("overview");
    if (k3 == 5) { gShowOverview = 0; gDS.oilView = 0; gTab = 5; gB = PRESETS[5]; OnBuildChanged(); camYaw = 2.3f; }
    if (k3 == 8) Shot("garage_oil_tilt");
    if (k3 == 9) { gDS.oilView = 0; gB = PRESETS[8]; OnBuildChanged(); gTab = 4; }
    if (k3 == 12) { Shot("garage_buggy"); }
    if (k3 == 13 || k3 == 17 || k3 == 21) {
     Build c = PRESETS[6]; c.layout = 5; c.lube = 2; c.oilL = 6; c.bore = 90; c.stroke = 80;
     if (k3 == 13) { c.ncust = 5; for (int i = 0; i < 5; i++) { c.cAng[i] = i * 72.f; if (c.cAng[i] > 180) c.cAng[i] -= 360; c.cSlot[i] = 0; c.cFlip[i] = 0; } }
     if (k3 == 17) { c.ncust = 12; c.cooling = 2; c.block = 1; for (int i = 0; i < 12; i++) { c.cAng[i] = (i % 3 - 1) * 60.f; c.cSlot[i] = i / 3; c.cFlip[i] = 0; } c.starter = 5; c.flywheel = 2; }
     if (k3 == 21) { c.ncust = 4; c.cooling = 1; for (int i = 0; i < 4; i++) { c.cAng[i] = 180; c.cSlot[i] = i; c.cFlip[i] = 0; } }
     c.cyl = c.ncust; c.name[0] = 0;
     gB = c; OnBuildChanged(); ResetCamDist(); gTab = 6; gBlockSub = 1; camYaw = 0.75f;
     printf("CUSTOM %d: n=%d vib %.2f solveLeft %d attach '%s' warn %d\n", k3, c.ncust, gS.vib, G.solveLeft, G.attachErr, gNWarn);
     for (int i = 0; i < gNWarn; i++) printf("   W%d %s\n", gWarnSev[i], gWarn[i]);
    }
    if (k3 == 16) Shot("custom_radial");
    if (k3 == 20) Shot("custom_w12");
    if (k3 == 24) { Shot("custom_inverted"); gBlockSub = 0; }
    if (k3 == 26) Shot("block_shape");
    if (k3 == 27) { gBlockSub = 2; gB.placeMan = 1; gB.altAng = 70; gB.stAng = 225; OnBuildChanged(); }
    if (k3 == 30) Shot("block_place");
    if (k3 > 31) break;
   }
  }
  if (gMode == 0) {
   gVisAng = fmodf(gVisAng + 20 * dt, 720); gDS.span = 0;
   gM.rpm = 0; gM.running = 0;
  } else {
   startHeld |= IsKeyDown(KEY_E) || IsKeyDown(KEY_ENTER) || gStartHeldUI;
   startPress |= IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || gBtnStart;
   if (!STARTERS[gB.starter].elec) startHeld = 0;
   if (IsKeyPressed(KEY_I) || gBtnIgn) { if (!gM.failed) { gM.ign ^= 1; SfxPush(SFX_KEY); if (!gM.ign) gM.dyno = 0; } }
   if ((IsKeyPressed(KEY_C) || gBtnChoke) && gB.delivery == 0) gM.choke ^= 1;
   if ((IsKeyPressed(KEY_L) || gBtnLim) && gB.gov) { gM.limiter ^= 1; ToastAdd(gM.limiter ? (gB.gov == 3 ? "Rev limiter ON" : "Governor connected") : (gB.gov == 3 ? "Rev limiter OFF. Careful." : "Governor disconnected. Careful."), gM.limiter ? COL_DIM : COL_RED); }
   if (IsKeyPressed(KEY_X) || gBtnCut) gDS.cut ^= 1;
   if (IsKeyPressed(KEY_D) || gBtnDyno) {
    if (gM.dyno) { gM.dyno = 0; gM.thrCmd = 0; ToastAdd("Pull aborted", COL_DIM); }
    else if (gM.running) {
     if (gM.nd > 5) { gM.npd = gM.nd; memcpy(gM.pdrpm, gM.drpm, sizeof gM.drpm); memcpy(gM.pdhp, gM.dhp, sizeof gM.dhp); memcpy(gM.pdtq, gM.dtq, sizeof gM.dtq); }
     gM.dyno = 1; gM.nd = 0; gM.dSum = 0; gM.dCnt = 0; gM.dynoPeakHp = gM.dynoPeakTq = 0; gM.dynoInt = 0;
     gM.dynoTarget = gM.dynoStart = fmaxf(gM.rpm, gS.idle + 300); gM.dynoEnd = (gM.limiter && gS.govRpm > 0) ? gS.govRpm * 0.97f : (gM.limiter || !gB.gov ? gS.redline : gS.redline * 1.04f) * 0.99f;
     ToastAdd("Dyno pull: wide open throttle to redline", COL_ACC); SfxPush(SFX_BEEP);
    } else ToastAdd("Start the engine first.", COL_YEL);
   }
   if ((IsKeyPressed(KEY_R) || gBtnRebuild) && gM.failed) { SimReset(&gM, &gS, 1); ToastAdd("Rebuilt. Fresh engine.", COL_GRN); SfxPush(SFX_RATCHET); gWasRunning = 0; gStartedOnce = 0; }
   if (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_ESCAPE) || gBtnBack) { gMode = 0; gM.dyno = 0; gShowAch = 0; for (int i = 0; i < NPUFF; i++) gPuff[i].life = 0; }
   if (!gAutotest) {
    static float lock = 0;
    for (int k = 0; k < 10; k++) if (IsKeyPressed(KEY_ONE + k) || IsKeyPressed(KEY_KP_1 + k)) lock = (k + 1) * 0.1f;
    if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) lock = 1.0f;
    if (IsKeyPressed(KEY_T)) lock = 0;
    Rectangle tb = {sw - 64, sh - 330, 40, 300};
    if (MDown && CheckCollisionPointRec(MP, (Rectangle){tb.x - 10, tb.y - 10, tb.width + 20, tb.height + 20})) lock = Clampf(1 - (MP.y - tb.y) / tb.height, 0, 1);
    float want = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE)) ? 1.f : lock;
    float rate = want > gM.thrCmd ? 7.f : 9.f;
    gM.thrCmd += Clampf(want - gM.thrCmd, -rate * dt, rate * dt);
   }
   gBtnIgn = gBtnStart = gBtnChoke = gBtnLim = gBtnDyno = gBtnCut = gBtnBack = gStartHeldUI = gBtnRebuild = 0;
   if (gMode == 1) TestUpdate(dt, startHeld, startPress);
   gDS.span = gSet.blur ? gM.rpm * 6.f * dt : 0;
   gVisAng = gM.angle;
   gDS.fanAng += fminf(gM.rpm * 0.9f / 60.f * 2 * PI * dt, 0.42f);
  }
  // audio params
  AU.rpm = gMode ? gM.rpm : 0; AU.temp = gM.temp; AU.load = gM.load; AU.q = gM.q; AU.knock = gM.knock; AU.spool = gM.spool; AU.boost = gM.boost;
  AU.engVol = gSet.eng; AU.sfxVol = gSet.sfx; gVolume = gSet.master;
  AU.thr = fmaxf(gM.thrAct, gM.idleOpen); AU.volts = gM.volts; AU.running = gMode && gM.running; AU.cut = gM.cut;
  AU.starter = gMode && gM.starterOn && gM.volts > 9.5f; AU.recoil = gMode && gM.recoilT > 0; AU.volume = gMuted ? 0 : gVolume;
  if (gM.bovCount != AU.bov) AU.bov = gM.bovCount;
  AU.musicVol = (gMuted || !gMusicOn) ? 0 : gSet.music * gVolume;
  AU.duck = gMode ? (gM.running ? 0.35f + 0.65f * Clampf(gM.rpm / fmaxf(gS.redline, 1), 0, 1) : 0) : 0;
  UpdateCamera3D(&cam, 1);
  BeginDrawing();
  ClearBackground(COL_BG);
  if (!gShop) Scene3D(cam);
  TopBar(sw);
  if (gShop) ShopScreen(sw, sh);
  else if (gMode == 0) { BuildPanel(sw, sh); SpecCard(sw, sh); ViewBar(sw); if (gShowOverview) Overview(sw, sh); }
  else TestHUD(sw, sh);
  if (gShowAch) AchPanel(sw, sh);
  if (gShowSettings) SettingsPanel(sw, sh);
  Toasts(sw, dt);
  EndDrawing();
  if (shopShot && ++shF == 6) { snprintf(gShotDir, 256, "%s", getenv("SHOPSHOT")); Shot(TextFormat("shop%d", shT)); shF = 0; if (++shT >= 9) break; gShopTab = shT; }
  if (uiShot && ++uiF == 8) { snprintf(gShotDir, 256, "%s", getenv("UISHOT")); Shot(TextFormat("ui%d", uiShot)); if (++uiShot > 2) break; uiF = 0; gTab = 4; gShowOverview = uiShot == 2; }
 }
 if (audioOk) { UnloadAudioStream(stream); CloseAudioDevice(); }
 CloseWindow();
 return 0;
}
