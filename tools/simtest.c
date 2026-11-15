#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sim.c"
int main(int argc, char **argv) {
 int pi = argc > 1 ? atoi(argv[1]) : 4;
 Build b = PRESETS[pi]; Spec s; ComputeSpec(&b, &s); Sim m; memset(&m, 0, sizeof m); SimReset(&m, &s, 0);
 m.ign = 1; m.choke = b.delivery == 0;
 printf("%s stall %.0f drag %.0f inertia %.3f idle %.0f redline %.0f est %.0fhp imep %.1f/%.1f\n", b.name, s.stall, s.compDrag, s.inertia, s.idle, s.redline, s.estHp, s.estImep, s.pLimit);
 for (int ms = 0; ms < 90000; ms++) {
  int crank = ms < 2500 && !m.running;
  if (!STARTERS[b.starter].elec && ms % 700 == 5 && !m.running) { m.recoilT = 0.35f; crank = 0; }
  if (m.choke && m.temp > 45) m.choke = 0;
  SimStep(&m, &b, &s, 0.001f, STARTERS[b.starter].elec ? crank : 0);
  if (ms % 5000 == 0) printf("t=%.1f rpm=%.0f q=%.2f run=%d tind=%.0f idleOpen=%.3f temp=%.0f V=%.1f starterOn=%d dead=%d %s\n", ms / 1000.f, m.rpm, m.q, m.running, m.tind, m.idleOpen, m.temp, m.volts, m.starterOn, m.starterDead, m.partMsg);
 }
}
