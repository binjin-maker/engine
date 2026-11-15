#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sim.c"
int main(void) {
 for (int p = 0; p < NPRESET; p++) {
  Build b = PRESETS[p]; Spec s; ComputeSpec(&b, &s);
  char w[16][96]; int sev[16]; int nw = BuildWarnings(&b, &s, w, sev, 16);
  printf("== %s  %.0fhp red %.0f\n", b.name, s.estHp, s.redline);
  for (int i = 0; i < nw; i++) printf("   W%d %s\n", sev[i], w[i]);
  Sim m; memset(&m, 0, sizeof m); SimReset(&m, &s, 0); m.ign = 1; m.choke = b.delivery == 0; m.temp = 85; m.oilT = 90;
  int started = 0;
  for (int ms = 0; ms < 60000; ms++) {
   int crank = !m.running && ms < 4000;
   if (!STARTERS[b.starter].elec && ms % 700 == 5 && !m.running) m.recoilT = 0.35f;
   if (ms == 3000 && m.running) { started = 1; m.dyno = 1; m.dynoTarget = m.dynoStart = fmaxf(m.rpm, s.idle + 300); m.dynoEnd = s.govRpm > 0 ? s.govRpm * 0.97f : s.redline * 0.99f; }
   if (m.dyno == 2) { m.dyno = 0; m.thrCmd = 1; }
   if (ms > 20000) m.thrCmd = 1;
   if (m.temp > 45) m.choke = 0;
   SimStep(&m, &b, &s, 0.001f, STARTERS[b.starter].elec ? crank : 0);
  }
  printf("   started=%d health %.1f peak %.0fhp temp %.0f oilT %.0f oilP %.0f batt %.2f failed=%d ", started, m.health, m.dynoPeakHp, m.temp, m.oilT, m.oilP, m.batt, m.failed);
  for (int i = 0; i < DMG_N; i++) if (m.dmg[i] > 0.05f) printf("[%s %.1f]", DMG_TITLE[i], m.dmg[i]);
  printf("\n");
 }
}
