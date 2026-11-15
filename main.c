#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
static float gVolume = 0.7f; static int gMuted = 0;
static char gDevBuf[8] = {0};

static Font fR, fB;
static Vector2 MP; static int MPress, MDown, MRel, gActive = -1;
static const char *gTip = NULL;
static const Color COL_BG = {17, 19, 23, 255}, COL_PANEL = {27, 30, 36, 255}, COL_PANEL2 = {38, 42, 50, 255}, COL_LINE = {54, 59, 68, 255};
static const Color COL_TXT = {232, 234, 238, 255}, COL_DIM = {140, 146, 158, 255}, COL_ACC = {255, 138, 28, 255}, COL_RED = {235, 70, 55, 255};
static const Color COL_GRN = {90, 205, 115, 255}, COL_YEL = {240, 196, 60, 255}, COL_BLUE = {80, 160, 255, 255};

// ===== toasts =====
typedef struct { char s[128]; float t; Color c; } Toast;
static Toast gToast[6];
static void ToastAdd(const char *s, Color c) {
 for (int i = 5; i > 0; i--) gToast[i] = gToast[i - 1];
 snprintf(gToast[0].s, 128, "%s", s); gToast[0].t = 4.5f; gToast[0].c = c;
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
 if (d && sz == (int)sizeof gSave) { memcpy(&gSave, d, sizeof gSave); if (gSave.magic != 0x454E4731) memset(&gSave, 0, sizeof gSave); }
 if (d) UnloadFileData(d);
}
static void Ach(int id) {
 if (gSave.ach & (1u << id)) return;
 gSave.ach |= 1u << id; SaveWrite();
 ToastAdd(TextFormat("CHALLENGE COMPLETE: %s", ACH_NAME[id]), COL_YEL);
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
 return hov && MPress;
}

static int UiSelect(Rectangle r, const char *label, const char *val, const char *tip, int enabled) {
 int hov = CheckCollisionPointRec(MP, r);
 if (hov) { gUiHover = 1; if (tip) gTip = tip; }
 DrawRectangleRounded(r, 0.18f, 8, hov && enabled ? COL_PANEL2 : COL_PANEL);
 Tx(label, r.x + 12, r.y + (r.height - 17) * 0.5f, 17, enabled ? COL_DIM : (Color){80, 85, 95, 255});
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
 if (*v != old) changed = 1;
 return changed;
}

static int Cyc(int v, int d, int n) { v += d; if (v < 0) v = n - 1; if (v >= n) v = 0; return v; }

// ===== build helpers =====
static void BuildName(const Build *b, char *out, int n) {
 for (int i = 0; i < NPRESET; i++) {
  Build p = PRESETS[i];
  memcpy(p.name, b->name, sizeof p.name);
  if (!memcmp(&p, b, sizeof p)) { snprintf(out, n, "%s", PRESETS[i].name); return; }
 }
 Spec s; ComputeSpec(b, &s);
 const char *lay = b->layout == 0 ? "Single" : (b->layout == 1 ? "I" : (b->layout == 4 ? "Flat-" : "V"));
 char cyl[16]; if (b->layout == 0) snprintf(cyl, 16, "Single"); else snprintf(cyl, 16, "%s%d", lay, b->cyl);
 snprintf(out, n, "Custom %s %.1fL%s%s", cyl, s.dispL, b->forced ? (b->forced == 4 ? " Blown" : " Turbo") : "", b->cycle ? " 2T" : (FUELS[b->fuel].diesel ? " Diesel" : ""));
}
static void FixBuild(Build *b) {
 if (b->cycle == 1) { if (b->valvetrain > 2) b->valvetrain = 2; }
 else if (b->exhaust == 3) b->exhaust = 1;
 int ok = 0;
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
 GeoCompute(&gB);
 gNWarn = BuildWarnings(&gB, &gS, gWarn, gWarnSev, 16);
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
  camYaw -= d.x * 0.006f; camPitch += d.y * 0.005f;
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) gDragging = 0;
 }
 if (!gUiHoverPrev && allowDrag) { float w = GetMouseWheelMove(); camDistT *= powf(0.88f, w); }
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
 float imb = IMBAL[gB.cyl > 16 ? 16 : gB.cyl] * (gB.layout == 4 ? 0.4f : (gB.layout >= 2 ? 0.8f : 1.f));
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
 DrawEngine(&gB, &gS, gVisAng, &gDS);
 PuffDraw();
 EndMode3D();
}
static Vector3 LocalToWorld(Vector3 p) { return Vector3Transform(p, gBase); }

// ===== garage UI =====
static void BuildPanel(int sw, int sh) {
 float pw = 470;
 Panel((Rectangle){12, 64, pw, sh - 76}, (Color){22, 25, 30, 245});
 const char *tabs[] = {"ENGINE", "MATERIALS", "VALVES", "INTAKE", "SYSTEMS"};
 float tw = (pw - 20) / 5;
 for (int i = 0; i < 5; i++) {
  Rectangle tr = {22 + i * tw, 74, tw - 4, 34};
  int hov = CheckCollisionPointRec(MP, tr);
  DrawRectangleRounded(tr, 0.3f, 8, gTab == i ? COL_ACC : (hov ? COL_PANEL2 : COL_PANEL));
  TxC(tabs[i], tr.x + tr.width * 0.5f, tr.y + 9, 14, gTab == i ? (Color){18, 18, 20, 255} : COL_TXT, 1);
  if (hov && MPress) gTab = i;
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
  if ((d = UiSelect(ROW(), "Layout", LAYOUTS[b->layout], "Cylinder arrangement. V and boxer engines have two banks; balance changes how much it shakes.", 1))) { b->layout = Cyc(b->layout, d, 5); ch = 1; } NEXT();
  snprintf(vb, 64, "%d", b->cyl);
  if ((d = UiSelect(ROW(), "Cylinders", vb, "More cylinders: smoother, revs higher for the same displacement, heavier.", b->layout != 0))) {
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
  if ((d = UiSelect(ROW(), "Block", BLOCKS[b->block].name, BLOCKS[b->block].tip, 1))) { b->block = Cyc(b->block, d, NBLOCK); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Pistons & rods", INTERN[b->internals].name, INTERN[b->internals].tip, 1))) { b->internals = Cyc(b->internals, d, NINT); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Flywheel", FLY[b->flywheel].name, FLY[b->flywheel].tip, 1))) { b->flywheel = Cyc(b->flywheel, d, 3); ch = 1; } NEXT();
  y += 6;
  Tx(TextFormat("Cylinder pressure limit  %.0f bar", gS.pLimit), x + 4, y, 17, COL_TXT); y += 24;
  Tx(TextFormat("Peak pressure (est.)  %.0f bar", gS.estImep), x + 4, y, 15, gS.estImep > gS.pLimit ? COL_RED : COL_DIM); y += 22;
  Tx(TextFormat("Piston speed limit  %.1f m/s  (%.0f rpm)", INTERN[b->internals].ps, gS.pistonRpm), x + 4, y, 15, COL_DIM); y += 22;
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
  const char *lb[] = {"piston speed", "valve float", "diesel combustion"};
  Tx(TextFormat("Redline  %.0f rpm  (limited by %s)", gS.redline, lb[gS.limitBy]), x + 4, y, 15, COL_DIM);
 } else if (gTab == 3) {
  int d;
  if ((d = UiSelect(ROW(), "Fuel delivery", DELIV[b->delivery].name, DELIV[b->delivery].tip, 1))) { b->delivery = Cyc(b->delivery, d, NDEL); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Air filter", FILTERS[b->filter].name, FILTERS[b->filter].tip, 1))) { b->filter = Cyc(b->filter, d, NFIL); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Forced induction", FORCED[b->forced].name, FORCED[b->forced].tip, 1))) { b->forced = Cyc(b->forced, d, NFORCED); if (b->forced && b->boost < 1) b->boost = 10; ch = 1; } NEXT();
  snprintf(vb, 64, "%.0f psi", b->boost);
  ch |= UiSlider(4, ROW(), "Boost", &b->boost, 0, 50, 1, vb, "Wastegate / pulley setting. More boost = more air = more power, heat, knock and pressure.", b->forced != 0); NEXT();
  if ((d = UiSelect(ROW(), "Intercooler", b->intercooler ? "Yes" : "No", "Cools compressed air: denser charge and much less knock.", b->forced != 0))) { b->intercooler ^= 1; ch = 1; } NEXT();
  y += 6;
  if (b->forced) {
   const ForcedDef *fd = &FORCED[b->forced];
   if (b->forced == 4) Tx(TextFormat("Blower safe max  %.0f psi", fd->maxb), x + 4, y, 17, COL_TXT);
   else Tx(TextFormat("Turbo max  %.0f psi    full spool ~%.0f rpm", fd->maxb, fd->full / fmaxf(gS.dispL * (b->cycle ? 2 : 1), 0.01f) * 1000), x + 4, y, 16, COL_TXT);
  } else Tx("Naturally aspirated.", x + 4, y, 17, COL_TXT);
 } else {
  int d;
  if ((d = UiSelect(ROW(), "Cooling", COOL[b->cooling].name, COOL[b->cooling].tip, 1))) { b->cooling = Cyc(b->cooling, d, 3); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Exhaust", EXH[b->exhaust].name, EXH[b->exhaust].tip, 1))) { b->exhaust = Cyc(b->exhaust, d, b->cycle ? 4 : 3); ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Starter", b->starter ? "Electric" : "Recoil (pull cord)", b->starter ? "Electric starter motor. Needs a battery; drains it while cranking." : "Pull cord. No battery needed. Only works if you can pull it through compression.", 1))) { b->starter ^= 1; ch = 1; } NEXT();
  if ((d = UiSelect(ROW(), "Alternator", b->alternator ? "Yes" : "None", "Charges the battery above ~1100 rpm. Costs a bit of power. EFI needs electricity.", 1))) { b->alternator ^= 1; ch = 1; } NEXT();
  if (b->cycle == 0) {
   if ((d = UiSelect(ROW(), "Engine oil", OILS[b->oil].name, OILS[b->oil].tip, 1))) { b->oil = Cyc(b->oil, d, 5); ch = 1; } NEXT();
  } else {
   if (b->mix < 1) snprintf(vb, 64, "NO OIL"); else snprintf(vb, 64, "%.0f : 1", b->mix);
   ch |= UiSlider(5, ROW(), "Oil mix", &b->mix, 0, 100, 1, vb, "Gas-to-oil ratio. 32-50:1 is typical. Too lean = seize. Too rich = smoke, fouled plugs.", 1); NEXT();
  }
  y += 6;
  Tx(TextFormat("Battery: %s", gS.hasBatt ? "yes" : "none (magneto ignition)"), x + 4, y, 16, COL_TXT); y += 24;
  Tx(TextFormat("Cooling capacity  ~%.0f kW", gS.coolCap / 1000), x + 4, y, 15, COL_DIM);
 }
#undef ROW
#undef NEXT
 // preset + save area
 float by = sh - 286;
 DrawLineEx((Vector2){22, by - 10}, (Vector2){22 + w, by - 10}, 1, COL_LINE);
 int d;
 if ((d = UiSelect((Rectangle){x, by, w - 110, 40}, "Preset", PRESETS[gPreset].name, "Start from a known engine, then mess with it.", 1))) gPreset = Cyc(gPreset, d, NPRESET);
 if (UiButton((Rectangle){x + w - 102, by, 102, 40}, "LOAD", 0, 1)) { gB = PRESETS[gPreset]; ch = 1; ResetCamDist(); ToastAdd(TextFormat("Loaded preset: %s", PRESETS[gPreset].name), COL_TXT); }
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

static void SpecCard(int sw, int sh) {
 float w = 400, x = sw - w - 12, y = 64;
 int nw = gNWarn > 6 ? 6 : gNWarn;
 float h = 432 + nw * 22 + (nw ? 26 : 0);
 Panel((Rectangle){x, y, w, h}, (Color){22, 25, 30, 240});
 char nm[64]; BuildName(&gB, nm, 64);
 float sz = 22; while (Tw(nm, sz, 1) > w - 28 && sz > 14) sz--;
 Tb(nm, x + 14, y + 12, sz, COL_TXT);
 const char *lay = gB.layout == 0 ? "Single" : (gB.layout == 1 ? "Inline" : (gB.layout == 4 ? "Boxer" : "V"));
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
 rows[5].k = "Parts cost"; snprintf(rows[5].v, 48, "$%.0f", gS.cost);
 for (int i = 0; i < 6; i++) {
  Tx(rows[i].k, x + 14, yy, 15, COL_DIM); TxR(rows[i].v, x + w - 14, yy, 15, COL_TXT, 1); yy += 21;
 }
 if (nw) {
  yy += 6;
  Tb("WARNINGS", x + 14, yy, 14, COL_YEL); yy += 20;
  for (int i = 0; i < nw; i++) {
   DrawCircleV((Vector2){x + 20, yy + 8}, 4, gWarnSev[i] == 2 ? COL_RED : COL_YEL);
   float s2 = 14; while (Tw(gWarn[i], s2, 0) > w - 46 && s2 > 10) s2 -= 0.5f;
   Tx(gWarn[i], x + 30, yy, s2, gWarnSev[i] == 2 ? (Color){255, 140, 125, 255} : COL_TXT); yy += 22;
  }
 }
 if (UiButton((Rectangle){x, sh - 78, w, 64}, "FIRE IT UP ON THE DYNO  >", 1, 1)) {
  gMode = 1; SimReset(&gM, &gS, 1); gDiagShown = 0; gWasRunning = 0; gStartedOnce = 0; gCrankT = 0;
  for (int i = 0; i < NPUFF; i++) gPuff[i].life = 0;
  ToastAdd(gB.starter ? "Hold E (or START) to crank. I = ignition." : "Press E (or PULL) to yank the cord. I = ignition.", COL_TXT);
 }
}

static const char *Diagnose(void) {
 const FuelDef *f = &FUELS[gB.fuel];
 if (!gM.ign) return "Ignition is off. Press I.";
 if (f->diesel && gB.delivery != 3 && gB.delivery != 4) return "Diesel can't run on a carb or port injection. Use direct or mechanical injection.";
 if (f->diesel && gB.cr < 14) return "Diesel won't self-ignite below 14:1 compression.";
 if (gS.needsElec && !gS.hasBatt) return "Fuel injection has no power. Add an alternator or electric start (battery).";
 if (gS.needsElec && gM.volts < 9) return "Battery too low to run the injectors.";
 if (gB.starter == 1 && gM.volts < 9.6f) return "Battery's dead: the starter just clicks.";
 if (gB.delivery == 0 && !gM.choke && gM.temp < 40) return "Cold carbureted engine: turn on the CHOKE (C).";
 if (f->cold < 0.5f && gM.temp < 35) return "Alcohol fuels hate cold starts. Keep trying, or add a choke/DI.";
 if (f->diesel && gB.cr < 16) return "Low compression diesel: cold start is hard. Keep cranking.";
 return "Keep cranking...";
}

static void TestUpdate(float dt, int startHeld, int startPress) {
 const Spec *s = &gS;
 if (startPress && !gM.ign && !gM.failed) { gM.ign = 1; ToastAdd("Ignition ON", COL_DIM); }
 int crank = 0;
 if (gB.starter == 1) {
  crank = startHeld && !gM.failed;
  if (startPress && s->hasBatt && gM.volts < 9.6f) AU.click++;
 } else if (startPress && gM.recoilT <= 0 && !gM.failed && gM.rpm < 400) {
  gM.recoilT = 0.35f; gM.recoilStuck = 0; gPullAnim = 0.001f;
 }
 if (gPullAnim > 0) { gPullAnim += dt / 0.6f; if (gPullAnim >= 1) gPullAnim = 0; }
 gDS.pullT = gPullAnim;
 int steps = (int)(dt * 1000 + 0.5f); if (steps < 1) steps = 1; if (steps > 50) steps = 50;
 int wasFailed = gM.failed;
 for (int i = 0; i < steps; i++) SimStep(&gM, &gB, s, 0.001f, crank);
 if (gM.recoilStuck) { gM.recoilStuck = 0; ToastAdd("Can't pull it through compression! Too big for a pull cord.", COL_RED); }
 if (gM.dyno == 2) {
  gM.dyno = 0; gM.thrCmd = 0;
  if (gM.nd > 5) {
   ToastAdd(TextFormat("Dyno: %.0f hp, %.0f lb-ft", gM.dynoPeakHp, gM.dynoPeakTq * 0.7376f), COL_ACC);
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
 if (gWasRunning && gM.rpm < 120 && !crank) { gWasRunning = 0; if (!gM.failed && gM.ign) ToastAdd("Stalled.", COL_YEL); }
 if (gM.failed && !wasFailed) {
  AU.bang++; Ach(7);
  Vector3 c = LocalToWorld((Vector3){0, G.deck, 0});
  for (int i = 0; i < 40; i++) PuffAdd(c, (Vector3){(SRand(&gM) - 0.5f) * 3, SRand(&gM) * 2.5f, (SRand(&gM) - 0.5f) * 3}, 2.5f + SRand(&gM) * 2, 0.25f + SRand(&gM) * 0.3f, (Color){90, 90, 95, 200});
 }
 // challenges
 if (gM.running && gM.runTime > 3) {
  if (gB.cyl == 1 && gB.starter == 0 && !gB.alternator) Ach(0);
  if (gB.starter == 0 && gS.dispL >= 0.8f) Ach(8);
 }
 if (gM.running && gB.cycle == 1 && gM.rpm > 10000) Ach(1);
 if (gM.running && gM.rpm > 12000) Ach(5);
 if (gM.wotTime >= 60) Ach(9);
 // smoke
 static float puffT = 0;
 puffT -= dt;
 if (puffT <= 0 && (gM.running || gM.failed)) {
  puffT = 0.05f;
  Vector3 o = LocalToWorld(G.exhOut);
  Vector3 v = {1.2f + gM.rpm / 4000.f, 0.2f, (SRand(&gM) - 0.5f) * 0.3f};
  float tw = Clampf(gM.load, 0, 1);
  if (gB.cycle == 1 && gS.lube > 0.6f) PuffAdd(o, v, 1.6f, 0.06f + 0.05f * gS.lube, (Color){170, 180, 200, (unsigned char)Clampf(40 + 35 * gS.lube, 0, 160)});
  if (FUELS[gB.fuel].diesel && tw > 0.6f && gM.spool < 0.6f) PuffAdd(o, v, 1.8f, 0.12f, (Color){30, 30, 32, 170});
  if (gM.choke && gM.temp > 50) PuffAdd(o, v, 1.5f, 0.08f, (Color){40, 40, 42, 120});
  if (gM.health < 45 && !gM.failed) PuffAdd(o, v, 1.8f, 0.1f, (Color){150, 165, 190, 120});
  if (!gB.cycle && fabsf(gB.tilt) > 65 && gM.running) PuffAdd(o, v, 1.6f, 0.07f, (Color){150, 165, 190, 90});
  if (gM.temp < 35 && gM.running) PuffAdd(o, v, 1.2f, 0.06f, (Color){230, 230, 235, 60});
  if (gM.temp > gS.danger) PuffAdd(LocalToWorld((Vector3){0, G.deck + G.headH, 0}), (Vector3){0, 1.2f, 0}, 1.5f, 0.12f, (Color){235, 235, 240, 110});
  if (gM.failed && gM.rpm > 10) PuffAdd(LocalToWorld((Vector3){0, G.deck, 0}), (Vector3){0, 0.8f, 0}, 2.5f, 0.15f, (Color){80, 80, 85, 150});
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
 WL(gM.dmgRate[DMG_OILP] > 0, "LOW OIL PRESSURE", COL_RED);
 WL(gM.dmgRate[DMG_LUBE] > 0.3f, "NOT ENOUGH OIL IN FUEL", COL_RED);
 WL(gM.dmgRate[DMG_DIRT] > 0.3f, "SUCKING IN DIRT", COL_YEL);
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
 float lx = sw * 0.5f - 270;
 if (gB.cooling == 0) Gauge(lx, gy + 30, 72, gM.temp, 20, 240, s->danger, "CHT  C", TextFormat("%.0f", gM.temp), 4, 1, COL_TXT);
 else Gauge(lx, gy + 30, 72, gM.temp, 20, 140, s->danger, "WATER  C", TextFormat("%.0f", gM.temp), 4, 1, COL_TXT);
 if (gB.cycle == 0) Gauge(lx - 175, gy + 30, 72, gM.oilP, 0, 80, 999, "OIL  psi", TextFormat("%.0f", gM.oilP), 4, 1, gM.oilP < 10 && gM.rpm > 2000 ? COL_RED : COL_TXT);
 else {
  float cx2 = lx - 175, cy2 = gy + 30;
  DrawCircleV((Vector2){cx2, cy2}, 80, (Color){12, 13, 16, 235});
  TxC("OIL MIX", cx2, cy2 - 30, 13, COL_DIM, 0);
  TxC(gB.mix < 1 ? "NONE" : TextFormat("%.0f:1", gB.mix), cx2, cy2 - 8, 24, s->lube < 0.6f ? COL_RED : COL_TXT, 1);
 }
 float rx = sw * 0.5f + 270;
 if (gB.forced) Gauge(rx, gy + 30, 72, gM.boost, 0, fmaxf(10, ceilf(gB.boost * 1.25f / 10) * 10), gB.forced == 4 ? FORCED[4].maxb : FORCED[gB.forced].maxb, "BOOST  psi", TextFormat("%.1f", gM.boost), 5, 1, COL_ACC);
 else Gauge(rx, gy + 30, 72, gM.thrAct * 100, 0, 100, 999, "THROTTLE  %", TextFormat("%.0f", fmaxf(gM.thrAct, gM.idleOpen) * 100), 5, 1, COL_TXT);
 Gauge(rx + 175, gy + 30, 72, s->hasBatt ? gM.volts : 0, 8, 16, 999, s->hasBatt ? "VOLTS" : "NO BATTERY", s->hasBatt ? TextFormat("%.1f", gM.volts) : "--", 4, 1, s->hasBatt && gM.volts < 11.5f ? COL_YEL : COL_TXT);
 // readout
 PanelR((Rectangle){sw * 0.5f - 105, gy - 222, 210, 58}, 0.2f, (Color){12, 13, 16, 220});
 TxC(TextFormat("%.0f hp   %.0f lb-ft", hp, fmaxf(0, gM.tbrake) * 0.7376f), sw * 0.5f, gy - 214, 19, COL_TXT, 1);
 TxC(gM.dyno ? "DYNO PULL IN PROGRESS" : (gM.running ? "running" : (gM.rpm > 30 ? "cranking" : "stopped")), sw * 0.5f, gy - 190, 14, gM.dyno ? COL_ACC : COL_DIM, 0);
 // throttle bar
 Rectangle tb = {sw - 64, sh - 330, 40, 300};
 PanelR((Rectangle){tb.x - 8, tb.y - 34, tb.width + 16, tb.height + 46}, 0.2f, (Color){22, 25, 30, 220});
 TxC("THR", tb.x + tb.width * 0.5f, tb.y - 26, 13, COL_DIM, 1);
 DrawRectangleRounded(tb, 0.3f, 6, (Color){14, 15, 18, 255});
 float tf = Clampf(fmaxf(gM.thrCmd, gM.dyno ? 1 : 0), 0, 1);
 DrawRectangleRounded((Rectangle){tb.x + 4, tb.y + 4 + (tb.height - 8) * (1 - tf), tb.width - 8, (tb.height - 8) * tf}, 0.3f, 6, COL_ACC);
 // controls
 float bx = 12, by = sh - 382, bw = 220, bh = 40;
 Panel((Rectangle){bx, by - 10, bw + 20, 380}, (Color){22, 25, 30, 220});
 bx += 10;
 extern int gBtnIgn, gBtnStart, gBtnChoke, gBtnLim, gBtnDyno, gBtnCut, gBtnBack, gStartHeldUI;
 gBtnIgn = UiButton((Rectangle){bx, by, bw, bh}, gM.ign ? "IGNITION: ON   [I]" : "IGNITION: OFF   [I]", gM.ign ? 2 : 0, !gM.failed); by += bh + 8;
 Rectangle sr = {bx, by, bw, bh};
 gBtnStart = UiButton(sr, gB.starter ? "START (hold)   [E]" : "PULL CORD   [E]", 1, !gM.failed);
 gStartHeldUI = CheckCollisionPointRec(MP, sr) && MDown && !gM.failed;
 by += bh + 8;
 gBtnChoke = UiButton((Rectangle){bx, by, bw, bh}, gB.delivery == 0 ? (gM.choke ? "CHOKE: ON   [C]" : "CHOKE: OFF   [C]") : "CHOKE: n/a (EFI)", gM.choke ? 2 : 0, gB.delivery == 0); by += bh + 8;
 gBtnLim = UiButton((Rectangle){bx, by, bw, bh}, gM.limiter ? "REV LIMITER: ON   [L]" : "REV LIMITER: OFF   [L]", gM.limiter ? 0 : 3, 1); by += bh + 8;
 gBtnDyno = UiButton((Rectangle){bx, by, bw, bh}, gM.dyno ? "ABORT PULL   [D]" : "DYNO PULL   [D]", gM.dyno ? 3 : 0, gM.running || gM.dyno); by += bh + 8;
 gBtnCut = UiButton((Rectangle){bx, by, bw, bh}, gDS.cut ? "CUTAWAY: ON   [X]" : "CUTAWAY: OFF   [X]", gDS.cut ? 2 : 0, 1); by += bh + 8;
 gBtnBack = UiButton((Rectangle){bx, by, bw, bh}, "< BACK TO GARAGE   [G]", 0, 1); by += bh + 8;
 Tx("Rev: hold W / Up / Space.  1-0 lock throttle", bx, by + 2, 13, COL_DIM);
 Tx("T = release lock.  M = mute.  +/- volume", bx, by + 18, 13, COL_DIM);
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
  case DMG_ROD: why = TextFormat("Piston speed passed %.1f m/s at %.0f rpm. Use stronger internals, a shorter stroke, or keep the limiter on.", INTERN[gB.internals].ps, s->pistonRpm); break;
  case DMG_VALVE: why = TextFormat("The valves floated past %.0f rpm and met the pistons. Lighter valvetrain, more valves per cylinder, or respect the redline.", s->floatRpm); break;
  case DMG_PRESS: why = TextFormat("Cylinder pressure beat the %.0f bar the block and internals can hold. Stronger block, forged/billet parts, or less boost.", s->pLimit); break;
  case DMG_HEAT: why = "It cooked itself. Bigger radiator, liquid cooling, or less sustained load."; break;
  case DMG_OILP: why = "Oil pressure dropped and a rod bearing spun. Thicker oil and keep temps under control."; break;
  case DMG_LUBE: why = "A 2-stroke gets ALL its lubrication from the fuel. Mix in oil: 32:1 to 50:1."; break;
  case DMG_DIRT: why = "Unfiltered air sandblasted the cylinder walls. Put a filter on it."; break;
  case DMG_TURBO: why = "The turbo spun past its limit and came apart. Lower the boost or use a bigger turbo."; break;
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

static void TopBar(int sw) {
 DrawRectangle(0, 0, sw, 52, (Color){13, 14, 17, 255});
 DrawRectangle(0, 52, sw, 1, COL_LINE);
 Tb("ENGINE", 18, 12, 26, COL_ACC);
 Tb("GARAGE", 18 + Tw("ENGINE ", 26, 1), 12, 26, COL_TXT);
 Tx(gMode ? "Dyno cell" : "Build bench", 26 + Tw("ENGINE GARAGE", 26, 1), 19, 16, COL_DIM);
 if (gDev) { DrawRectangleRounded((Rectangle){340, 13, 120, 26}, 0.4f, 6, COL_RED); TxC("DEV 2323", 400, 17, 15, COL_TXT, 1); }
 if (UiButton((Rectangle){sw - 222, 9, 210, 34}, TextFormat("CHALLENGES  %d/%d", AchCount(), NACH), gShowAch ? 1 : 0, 1)) gShowAch ^= 1;
 Tx(gMuted ? "SOUND: MUTED [M]" : TextFormat("VOL %.0f%%", gVolume * 100), sw - 380, 18, 14, COL_DIM);
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
 for (int i = 0; i < 6; i++) {
  if (gToast[i].t <= 0) continue;
  gToast[i].t -= dt;
  float a = Clampf(gToast[i].t, 0, 1);
  float sz = 18, w = Tw(gToast[i].s, sz, 1) + 32;
  float x = sw * 0.5f - w * 0.5f;
  if (gMode == 0) x = 500;
  DrawRectangleRounded((Rectangle){x, y, w, 34}, 0.4f, 6, Fade((Color){12, 13, 16, 255}, 0.9f * a));
  Tb(gToast[i].s, x + 16, y + 7, sz, Fade(gToast[i].c, a));
  y += 40;
 }
}

// ===== autotest =====
static void Shot(const char *name) { TakeScreenshot(TextFormat("%s/%s.png", gShotDir, name)); }
static int gAtFrame = 0, gAtStart = 0, gAtThr = 0;

static void AutoAudioTest(void) {
 float buf[4096 * 2];
 FILE *f = fopen(TextFormat("%s/audio.raw", gShotDir), "wb");
 double sum = 0; float peak = 0; int nan = 0;
 AU.running = 1; AU.q = 1; AU.volume = 0.7f;
 for (int blk = 0; blk < 60; blk++) {
  AU.rpm = 800 + blk * 100.f; AU.load = blk < 30 ? 0.3f : 1.0f; AU.thr = AU.load; AU.spool = blk / 60.f;
  AudioCB(buf, 4096);
  for (int i = 0; i < 8192; i++) { if (isnan(buf[i])) nan++; sum += buf[i] * buf[i]; if (fabsf(buf[i]) > peak) peak = fabsf(buf[i]); }
  if (f) fwrite(buf, sizeof(float), 8192, f);
 }
 if (f) fclose(f);
 printf("AUDIO rms=%.4f peak=%.3f nan=%d\n", sqrt(sum / (60 * 8192)), peak, nan);
}

int main(int argc, char **argv) {
 for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--autotest") && i + 1 < argc) { gAutotest = 1; snprintf(gShotDir, 256, "%s", argv[i + 1]); }
 SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
 InitWindow(1600, 900, "Engine Garage - DOOG");
 SetWindowMinSize(1280, 820);
 SetExitKey(0);
 SetTargetFPS(gAutotest ? 0 : 120);
 int cps[100]; int nc = 0; for (int c = 32; c < 127; c++) cps[nc++] = c; cps[nc++] = 176;
 fR = LoadFontFromMemory(".ttf", reg_ttf, reg_ttf_len, 48, cps, nc);
 fB = LoadFontFromMemory(".ttf", bold_ttf, bold_ttf_len, 48, cps, nc);
 GenTextureMipmaps(&fR.texture); SetTextureFilter(fR.texture, TEXTURE_FILTER_TRILINEAR);
 GenTextureMipmaps(&fB.texture); SetTextureFilter(fB.texture, TEXTURE_FILTER_TRILINEAR);
 rlSetClipPlanes(0.05, 300);
 RenderInit();
 SaveRead();
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
 Camera3D cam = {0}; cam.up = (Vector3){0, 1, 0}; cam.fovy = 38; cam.projection = CAMERA_PERSPECTIVE;
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
  while ((chr = GetCharPressed())) {
   if (chr >= '0' && chr <= '9') { memmove(gDevBuf, gDevBuf + 1, 3); gDevBuf[3] = (char)chr; gDevBuf[4] = 0; if (!strcmp(gDevBuf, "2323")) { gDev ^= 1; ToastAdd(gDev ? "DEV MODE: unbreakable engine, infinite battery" : "Dev mode off", COL_RED); memset(gDevBuf, 0, sizeof gDevBuf); } }
  }
  if (IsKeyPressed(KEY_M)) gMuted ^= 1;
  if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) gVolume = Clampf(gVolume + 0.1f, 0, 1);
  if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) gVolume = Clampf(gVolume - 0.1f, 0, 1);
  int startHeld = 0, startPress = 0;
  // autotest script
  if (gAutotest) {
   int f = gAtFrame++;
   static int at_preset = 0, at_phase = 0, at_t = 0;
   if (f == 2) { gTab = 0; }
   if (at_phase == 0) {
    if (f % 4 == 1) { gB = PRESETS[at_preset]; OnBuildChanged(); ResetCamDist(); gTab = at_preset % 5; }
    if (f % 4 == 3) {
     printf("PRESET %-20s est %.0f hp @%.0f  %.0f lbft @%.0f  red %.0f  w %.0fkg  imep %.1f/%.1f  warn %d\n", PRESETS[at_preset].name, gS.estHp, gS.estHpRpm, gS.estTq * 0.7376f, gS.estTqRpm, gS.redline, gS.weight, gS.estImep, gS.pLimit, gNWarn);
     for (int i = 0; i < gNWarn; i++) printf("   W: %s\n", gWarn[i]);
     Shot(TextFormat("build_%d", at_preset));
     if (at_preset == 2) { gB.tilt = 30; OnBuildChanged(); Shot("build_tilt"); }
     if (++at_preset >= NPRESET) { at_phase = 1; at_preset = 0; at_t = 0; }
    }
   } else if (at_phase == 1) {
    // per preset dyno test
    static const int order[] = {3, 0, 1, 2, 4, 5, 6, 7};
    if (at_t == 0) { gB = PRESETS[order[at_preset]]; OnBuildChanged(); ResetCamDist(); gMode = 1; SimReset(&gM, &gS, 0); gDiagShown = 0; gWasRunning = 0; gStartedOnce = 0; gCrankT = 0; gM.choke = gB.delivery == 0; gM.ign = 1; }
    at_t++;
    if (gB.starter == 1) startHeld = at_t < 150 && !gM.running;
    else if (at_t % 50 == 5 && !gM.running) startPress = 1;
    if (gM.choke && gM.temp > 45) gM.choke = 0;
    if (at_t == 400) { gM.thrCmd = 1; }
    if (at_t == 430) { Shot(TextFormat("test_rev_%d", order[at_preset])); gM.thrCmd = 0; }
    if (at_t == 600 && gM.running) { gM.dyno = 1; gM.nd = 0; gM.dSum = 0; gM.dCnt = 0; gM.dynoPeakHp = gM.dynoPeakTq = 0; gM.dynoTarget = gM.dynoStart = fmaxf(gM.rpm, gS.idle + 300); gM.dynoInt = 0; gM.dynoEnd = gS.redline * 0.99f; }
    if (at_t > 600 && at_t < 2400 && gM.dyno == 0 && gM.nd > 5 && at_t % 1 == 0 && !gM.failed) {
     static int shot = -1;
     if (shot != at_preset) { shot = at_preset; Shot(TextFormat("test_dyno_%d", order[at_preset])); gDS.cut = 1; at_t = 2400; }
    }
    if (at_t == 2405) { Shot(TextFormat("test_cut_%d", order[at_preset])); gDS.cut = 0; }
    if (at_t == 2410 || (at_t == 2399)) {
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
    gMode = 0; gShowAch = 1; static int k3 = 0; if (++k3 == 5) Shot("ach"); if (k3 > 8) break;
   }
  }
  if (gMode == 0) {
   gVisAng = fmodf(gVisAng + 20 * dt, 720);
   gM.rpm = 0; gM.running = 0;
  } else {
   startHeld |= IsKeyDown(KEY_E) || IsKeyDown(KEY_ENTER) || gStartHeldUI;
   startPress |= IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || gBtnStart;
   if (gB.starter == 0) startHeld = 0;
   if (IsKeyPressed(KEY_I) || gBtnIgn) { if (!gM.failed) { gM.ign ^= 1; if (!gM.ign) gM.dyno = 0; } }
   if ((IsKeyPressed(KEY_C) || gBtnChoke) && gB.delivery == 0) gM.choke ^= 1;
   if (IsKeyPressed(KEY_L) || gBtnLim) { gM.limiter ^= 1; ToastAdd(gM.limiter ? "Rev limiter ON" : "Rev limiter OFF. Careful.", gM.limiter ? COL_DIM : COL_RED); }
   if (IsKeyPressed(KEY_X) || gBtnCut) gDS.cut ^= 1;
   if (IsKeyPressed(KEY_D) || gBtnDyno) {
    if (gM.dyno) { gM.dyno = 0; gM.thrCmd = 0; ToastAdd("Pull aborted", COL_DIM); }
    else if (gM.running) {
     if (gM.nd > 5) { gM.npd = gM.nd; memcpy(gM.pdrpm, gM.drpm, sizeof gM.drpm); memcpy(gM.pdhp, gM.dhp, sizeof gM.dhp); memcpy(gM.pdtq, gM.dtq, sizeof gM.dtq); }
     gM.dyno = 1; gM.nd = 0; gM.dSum = 0; gM.dCnt = 0; gM.dynoPeakHp = gM.dynoPeakTq = 0; gM.dynoInt = 0;
     gM.dynoTarget = gM.dynoStart = fmaxf(gM.rpm, gS.idle + 300); gM.dynoEnd = (gM.limiter ? gS.redline : gS.redline * 1.04f) * 0.99f;
     ToastAdd("Dyno pull: wide open throttle to redline", COL_ACC);
    } else ToastAdd("Start the engine first.", COL_YEL);
   }
   if ((IsKeyPressed(KEY_R) || gBtnRebuild) && gM.failed) { SimReset(&gM, &gS, 1); ToastAdd("Rebuilt. Fresh engine.", COL_GRN); gWasRunning = 0; gStartedOnce = 0; }
   if (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_ESCAPE) || gBtnBack) { gMode = 0; gM.dyno = 0; gShowAch = 0; }
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
   float adv = fminf(gM.rpm * 6.f * dt, 26.f);
   gVisAng = fmodf(gVisAng + adv, 720.f);
   gDS.fanAng += fminf(gM.rpm * 0.9f / 60.f * 2 * PI * dt, 0.42f);
  }
  // audio params
  AU.rpm = gMode ? gM.rpm : 0; AU.load = gM.load; AU.q = gM.q; AU.knock = gM.knock; AU.spool = gM.spool; AU.boost = gM.boost;
  AU.thr = fmaxf(gM.thrAct, gM.idleOpen); AU.volts = gM.volts; AU.running = gMode && gM.running; AU.cut = gM.cut;
  AU.starter = gMode && gM.starterOn && gM.volts > 9.5f; AU.recoil = gMode && gM.recoilT > 0; AU.volume = gMuted ? 0 : gVolume;
  if (gM.bovCount != AU.bov) AU.bov = gM.bovCount;
  UpdateCamera3D(&cam, 1);
  BeginDrawing();
  ClearBackground(COL_BG);
  Scene3D(cam);
  TopBar(sw);
  if (gMode == 0) { BuildPanel(sw, sh); SpecCard(sw, sh); }
  else TestHUD(sw, sh);
  if (gShowAch) AchPanel(sw, sh);
  Toasts(sw, dt);
  EndDrawing();
 }
 if (audioOk) { UnloadAudioStream(stream); CloseAudioDevice(); }
 CloseWindow();
 return 0;
}
