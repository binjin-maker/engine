// ===== engine shop career: build, stock, list, advertise, sell =====
#define NMAT 7
#define NSEG 10
#define NSITE 5
#define NAD 5
#define NEQ 8
#define NITEM 24
#define NJOB 4
#define NOFFER 12
#define NCON 6
#define NLOG 60

static const char *MAT_NAME[NMAT] = {"Cast iron", "Aluminum ingot", "CGI iron", "Magnesium", "Billet aluminum", "Steel bar", "Titanium"};
static const float MAT_BASE[NMAT] = {1.4f, 3.2f, 2.6f, 9.0f, 7.5f, 2.3f, 38.f};

typedef struct { const char *name, *desc; float cost; } EquipDef;
static const EquipDef EQUIP[NEQ] = {
 {"Fabrication bench", "Lathe, mill, welder and hand tools. Builds engines one at a time.", 3500},
 {"Engine dyno", "Every engine leaves with a dyno sheet. Buyers trust proven numbers and pay ~6% more.", 2500},
 {"CNC machining center", "Machines twice as fast, wastes less stock. Needed for billet blocks and billet internals.", 14000},
 {"Foundry", "Pour your own castings: cast blocks cost far less. Needed for CGI and magnesium blocks.", 22000},
 {"Second build bay", "Run two builds at once.", 9000},
 {"Production line", "Batch-build copies of a design, 5 engines a day, parts a bit cheaper.", 75000},
 {"Warehouse", "Store up to 24 finished engines instead of 4.", 6000},
 {"Paint & finish booth", "Powder coat and detail every engine. Buyers pay about 8% more.", 4000},
};

typedef struct { const char *name, *desc; float traffic, pay, fee, listFee, monthly, setup, rep, offerK, scam; float mix[NSEG]; } SiteDef;
//                         lawn kart moto car  race truck marine ind  air  odd
static const SiteDef SITES[NSITE] = {
 {"LocalList", "Free local classifieds. No fees, lots of lowballers (and the odd scammer).", 14, 0.85f, 0, 0, 0, 0, 0, 1.4f, 0.05f, {2, 2, 1, 1, 0.4f, 0.8f, 0.5f, 0.6f, 0.1f, 0.6f}},
 {"SwapBay", "Huge nationwide auction site. $6 per listing each week + 10% final value fee.", 22, 1.0f, 0.10f, 6, 0, 0, 0, 1.0f, 0.01f, {1, 1, 1, 1, 1, 1, 1, 1, 0.5f, 1}},
 {"TorqueTrader", "Enthusiast marketplace. Performance buyers who pay up. $79/month + 6%. Needs reputation 15.", 14, 1.12f, 0.06f, 0, 79, 0, 15, 0.8f, 0, {0.1f, 0.4f, 2, 2, 3, 0.6f, 0.6f, 0.1f, 0.8f, 1.2f}},
 {"Your website", "Your own storefront. $1,200 to set up, $35/month, 3% card fees. Traffic comes from ads and reputation.", 2, 1.06f, 0.03f, 0, 35, 1200, 0, 0.7f, 0, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
 {"IndustrialSource", "B2B marketplace for generators, marine, trucks and aircraft. $150/month + 4%. Needs reputation 25.", 10, 1.08f, 0.04f, 0, 150, 0, 25, 0.6f, 0, {1, 0.1f, 0.1f, 0.2f, 0.1f, 2, 2, 3, 1.2f, 0.2f}},
};

typedef struct { const char *name, *desc; float perDay; } AdDef;
static const AdDef ADS[NAD] = {
 {"Flyers & corkboards", "Hardware stores and feed shops. More LocalList visitors, lawn and kart buyers.", 15},
 {"Social media", "A bit more traffic everywhere. Kart, bike and car buyers. Some website visits.", 45},
 {"Search ads", "Sends buyers straight to your website, plus some SwapBay traffic.", 70},
 {"Racing magazines", "Race, car and bike buyers on every site. Slowly builds reputation.", 140},
 {"Trade journals", "Industrial, marine, truck and aircraft buyers. Boosts IndustrialSource.", 110},
};

static const char *SEG_NAME[NSEG] = {"Lawn & garden", "Go-kart / minibike", "Motorcycle", "Street car", "Racing", "Truck / towing", "Marine", "Industrial / generator", "Aircraft", "Collector"};
static const float SEG_MULT[NSEG] = {0.9f, 1.0f, 1.25f, 1.0f, 1.6f, 1.1f, 1.3f, 1.0f, 2.0f, 1.0f};
static const float SEG_BUDGET[NSEG] = {700, 1800, 14000, 30000, 140000, 26000, 45000, 32000, 70000, 22000};
static const float SEG_W[NSEG] = {18, 16, 12, 18, 6, 9, 5, 9, 3, 4};

typedef struct {
 int used; Build b; char name[32];
 float hp, lbft, kg, dispL, redline, vib, hpL, pw, costBasis, price;
 int gov, diesel, liquid, cyl, risk, dyno, finish, exotic, nitro;
 int site, warranty, days, views; float fit[NSEG];
} Item;
typedef struct { int used, qty, batch; Build b; float hoursLeft, hoursTotal, costEach; char name[32]; } Job;
typedef struct { int used, item, scam, days; float amount; char who[24]; } Offer;
typedef struct { int used, seg, qty, done, needGov, needDiesel, deadline; float minHp, maxKg, pay; char client[32]; } Contract;
typedef struct { int failDay; float refund; int warranty; char name[32]; } Sold;
typedef struct { int day; float amt; char s[88]; } LogLine;

typedef struct { const char *co, *name; int seg; float hp, q; int diesel; } CompDef;
#define NCOMP 22
static const CompDef COMPS[NCOMP] = {
 {"Bramble Power", "B-140 mower engine", 0, 4, 0.95f, 0}, {"Yardking", "YK 6.5 OHV", 0, 6.5f, 1.02f, 0},
 {"Bramble Power", "Kart Champ 212", 1, 7, 0.95f, 0}, {"Torx Small Engines", "Hustler 420", 1, 13, 1.0f, 0},
 {"Rivet Moto", "R250 two-stroke", 2, 45, 1.0f, 0}, {"Kestrel", "K650 twin", 2, 70, 1.05f, 0}, {"Kestrel", "K1000 four", 2, 160, 1.1f, 0},
 {"Apex Engines", "LX 2.0T", 3, 250, 1.05f, 0}, {"Ironside", "Ironside 350 crate", 3, 330, 1.0f, 0}, {"Apex Engines", "V8 6.2 crate", 3, 450, 1.08f, 0},
 {"Apex Engines", "R-Spec 2.0", 4, 350, 1.12f, 0}, {"Velocity Race", "427 race V8", 4, 650, 1.1f, 0}, {"Velocity Race", "Sprint 1000", 4, 1000, 1.05f, 0},
 {"Ironclad Diesel", "IC 5.9", 5, 300, 1.0f, 1}, {"Ironclad Diesel", "IC 6.7", 5, 380, 1.05f, 1},
 {"Tidewater", "TW 5.7 marine", 6, 300, 1.05f, 0}, {"Tidewater", "TW 8.1 marine", 6, 420, 1.05f, 0},
 {"GenCo", "G-20 genset", 7, 20, 1.0f, 0}, {"Ironclad Diesel", "IC 4.5 power unit", 7, 120, 1.05f, 1},
 {"Aerolite", "AL-100", 8, 100, 1.1f, 0}, {"Aerolite", "AL-180", 8, 180, 1.1f, 0},
 {"Odd Iron Works", "Replica radial", 9, 120, 0.95f, 0},
};
typedef struct { float price, base, q, press, sales30, lastLogged; } CompState;

typedef struct {
 int magic, ver, started, over;
 float money, rep, loan; int day, lastShow;
 int equip[NEQ], siteOn[NSITE], ad[NAD];
 float stock[NMAT], stockCost[NMAT], price[NMAT];
 int bulkDays[NMAT]; float bulkKg[NMAT], bulkCost[NMAT];
 Item inv[NITEM]; Job job[NJOB]; Offer off[NOFFER]; Contract con[NCON]; Sold sold[16];
 LogLine log[NLOG]; int nlog;
 float income30, spend30, cash[60];
 int sales, warrantyClaims; unsigned rng;
 float pendUtil, pendAds;
 int compInit; CompState comp[NCOMP]; float mySeg30[NSEG], compSeg30[NSEG]; int lost[NITEM];
} Career;
static Career C;
static int gShop = 0, gShopTab = 0, gInvSel = -1;

static float CR01(void) { C.rng = C.rng * 1664525u + 1013904223u; return ((C.rng >> 8) & 0xffffff) / 16777216.f; }
static int CPois(float lam) { if (lam <= 0) return 0; int k = 0; float L = expf(-fminf(lam, 60)), p = 1; do { k++; p *= CR01(); } while (p > L && k < 200); return k - 1; }
static const char *CareerPath(void) { return "engine_garage_shop.dat"; }
static int gNoCareerSave = 0;
static void CareerSave(void) { if (!gAutotest && !gNoCareerSave) SaveFileData(CareerPath(), &C, sizeof C); }
static void CLog(float amt, const char *fmt, ...) {
 if (C.nlog >= NLOG) { memmove(C.log, C.log + 1, sizeof(LogLine) * (NLOG - 1)); C.nlog = NLOG - 1; }
 LogLine *l = &C.log[C.nlog++]; l->day = C.day; l->amt = amt;
 va_list ap; va_start(ap, fmt); vsnprintf(l->s, sizeof l->s, fmt, ap); va_end(ap);
}
static void Pay(float amt, const char *why) { C.money -= amt; C.spend30 += amt; if (amt >= 1 && why) CLog(-amt, "%s", why); }
static void Earn(float amt, const char *why) { C.money += amt; C.income30 += amt; CLog(amt, "%s", why); }

static int PriceField(Rectangle r, float *v);
static void CompInit(void);
static void CareerNew(void) {
 memset(&C, 0, sizeof C);
 C.magic = 0x53484F50; C.ver = 1; C.started = 1; C.money = 10000; C.rep = 5; C.day = 1; C.rng = (unsigned)time(NULL) | 1; C.lastShow = -99;
 for (int m = 0; m < NMAT; m++) C.price[m] = MAT_BASE[m];
 C.siteOn[0] = 1;
 CompInit();
 for (int i = 0; i < 60; i++) C.cash[i] = 10000;
 CLog(10000, "Started the shop with $10,000 in savings");
}
static void CareerLoad(void) {
 int sz = 0; unsigned char *d = LoadFileData(CareerPath(), &sz);
 if (d && sz <= (int)sizeof C && sz > 64 && ((Career *)d)->magic == 0x53484F50) { memset(&C, 0, sizeof C); memcpy(&C, d, sz); if (!C.compInit) CompInit(); } else CareerNew();
 if (d) UnloadFileData(d);
}

// ---------- engine appraisal ----------
static void ItemFromBuild(Item *it, const Build *b) {
 Spec s; ComputeSpec(b, &s);
 memset(it, 0, sizeof *it); it->used = 1; it->b = *b;
 BuildName(b, it->name, 32);
 it->hp = s.estHp; it->lbft = s.estTq * 0.7376f; it->kg = s.weight; it->dispL = s.dispL; it->redline = s.redline; it->vib = s.vib;
 it->hpL = s.estHp / fmaxf(s.dispL, 0.01f); it->pw = s.estHp / fmaxf(s.weight, 1);
 it->gov = s.govRpm > 0; it->diesel = FUELS[b->fuel].diesel; it->liquid = b->cooling > 0; it->cyl = NumCyl(b); it->nitro = b->fuel == 6;
 it->exotic = b->layout == 5 && b->ncust >= 3;
 char w[16][96]; int sev[16]; int nw = BuildWarnings(b, &s, w, sev, 16);
 for (int i = 0; i < nw; i++) { if (sev[i] == 2) it->risk += 2; else if (sev[i] == 1 && strstr(w[i], "ooling")) it->risk += 1; }
 it->dyno = C.equip[1]; it->finish = C.equip[7];
 float *f = it->fit, hp = it->hp, lb = it->lbft;
 f[0] = (hp < 15 && it->gov) ? 1 : ((hp < 15 && it->dispL < 0.5f) ? 0.5f : 0);
 f[1] = (hp >= 3 && hp < 25 && it->dispL < 0.5f) ? 1 : 0;
 f[2] = ((hp >= 20 && hp < 220 && it->kg < 90 && it->cyl <= 6) || (b->cycle && it->pw > 0.8f && it->kg < 40)) ? 1 : 0;
 f[3] = (hp >= 60 && hp < 750 && !it->diesel && !it->nitro && it->kg > 50) ? 1 : 0;
 f[4] = (!it->gov && !it->diesel && (it->pw > 2.0f || it->redline > 8500 || it->hpL > 105 || it->nitro)) ? 1 : 0;
 f[5] = (lb > 300 && it->redline < 5800) ? (it->diesel ? 1 : 0.75f) : 0;
 f[6] = (lb > 200 && it->liquid && it->redline < 6500) ? 1 : 0;
 f[7] = ((it->gov && hp > 8) || (it->diesel && lb > 250)) ? 1 : 0;
 f[8] = (b->cooling == 0 && (b->layout == 4 || b->layout == 1 || it->exotic) && hp > 60 && hp < 450 && it->pw > 0.6f && it->redline < 7000) ? 1 : 0;
 f[9] = it->exotic ? 1 : 0.3f;
 for (int k = 0; k < NSEG; k++) if (f[k] > 0 && it->vib > 0.6f && (k == 2 || k == 3 || k == 8)) f[k] *= 0.85f;
}
static float ItemQuality(const Item *it) {
 return (it->dyno ? 1.f : 0.94f) * (it->finish ? 1.08f : 1.f) * (it->warranty ? 1.1f : 1.f) * (0.93f + C.rep * 0.002f)
  * (it->vib > 0.6f ? 0.93f : 1.f) * (it->risk ? 0.88f : 1.f) * (it->b.internals >= 2 ? 1.05f : 1.f) * ((it->b.block == 2 || it->b.block == 4) ? 1.03f : 1.f);
}
static float ItemValue(const Item *it, int seg, int site) {
 if (it->fit[seg] <= 0) return 0;
 float v = (150 + 26 * it->hp) * SEG_MULT[seg] * it->fit[seg] * ((it->diesel && seg >= 5 && seg <= 7) ? 1.6f : 1.f);
 v *= ItemQuality(it);
 if (site >= 0) v *= SITES[site].pay;
 return fminf(v, SEG_BUDGET[seg]);
}
static float BestValue(const Item *it, int site, int *segOut) {
 float best = 0; int bs = 9;
 for (int k = 0; k < NSEG; k++) { float v = ItemValue(it, k, site); if (v > best) { best = v; bs = k; } }
 if (segOut) *segOut = bs;
 return best;
}
static float CompValue(int j) {
 const CompDef *d = &COMPS[j];
 float v = (150 + 26 * d->hp) * SEG_MULT[d->seg] * C.comp[j].q * ((d->diesel && d->seg >= 5 && d->seg <= 7) ? 1.6f : 1.f);
 return fminf(v, SEG_BUDGET[d->seg]);
}
static void CompInit(void) {
 for (int j = 0; j < NCOMP; j++) { C.comp[j].q = COMPS[j].q; float v = CompValue(j); C.comp[j].base = C.comp[j].price = roundf(v * 1.0f / 5) * 5; C.comp[j].lastLogged = C.comp[j].price; }
 C.compInit = 1;
}
static int CompFor(int seg, float hp) {
 int best = -1; float bd = 0.5f;
 for (int j = 0; j < NCOMP; j++) if (COMPS[j].seg == seg) { float d = fabsf(logf(fmaxf(hp, 0.5f) / COMPS[j].hp)); if (d < bd) { bd = d; best = j; } }
 return best;
}
static void Stars(float x, float y, float q) {
 int n = (int)roundf(Clampf((q - 0.7f) / 0.5f * 5, 1, 5));
 for (int k = 0; k < 5; k++) DrawCircleV((Vector2){x + k * 13, y + 7}, 4.5f, k < n ? COL_YEL : COL_LINE);
}
static int InvCap(void) { return C.equip[6] ? NITEM : 4; }
static int InvCount(void) { int n = 0; for (int i = 0; i < NITEM; i++) n += C.inv[i].used; return n; }

// ---------- building ----------
typedef struct { float matKg[NMAT], matCost, bought, consum, hours, days; int ok; char why[96]; } Quote;
static Quote ShopQuote(const Build *b, int batch) {
 Quote q; memset(&q, 0, sizeof q);
 Spec s; ComputeSpec(b, &s);
 float waste = C.equip[2] ? 0.8f : 1.f;
 for (int m = 0; m < NMAT; m++) {
  float kg = s.matKg[m] * (m == 4 ? waste : (C.equip[2] ? 0.93f : 1.f)) * 0.85f;
  q.matKg[m] = kg;
  float pr = C.price[m] * ((m <= 3 && !C.equip[3]) ? 1.2f : 1.f);
  q.matCost += kg * pr;
 }
 // priced like a 10k-unit run: parts at volume prices, labor at line rates
 q.bought = s.bought * (batch ? 0.22f : 0.25f);
 q.consum = 4 + 3 * NumCyl(b) + s.machHours * 0.12f * 40.f;
 q.hours = batch ? 8.f / 5.f : s.machHours * 3.f / (C.equip[2] ? 2.f : 1.f);
 q.days = q.hours / 8.f;
 q.ok = 1;
 char w[16][96]; int sev[16]; int nw = BuildWarnings(b, &s, w, sev, 16);
 for (int i = 0; i < nw; i++) if (sev[i] == 3) { q.ok = 0; snprintf(q.why, 96, "Design doesn't go together: %s", w[i]); }
 if (!C.equip[0]) { q.ok = 0; snprintf(q.why, 96, "Buy a fabrication bench first (WORKSHOP)."); }
 else if ((b->block == 4 || b->internals == 3) && !C.equip[2]) { q.ok = 0; snprintf(q.why, 96, "Billet parts need a CNC machining center."); }
 else if ((b->block == 2 || b->block == 3) && !C.equip[3]) { q.ok = 0; snprintf(q.why, 96, "CGI and magnesium blocks need your own foundry."); }
 else if (batch && !C.equip[5]) { q.ok = 0; snprintf(q.why, 96, "Batch builds need a production line."); }
 return q;
}
static float QuoteTotal(const Quote *q, int qty) {
 float mat = 0;
 for (int m = 0; m < NMAT; m++) {
  float need = q->matKg[m] * qty, have = fminf(C.stock[m], need);
  float pr = C.price[m] * ((m <= 3 && !C.equip[3]) ? 1.2f : 1.f);
  mat += have * (C.stock[m] > 0 ? C.stockCost[m] / C.stock[m] : pr) + (need - have) * pr * 1.25f;
 }
 return mat + (q->bought + q->consum) * qty;
}
static int JobSlots(void) { return C.equip[4] ? 2 : 1; }
static int StartBuild(const Build *b, int qty) {
 int batch = qty > 1;
 Quote q = ShopQuote(b, batch);
 if (!q.ok) { ToastAdd(q.why, COL_RED); return 0; }
 int busy = 0; for (int j = 0; j < NJOB; j++) busy += C.job[j].used && !C.job[j].batch;
 int slot = -1; for (int j = 0; j < NJOB; j++) if (!C.job[j].used) { slot = j; break; }
 if (slot < 0 || (!batch && busy >= JobSlots())) { ToastAdd("All build bays are busy.", COL_YEL); return 0; }
 float total = QuoteTotal(&q, qty);
 if (total > C.money) { ToastAdd(TextFormat("Need $%.0f, you have $%.0f.", total, C.money), COL_RED); return 0; }
 // consume stock, rush-buy the shortfall
 for (int m = 0; m < NMAT; m++) {
  float need = q.matKg[m] * qty, have = fminf(C.stock[m], need);
  if (have > 0) { float avg = C.stockCost[m] / C.stock[m]; C.stock[m] -= have; C.stockCost[m] -= avg * have; if (C.stock[m] < 0.01f) C.stock[m] = C.stockCost[m] = 0; }
 }
 Job *jb = &C.job[slot]; memset(jb, 0, sizeof *jb);
 jb->used = 1; jb->qty = qty; jb->batch = batch; jb->b = *b; jb->hoursTotal = jb->hoursLeft = batch ? fmaxf(8, qty * q.hours) : q.hours; jb->costEach = total / qty;
 BuildName(b, jb->name, 32);
 C.money -= total; C.spend30 += total;
 CLog(-total, "Started building %s%s", qty > 1 ? TextFormat("%d x ", qty) : "", jb->name);
 ToastAdd(TextFormat("Build started: %.1f days", jb->hoursTotal / 8.f), COL_GRN); SfxPush(SFX_RATCHET);
 CareerSave();
 return 1;
}

// ---------- contracts ----------
static const char *CLIENTS[NSEG][2] = {{"Greenway Mowers", "Hendricks Farm Supply"}, {"Valley Kart League", "Minibike Mike's"}, {"Coast Moto Works", "Ridgeline Customs"}, {"Westside Restomods", "Bayview Auto"}, {"Desert Drag Assoc.", "Ridgeline Motorsports"}, {"Ironhorse Trucking", "Big Rig Supply"}, {"Bayside Marine", "Harbor Boatworks"}, {"Pacific Gensets", "Delta Pump Co."}, {"Skylark Aero Club", "Coastal Bush Planes"}, {"Odd Iron Museum", "Garage Legends TV"}};
static void NewContract(void) {
 int k = -1; for (int i = 0; i < NCON; i++) if (!C.con[i].used) { k = i; break; }
 if (k < 0) return;
 static const int segs[] = {0, 1, 1, 2, 3, 4, 5, 6, 7, 8};
 int seg = segs[(int)(CR01() * 10) % 10];
 static const float lo[NSEG] = {3, 5, 30, 120, 250, 250, 150, 10, 90, 50}, hi[NSEG] = {9, 14, 120, 400, 800, 450, 400, 60, 220, 300};
 Contract *c = &C.con[k]; memset(c, 0, sizeof *c);
 c->used = 1; c->seg = seg; c->minHp = roundf((lo[seg] + (hi[seg] - lo[seg]) * CR01()) / 5) * 5; if (c->minHp < 3) c->minHp = 3;
 c->maxKg = seg <= 2 ? roundf(15 + c->minHp * 1.4f + CR01() * 20) : (seg == 8 ? roundf(c->minHp * 1.1f + 60) : 0);
 c->needGov = seg == 0 || seg == 7; c->needDiesel = seg == 5 && CR01() < 0.5f;
 c->qty = seg >= 3 && seg != 7 ? 1 + (int)(CR01() * 2) : 2 + (int)(CR01() * 4);
 c->pay = roundf((150 + 26 * c->minHp) * SEG_MULT[seg] * (1.15f + 0.25f * CR01()) / 10) * 10;
 c->deadline = C.day + 10 + (int)(CR01() * 20);
 snprintf(c->client, 32, "%s", CLIENTS[seg][CR01() < 0.5f]);
}
static int ItemMeets(const Item *it, const Contract *c) {
 if (it->fit[c->seg] <= 0 || it->hp < c->minHp) return 0;
 if (c->maxKg > 0 && it->kg > c->maxKg) return 0;
 if (c->needGov && !it->gov) return 0;
 if (c->needDiesel && !it->diesel) return 0;
 return 1;
}

// ---------- the day tick ----------
static void FinishItem(const Build *b, float cost) {
 for (int i = 0; i < NITEM; i++) if (!C.inv[i].used) {
  if (InvCount() >= InvCap()) return;
  ItemFromBuild(&C.inv[i], b); C.inv[i].costBasis = cost; C.inv[i].site = -1; C.lost[i] = 0;
  C.inv[i].price = roundf(BestValue(&C.inv[i], 0, NULL) / 25) * 25;
  return;
 }
}
static void SellItem(int i, float price, int site, const char *buyer) {
 Item *it = &C.inv[i];
 { int bs; BestValue(it, -1, &bs); C.mySeg30[bs] += 1;
   for (int j = 0; j < NCOMP; j++) if (it->fit[COMPS[j].seg] > 0 && fabsf(logf(fmaxf(it->hp, 0.5f) / COMPS[j].hp)) < 0.5f) C.comp[j].press += 1; }
 float fee = site >= 0 ? price * SITES[site].fee : 0;
 Earn(price - fee, TextFormat("Sold %s%s for $%.0f%s", it->name, site >= 0 ? TextFormat(" on %s", SITES[site].name) : "", price, fee > 0 ? TextFormat(" (fee $%.0f)", fee) : ""));
 ToastAdd(TextFormat("SOLD: %s to %s, $%.0f", it->name, buyer, price - fee), COL_GRN); SfxPush(SFX_DONE);
 C.sales++;
 // a weak design comes back
 float pf = it->risk * 0.22f;
 for (int k = 0; k < 16; k++) if (!C.sold[k].failDay) {
  if (CR01() < pf) { C.sold[k].failDay = C.day + 2 + (int)(CR01() * 25); C.sold[k].refund = price; C.sold[k].warranty = it->warranty; snprintf(C.sold[k].name, 32, "%s", it->name); }
  break;
 }
 if (!it->risk) C.rep = fminf(100, C.rep + Clampf(price / 2500.f, 0.15f, 1.5f));
 for (int o = 0; o < NOFFER; o++) if (C.off[o].used && C.off[o].item == i) C.off[o].used = 0;
 it->used = 0;
}
static const char *BuyerName(void) {
 static const char *F[] = {"Dale", "Marisol", "Trev", "Kenji", "Big Mike", "Priya", "Hank", "Sofia", "Duane", "Lucy", "Rafael", "Bo"};
 return F[(int)(CR01() * 12) % 12];
}
static void ShopDay(void) {
 C.day++;
 // material prices drift; the odd shortage
 for (int m = 0; m < NMAT; m++) {
  C.price[m] *= 1 + (CR01() - 0.5f) * 0.05f + (MAT_BASE[m] - C.price[m]) / MAT_BASE[m] * 0.06f;
  if (CR01() < 0.004f) { C.price[m] *= 1.4f; CLog(0, "%s shortage! Prices jumped.", MAT_NAME[m]); }
  if (C.bulkDays[m] > 0 && --C.bulkDays[m] == 0) { C.stock[m] += C.bulkKg[m]; C.stockCost[m] += C.bulkCost[m]; CLog(0, "Bulk %s delivery arrived (%.0f kg)", MAT_NAME[m], C.bulkKg[m]); C.bulkKg[m] = C.bulkCost[m] = 0; }
 }
 // shop floor
 int singles = 0;
 for (int j = 0; j < NJOB; j++) {
  Job *jb = &C.job[j]; if (!jb->used) continue;
  if (!jb->batch && singles++ >= JobSlots()) continue;
  jb->hoursLeft -= 8.f;
  if (jb->hoursLeft <= 0) {
   int room = InvCap() - InvCount();
   if (room <= 0) { jb->hoursLeft = 0.01f; continue; }
   int n = jb->qty < room ? jb->qty : room;
   for (int k = 0; k < n; k++) FinishItem(&jb->b, jb->costEach);
   jb->qty -= n;
   CLog(0, "Finished %d x %s", n, jb->name);
   ToastAdd(TextFormat("Built: %s%s", jb->name, n > 1 ? TextFormat(" x%d", n) : ""), COL_GRN);
   if (jb->qty <= 0) jb->used = 0; else jb->hoursLeft = 0.01f;
  }
 }
 // bills
 float util = 3; for (int e = 0; e < NEQ; e++) util += C.equip[e] * (e == 5 ? 50 : (e == 3 ? 20 : 4));
 Pay(util, NULL); C.pendUtil += util;
 float adSpend = 0; for (int a = 0; a < NAD; a++) adSpend += C.ad[a] * ADS[a].perDay;
 if (adSpend > 0) { Pay(adSpend, NULL); C.pendAds += adSpend; }
 C.rep = fminf(100, C.rep + C.ad[3] * 0.06f + C.ad[4] * 0.03f);
 if (C.day % 30 == 0) {
  Pay(500, "Shop rent");
  for (int si = 0; si < NSITE; si++) if (C.siteOn[si] && SITES[si].monthly > 0) Pay(SITES[si].monthly, TextFormat("%s subscription", SITES[si].name));
 }
 if (C.day % 7 == 0) {
  if (C.pendUtil > 0) CLog(-C.pendUtil, "Utilities, last 7 days"); if (C.pendAds > 0) CLog(-C.pendAds, "Advertising, last 7 days"); C.pendUtil = C.pendAds = 0;
  int nl = 0; for (int i = 0; i < NITEM; i++) if (C.inv[i].used && C.inv[i].site == 1) nl++;
  if (nl) Pay(nl * SITES[1].listFee, "SwapBay listing fees");
  if (C.loan > 0) Pay(C.loan * 0.015f, "Loan interest");
  if (C.rep >= 10 && CR01() < 0.6f) { NewContract(); ToastAdd("New contract request", COL_ACC); }
 }
 // buyers
 for (int i = 0; i < NITEM; i++) {
  Item *it = &C.inv[i];
  if (!it->used || it->site < 0) continue;
  int si = it->site; const SiteDef *sd = &SITES[si];
  it->days++;
  int nOn = 0; for (int k = 0; k < NITEM; k++) nOn += C.inv[k].used && C.inv[k].site == si;
  float traffic = sd->traffic;
  if (si == 0) traffic *= 1 + 0.6f * C.ad[0];
  traffic *= 1 + 0.2f * C.ad[1];
  if (si == 1) traffic *= 1 + 0.15f * C.ad[2];
  if (si == 3) traffic += 2 * C.ad[1] + 6 * C.ad[2] + C.rep * 0.15f;
  if (si == 4) traffic *= 1 + 0.4f * C.ad[4];
  traffic *= (0.7f + C.rep * 0.006f) / sqrtf((float)nOn);
  float mix[NSEG], tot = 0;
  for (int k = 0; k < NSEG; k++) {
   float m = sd->mix[k] * SEG_W[k];
   if (k <= 1) m *= 1 + 0.5f * C.ad[0] + 0.3f * C.ad[1];
   if (k >= 1 && k <= 3) m *= 1 + 0.3f * C.ad[1];
   if (k >= 2 && k <= 4) m *= 1 + 0.6f * C.ad[3];
   if (k >= 5 && k <= 8) m *= 1 + 0.6f * C.ad[4];
   mix[k] = m; tot += m;
  }
  int vis = CPois(traffic);
  it->views += vis;
  for (int v = 0; v < vis && it->used; v++) {
   float r = CR01() * tot; int seg = 0; while (seg < NSEG - 1 && r > mix[seg]) { r -= mix[seg]; seg++; }
   float val = ItemValue(it, seg, si) * (0.82f + 0.36f * CR01());
   if (val <= 0) continue;
   int cj = CompFor(seg, it->hp);
   if (cj >= 0) {
    float valC = CompValue(cj) * SITES[si].pay * (0.82f + 0.36f * CR01()), sC = valC - C.comp[cj].price, sY = val - it->price;
    int toComp = 0;
    if (sC >= 0 && sY < 0) toComp = CR01() < 0.85f;
    else if (sC >= 0 && sY >= 0) toComp = CR01() > 1.f / (1.f + expf((sC - sY) / (0.06f * it->price)));
    if (toComp) { C.comp[cj].sales30 += 1; C.compSeg30[seg] += 1; C.lost[i]++; continue; }
   }
   if (val >= it->price) { SellItem(i, it->price, si, BuyerName()); break; }
   if (val >= it->price * (si == 0 ? 0.55f : 0.72f) && CR01() < 0.35f * sd->offerK) {
    for (int o = 0; o < NOFFER; o++) if (!C.off[o].used) {
     Offer *of = &C.off[o]; of->used = 1; of->item = i; of->days = 3; of->scam = 0;
     of->amount = roundf(val * (si == 0 ? 0.8f + 0.15f * CR01() : 0.95f) / 25) * 25;
     snprintf(of->who, 24, "%s", BuyerName());
     ToastAdd(TextFormat("Offer: $%.0f for %s", of->amount, it->name), COL_ACC);
     break;
    }
   }
  }
  if (it->used && sd->scam > 0 && CR01() < sd->scam) {
   for (int o = 0; o < NOFFER; o++) if (!C.off[o].used) {
    Offer *of = &C.off[o]; of->used = 1; of->item = i; of->days = 2; of->scam = 1;
    of->amount = roundf(it->price * (1.15f + 0.3f * CR01()) / 25) * 25; snprintf(of->who, 24, "Overseas buyer");
    ToastAdd(TextFormat("Offer: $%.0f for %s", of->amount, it->name), COL_ACC);
    break;
   }
  }
 }
 for (int o = 0; o < NOFFER; o++) if (C.off[o].used && (--C.off[o].days <= 0 || !C.inv[C.off[o].item].used)) C.off[o].used = 0;
 // competitors: undercut when you take their buyers, drift back when you don't
 for (int j = 0; j < NCOMP; j++) {
  CompState *cs = &C.comp[j];
  float floorP = cs->base * 0.8f;
  if (cs->press > 0.4f) cs->price = fmaxf(floorP, cs->price * (1 - 0.006f * fminf(cs->press, 2)));
  else cs->price += (cs->base - cs->price) * 0.03f;
  if (cs->press > 1.5f && CR01() < 0.04f && cs->q < 1.25f) { cs->q += 0.03f; CLog(0, "%s upgraded its %s (better quality)", COMPS[j].co, COMPS[j].name); }
  cs->press *= 0.93f;
  float bg = CPois(0.4f * SEG_W[COMPS[j].seg] / 10.f);
  cs->sales30 += bg; C.compSeg30[COMPS[j].seg] += bg;
  if (cs->price < cs->lastLogged * 0.94f) { CLog(0, "%s cut the %s to $%.0f", COMPS[j].co, COMPS[j].name, cs->price); ToastAdd(TextFormat("%s cut prices on the %s", COMPS[j].co, COMPS[j].name), COL_YEL); cs->lastLogged = cs->price; }
  else if (cs->price > cs->lastLogged * 1.06f) cs->lastLogged = cs->price;
  cs->sales30 *= 0.967f;
 }
 for (int k = 0; k < NSEG; k++) { C.mySeg30[k] *= 0.967f; C.compSeg30[k] *= 0.967f; }
 // warranty failures
 for (int k = 0; k < 16; k++) if (C.sold[k].failDay && C.sold[k].failDay <= C.day) {
  if (C.sold[k].warranty) { Pay(C.sold[k].refund, TextFormat("Warranty refund: %s blew up", C.sold[k].name)); C.rep = fmaxf(0, C.rep - 6); }
  else { C.rep = fmaxf(0, C.rep - 9); CLog(0, "A sold %s failed. Bad reviews (sold as-is).", C.sold[k].name); }
  ToastAdd(TextFormat("A %s you sold failed. Reputation hit.", C.sold[k].name), COL_RED);
  C.warrantyClaims++; memset(&C.sold[k], 0, sizeof C.sold[k]);
 }
 for (int k = 0; k < NCON; k++) if (C.con[k].used && C.day > C.con[k].deadline) {
  if (C.con[k].done > 0) C.rep = fmaxf(0, C.rep - 3);
  C.rep = fmaxf(0, C.rep - (C.con[k].done ? 0 : 1));
  CLog(0, "Contract with %s expired (%d/%d delivered)", C.con[k].client, C.con[k].done, C.con[k].qty);
  C.con[k].used = 0;
 }
 memmove(C.cash, C.cash + 1, sizeof(float) * 59); C.cash[59] = C.money;
 if (C.day % 30 == 0) { C.income30 = C.spend30 = 0; }
 if (C.money < -15000 && C.loan >= 0) { C.over = 1; }
 CareerSave();
}

// ---------- UI ----------
static float ShopRow(float x, float y, float w, const char *k, const char *v, Color c) { Tx(k, x, y, 15, COL_DIM); TxR(v, x + w, y, 15, c, 1); return y + 21; }
static void Money(float x, float y, float sz, float v) { Tb(TextFormat("%s$%.0f", v < 0 ? "-" : "", fabsf(v)), x, y, sz, v < 0 ? COL_RED : COL_GRN); }

static void TabDashboard(float x, float y, float w, float h) {
 float cw = (w - 20) * 0.5f;
 Tb("THE SHOP", x, y, 18, COL_DIM); float yy = y + 30;
 yy = ShopRow(x, yy, cw, "Day", TextFormat("%d", C.day), COL_TXT);
 yy = ShopRow(x, yy, cw, "Cash", TextFormat("$%.0f", C.money), C.money < 0 ? COL_RED : COL_GRN);
 yy = ShopRow(x, yy, cw, "Loan", TextFormat("$%.0f", C.loan), COL_TXT);
 yy = ShopRow(x, yy, cw, "Reputation", TextFormat("%.0f / 100", C.rep), COL_TXT);
 yy = ShopRow(x, yy, cw, "Engines sold", TextFormat("%d", C.sales), COL_TXT);
 yy = ShopRow(x, yy, cw, "Failures in the field", TextFormat("%d", C.warrantyClaims), C.warrantyClaims ? COL_RED : COL_TXT);
 yy = ShopRow(x, yy, cw, "This month in / out", TextFormat("$%.0f / $%.0f", C.income30, C.spend30), COL_TXT);
 yy = ShopRow(x, yy, cw, "In stock", TextFormat("%d / %d", InvCount(), InvCap()), COL_TXT);
 float util = 3; for (int e = 0; e < NEQ; e++) util += C.equip[e] * (e == 5 ? 50 : (e == 3 ? 20 : 4));
 float ad = 0; for (int a = 0; a < NAD; a++) ad += C.ad[a] * ADS[a].perDay;
 yy = ShopRow(x, yy, cw, "Daily costs (utilities + ads)", TextFormat("$%.0f", util + ad), COL_TXT);
 yy = ShopRow(x, yy, cw, "Rent", "$500 every 30 days", COL_TXT);
 // cash chart
 yy += 14; Tb("CASH, LAST 60 DAYS", x, yy, 14, COL_DIM); yy += 22;
 Rectangle g = {x, yy, cw, 150};
 DrawRectangleRounded(g, 0.05f, 6, (Color){14, 15, 18, 255});
 float mn = 0, mx = 1; for (int i = 0; i < 60; i++) { mn = fminf(mn, C.cash[i]); mx = fmaxf(mx, C.cash[i]); }
 if (mn < 0) DrawLine((int)g.x, (int)(g.y + g.height * mx / (mx - mn)), (int)(g.x + g.width), (int)(g.y + g.height * mx / (mx - mn)), COL_LINE);
 for (int i = 1; i < 60; i++) {
  Vector2 a = {g.x + g.width * (i - 1) / 59.f, g.y + g.height * (mx - C.cash[i - 1]) / (mx - mn)}, b2 = {g.x + g.width * i / 59.f, g.y + g.height * (mx - C.cash[i]) / (mx - mn)};
  DrawLineEx(a, b2, 2, COL_ACC);
 }
 TxR(TextFormat("$%.0f", mx), g.x + g.width - 6, g.y + 4, 12, COL_DIM, 0);
 // right: how to
 float rx = x + cw + 20;
 Tb("HOW IT WORKS", rx, y, 18, COL_DIM); float ry = y + 30;
 const char *steps[] = {
  "1. WORKSHOP: buy a fabrication bench.",
  "2. Design an engine on the build bench, then BUILD it here.",
  "   Parts are bought new; metal comes from your stock or",
  "   gets rush-bought at +25%. Stock up in MATERIALS.",
  "3. INVENTORY: price it, pick a site, list it.",
  "4. Press NEXT DAY. Buyers browse, buy or make offers.",
  "5. ADVERTISING brings more buyers of the right kind.",
  "6. Weak designs (red warnings) fail after the sale:",
  "   refunds with a warranty, bad reviews without one.",
  "7. Reputation unlocks better sites and contracts.",
  "",
  "Rivals undercut you when you steal their buyers. Beat them",
  "on price or quality, or build what nobody else sells.",
  "",
 };
 for (int i = 0; i < 14; i++) { Tx(steps[i], rx, ry, 15, i >= 11 ? COL_YEL : COL_TXT); ry += 21; }
 ry += 10; Tb("RECENT", rx, ry, 14, COL_DIM); ry += 22;
 for (int i = C.nlog - 1; i >= 0 && ry < y + h - 24; i--) {
  LogLine *l = &C.log[i];
  Tx(TextFormat("Day %d  %s", l->day, l->s), rx, ry, 13, COL_TXT);
  if (l->amt != 0) TxR(TextFormat("%s$%.0f", l->amt < 0 ? "-" : "+", fabsf(l->amt)), rx + cw, ry, 13, l->amt < 0 ? COL_RED : COL_GRN, 1);
  ry += 18;
 }
}

static void TabWorkshop(float x, float y, float w, float h) {
 float cw = (w - 20) * 0.5f;
 Tb("EQUIPMENT", x, y, 18, COL_DIM);
 float yy = y + 30;
 for (int e = 0; e < NEQ; e++) {
  Rectangle r = {x, yy, cw, 58};
  DrawRectangleRounded(r, 0.12f, 6, C.equip[e] ? (Color){30, 44, 34, 255} : COL_PANEL);
  Tb(EQUIP[e].name, r.x + 12, r.y + 7, 16, C.equip[e] ? COL_GRN : COL_TXT);
  float s2 = 13; while (Tw(EQUIP[e].desc, s2, 0) > cw - 150 && s2 > 10) s2 -= 0.5f;
  Tx(EQUIP[e].desc, r.x + 12, r.y + 31, s2, COL_DIM);
  if (C.equip[e]) TxR("OWNED", r.x + r.width - 14, r.y + 20, 15, COL_GRN, 1);
  else if (UiButton((Rectangle){r.x + r.width - 128, r.y + 12, 116, 34}, TextFormat("$%.0f", EQUIP[e].cost), 1, C.money >= EQUIP[e].cost)) {
   Pay(EQUIP[e].cost, TextFormat("Bought %s", EQUIP[e].name)); C.equip[e] = 1; SfxPush(SFX_DONE); CareerSave();
  }
  yy += 64;
 }
 // right: current design quote + bays
 float rx = x + cw + 20;
 Tb("BUILD THE DESIGN ON YOUR BENCH", rx, y, 18, COL_DIM);
 char nm[64]; BuildName(&gB, nm, 64);
 float ry = y + 30;
 Tb(nm, rx, ry, 22, COL_TXT); ry += 30;
 Quote q = ShopQuote(&gB, 0);
 float tot = QuoteTotal(&q, 1);
 Item tmp; ItemFromBuild(&tmp, &gB);
 int bs; float val = BestValue(&tmp, 1, &bs);
 ry = ShopRow(rx, ry, cw, "Bought parts at volume prices", TextFormat("$%.0f", q.bought), COL_TXT);
 float mk = 0; for (int m = 0; m < NMAT; m++) mk += q.matKg[m];
 ry = ShopRow(rx, ry, cw, TextFormat("Metal (%.0f kg, stock first, rest rush-bought)", mk), TextFormat("$%.0f", tot - q.bought - q.consum), COL_TXT);
 ry = ShopRow(rx, ry, cw, "Labor, consumables & power", TextFormat("$%.0f", q.consum), COL_TXT);
 ry = ShopRow(rx, ry, cw, "Build time", TextFormat("%.0f h  (%.1f days)", q.hours, q.days), COL_TXT);
 ry = ShopRow(rx, ry, cw, "Total cost to build one", TextFormat("$%.0f", tot), COL_ACC);
 ry = ShopRow(rx, ry, cw, TextFormat("Est. market value (%s)", SEG_NAME[bs]), val > 0 ? TextFormat("$%.0f", val) : "nobody wants it", val > tot ? COL_GRN : COL_RED);
 if (tmp.risk) ry = ShopRow(rx, ry, cw, "Reliability", TextFormat("%d red warning%s: likely to fail after sale", tmp.risk / 2, tmp.risk > 2 ? "s" : ""), COL_RED);
 ry += 6;
 if (!q.ok) { float s2 = 14; while (Tw(q.why, s2, 0) > cw && s2 > 10) s2 -= 0.5f; Tx(q.why, rx, ry, s2, (Color){255, 140, 125, 255}); ry += 22; }
 if (UiButton((Rectangle){rx, ry, cw * 0.5f - 4, 44}, "BUILD ONE", 1, q.ok)) StartBuild(&gB, 1);
 if (C.equip[5]) {
  static float bq = 10;
  Quote qb = ShopQuote(&gB, 1);
  if (UiButton((Rectangle){rx + cw * 0.5f + 4, ry, cw * 0.5f - 4, 44}, TextFormat("BATCH x%.0f  $%.0f", bq, QuoteTotal(&qb, (int)bq)), 2, qb.ok)) StartBuild(&gB, (int)bq);
  ry += 50; UiSlider(60, (Rectangle){rx, ry, cw, 38}, "Batch size", &bq, 2, 50, 1, TextFormat("%.0f", bq), "How many copies the production line makes.", 1);
 }
 ry += 56;
 Tb("BUILD BAYS", rx, ry, 16, COL_DIM); ry += 24;
 int any = 0;
 for (int j = 0; j < NJOB; j++) {
  Job *jb = &C.job[j]; if (!jb->used) continue; any = 1;
  Rectangle r = {rx, ry, cw, 46};
  DrawRectangleRounded(r, 0.15f, 6, COL_PANEL);
  Tb(TextFormat("%s%s", jb->qty > 1 ? TextFormat("%d x ", jb->qty) : "", jb->name), r.x + 12, r.y + 6, 15, COL_TXT);
  float t = 1 - jb->hoursLeft / fmaxf(jb->hoursTotal, 0.01f);
  DrawRectangleRounded((Rectangle){r.x + 12, r.y + 30, r.width - 120, 8}, 1, 6, (Color){14, 15, 18, 255});
  DrawRectangleRounded((Rectangle){r.x + 12, r.y + 30, (r.width - 120) * t, 8}, 1, 6, COL_ACC);
  TxR(jb->hoursLeft <= 0.02f ? "no room!" : TextFormat("%.1f days", jb->hoursLeft / 8.f), r.x + r.width - 12, r.y + 22, 14, jb->hoursLeft <= 0.02f ? COL_RED : COL_DIM, 0);
  ry += 52;
 }
 if (!any) Tx(C.equip[0] ? "Bays are empty." : "No fabrication bench yet.", rx, ry, 15, COL_DIM);
 (void)h;
}

static void TabMaterials(float x, float y, float w, float h) {
 Tb("RAW STOCK", x, y, 18, COL_DIM);
 Tx("Buy ahead when prices are low. Without a foundry, cast metals come as outsourced castings (+20%). Running short during a build rush-buys at +25%.", x, y + 26, 14, COL_DIM);
 float yy = y + 60;
 float cols[] = {x, x + 190, x + 330, x + 470, x + 610};
 Tb("Material", cols[0], yy, 14, COL_DIM); Tb("Price/kg", cols[1], yy, 14, COL_DIM); Tb("In stock", cols[2], yy, 14, COL_DIM); Tb("Avg paid", cols[3], yy, 14, COL_DIM); Tb("Buy now (local)          Bulk 1 t: -20%, 4 days", cols[4], yy, 14, COL_DIM);
 yy += 24;
 Quote q = ShopQuote(&gB, 0);
 for (int m = 0; m < NMAT; m++) {
  DrawRectangleRounded((Rectangle){x, yy - 4, w, 40}, 0.15f, 6, m & 1 ? COL_PANEL : (Color){24, 27, 32, 255});
  Tb(MAT_NAME[m], cols[0] + 8, yy + 6, 15, COL_TXT);
  float tr = C.price[m] / MAT_BASE[m];
  Tx(TextFormat("$%.2f", C.price[m]), cols[1], yy + 6, 15, tr > 1.1f ? COL_RED : (tr < 0.93f ? COL_GRN : COL_TXT));
  Tx(TextFormat("%.0f kg%s", C.stock[m], C.bulkDays[m] ? TextFormat(" (+%.0f in %dd)", C.bulkKg[m], C.bulkDays[m]) : ""), cols[2], yy + 6, 15, COL_TXT);
  Tx(C.stock[m] > 0 ? TextFormat("$%.2f", C.stockCost[m] / C.stock[m]) : "-", cols[3], yy + 6, 15, COL_DIM);
  static const float AMT[3] = {25, 100, 500};
  for (int k = 0; k < 3; k++) {
   float kg = AMT[k], c = kg * C.price[m] * (k == 0 ? 1.1f : 1.f);
   if (UiButton((Rectangle){cols[4] + k * 96, yy, 90, 32}, TextFormat("%.0f kg", kg), 0, C.money >= c)) { Pay(c, TextFormat("Bought %.0f kg %s", kg, MAT_NAME[m])); C.stock[m] += kg; C.stockCost[m] += c; CareerSave(); }
  }
  float bc = 1000 * C.price[m] * 0.8f;
  if (UiButton((Rectangle){cols[4] + 300, yy, 150, 32}, TextFormat("1 t  $%.0f", bc), 0, C.money >= bc && !C.bulkDays[m])) { Pay(bc, TextFormat("Bulk order: 1 t %s", MAT_NAME[m])); C.bulkDays[m] = 4; C.bulkKg[m] = 1000; C.bulkCost[m] = bc; CareerSave(); }
  if (q.matKg[m] > 0.5f) TxR(TextFormat("design needs %.0f kg", q.matKg[m]), x + w - 8, yy + 8, 13, C.stock[m] >= q.matKg[m] ? COL_GRN : COL_YEL, 0);
  yy += 46;
 }
 (void)h;
}

static void TabInventory(float x, float y, float w, float h) {
 Tb(TextFormat("FINISHED ENGINES  %d / %d", InvCount(), InvCap()), x, y, 18, COL_DIM);
 float lw = w * 0.42f, yy = y + 30;
 int any = 0;
 for (int i = 0; i < NITEM; i++) {
  Item *it = &C.inv[i]; if (!it->used) continue; any = 1;
  if (gInvSel < 0 || !C.inv[gInvSel].used) gInvSel = i;
  Rectangle r = {x, yy, lw, 48};
  int hov = CheckCollisionPointRec(MP, r);
  DrawRectangleRounded(r, 0.15f, 6, gInvSel == i ? COL_PANEL2 : (hov ? (Color){33, 36, 43, 255} : COL_PANEL));
  if (hov && MPress) { gInvSel = i; SfxPush(SFX_SELECT); }
  Tb(it->name, r.x + 12, r.y + 6, 15, COL_TXT);
  Tx(TextFormat("%.0f hp  %.0f kg  cost $%.0f", it->hp, it->kg, it->costBasis), r.x + 12, r.y + 27, 13, COL_DIM);
  TxR(it->site >= 0 ? TextFormat("%s  $%.0f", SITES[it->site].name, it->price) : "not listed", r.x + r.width - 12, r.y + 8, 14, it->site >= 0 ? COL_ACC : COL_DIM, 1);
  if (it->site >= 0) TxR(TextFormat("%d days, %d views, %d lost", it->days, it->views, C.lost[i]), r.x + r.width - 12, r.y + 27, 12, COL_DIM, 0);
  yy += 54;
 }
 if (!any) { Tx("Nothing in stock. Build something in the WORKSHOP.", x, yy, 15, COL_DIM); yy += 26; }
 // offers inbox
 yy += 10; Tb("OFFERS", x, yy, 16, COL_DIM); yy += 24;
 int no = 0;
 for (int o = 0; o < NOFFER; o++) {
  Offer *of = &C.off[o]; if (!of->used) continue; no++;
  Item *it = &C.inv[of->item];
  Rectangle r = {x, yy, lw, 52};
  DrawRectangleRounded(r, 0.15f, 6, COL_PANEL);
  Tb(TextFormat("%s offers $%.0f", of->who, of->amount), r.x + 12, r.y + 5, 15, COL_TXT);
  Tx(of->scam ? TextFormat("for %s. \"Will send cashier's check for MORE, my shipper picks up\"", it->name) : TextFormat("for %s (asking $%.0f), %d day%s left", it->name, it->price, of->days, of->days == 1 ? "" : "s"), r.x + 12, r.y + 28, 12, COL_DIM);
  if (UiButton((Rectangle){r.x + r.width - 176, r.y + 9, 80, 34}, "ACCEPT", 2, 1)) {
   if (of->scam) {
    CLog(0, "Scammed! The check for %s bounced and the engine is gone.", it->name);
    ToastAdd("The cashier's check bounced. The engine's gone.", COL_RED); SfxPush(SFX_WARN);
    it->used = 0; of->used = 0; C.rep = fmaxf(0, C.rep - 1);
   } else { SellItem(of->item, of->amount, it->site, of->who); of->used = 0; }
   CareerSave();
  }
  if (UiButton((Rectangle){r.x + r.width - 90, r.y + 9, 80, 34}, "DECLINE", 0, 1)) of->used = 0;
  yy += 58;
 }
 if (!no) Tx("No offers yet.", x, yy, 14, COL_DIM);
 // detail of selected
 float rx = x + lw + 20, rw = w - lw - 20;
 if (gInvSel < 0 || !C.inv[gInvSel].used) return;
 Item *it = &C.inv[gInvSel];
 float ry = y;
 Tb(it->name, rx, ry, 22, COL_TXT); ry += 30;
 Tx(TextFormat("%.0f hp, %.0f lb-ft, %.0f kg, %.2f L, redline %.0f%s", it->hp, it->lbft, it->kg, it->dispL, it->redline, it->gov ? ", governed" : ""), rx, ry, 15, COL_DIM); ry += 22;
 Tx(TextFormat("%s%s%s", it->dyno ? "Dyno sheet included.  " : "No dyno sheet (buyers pay ~6% less).  ", it->finish ? "Powder coated.  " : "", it->risk ? "Has red warnings: may fail after sale." : "Solid build."), rx, ry, 14, it->risk ? (Color){255, 140, 125, 255} : COL_DIM); ry += 30;
 Tb("WHAT BUYERS WOULD PAY", rx, ry, 14, COL_DIM); ry += 22;
 for (int k = 0; k < NSEG; k++) {
  float v = ItemValue(it, k, -1); if (v <= 0) continue;
  ry = ShopRow(rx, ry, rw * 0.6f, SEG_NAME[k], TextFormat("~$%.0f", v), COL_TXT);
 }
 ry += 10;
 int site = it->site;
 static int pickSite = 0;
 if (site >= 0) pickSite = site;
 int d;
 if ((d = UiSelect((Rectangle){rx, ry, rw, 40}, "Marketplace", SITES[pickSite].name, SITES[pickSite].desc, site < 0))) pickSite = (pickSite + d + NSITE) % NSITE;
 ry += 44;
 { float s2 = 13; while (Tw(SITES[pickSite].desc, s2, 0) > rw && s2 > 10) s2 -= 0.5f; Tx(SITES[pickSite].desc, rx, ry, s2, COL_DIM); ry += 22; }
 DrawRectangleRounded((Rectangle){rx, ry, rw, 40}, 0.18f, 8, COL_PANEL);
 Tx("Asking price", rx + 12, ry + 11, 17, COL_DIM);
 PriceField((Rectangle){rx + rw * 0.4f, ry + 4, rw * 0.3f, 32}, &it->price);
 if (UiButton((Rectangle){rx + rw * 0.72f, ry + 4, rw * 0.13f - 3, 32}, "-5%", 0, 1)) it->price = roundf(it->price * 0.95f / 5) * 5;
 if (UiButton((Rectangle){rx + rw * 0.85f + 3, ry + 4, rw * 0.15f - 3, 32}, "+5%", 0, 1)) it->price = roundf(it->price * 1.05f / 5) * 5;
 ry += 44;
 { int bs; BestValue(it, -1, &bs); int cj = CompFor(bs, it->hp);
   Tx("Your quality", rx, ry + 2, 15, COL_DIM); Stars(rx + 110, ry + 2, ItemQuality(it));
   if (cj >= 0) {
    Tx(TextFormat("Rival: %s %s, %.0f hp, $%.0f", COMPS[cj].co, COMPS[cj].name, COMPS[cj].hp, C.comp[cj].price), rx + 200, ry + 2, 15, COL_TXT);
    Stars(rx + rw - 66, ry + 2, C.comp[cj].q);
   } else Tx(TextFormat("No direct rival for %s buyers at this power.", SEG_NAME[bs]), rx + 200, ry + 2, 15, COL_GRN);
   ry += 26; }
 if ((d = UiSelect((Rectangle){rx, ry, rw, 40}, "Warranty", it->warranty ? "30-day warranty" : "Sold as-is", "With a warranty, buyers pay ~10% more but you refund engines that fail. As-is: no refunds, but failures hurt your reputation more.", site < 0))) it->warranty ^= 1;
 ry += 48;
 float sug = roundf(BestValue(it, pickSite, NULL) / 25) * 25;
 if (UiButton((Rectangle){rx, ry, rw * 0.32f, 40}, TextFormat("PRICE AT $%.0f", sug), 0, sug > 0)) it->price = sug;
 if (site < 0) {
  int ok = C.siteOn[pickSite] && C.rep >= SITES[pickSite].rep;
  const char *lbl = ok ? "LIST IT" : (C.rep < SITES[pickSite].rep ? TextFormat("NEEDS REP %.0f", SITES[pickSite].rep) : "JOIN IN SITES TAB");
  if (UiButton((Rectangle){rx + rw * 0.34f, ry, rw * 0.32f, 40}, lbl, 1, ok)) { it->site = pickSite; it->days = 0; it->views = 0; C.lost[gInvSel] = 0; if (pickSite == 1) Pay(SITES[1].listFee, "SwapBay listing fee"); CareerSave(); }
 } else if (UiButton((Rectangle){rx + rw * 0.34f, ry, rw * 0.32f, 40}, "TAKE DOWN", 0, 1)) { it->site = -1; CareerSave(); }
 float whole = roundf(BestValue(it, -1, NULL) * 0.55f / 5) * 5;
 if (UiButton((Rectangle){rx + rw * 0.68f, ry, rw * 0.32f, 40}, TextFormat("DEALER: $%.0f", whole), 3, whole > 0)) { SellItem(gInvSel, whole, -1, "a dealer"); CareerSave(); }
 ry += 48;
 if (UiButton((Rectangle){rx, ry, rw * 0.32f, 34}, "LOAD DESIGN", 0, 1)) { gB = it->b; OnBuildChanged(); ToastAdd("Design loaded onto the build bench.", COL_DIM); }
 // contracts this engine could fill
 for (int c = 0; c < NCON; c++) if (C.con[c].used && ItemMeets(it, &C.con[c])) {
  if (UiButton((Rectangle){rx + rw * 0.34f, ry, rw * 0.66f, 34}, TextFormat("DELIVER TO %s ($%.0f)", C.con[c].client, C.con[c].pay), 2, 1)) {
   Earn(C.con[c].pay, TextFormat("Contract: %s took a %s", C.con[c].client, it->name));
   C.con[c].done++; it->used = 0; C.rep = fminf(100, C.rep + 1);
   if (C.con[c].done >= C.con[c].qty) { C.rep = fminf(100, C.rep + 3); C.con[c].used = 0; ToastAdd("Contract complete!", COL_GRN); SfxPush(SFX_ACH); }
   CareerSave();
  }
  break;
 }
 (void)h;
}

static int gPriceEdit = 0; static char gPriceBuf[12];
static int PriceField(Rectangle r, float *v) {
 int hov = CheckCollisionPointRec(MP, r), changed = 0;
 if (hov) { gUiHover = 1; gTip = "Click and type a price, Enter to set."; }
 if (!gPriceEdit && hov && MPress) { gPriceEdit = 1; snprintf(gPriceBuf, 12, "%.0f", *v); }
 DrawRectangleRounded(r, 0.2f, 6, (Color){14, 15, 18, 255});
 if (gPriceEdit) {
  DrawRectangleRoundedLinesEx(r, 0.2f, 6, 1.5f, COL_ACC);
  int c; while ((c = GetCharPressed())) { int l = (int)strlen(gPriceBuf); if (c >= '0' && c <= '9' && l < 7) { gPriceBuf[l] = (char)c; gPriceBuf[l + 1] = 0; } }
  if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && strlen(gPriceBuf)) gPriceBuf[strlen(gPriceBuf) - 1] = 0;
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || (MPress && !hov)) { float nv = (float)atof(gPriceBuf); if (nv >= 1) { *v = nv; changed = 1; } gPriceEdit = 0; SfxPush(SFX_SELECT); }
  if (IsKeyPressed(KEY_ESCAPE)) gPriceEdit = 0;
  Tb(TextFormat("$%s%s", gPriceBuf, ((int)(GetTime() * 2)) & 1 ? "_" : ""), r.x + 12, r.y + (r.height - 20) * 0.5f, 20, COL_TXT);
 } else {
  Tb(TextFormat("$%.0f", *v), r.x + 12, r.y + (r.height - 20) * 0.5f, 20, hov ? COL_ACC : COL_TXT);
  TxR("type", r.x + r.width - 10, r.y + (r.height - 13) * 0.5f, 13, COL_DIM, 0);
 }
 return changed;
}

static void TabMarket(float x, float y, float w, float h) {
 Tb("THE MARKET", x, y, 18, COL_DIM);
 Tx("Rival engine makers. Sell into their segment and they cut prices to win buyers back; they drift back up when you stop. Buyers pick whoever gives them the better deal for the quality.", x, y + 26, 14, COL_DIM);
 float yy = y + 58;
 float cx[] = {x, x + 200, x + 410, x + 620, x + 720, x + 820, x + 930, x + 1050, x + 1170};
 const char *hd[] = {"Maker", "Engine", "Segment", "Power", "Quality", "Price", "vs normal", "Sales/mo", "Your share"};
 for (int k = 0; k < 9; k++) Tb(hd[k], cx[k] + 8, yy, 14, COL_DIM);
 yy += 24;
 for (int j = 0; j < NCOMP; j++) {
  const CompDef *d = &COMPS[j]; CompState *cs = &C.comp[j];
  DrawRectangleRounded((Rectangle){x, yy - 3, w, 26}, 0.2f, 6, j & 1 ? COL_PANEL : (Color){24, 27, 32, 255});
  Tb(d->co, cx[0] + 8, yy + 2, 14, COL_TXT);
  Tx(d->name, cx[1] + 8, yy + 2, 14, COL_TXT);
  Tx(SEG_NAME[d->seg], cx[2] + 8, yy + 2, 14, COL_DIM);
  Tx(TextFormat("%.0f hp", d->hp), cx[3] + 8, yy + 2, 14, COL_TXT);
  Stars(cx[4] + 12, yy + 1, cs->q);
  Tb(TextFormat("$%.0f", cs->price), cx[5] + 8, yy + 2, 14, COL_TXT);
  float pct = (cs->price / cs->base - 1) * 100;
  Tx(fabsf(pct) < 1 ? "-" : TextFormat("%+.0f%%", pct), cx[6] + 8, yy + 2, 14, pct < -1 ? COL_RED : (pct > 1 ? COL_GRN : COL_DIM));
  Tx(TextFormat("%.0f", cs->sales30), cx[7] + 8, yy + 2, 14, COL_TXT);
  float my = C.mySeg30[d->seg], tot = my + C.compSeg30[d->seg];
  Tx(tot > 0.5f ? TextFormat("%.0f%%", my / tot * 100) : "-", cx[8] + 8, yy + 2, 14, my > 0.5f ? COL_ACC : COL_DIM);
  yy += 28;
 }
 (void)h;
}

static void TabSites(float x, float y, float w, float h) {
 Tb("MARKETPLACES", x, y, 18, COL_DIM);
 float yy = y + 32;
 for (int s = 0; s < NSITE; s++) {
  const SiteDef *sd = &SITES[s];
  Rectangle r = {x, yy, w, 74};
  DrawRectangleRounded(r, 0.1f, 6, C.siteOn[s] ? (Color){30, 40, 52, 255} : COL_PANEL);
  Tb(sd->name, r.x + 14, r.y + 8, 18, C.siteOn[s] ? COL_BLUE : COL_TXT);
  Tx(sd->desc, r.x + 14, r.y + 32, 14, COL_DIM);
  int nl = 0; for (int i = 0; i < NITEM; i++) nl += C.inv[i].used && C.inv[i].site == s;
  Tx(TextFormat("~%.0f visitors/day   buyers pay %.0f%%   fee %.0f%%%s%s   %d listed", sd->traffic, sd->pay * 100, sd->fee * 100, sd->monthly > 0 ? TextFormat("   $%.0f/month", sd->monthly) : "", sd->listFee > 0 ? TextFormat("   $%.0f/listing/week", sd->listFee) : "", nl), r.x + 14, r.y + 52, 13, COL_TXT);
  if (C.siteOn[s]) {
   if (s && UiButton((Rectangle){r.x + r.width - 150, r.y + 20, 136, 36}, "CANCEL", 0, nl == 0)) { C.siteOn[s] = 0; CareerSave(); }
   else if (!s) TxR("ALWAYS FREE", r.x + r.width - 16, r.y + 28, 15, COL_GRN, 1);
  } else {
   int ok = C.rep >= sd->rep;
   float up = sd->setup + sd->monthly;
   if (UiButton((Rectangle){r.x + r.width - 150, r.y + 20, 136, 36}, ok ? (up > 0 ? TextFormat("JOIN  $%.0f", up) : "JOIN") : TextFormat("REP %.0f", sd->rep), 1, ok && C.money >= up)) {
    if (up > 0) Pay(up, TextFormat("Joined %s", sd->name)); C.siteOn[s] = 1; CareerSave();
   }
  }
  yy += 82;
 }
 (void)h;
}

static void TabAds(float x, float y, float w, float h) {
 Tb("ADVERTISING", x, y, 18, COL_DIM);
 Tx("Set how hard each channel runs. Charged every day.", x, y + 26, 14, COL_DIM);
 float yy = y + 56;
 for (int a = 0; a < NAD; a++) {
  Rectangle r = {x, yy, w, 64};
  DrawRectangleRounded(r, 0.1f, 6, C.ad[a] ? (Color){44, 36, 26, 255} : COL_PANEL);
  Tb(ADS[a].name, r.x + 14, r.y + 8, 17, C.ad[a] ? COL_ACC : COL_TXT);
  Tx(ADS[a].desc, r.x + 14, r.y + 34, 14, COL_DIM);
  static const char *LV[] = {"OFF", "LOW", "MED", "HIGH"};
  for (int l = 0; l < 4; l++) {
   if (UiButton((Rectangle){r.x + r.width - 420 + l * 84, r.y + 14, 78, 36}, l ? TextFormat("%s $%.0f", LV[l], ADS[a].perDay * l) : "OFF", C.ad[a] == l ? 1 : 0, 1)) { C.ad[a] = l; CareerSave(); }
  }
  yy += 72;
 }
 yy += 10;
 Rectangle r = {x, yy, w, 70};
 DrawRectangleRounded(r, 0.1f, 6, COL_PANEL);
 Tb("Trade show booth", r.x + 14, r.y + 8, 17, COL_TXT);
 Tx("Rent a booth for a weekend: brings in 3 contract requests and +4 reputation. Once every 14 days.", r.x + 14, r.y + 36, 14, COL_DIM);
 int can = C.day - C.lastShow >= 14;
 if (UiButton((Rectangle){r.x + r.width - 200, r.y + 16, 186, 38}, can ? "BOOK  $2,500" : TextFormat("again in %d days", 14 - (C.day - C.lastShow)), 1, can && C.money >= 2500)) {
  Pay(2500, "Trade show booth"); C.lastShow = C.day; C.rep = fminf(100, C.rep + 4);
  for (int k = 0; k < 3; k++) NewContract();
  ToastAdd("Great show! New contract requests are in.", COL_GRN); SfxPush(SFX_ACH); CareerSave();
 }
 (void)h;
}

static void TabContracts(float x, float y, float w, float h) {
 Tb("CONTRACTS", x, y, 18, COL_DIM);
 Tx("Customers who want a specific engine. Deliver matching engines from INVENTORY. Reputation 10+ brings them in weekly; trade shows bring more.", x, y + 26, 14, COL_DIM);
 float yy = y + 60; int any = 0;
 for (int c = 0; c < NCON; c++) {
  Contract *k = &C.con[c]; if (!k->used) continue; any = 1;
  Rectangle r = {x, yy, w, 70};
  DrawRectangleRounded(r, 0.1f, 6, COL_PANEL);
  Tb(TextFormat("%s wants %d engine%s for %s", k->client, k->qty, k->qty > 1 ? "s" : "", SEG_NAME[k->seg]), r.x + 14, r.y + 8, 17, COL_TXT);
  Tx(TextFormat("At least %.0f hp%s%s%s.   Pays $%.0f each.   %d/%d delivered.   %d days left.", k->minHp, k->maxKg > 0 ? TextFormat(", under %.0f kg", k->maxKg) : "", k->needGov ? ", governed" : "", k->needDiesel ? ", diesel" : "", k->pay, k->done, k->qty, k->deadline - C.day), r.x + 14, r.y + 38, 15, COL_DIM);
  int have = 0; for (int i = 0; i < NITEM; i++) have += C.inv[i].used && ItemMeets(&C.inv[i], k);
  TxR(have ? TextFormat("%d in stock fit", have) : "nothing in stock fits", r.x + r.width - 14, r.y + 10, 14, have ? COL_GRN : COL_DIM, 1);
  if (UiButton((Rectangle){r.x + r.width - 110, r.y + 32, 96, 30}, "DECLINE", 0, 1)) { k->used = 0; CareerSave(); }
  yy += 78;
 }
 if (!any) Tx(C.rep < 10 ? "No contracts yet. Get your reputation to 10 or book a trade show." : "No open contracts right now.", x, yy, 15, COL_DIM);
 (void)h;
}

static void TabBank(float x, float y, float w, float h) {
 Tb("BANK", x, y, 18, COL_DIM);
 float lim = 25000 + C.rep * 500;
 float yy = y + 32;
 yy = ShopRow(x, yy, 420, "Loan balance", TextFormat("$%.0f", C.loan), COL_TXT);
 yy = ShopRow(x, yy, 420, "Credit limit", TextFormat("$%.0f", lim), COL_TXT);
 yy = ShopRow(x, yy, 420, "Interest", "1.5% per week", COL_TXT);
 yy += 10;
 if (UiButton((Rectangle){x, yy, 200, 40}, "BORROW $5,000", 1, C.loan + 5000 <= lim)) { C.loan += 5000; Earn(5000, "Bank loan"); CareerSave(); }
 if (UiButton((Rectangle){x + 210, yy, 200, 40}, "REPAY $5,000", 0, C.loan >= 5000 && C.money >= 5000)) { C.loan -= 5000; Pay(5000, "Loan repayment"); CareerSave(); }
 yy += 60;
 Tb("LEDGER", x, yy, 16, COL_DIM); yy += 24;
 for (int i = C.nlog - 1; i >= 0 && yy < y + h - 20; i--) {
  LogLine *l = &C.log[i];
  Tx(TextFormat("Day %d", l->day), x, yy, 14, COL_DIM); Tx(l->s, x + 80, yy, 14, COL_TXT);
  if (l->amt != 0) TxR(TextFormat("%s$%.0f", l->amt < 0 ? "-" : "+", fabsf(l->amt)), x + w * 0.7f, yy, 14, l->amt < 0 ? COL_RED : COL_GRN, 1);
  yy += 19;
 }
 yy = y + 32;
 float rx = x + w * 0.5f;
 Tb("START OVER", rx, y, 18, COL_DIM);
 Tx("Sell everything off and start fresh with $10,000.", rx, yy, 15, COL_DIM);
 static int confirm = 0;
 if (UiButton((Rectangle){rx, yy + 28, 220, 38}, confirm ? "REALLY? CLICK AGAIN" : "NEW CAREER", 3, 1)) { if (confirm) { CareerNew(); CareerSave(); confirm = 0; } else confirm = 1; }
}

static void ShopScreen(int sw, int sh) {
 DrawRectangle(0, 53, sw, sh - 53, (Color){15, 16, 20, 255});
 gUiHover = 1;
 float x = 20, y = 64, w = sw - 40;
 // header
 Tb("YOUR SHOP", x, y, 26, COL_TXT);
 Tx(TextFormat("Day %d", C.day), x + 190, y + 7, 18, COL_DIM);
 Money(x + 290, y + 2, 24, C.money);
 Tx(TextFormat("Rep %.0f", C.rep), x + 470, y + 7, 18, COL_YEL);
 int wk = 0; for (int j = 0; j < NJOB; j++) wk += C.job[j].used;
 int ls = 0; for (int i = 0; i < NITEM; i++) ls += C.inv[i].used && C.inv[i].site >= 0;
 int of = 0; for (int o = 0; o < NOFFER; o++) of += C.off[o].used;
 Tx(TextFormat("%d building   %d listed   %d offer%s", wk, ls, of, of == 1 ? "" : "s"), x + 570, y + 8, 16, COL_DIM);
 if (UiButton((Rectangle){sw - 470, y - 2, 140, 40}, "BACK TO BENCH", 0, 1)) gShop = 0;
 if (UiButton((Rectangle){sw - 320, y - 2, 140, 40}, "NEXT DAY", 1, !C.over)) { ShopDay(); SfxPush(SFX_TICK); }
 if (UiButton((Rectangle){sw - 170, y - 2, 150, 40}, "SKIP 7 DAYS", 0, !C.over)) { for (int i = 0; i < 7; i++) { int s0 = C.sales, o0 = 0; for (int o = 0; o < NOFFER; o++) o0 += C.off[o].used; ShopDay(); int o1 = 0; for (int o = 0; o < NOFFER; o++) o1 += C.off[o].used; if (C.sales != s0 || o1 > o0) break; } }
 // tabs
 static const char *T[] = {"DASHBOARD", "WORKSHOP", "MATERIALS", "INVENTORY", "MARKET", "SITES", "ADVERTISING", "CONTRACTS", "BANK"};
 float tw = (w) / 9.f, ty = y + 52;
 for (int i = 0; i < 9; i++) {
  Rectangle tr = {x + i * tw, ty, tw - 6, 36};
  int hov = CheckCollisionPointRec(MP, tr);
  DrawRectangleRounded(tr, 0.3f, 8, gShopTab == i ? COL_ACC : (hov ? COL_PANEL2 : COL_PANEL));
  const char *lbl = T[i];
  if (i == 3 && of) lbl = TextFormat("INVENTORY (%d)", of);
  TxC(lbl, tr.x + tr.width * 0.5f, tr.y + 10, 14, gShopTab == i ? (Color){18, 18, 20, 255} : COL_TXT, 1);
  if (hov && MPress && gShopTab != i) { gShopTab = i; SfxPush(SFX_SELECT); }
 }
 float cy = ty + 52, ch = sh - cy - 16;
 Panel((Rectangle){x, cy, w, ch}, (Color){22, 25, 30, 255});
 float px = x + 20, py = cy + 16, pw = w - 40, ph = ch - 32;
 if (C.over) {
  TxC("BANKRUPT", sw * 0.5f, py + 120, 48, COL_RED, 1);
  TxC("The bank took the shop. Start a new career from the BANK tab.", sw * 0.5f, py + 180, 18, COL_TXT, 0);
  if (gShopTab != 8) return;
 }
 switch (gShopTab) {
 case 0: TabDashboard(px, py, pw, ph); break;
 case 1: TabWorkshop(px, py, pw, ph); break;
 case 2: TabMaterials(px, py, pw, ph); break;
 case 3: TabInventory(px, py, pw, ph); break;
 case 4: TabMarket(px, py, pw, ph); break;
 case 5: TabSites(px, py, pw, ph); break;
 case 6: TabAds(px, py, pw, ph); break;
 case 7: TabContracts(px, py, pw, ph); break;
 case 8: TabBank(px, py, pw, ph); break;
 }
}
