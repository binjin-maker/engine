// ===== procedural engine audio =====
#define SR 44100
typedef struct { float b0, b1, b2, a1, a2, z1, z2; } Bq;
static void BqBP(Bq *q, float f, float Q) {
 f = Clampf(f, 20, SR * 0.45f);
 float w = 2 * PI * f / SR, al = sinf(w) / (2 * Q), c = cosf(w), a0 = 1 + al;
 q->b0 = al / a0; q->b1 = 0; q->b2 = -al / a0; q->a1 = -2 * c / a0; q->a2 = (1 - al) / a0;
}
static void BqLP(Bq *q, float f, float Q) {
 f = Clampf(f, 20, SR * 0.45f);
 float w = 2 * PI * f / SR, al = sinf(w) / (2 * Q), c = cosf(w), a0 = 1 + al;
 q->b0 = (1 - c) / 2 / a0; q->b1 = (1 - c) / a0; q->b2 = q->b0; q->a1 = -2 * c / a0; q->a2 = (1 - al) / a0;
}
static void BqHP(Bq *q, float f, float Q) {
 f = Clampf(f, 20, SR * 0.45f);
 float w = 2 * PI * f / SR, al = sinf(w) / (2 * Q), c = cosf(w), a0 = 1 + al;
 q->b0 = (1 + c) / 2 / a0; q->b1 = -(1 + c) / a0; q->b2 = q->b0; q->a1 = -2 * c / a0; q->a2 = (1 - al) / a0;
}
static inline float BqRun(Bq *q, float x) {
 float y = q->b0 * x + q->z1;
 q->z1 = q->b1 * x - q->a1 * y + q->z2;
 q->z2 = q->b2 * x - q->a2 * y;
 return y;
}

typedef struct {
 volatile float rpm, load, q, knock, spool, boost, thr, volts, vcylL, volume, egt, temp;
 volatile int cam; volatile int running, ncyl, two, exhaust, diesel, forced, starter, recoil, bov, bang, cut, filter, valvetrain, nb, click, failed, intercooler;
 float fire[16]; int bank[16]; float gain[16]; float delay[16];
 volatile float musicVol, duck, engVol, sfxVol;
 volatile int sfxHead; int sfxQ[32];
} AudioIn;
static AudioIn AU = {0};

typedef struct { long long when; int bank, kind; float amp; } Ev;
#define NEV 96
#define COMB 2048
static struct {
 double ang; long long n;
 Ev ev[NEV];
 float eS[2], eB[2], eN[2], eI, clat[2];
 float comb[2][COMB]; int ci;
 Bq body[2], ring[2], lp[2], lp2[2], intake, intakeLP, hiss, bovf, rope, bangf, clatHP, mechBP;
 Bq inRes, subLP, flutBP, starterBP, rvLP[8];
 float eIimp, sub, flutEnv, flutPh, rvC[8][1700], rvA[4][700]; int rvI[8], rvAi[4];
 float turboPh, scPh, stPh, gearPh, tick, ping, pingPh, bovEnv, bangEnv, ropeEnv, clickEnv, crackle, smoothRpm, smoothLoad;
 int lastBov, lastBang, lastClick;
 unsigned rng;
 float dcL, dcR;
} SY;

static inline float Nz(void) { SY.rng = SY.rng * 1664525u + 1013904223u; return ((SY.rng >> 8) & 0xffff) / 32768.f - 1.f; }
static inline float Rnd01(void) { return (Nz() + 1) * 0.5f; }

static void EvPush(long long when, int bank, int kind, float amp) {
 for (int i = 0; i < NEV; i++) if (SY.ev[i].kind == 0) { SY.ev[i] = (Ev){when, bank, kind, amp}; return; }
}

// ----- sfx -----
enum { SFX_CLICK, SFX_SELECT, SFX_TICK, SFX_POP, SFX_ACH, SFX_KEY, SFX_RATCHET, SFX_BEEP, SFX_WARN, SFX_DONE, SFX_STALL };
typedef struct { int type, on; float t, ph, ph2; } Voice;
static Voice gVox[12];
static int gSfxTail = 0;
static void SfxPush(int type) { AU.sfxQ[AU.sfxHead & 31] = type; AU.sfxHead++; }
static void SfxStart(int type) {
 int k = 0; float best = -1;
 for (int i = 0; i < 12; i++) { if (!gVox[i].on) { k = i; best = 1e9f; break; } if (gVox[i].t > best) { best = gVox[i].t; k = i; } }
 gVox[k] = (Voice){type, 1, 0, 0, 0};
}
static float NoteHz(float midi) { return 440.f * powf(2.f, (midi - 69) / 12.f); }
static float SfxRun(void) {
 float out = 0, dt = 1.f / SR;
 for (int i = 0; i < 12; i++) {
  Voice *v = &gVox[i];
  if (!v->on) continue;
  float t = v->t, s = 0, len = 0.1f;
  switch (v->type) {
  case SFX_CLICK: len = 0.05f; v->ph += 2 * PI * (1800 - t * 14000) * dt; s = sinf(v->ph) * expf(-t * 90) * 0.35f + Nz() * expf(-t * 400) * 0.15f; break;
  case SFX_SELECT: len = 0.07f; v->ph += 2 * PI * (t < 0.03f ? 1100 : 1500) * dt; s = (sinf(v->ph) > 0 ? 1 : -1) * expf(-t * 50) * 0.08f; break;
  case SFX_TICK: len = 0.025f; s = Nz() * expf(-t * 300) * 0.12f + sinf(2 * PI * 3200 * t) * expf(-t * 200) * 0.06f; break;
  case SFX_POP: len = 0.12f; v->ph += 2 * PI * (600 + t * 3000) * dt; s = sinf(v->ph) * expf(-t * 35) * 0.16f; break;
  case SFX_ACH: {
   len = 1.0f; static const float n[4] = {72, 76, 79, 84};
   int k = (int)(t / 0.11f); if (k > 3) k = 3;
   float lt = t - k * 0.11f;
   v->ph += 2 * PI * NoteHz(n[k]) * dt; v->ph2 += 2 * PI * NoteHz(n[k]) * 2.003f * dt;
   s = (sinf(v->ph) * 0.6f + sinf(v->ph2) * 0.25f) * expf(-lt * (k == 3 ? 3.5f : 12)) * 0.28f; break; }
  case SFX_KEY: len = 0.18f; s = Nz() * (expf(-t * 250) + (t > 0.09f ? expf(-(t - 0.09f) * 250) : 0)) * 0.3f; break;
  case SFX_RATCHET: len = 0.5f; { float ph = fmodf(t, 0.06f); s = Nz() * expf(-ph * 260) * 0.25f * (1 - t * 1.6f); } break;
  case SFX_BEEP: len = 0.42f; s = ((t < 0.12f) || (t > 0.24f && t < 0.36f)) ? sinf(2 * PI * 1320 * t) * 0.12f : 0; break;
  case SFX_WARN: len = 0.3f; s = sinf(2 * PI * (t < 0.15f ? 880 : 660) * t) * 0.10f; break;
  case SFX_STALL: len = 0.55f; v->ph += 2 * PI * (420.f * expf(-t * 2.6f) + 60.f) * dt; s = (sinf(v->ph) + 0.3f * sinf(v->ph * 2)) * fminf(1, t * 60) * expf(-t * 3.5f) * 0.32f; break;
  case SFX_DONE: len = 0.5f; v->ph += 2 * PI * NoteHz(t < 0.14f ? 79 : 84) * dt; s = sinf(v->ph) * expf(-fmodf(t, 0.14f) * 10) * 0.2f; break;
  }
  out += s;
  v->t += dt;
  if (v->t > len) v->on = 0;
 }
 return out;
}

// ----- music: procedural synthwave/lo-fi loop -----
static struct { double t; float pad[6], bassPh, leadPh, kickPh, lp1, lp2, hp, rev[4096]; int ri; } MU;
static float MusicRun(void) {
 const float bpm = 96;
 float beat = 60.f / bpm, step = beat / 4;
 double t = MU.t; MU.t += 1.0 / SR;
 int stp = (int)(t / step), bar = stp / 16, s16 = stp % 16;
 float st = (float)(t - stp * step);
 float bt = (float)fmod(t, beat);
 static const int prog[8][3] = {{57,60,64},{53,57,60},{48,52,55},{55,59,62},{57,60,64},{53,57,60},{50,53,57},{52,56,59}};
 static const int bassR[8] = {45,41,36,43,45,41,38,40};
 const int *ch = prog[bar % 8];
 int section = (bar / 8) % 4;
 float o = 0;
 // pad
 float padEnv = Clampf((float)fmod(t, beat * 4) * 3, 0, 1);
 float pad = 0;
 for (int i = 0; i < 3; i++) for (int d = 0; d < 2; d++) {
  float *ph = &MU.pad[i * 2 + d];
  *ph += NoteHz(ch[i]) * (d ? 1.004f : 0.997f) / SR; if (*ph > 1) *ph -= 1;
  pad += (*ph * 2 - 1);
 }
 MU.lp1 += (pad - MU.lp1) * 0.035f; MU.lp2 += (MU.lp1 - MU.lp2) * 0.035f;
 o += MU.lp2 * 0.05f * padEnv;
 // bass
 int bnote = bassR[bar % 8] + ((s16 == 6 || s16 == 14) ? 12 : 0);
 MU.bassPh += 2 * PI * NoteHz(bnote) / SR;
 float bEnv = (s16 % 2 == 0 || s16 == 6 || s16 == 14) ? expf(-st * 6) : expf(-(st + step) * 6);
 o += (sinf(MU.bassPh) + 0.3f * sinf(MU.bassPh * 2)) * 0.13f * bEnv;
 // drums
 int kickHit = (s16 == 0 || s16 == 8 || (section >= 2 && s16 == 10));
 if (kickHit && section > 0) { float kp = 2 * PI * (45 + 120 * expf(-st * 40)) / SR; MU.kickPh += kp; o += sinf(MU.kickPh) * expf(-st * 12) * 0.32f; } else MU.kickPh = 0;
 if ((s16 == 4 || s16 == 12) && section > 0) o += Nz() * expf(-st * 22) * 0.10f + sinf(2 * PI * 190 * st) * expf(-st * 30) * 0.08f;
 if (s16 % 2 == 0 && section != 0) { float n = Nz(); MU.hp = n - MU.hp * 0.0f; o += (n - MU.lp1 * 0) * expf(-st * 90) * (s16 % 4 == 2 ? 0.045f : 0.025f); }
 // lead arp
 if (section == 1 || section == 3) {
  int arp[4] = {ch[0] + 12, ch[1] + 12, ch[2] + 12, ch[1] + 24};
  int nn = arp[s16 % 4];
  MU.leadPh += NoteHz(nn) / SR; if (MU.leadPh > 1) MU.leadPh -= 1;
  float tri = 4 * fabsf(MU.leadPh - 0.5f) - 1;
  o += tri * expf(-st * 9) * 0.06f;
 }
 (void)bt;
 // tiny echo
 float e = MU.rev[MU.ri];
 MU.rev[MU.ri] = o + e * 0.35f;
 MU.ri = (MU.ri + 1) % 4096;
 return o + e * 0.3f;
}

static void AudioCB(void *buf, unsigned int frames) {
 float *out = (float *)buf;
 float rpmT = AU.rpm, q = AU.q;
 int n = AU.ncyl < 1 ? 1 : AU.ncyl;
 float cycle = AU.two ? 360.f : 720.f;
 float vc = fmaxf(AU.vcylL, 0.03f);
 float size = powf(vc / 0.5f, 0.5f);
 int ex = AU.exhaust & 3;
 int turbo = AU.forced >= 1 && AU.forced <= 3;
 // exhaust character: body freq, body Q, lowpass, comb feedback, pipe length (m), drive, volume
 static const float EXC[4][7] = {
  {95, 1.3f, 520, 0.35f, 2.6f, 1.2f, 0.55f},
  {110,1.6f, 1500,0.55f, 1.9f, 2.0f, 0.60f},
  {125,1.2f, 4800,0.70f, 1.2f, 3.2f, 1.15f},
  {170,2.0f, 3600,0.80f, 1.0f, 2.6f, 0.90f},
 };
 const float *E = EXC[ex];
 float sc = powf(0.5f / vc, 0.22f);
 float lpf = E[2] * (turbo ? 0.55f : 1.f) * (AU.valvetrain == 0 ? 0.7f : 1.f) * (AU.diesel ? 1.2f : 1.f);
 int D = (int)(SR * 2 * E[4] / 520.f);
 if (ex == 3) D = (int)(SR / fmaxf(40.f, 140 + rpmT * 0.06f));
 D = D >= COMB ? COMB - 1 : (D < 8 ? 8 : D);
 for (int bk = 0; bk < 2; bk++) {
  float bm = bk ? 1.08f : 1.0f;
  BqBP(&SY.body[bk], E[0] * sc * bm * (0.8f + 0.4f * Clampf(rpmT / 7000.f, 0, 1.5f)), E[1]);
  BqBP(&SY.ring[bk], ex == 3 ? 300 + rpmT * 0.1f : 420 * sc * bm, ex == 3 ? 6.f : 2.5f);
  BqLP(&SY.lp[bk], lpf, 0.75f);
  BqLP(&SY.lp2[bk], lpf * 1.6f, 0.6f);
 }
 BqBP(&SY.intake, (AU.filter == 0 ? 520 : 300) + rpmT * 0.05f, 2.2f);
 BqLP(&SY.intakeLP, AU.filter == 0 ? 4000 : (AU.filter == 3 ? 2500 : 1200), 0.7f);
 BqBP(&SY.hiss, 3800, 1.5f);
 BqBP(&SY.bovf, 2300, 1.2f);
 BqBP(&SY.rope, 1400, 2.0f);
 BqLP(&SY.bangf, 700, 0.9f);
 BqBP(&SY.clatHP, 2400, 1.1f);
 BqBP(&SY.mechBP, 1400 + rpmT * 0.05f, 0.9f);
 BqBP(&SY.inRes, 160 + rpmT * 0.035f, 5.0f);
 BqLP(&SY.subLP, 70 + rpmT * 0.008f, 0.8f);
 BqBP(&SY.flutBP, 900, 1.4f);
 BqBP(&SY.starterBP, 2400, 3.0f);
 for (int k = 0; k < 8; k++) BqLP(&SY.rvLP[k], 3200, 0.6f);
 float cold = Clampf((60.f - AU.temp) / 45.f, 0, 1);
 if (AU.bov != SY.lastBov) { SY.lastBov = AU.bov; SY.bovEnv = Clampf(AU.boost / 12.f, 0.3f, 1); if (AU.spool > 0.45f) SY.flutEnv = AU.spool; }
 if (AU.bang != SY.lastBang) { SY.lastBang = AU.bang; SY.bangEnv = 1.0f; }
 if (AU.click != SY.lastClick) { SY.lastClick = AU.click; SY.clickEnv = 1.0f; }
 while (gSfxTail != AU.sfxHead) { SfxStart(AU.sfxQ[gSfxTail & 31]); gSfxTail++; }
 float mvol = AU.musicVol * (1 - 0.7f * Clampf(AU.duck, 0, 1));
 float vol = AU.volume * AU.engVol * E[6];
 float intakeLoud = AU.filter == 0 ? 2.5f : (AU.filter == 3 ? 1.6f : (AU.filter == 4 ? 0.6f : 1.0f));
 float tauB = AU.two ? 0.0018f : (AU.diesel ? 0.0035f : 0.0028f);
 float tauN = AU.two ? 0.0025f : 0.004f;
 float dS = expf(-1.f / (SR * 0.00025f)), dB = expf(-1.f / (SR * tauB)), dN = expf(-1.f / (SR * tauN)), dI = expf(-1.f / (SR * 0.006f));
 for (unsigned i = 0; i < frames; i++) {
  SY.smoothRpm += (rpmT - SY.smoothRpm) * 0.002f;
  SY.smoothLoad += (AU.load - SY.smoothLoad) * 0.002f;
  float rpm = SY.smoothRpm, load = SY.smoothLoad;
  double prev = SY.ang;
  SY.ang += rpm * 6.0 / SR;
  int wrapped = 0;
  if (SY.ang >= cycle) { SY.ang -= cycle; wrapped = 1; }
  for (int k = 0; k < n; k++) {
   float fa = AU.fire[k];
   int hit = wrapped ? (fa > prev || fa <= SY.ang) : (fa > prev && fa <= SY.ang);
   if (hit) {
    int bk = AU.bank[k] & 1;
    long long when = SY.n + (long long)(AU.delay[k] * SR);
    if (AU.running && !AU.cut && Rnd01() < q) {
     float A = size * (0.24f + 0.76f * powf(Clampf(load, 0, 1.3f), 1.3f)) * AU.gain[k];
     // cycle-to-cycle variation: rougher when cold, lumpy at idle with a big cam
     float lope = AU.two ? 0 : (AU.cam >= 2 ? (AU.cam == 3 ? 0.55f : 0.25f) : 0) * Clampf(1.f - rpm / 2600.f, 0, 1);
     A *= 1.f + Nz() * (0.06f + 0.18f * cold) + lope * (Nz() > 0 ? 0.8f : -0.5f);
     when += (long long)(Nz() * (8 + 30 * lope));
     EvPush(when, bk, 1, A);
     if (AU.knock > 0.05f && Rnd01() < AU.knock * 0.6f) SY.ping = fminf(1, SY.ping + 0.7f);
     if (!AU.two) SY.tick += AU.valvetrain <= 1 ? 0.06f : 0.025f;
    } else if (rpm > 20) {
     EvPush(when, bk, 2, AU.running ? 0.05f * size : 0.14f * size);
     if (AU.running && AU.thr < 0.06f && rpm > 2500 && ex >= 1 && Rnd01() < (ex == 2 ? 0.12f : 0.035f)) EvPush(when + (long long)(Rnd01() * 600), bk, 3, size * (0.6f + Rnd01() * 0.8f));
    }
    if (AU.running) {
     float ia = AU.two ? 180.f : 360.f;
     float when2 = ia / fmaxf(rpm * 6.f, 1.f);
     if (when2 < 0.5f) EvPush(SY.n + (long long)(when2 * SR), 0, 4, AU.thr * size * 0.6f);
    }
   }
  }
  for (int e = 0; e < NEV; e++) {
   Ev *v = &SY.ev[e];
   if (!v->kind || v->when > SY.n) continue;
   int bk = v->bank;
   if (v->kind == 1) { SY.sub += v->amp * 0.9f; SY.eS[bk] += v->amp; SY.eB[bk] += v->amp; SY.eN[bk] += v->amp * 0.7f; if (AU.diesel) SY.clat[bk] += v->amp * 0.9f; }
   else if (v->kind == 2) { SY.eB[bk] += v->amp; SY.eN[bk] += v->amp * 0.3f; }
   else if (v->kind == 3) { SY.eS[bk] += v->amp * 1.4f; SY.eN[bk] += v->amp * 1.6f; }
   else if (v->kind == 4) { SY.eI += v->amp; SY.eIimp += v->amp; }
   v->kind = 0;
  }
  float o[2];
  float drive = E[5] * (0.6f + 1.4f * Clampf(load, 0, 1.2f)) * (turbo ? 0.7f : 1.f);
  for (int bk = 0; bk < 2; bk++) {
   float nz = Nz();
   float x = SY.eS[bk] * 1.3f - SY.eB[bk] * 0.5f + SY.eN[bk] * nz * 0.8f;
   if (AU.two) x += SY.eS[bk] * Nz() * 0.6f;
   SY.eS[bk] *= dS; SY.eB[bk] *= dB; SY.eN[bk] *= dN;
   int r = (SY.ci - D + COMB) % COMB;
   float y = x + SY.comb[bk][r] * E[3];
   SY.comb[bk][SY.ci] = BqRun(&SY.lp2[bk], y);
   float s = BqRun(&SY.body[bk], y) * 2.2f + BqRun(&SY.ring[bk], y) * (ex == 3 ? 1.4f : 0.5f) + y * 0.35f;
   s = BqRun(&SY.lp[bk], s);
   if (AU.diesel) { s += BqRun(&SY.clatHP, SY.clat[bk] * Nz()) * 0.35f; SY.clat[bk] *= 0.985f; }
   o[bk] = tanhf(s * drive) / fmaxf(tanhf(drive), 0.3f);
  }
  SY.ci = (SY.ci + 1) % COMB;
  SY.n++;
  float common = 0;
  float inn = BqRun(&SY.intake, Nz() * SY.eI);
  common += BqRun(&SY.intakeLP, inn) * 0.9f * intakeLoud;
  common += BqRun(&SY.inRes, SY.eIimp) * 0.35f * intakeLoud * (0.3f + 0.7f * AU.thr);
  SY.eIimp = 0;
  SY.eI *= dI;
  float subv = BqRun(&SY.subLP, SY.sub) * 0.55f * Clampf(sqrtf(vc / 0.5f), 0.3f, 1.6f);
  SY.sub *= 0.93f;
  common += subv;
  if (SY.flutEnv > 0.001f) {
   SY.flutPh += 2 * PI * (11.f + 9.f * SY.flutEnv) / SR;
   float g = 0.5f + 0.5f * sinf(SY.flutPh); g *= g * g;
   common += BqRun(&SY.flutBP, Nz()) * g * SY.flutEnv * 0.55f;
   SY.flutEnv *= 0.99988f;
  }
  if (SY.tick > 0.001f) { common += BqRun(&SY.mechBP, Nz()) * SY.tick * 0.5f; SY.tick *= 0.97f; }
  if (SY.ping > 0.001f) { SY.pingPh += 2 * PI * 6300.f / SR; common += sinf(SY.pingPh) * SY.ping * 0.22f; SY.ping *= 0.9993f; }
  if (turbo) {
   float sp = AU.spool;
   SY.turboPh += 2 * PI * (1800.f + 7800.f * sp) / SR;
   common += sinf(SY.turboPh) * 0.05f * sp * sp * (0.3f + 0.7f * AU.thr);
   common += BqRun(&SY.hiss, Nz()) * 0.07f * sp * AU.thr;
  }
  if (AU.forced == 4) {
   SY.scPh += 2 * PI * (rpm / 60.f * 11.f) / SR;
   float w = sinf(SY.scPh) + 0.5f * sinf(SY.scPh * 2.f) + 0.25f * sinf(SY.scPh * 3.f);
   common += w * 0.05f * Clampf(rpm / 6000.f, 0, 1.3f) * (0.4f + 0.6f * AU.thr);
  }
  if (SY.bovEnv > 0.001f) { common += BqRun(&SY.bovf, Nz()) * SY.bovEnv * 0.6f; SY.bovEnv *= 0.99985f; }
  if (AU.starter) {
   SY.stPh += 2 * PI * (110.f + rpm * 1.3f) / SR;
   float s = sinf(SY.stPh); s += 0.35f * (s > 0 ? 1 : -1);
   common += (s * 0.10f + Nz() * 0.03f) * Clampf((AU.volts - 8.f) / 4.f, 0.1f, 1);
   common += BqRun(&SY.starterBP, sinf(SY.stPh * 9.f) + Nz() * 0.3f) * 0.05f;
  }
  if (AU.recoil) SY.ropeEnv = fminf(1, SY.ropeEnv + 0.002f); else SY.ropeEnv *= 0.999f;
  if (SY.ropeEnv > 0.001f) common += BqRun(&SY.rope, Nz()) * SY.ropeEnv * 0.3f;
  if (SY.clickEnv > 0.001f) { common += Nz() * SY.clickEnv * 0.5f; SY.clickEnv *= 0.992f; }
  if (SY.bangEnv > 0.001f) { common += BqRun(&SY.bangf, Nz()) * SY.bangEnv * 3.0f + Nz() * SY.bangEnv * SY.bangEnv * 0.5f; SY.bangEnv *= 0.99992f; }
  float L, R;
  if (AU.nb > 1) { L = o[0] * 0.72f + o[1] * 0.28f; R = o[0] * 0.28f + o[1] * 0.72f; }
  else { L = R = o[0]; }
  float mus = MusicRun() * mvol, fx = SfxRun() * AU.volume * AU.sfxVol * 2.2f;
  float ev = AU.volume * AU.engVol;
  L = tanhf(L * 1.75f) * E[6] * ev * 1.6f + common * 1.8f * ev; R = tanhf(R * 1.75f) * E[6] * ev * 1.6f + common * 1.8f * ev;
  {
   // small garage room: parallel damped combs into allpasses, mixed in low
   static const int CL[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617}, AL[4] = {556, 441, 341, 225};
   float in = (L + R) * 0.5f, wl = 0, wr = 0;
   for (int k = 0; k < 8; k++) {
    float y = SY.rvC[k][SY.rvI[k]];
    SY.rvC[k][SY.rvI[k]] = in + BqRun(&SY.rvLP[k], y) * 0.72f;
    SY.rvI[k] = (SY.rvI[k] + 1) % CL[k];
    if (k & 1) wr += y; else wl += y;
   }
   for (int k = 0; k < 4; k++) {
    float *w = (k & 1) ? &wr : &wl, b = SY.rvA[k][SY.rvAi[k]];
    SY.rvA[k][SY.rvAi[k]] = *w + b * 0.5f; *w = b - *w * 0.5f;
    SY.rvAi[k] = (SY.rvAi[k] + 1) % AL[k];
   }
   L += wl * 0.045f; R += wr * 0.045f;
  }
  SY.dcL = SY.dcL * 0.9995f + L * 0.0005f; L -= SY.dcL;
  SY.dcR = SY.dcR * 0.9995f + R * 0.0005f; R -= SY.dcR;
  out[i * 2] = tanhf(L + mus + fx) * 0.97f;
  out[i * 2 + 1] = tanhf(R + mus + fx) * 0.97f;
 }
}

static void AudioSetupEngine(const Build *b, const Spec *s) {
 float x[16], an[16]; int bk[16];
 int n = CylGeom(b, x, an, bk);
 AU.ncyl = 0;
 FireAngles(b, AU.fire);
 int nb = 1;
 for (int i = 0; i < n; i++) { AU.bank[i] = bk[i] & 1; if (bk[i]) nb = 2; }
 float xmax = -1e9f;
 for (int i = 0; i < n; i++) xmax = fmaxf(xmax, x[i]);
 for (int i = 0; i < n; i++) {
  float len = 0.35f + (xmax - x[i]) * 1e-3f * (b->exhaust == 1 ? 0.4f : 1.f);
  AU.delay[i] = len / 520.f;
 }
 unsigned r = 777;
 for (int i = 0; i < n; i++) { r = r * 1103515245u + 12345u; AU.gain[i] = 0.82f + 0.36f * ((r >> 16) & 1023) / 1023.f; }
 AU.cam = b->cam; AU.two = b->cycle; AU.exhaust = b->exhaust; AU.diesel = FUELS[b->fuel].diesel; AU.forced = b->forced; AU.intercooler = b->intercooler;
 AU.filter = b->filter; AU.valvetrain = b->cycle ? 9 : b->valvetrain; AU.nb = nb; AU.vcylL = s->vcylL;
 AU.ncyl = n;
}
