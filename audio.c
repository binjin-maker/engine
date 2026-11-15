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
static inline float BqRun(Bq *q, float x) {
 float y = q->b0 * x + q->z1;
 q->z1 = q->b1 * x - q->a1 * y + q->z2;
 q->z2 = q->b2 * x - q->a2 * y;
 return y;
}

typedef struct {
 volatile float rpm, load, q, knock, spool, boost, thr, volts, vcylL, volume, egt;
 volatile int running, ncyl, two, exhaust, diesel, forced, starter, recoil, bov, bang, cut, filter, valvetrain, nb, click, failed;
 float fire[16]; int bank[16]; float gain[16];
} AudioIn;
static AudioIn AU = {0};

static struct {
 double ang;
 float env[2], envF[2], clat;
 Bq r1[2], r2[2], lp[2], intake, hiss, bovf, rope, bangf, dcL, dcR;
 float turboPh, scPh, stPh, tick, ping, pingPh, bovEnv, bangEnv, ropeEnv, clickEnv, crackle, smoothRpm;
 int lastBov, lastBang, lastClick;
 unsigned rng;
 float hpL, hpR;
} SY;

static inline float Nz(void) { SY.rng = SY.rng * 1664525u + 1013904223u; return ((SY.rng >> 8) & 0xffff) / 32768.f - 1.f; }
static inline float Rnd01(void) { return (Nz() + 1) * 0.5f; }

static void AudioCB(void *buf, unsigned int frames) {
 float *out = (float *)buf;
 float rpmT = AU.rpm, load = AU.load, q = AU.q;
 int n = AU.ncyl < 1 ? 1 : AU.ncyl;
 float cycle = AU.two ? 360.f : 720.f;
 float sc = powf(0.5f / fmaxf(AU.vcylL, 0.04f), 0.2f);
 static const float EXF[4][7] = {
  {85, 1.2f, 240, 2.0f, 0.45f, 650, 0.55f},
  {105,1.5f, 360, 3.0f, 0.60f, 1700,0.80f},
  {125,1.1f, 500, 2.5f, 0.85f, 4200,1.00f},
  {190,1.6f, 850, 5.0f, 0.90f, 3400,0.85f},
 };
 const float *ex = EXF[AU.exhaust & 3];
 for (int bk = 0; bk < 2; bk++) {
  float bm = bk ? 1.07f : 1.0f;
  BqBP(&SY.r1[bk], ex[0] * sc * bm * (0.85f + 0.3f * Clampf(rpmT / 7000.f, 0, 1.5f)), ex[1]);
  float f2 = AU.exhaust == 3 ? 300 + rpmT * 0.09f : ex[2] * sc * bm;
  BqBP(&SY.r2[bk], f2, ex[3]);
  BqLP(&SY.lp[bk], ex[5] * (AU.diesel ? 1.3f : 1.f), 0.8f);
 }
 BqBP(&SY.intake, 300 + rpmT * 0.12f, 1.2f);
 BqBP(&SY.hiss, 3800, 1.5f);
 BqBP(&SY.bovf, 2300, 1.2f);
 BqBP(&SY.rope, 600 + 1800 * Clampf(AU.recoil ? 1 : 0, 0, 1), 2.0f);
 BqLP(&SY.bangf, 700, 0.9f);
 if (AU.bov != SY.lastBov) { SY.lastBov = AU.bov; SY.bovEnv = Clampf(AU.boost / 12.f, 0.3f, 1); }
 if (AU.bang != SY.lastBang) { SY.lastBang = AU.bang; SY.bangEnv = 1.0f; }
 if (AU.click != SY.lastClick) { SY.lastClick = AU.click; SY.clickEnv = 1.0f; }
 float vol = AU.volume * ex[6];
 float filtLoud = AU.filter == 0 ? 2.2f : (AU.filter == 3 ? 1.5f : 1.0f);
 for (unsigned i = 0; i < frames; i++) {
  SY.smoothRpm += (rpmT - SY.smoothRpm) * 0.0015f;
  float rpm = SY.smoothRpm;
  double prev = SY.ang;
  SY.ang += rpm * 6.0 / SR;
  int wrapped = 0;
  if (SY.ang >= cycle) { SY.ang -= cycle; wrapped = 1; }
  for (int k = 0; k < n; k++) {
   float fa = AU.fire[k];
   int hit = wrapped ? (fa > prev || fa <= SY.ang) : (fa > prev && fa <= SY.ang);
   if (!hit) continue;
   int bk = AU.bank[k] & 1;
   if (AU.running && !AU.cut && Rnd01() < q) {
    float a = (0.22f + 0.78f * Clampf(load, 0, 1.2f)) * AU.gain[k];
    SY.env[bk] += a; SY.envF[bk] += a * 0.8f;
    if (AU.diesel) SY.clat += 0.35f + 0.3f * load;
    if (AU.knock > 0.05f && Rnd01() < AU.knock * 0.6f) SY.ping = fminf(1, SY.ping + 0.6f);
    if (!AU.two) SY.tick += AU.valvetrain <= 1 ? 0.05f : 0.025f;
   } else if (rpm > 20) {
    SY.env[bk] += AU.running ? 0.03f : 0.10f;
    if (AU.running && AU.exhaust >= 1 && AU.thr < 0.05f && rpm > 2500 && Rnd01() < (AU.exhaust == 2 ? 0.08f : 0.025f)) SY.crackle = 0.9f;
   }
  }
  float tau = 0.0035f + 0.006f * (1 - Clampf(rpm / 7000.f, 0, 1));
  float dk = expf(-1.f / (SR * tau)), dkF = 0.9965f;
  float o[2];
  for (int bk = 0; bk < 2; bk++) {
   float nz = Nz();
   float src = SY.env[bk] * (0.6f + 0.4f * nz) + SY.envF[bk] * Nz() * 0.7f;
   float y = BqRun(&SY.r1[bk], src) * 1.6f + BqRun(&SY.r2[bk], src) * ex[4] + src * 0.12f;
   o[bk] = BqRun(&SY.lp[bk], y);
   SY.env[bk] *= dk; SY.envF[bk] *= dkF;
  }
  float common = 0;
  if (SY.crackle > 0.01f) { common += SY.crackle * Nz() * 0.8f; SY.crackle *= 0.996f; }
  common += BqRun(&SY.intake, Nz()) * 0.06f * AU.thr * Clampf(rpm / 6000.f, 0, 1.5f) * filtLoud * (AU.running ? 1 : 0);
  if (AU.diesel) { common += SY.clat * Nz() * 0.25f; SY.clat *= 0.985f; }
  if (SY.tick > 0.001f) { common += SY.tick * Nz() * 0.6f; SY.tick *= 0.97f; }
  if (SY.ping > 0.001f) { SY.pingPh += 2 * PI * 6300.f / SR; common += sinf(SY.pingPh) * SY.ping * 0.18f; SY.ping *= 0.9993f; }
  if (AU.forced && AU.forced != 4) {
   float sp = AU.spool;
   SY.turboPh += 2 * PI * (1800.f + 7200.f * sp) / SR;
   common += sinf(SY.turboPh) * 0.035f * sp * sp * (0.3f + 0.7f * AU.thr);
   common += BqRun(&SY.hiss, Nz()) * 0.05f * sp * AU.thr;
  }
  if (AU.forced == 4) {
   SY.scPh += 2 * PI * (rpm / 60.f * 11.f) / SR;
   float w = sinf(SY.scPh) + 0.4f * sinf(SY.scPh * 2.f);
   common += w * 0.035f * Clampf(rpm / 6000.f, 0, 1.3f) * (0.4f + 0.6f * AU.thr);
  }
  if (SY.bovEnv > 0.001f) { common += BqRun(&SY.bovf, Nz()) * SY.bovEnv * 0.5f; SY.bovEnv *= 0.99985f; }
  if (AU.starter) {
   float fm = 110.f + rpm * 1.3f;
   SY.stPh += 2 * PI * fm / SR;
   float s = sinf(SY.stPh); s = s + 0.5f * (s > 0 ? 1 : -1) * 0.5f;
   common += (s * 0.10f + Nz() * 0.03f) * Clampf((AU.volts - 8.f) / 4.f, 0.1f, 1);
  }
  if (AU.recoil) { SY.ropeEnv = fminf(1, SY.ropeEnv + 0.002f); } else SY.ropeEnv *= 0.999f;
  if (SY.ropeEnv > 0.001f) common += BqRun(&SY.rope, Nz()) * SY.ropeEnv * 0.25f;
  if (SY.clickEnv > 0.001f) { common += Nz() * SY.clickEnv * 0.5f; SY.clickEnv *= 0.992f; }
  if (SY.bangEnv > 0.001f) { common += BqRun(&SY.bangf, Nz()) * SY.bangEnv * 2.5f + Nz() * SY.bangEnv * SY.bangEnv * 0.4f; SY.bangEnv *= 0.99992f; }
  float L, R;
  if (AU.nb > 1) { L = o[0] * 0.7f + o[1] * 0.3f; R = o[0] * 0.3f + o[1] * 0.7f; }
  else { L = R = o[0]; }
  L = (L + common) * vol; R = (R + common) * vol;
  float hl = L - SY.hpL * 0.0f; (void)hl;
  SY.dcL.z1 = SY.dcL.z1 * 0.9995f + L * 0.0005f; L -= SY.dcL.z1;
  SY.dcR.z1 = SY.dcR.z1 * 0.9995f + R * 0.0005f; R -= SY.dcR.z1;
  out[i * 2] = tanhf(L * 1.3f) * 0.8f;
  out[i * 2 + 1] = tanhf(R * 1.3f) * 0.8f;
 }
}

static void AudioSetupEngine(const Build *b, const Spec *s) {
 int n = b->cyl;
 float cyc = b->cycle ? 360.f : 720.f;
 int nb = b->layout >= 2 ? 2 : 1;
 AU.ncyl = 0;
 for (int i = 0; i < n; i++) { AU.fire[i] = cyc * i / n; AU.bank[i] = nb > 1 ? (i & 1) : 0; }
 if (nb > 1 && n == 8 && !b->cycle) { int pat[8] = {0,1,1,0,1,0,0,1}; for (int i = 0; i < 8; i++) AU.bank[i] = pat[i]; }
 if (n == 2 && b->layout >= 2 && b->layout <= 3 && !b->cycle) AU.fire[1] = 360.f - 2 * LAYOUT_HALF[b->layout];
 if (n == 2 && b->layout >= 2 && b->layout <= 3 && b->cycle) AU.fire[1] = 180.f;
 unsigned r = 777;
 for (int i = 0; i < n; i++) { r = r * 1103515245u + 12345u; AU.gain[i] = 0.85f + 0.3f * ((r >> 16) & 1023) / 1023.f; }
 AU.two = b->cycle; AU.exhaust = b->exhaust; AU.diesel = FUELS[b->fuel].diesel; AU.forced = b->forced;
 AU.filter = b->filter; AU.valvetrain = b->cycle ? 9 : b->valvetrain; AU.nb = nb; AU.vcylL = s->vcylL;
 AU.ncyl = n;
}
