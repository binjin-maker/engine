// ===== 3D rendering: procedural engine =====
static const char *VS_LIT =
"#version 330\n"
"in vec3 vertexPosition; in vec3 vertexNormal;\n"
"uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;\n"
"out vec3 fp; out vec3 fn;\n"
"void main(){ fp=vec3(matModel*vec4(vertexPosition,1.0)); fn=vec3(matNormal*vec4(vertexNormal,0.0)); gl_Position=mvp*vec4(vertexPosition,1.0); }\n";
static const char *FS_LIT =
"#version 330\n"
"in vec3 fp; in vec3 fn; out vec4 o;\n"
"uniform vec4 colDiffuse; uniform vec3 viewPos; uniform vec3 emissive; uniform float metal;\n"
"void main(){\n"
" vec3 n=normalize(fn); if(!gl_FrontFacing) n=-n;\n"
" vec3 base=pow(colDiffuse.rgb,vec3(2.2));\n"
" vec3 v=normalize(viewPos-fp);\n"
" vec3 L1=normalize(vec3(-0.45,0.85,0.35)); vec3 L2=normalize(vec3(0.6,0.3,-0.7));\n"
" float d1=max(dot(n,L1),0.0), d2=max(dot(n,L2),0.0);\n"
" float hemi=0.5+0.5*n.y;\n"
" vec3 amb=mix(vec3(0.05,0.045,0.04),vec3(0.17,0.19,0.23),hemi);\n"
" float sp=mix(20.0,80.0,metal);\n"
" float s1=pow(max(dot(n,normalize(L1+v)),0.0),sp), s2=pow(max(dot(n,normalize(L2+v)),0.0),sp);\n"
" vec3 spec=mix(vec3(0.05),base,metal);\n"
" float rim=pow(1.0-max(dot(n,v),0.0),3.0)*0.22;\n"
" vec3 c=base*(amb+d1*vec3(1.0,0.95,0.88)*1.15+d2*vec3(0.32,0.40,0.55))*(1.0-metal*0.45)\n"
"   +spec*(s1*1.7+s2*0.5)*(0.45+metal)+base*metal*amb*1.6+rim*vec3(0.25,0.3,0.4)+emissive;\n"
" c=c/(1.0+c*0.22);\n"
" o=vec4(pow(c,vec3(1.0/2.2)),colDiffuse.a);\n"
"}\n";
static const char *VS_FLOOR =
"#version 330\n"
"in vec3 vertexPosition; uniform mat4 mvp; uniform mat4 matModel; out vec3 wp;\n"
"void main(){ wp=vec3(matModel*vec4(vertexPosition,1.0)); gl_Position=mvp*vec4(vertexPosition,1.0); }\n";
static const char *FS_FLOOR =
"#version 330\n"
"in vec3 wp; out vec4 o; uniform vec3 bg; uniform vec3 ctr; uniform float shR;\n"
"float grid(vec2 g){ vec2 d=fwidth(g); vec2 a=abs(fract(g-0.5)-0.5)/max(d,vec2(1e-4)); float l=1.0-min(min(a.x,a.y),1.0); return l*clamp(1.0-max(d.x,d.y)*2.0,0.0,1.0); }\n"
"void main(){\n"
" float dist=length(wp.xz-ctr.xz);\n"
" vec3 col=vec3(0.135,0.145,0.16)+grid(wp.xz)*0.025+grid(wp.xz/5.0)*0.05;\n"
" vec2 q=(wp.xz-ctr.xz)/vec2(shR*1.25,shR*0.8);\n"
" col*=mix(0.45,1.0,smoothstep(0.2,1.3,length(q)));\n"
" col=mix(col,bg,smoothstep(12.0,40.0,dist));\n"
" o=vec4(col,1.0);\n"
"}\n";

static Shader gSh, gFloorSh;
static Material gMat, gFloorMat;
static int locEmis, locMetal, locView, locBg, locCtr, locShR;
static Mesh mCube, mCyl, mSph, mCone, mPlane, mPuff;
static Matrix gBase;
static Vector3 gNoEmis = {0, 0, 0};

// part groups, for hiding and for the collision solver
enum { G_BLOCK, G_HEAD, G_PLUG, G_ROT, G_FRONT, G_FLY, G_STARTER, G_ALT, G_INTAKE, G_FILTER, G_SC, G_TURBO, G_CHARGE, G_EXH, G_COOL, G_STAND, G_OILSYS, G_N };
static const char *GROUP_NAME[] = {"block","head","plug","crank","front","flywheel","starter","alternator","intake","filter","blower","turbo","charge pipe","exhaust","cooling","stand","oil system"};
static int gGroup = 0, gSolving = 0, gHide = 0;
static unsigned gOnly = 0; static int gAlpha = 0;
static unsigned gAllow[G_N];
typedef struct { Matrix m, inv; int mesh, group; Vector3 lo, hi; } Prim;
#define MAXPRIM 6000
static Prim gPrim[MAXPRIM]; static int gNPrim = 0, gRecord = 0;

static void AllowInit(void);
static void RenderInit(void) {
 gSh = LoadShaderFromMemory(VS_LIT, FS_LIT);
 locEmis = GetShaderLocation(gSh, "emissive"); locMetal = GetShaderLocation(gSh, "metal"); locView = GetShaderLocation(gSh, "viewPos");
 gFloorSh = LoadShaderFromMemory(VS_FLOOR, FS_FLOOR);
 locBg = GetShaderLocation(gFloorSh, "bg"); locCtr = GetShaderLocation(gFloorSh, "ctr"); locShR = GetShaderLocation(gFloorSh, "shR");
 gMat = LoadMaterialDefault(); gMat.shader = gSh;
 gFloorMat = LoadMaterialDefault(); gFloorMat.shader = gFloorSh;
 mCube = GenMeshCube(1, 1, 1); mCyl = GenMeshCylinder(1, 1, 28); mSph = GenMeshSphere(1, 18, 24); mCone = GenMeshCone(1, 1, 28);
 mPlane = GenMeshPlane(200, 200, 1, 1);
 mPuff = GenMeshSphere(1, 8, 10);
 AllowInit();
}

static void AllowPair(int a, int b) { gAllow[a] |= 1u << b; gAllow[b] |= 1u << a; }
static void AllowInit(void) {
 for (int i = 0; i < G_N; i++) gAllow[i] = 1u << i;
 AllowPair(G_BLOCK, G_HEAD); AllowPair(G_BLOCK, G_ROT); AllowPair(G_BLOCK, G_STAND); AllowPair(G_HEAD, G_STAND); AllowPair(G_BLOCK, G_INTAKE);
 AllowPair(G_BLOCK, G_COOL); AllowPair(G_HEAD, G_PLUG); AllowPair(G_HEAD, G_INTAKE); AllowPair(G_HEAD, G_EXH); AllowPair(G_HEAD, G_COOL);
 AllowPair(G_ROT, G_FRONT); AllowPair(G_ROT, G_FLY); AllowPair(G_FRONT, G_ALT); AllowPair(G_FRONT, G_SC); AllowPair(G_FRONT, G_BLOCK);
 AllowPair(G_INTAKE, G_FILTER); AllowPair(G_INTAKE, G_SC); AllowPair(G_INTAKE, G_CHARGE); AllowPair(G_FILTER, G_TURBO); AllowPair(G_FILTER, G_SC);
 AllowPair(G_OILSYS, G_BLOCK); AllowPair(G_OILSYS, G_FRONT); AllowPair(G_TURBO, G_EXH); AllowPair(G_TURBO, G_CHARGE); AllowPair(G_STARTER, G_FLY); AllowPair(G_STARTER, G_BLOCK); AllowPair(G_ALT, G_BLOCK); AllowPair(G_ALT, G_HEAD);
}

static int MeshId(Mesh m) { return m.vaoId == mCube.vaoId ? 0 : m.vaoId == mCyl.vaoId ? 1 : (m.vaoId == mSph.vaoId || m.vaoId == mPuff.vaoId) ? 2 : 3; }
static void Put(Mesh m, Matrix local, Color c, float metal, Vector3 emis) {
 Matrix w = MatrixMultiply(local, gBase);
 if (gRecord && gNPrim < MAXPRIM) {
  Prim *p = &gPrim[gNPrim++];
  p->m = w; p->inv = MatrixInvert(w); p->mesh = MeshId(m); p->group = gGroup;
  Vector3 lo = {1e9f, 1e9f, 1e9f}, hi = {-1e9f, -1e9f, -1e9f};
  float y0 = p->mesh == 0 || p->mesh == 2 ? -1 : 0, s = p->mesh == 0 ? 0.5f : 1;
  for (int k = 0; k < 8; k++) {
   Vector3 u = {(k & 1) ? s : -s, p->mesh == 0 ? ((k & 2) ? 0.5f : -0.5f) : ((k & 2) ? 1 : y0), (k & 4) ? s : -s};
   Vector3 v = Vector3Transform(u, w);
   lo = Vector3Min(lo, v); hi = Vector3Max(hi, v);
  }
  p->lo = lo; p->hi = hi;
 }
 if (gSolving || ((gHide >> gGroup) & 1)) return;
 if (gOnly && !((gOnly >> gGroup) & 1)) return;
 if (gAlpha) { c.a = (unsigned char)gAlpha; metal *= 0.5f; }
 gMat.maps[MATERIAL_MAP_DIFFUSE].color = c;
 SetShaderValue(gSh, locMetal, &metal, SHADER_UNIFORM_FLOAT);
 SetShaderValue(gSh, locEmis, &emis, SHADER_UNIFORM_VEC3);
 DrawMesh(m, gMat, w);
}

// point-inside test in a primitive's unit space (shrunk a hair so touching faces don't count)
static int Inside(const Prim *p, Vector3 w) {
 Vector3 u = Vector3Transform(w, p->inv);
 const float e = 0.035f;
 switch (p->mesh) {
 case 0: return fabsf(u.x) < 0.5f - e && fabsf(u.y) < 0.5f - e && fabsf(u.z) < 0.5f - e;
 case 1: return u.x * u.x + u.z * u.z < (1 - e) * (1 - e) && u.y > e && u.y < 1 - e;
 case 2: return u.x * u.x + u.y * u.y + u.z * u.z < (1 - e) * (1 - e);
 default: { float r = (1 - u.y) * (1 - e); return u.y > e && u.y < 1 - e && u.x * u.x + u.z * u.z < r * r; }
 }
}
static int SamplePts(const Prim *p, Vector3 *out) {
 int n = 0;
 if (p->mesh == 0) {
  for (int f = 0; f < 6; f++) for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
   float a = (i - 1) * 0.42f, b2 = (j - 1) * 0.42f, c = (f & 1) ? 0.5f : -0.5f;
   Vector3 u = f < 2 ? (Vector3){c, a, b2} : f < 4 ? (Vector3){a, c, b2} : (Vector3){a, b2, c};
   out[n++] = Vector3Transform(u, p->m);
  }
 } else if (p->mesh == 2) {
  for (int i = 0; i < 26; i++) { int x = i % 3 - 1, y = (i / 3) % 3 - 1, z = i / 9 - 1; if (!x && !y && !z) continue; Vector3 u = Vector3Normalize((Vector3){(float)x, (float)y, (float)z}); out[n++] = Vector3Transform(u, p->m); }
 } else {
  for (int h = 0; h < 4; h++) for (int k = 0; k < 10; k++) {
   float y = 0.08f + 0.84f * h / 3.f, an = k * 2 * PI / 10, r = p->mesh == 3 ? 1 - y : 1;
   out[n++] = Vector3Transform((Vector3){cosf(an) * r, y, sinf(an) * r}, p->m);
  }
  out[n++] = Vector3Transform((Vector3){0, 0.02f, 0}, p->m);
  out[n++] = Vector3Transform((Vector3){0, p->mesh == 3 ? 0.9f : 0.98f, 0}, p->m);
 }
 return n;
}
static int AabbHit(const Prim *a, const Prim *b) { return a->lo.x < b->hi.x && a->hi.x > b->lo.x && a->lo.y < b->hi.y && a->hi.y > b->lo.y && a->lo.z < b->hi.z && a->hi.z > b->lo.z; }
// conflict matrix between groups: number of sampled points of A inside B
static int Conflicts(int out[G_N][G_N]) {
 memset(out, 0, sizeof(int) * G_N * G_N);
 int tot = 0;
 Vector3 pts[64];
 for (int i = 0; i < gNPrim; i++) {
  Prim *a = &gPrim[i];
  int np = -1;
  for (int j = 0; j < gNPrim; j++) {
   Prim *b = &gPrim[j];
   if ((gAllow[a->group] >> b->group) & 1) continue;
   if (!AabbHit(a, b)) continue;
   if (np < 0) np = SamplePts(a, pts);
   int c = 0;
   for (int k = 0; k < np; k++) if (Inside(b, pts[k])) c++;
   if (c) { out[a->group][b->group] += c; tot += c; }
  }
 }
 return tot;
}

static int PathHits(const Vector3 *pts, int n, float rad, int group) {
 if (!gRecord) return 0;
 int hits = 0;
 for (int s = 0; s + 1 < n; s++) {
  Vector3 a = pts[s], b = pts[s + 1];
  float len = Vector3Distance(a, b);
  int k = (int)(len / 12.f) + 1;
  Vector3 d = Vector3Scale(Vector3Subtract(b, a), 1.f / fmaxf(len, 1e-3f));
  Vector3 u = fabsf(d.y) < 0.9f ? Vector3Normalize(Vector3CrossProduct(d, (Vector3){0, 1, 0})) : (Vector3){1, 0, 0};
  Vector3 v = Vector3CrossProduct(d, u);
  for (int i = 0; i <= k; i++) {
   Vector3 c = Vector3Lerp(a, b, (float)i / k);
   for (int j = 0; j < 5; j++) {
    Vector3 p = j == 0 ? c : Vector3Add(c, Vector3Scale(j < 3 ? u : v, (j & 1 ? 1 : -1) * rad * 0.9f));
    p = Vector3Transform(p, gBase);
    for (int q = 0; q < gNPrim; q++) {
     Prim *pr = &gPrim[q];
     if ((gAllow[group] >> pr->group) & 1) continue;
     if (p.x < pr->lo.x || p.x > pr->hi.x || p.y < pr->lo.y || p.y > pr->hi.y || p.z < pr->lo.z || p.z > pr->hi.z) continue;
     if (Inside(pr, p)) { hits++; break; }
    }
   }
  }
 }
 return hits;
}

// conflicts between prims added since `start` and everything before (both directions)
static int NewConflicts(int start) {
 int hits = 0; Vector3 pts[64];
 for (int i = start; i < gNPrim; i++) for (int j = 0; j < start; j++) {
  Prim *a = &gPrim[i], *b = &gPrim[j];
  if ((gAllow[a->group] >> b->group) & 1) continue;
  if (!AabbHit(a, b)) continue;
  int n = SamplePts(a, pts); for (int k = 0; k < n; k++) if (Inside(b, pts[k])) hits++;
  n = SamplePts(b, pts); for (int k = 0; k < n; k++) if (Inside(a, pts[k])) hits++;
 }
 return hits;
}

static void Box(Vector3 c, Vector3 sz, Matrix rot, Color col, float metal) {
 Put(mCube, MatrixMultiply(MatrixMultiply(MatrixScale(sz.x, sz.y, sz.z), rot), MatrixTranslate(c.x, c.y, c.z)), col, metal, gNoEmis);
}
static Matrix AlignY(Vector3 dn) {
 Vector3 up = {0, 1, 0};
 Vector3 ax = Vector3CrossProduct(up, dn);
 float s = Vector3Length(ax), c = Vector3DotProduct(up, dn);
 if (s < 1e-6f) return c > 0 ? MatrixIdentity() : MatrixRotateX(PI);
 return MatrixRotate(Vector3Scale(ax, 1.f / s), atan2f(s, c));
}
static void CylE(Vector3 a, Vector3 b, float r, Color col, float metal, Vector3 emis) {
 Vector3 d = Vector3Subtract(b, a);
 float len = Vector3Length(d);
 if (len < 1e-3f) return;
 Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(r, len, r), AlignY(Vector3Scale(d, 1.f / len))), MatrixTranslate(a.x, a.y, a.z));
 Put(mCyl, m, col, metal, emis);
}
static void Cyl(Vector3 a, Vector3 b, float r, Color col, float metal) { CylE(a, b, r, col, metal, gNoEmis); }
static void ConeP(Vector3 a, Vector3 b, float r, Color col, float metal) {
 Vector3 d = Vector3Subtract(b, a);
 float len = Vector3Length(d);
 if (len < 1e-3f) return;
 Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(r, len, r), AlignY(Vector3Scale(d, 1.f / len))), MatrixTranslate(a.x, a.y, a.z));
 Put(mCone, m, col, metal, gNoEmis);
}
static void SphE(Vector3 c, Vector3 r, Color col, float metal, Vector3 emis) {
 Put(mSph, MatrixMultiply(MatrixScale(r.x, r.y, r.z), MatrixTranslate(c.x, c.y, c.z)), col, metal, emis);
}
static void Sph(Vector3 c, float r, Color col, float metal) { SphE(c, (Vector3){r, r, r}, col, metal, gNoEmis); }
static void Taper(Vector3 a, Vector3 b, float r0, float r1, Color col, float metal, Vector3 emis) {
 int n = 8;
 for (int i = 0; i < n; i++) {
  float t0 = (float)i / n, t1 = (float)(i + 1) / n;
  Vector3 p0 = Vector3Lerp(a, b, t0), p1 = Vector3Lerp(a, b, t1 + 0.002f);
  CylE(p0, p1, Lerpf(r0, r1, (t0 + t1) * 0.5f), col, metal, emis);
 }
}
static void Pipe(Vector3 *pts, int n, float r, Color col, float metal, Vector3 emis) {
 for (int i = 0; i + 1 < n; i++) CylE(pts[i], pts[i + 1], r, col, metal, emis);
 for (int i = 1; i + 1 < n; i++) SphE(pts[i], (Vector3){r, r, r}, col, metal, emis);
}

typedef struct {
 int n, nb, m;
 float x[16], a[16]; int bank[16]; float tdc[16];
 float r, l, deck, v0, headH, headW, pitch, L, ccW, ccTop, ccBot, panH, H, jw, flyR, flyT;
 float bankA[2], bx0[2], bx1[2];
 Vector3 inP[16], exP[16], inD[16], exD[16];
 Vector3 exhOut[2], intakeTop, fanC; float fanR, pulR, pulX;
 int nOut, hoseSel[2]; float laneSide[2];
 int custom, inSide, itb; Vector3 tankP; float bedH, lowY, stTop, is[16]; char attachErr[96];
 int hasTurbo, oilFlip; Vector3 turboPos; float turboK;
 // solver offsets for movable groups
 Vector3 offOil, offAlt, offStarter, offTurbo, offFilter, offRad, offLane[2], offSC, offCharge;
 int solveIters, solveLeft;
} Geo;
static Geo G;

static Vector3 BP(float a, float x, float v, float w) { return (Vector3){x, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }
static Vector3 BD(float a, float v, float w) { return (Vector3){0, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }
static Color Shade(Color c, float k) { return (Color){(unsigned char)Clampf(c.r * k, 0, 255), (unsigned char)Clampf(c.g * k, 0, 255), (unsigned char)Clampf(c.b * k, 0, 255), c.a}; }

static const Color C_STEEL = {150, 152, 158, 255}, C_DARK = {38, 40, 44, 255}, C_BLACK = {24, 25, 28, 255}, C_CHROME = {225, 228, 235, 255};
static const Color C_RUBBER = {20, 20, 22, 255}, C_PISTON = {200, 200, 205, 255};

typedef struct { int cut; float fanAng; float vis; float glow; float pullT; float span; int oilView; float oilSump, oilHead, oilNeed, oilSupply, oilFlow, oilT; float covMain, covRod, covWall, covHead, covTurbo; } DrawState;

static void GeoBase(const Build *b) {
 Geo *g = &G;
 memset(g, 0, sizeof *g);
 int custom = b->layout == 5;
 float fire[16];
 FireAngles(b, fire);
 int n = CylGeom(b, g->x, g->a, g->bank);
 g->n = n; g->custom = custom;
 float tlt = b->tilt * DEG2RAD, half = LAYOUT_HALF[b->layout] * DEG2RAD;
 float wallX = (WallMM(b) - 8.f);
 g->r = b->stroke / 2; g->l = 1.75f * b->stroke;
 g->pitch = CylPitch(b);
 g->deck = g->r + g->l + 0.25f * b->bore + 1;
 g->v0 = g->r * 1.25f + 10;
 g->headH = (b->cycle ? 0.42f : VT4[b->valvetrain].hgt) * b->bore;
 g->headW = b->bore * 1.35f;
 g->jw = b->bore * 1.28f + wallX * 1.5f;
 for (int i = 0; i < n; i++) g->tdc[i] = fire[i];
 if (custom) {
  int s0 = 0, s1 = 0; float a0 = 0, a1 = 0;
  for (int i = 0; i < n; i++) { if (g->bank[i]) { s1++; a1 += g->a[i]; } else { s0++; a0 += g->a[i]; } }
  if (!s0) for (int i = 0; i < n; i++) g->bank[i] = 0;
  g->nb = (s0 && s1) ? 2 : 1;
  g->bankA[0] = s0 ? a0 / s0 : a1 / fmaxf(s1, 1); g->bankA[1] = s1 ? a1 / s1 : 0;
  float xmin = 1e9f, xmax = -1e9f;
  for (int i = 0; i < n; i++) { xmin = fminf(xmin, g->x[i]); xmax = fmaxf(xmax, g->x[i]); }
  g->L = (xmax - xmin) + g->pitch * 1.45f;
  g->m = n;
 } else {
  g->nb = b->layout >= 2 ? 2 : 1; g->m = g->nb == 2 ? (n + 1) / 2 : n;
  g->L = g->m * g->pitch + 0.45f * g->pitch + (g->nb == 2 ? 0.25f * g->pitch : 0);
  g->bankA[0] = (g->nb == 2 ? half : 0) + tlt; g->bankA[1] = -half + tlt;
 }
 for (int bk = 0; bk < 2; bk++) { g->bx0[bk] = 1e9f; g->bx1[bk] = -1e9f; }
 for (int i = 0; i < n; i++) { int bk = g->bank[i]; g->bx0[bk] = fminf(g->bx0[bk], g->x[i] - g->pitch * 0.5f); g->bx1[bk] = fmaxf(g->bx1[bk], g->x[i] + g->pitch * 0.5f); }
 float ccWi = 2 * g->r + b->bore * 0.9f + 20;
 if (custom) {
  float sw = 0, top = g->r * 1.3f + b->bore * 0.3f, bot = -(g->r * 1.35f + 12);
  for (int i = 0; i < n; i++) { sw = fmaxf(sw, fabsf(sinf(g->a[i]))); top = fmaxf(top, g->v0 * cosf(g->a[i]) + b->bore * 0.35f); bot = fminf(bot, g->v0 * cosf(g->a[i]) - b->bore * 0.35f); }
  g->ccW = fmaxf(2 * sw * g->v0 + b->bore * 1.25f, ccWi); g->ccTop = top; g->ccBot = bot;
 } else {
  if (b->layout <= 1) { g->ccW = fmaxf(ccWi, g->jw + 14) + 2 * fabsf(sinf(tlt)) * g->v0 * 0.8f; g->ccTop = g->v0 * fmaxf(cosf(tlt), 0.35f) + 10 + fabsf(sinf(tlt)) * b->bore * 0.3f; }
  else if (b->layout <= 3) { g->ccW = fmaxf(2 * sinf(half) * g->v0 + b->bore * 1.25f, ccWi); g->ccTop = g->v0 * cosf(half) + b->bore * 0.35f; }
  else { g->ccW = 2 * g->v0 + 24; g->ccTop = g->r * 1.3f + b->bore * 0.45f; }
  g->ccBot = -(g->r * 1.35f + 12);
 }
 g->ccW += wallX * 2;
 if (b->skirt == 1) g->ccBot += g->r * 0.2f;
 if (b->skirt == 2) g->ccBot -= g->r * 0.25f + 6;
 g->bedH = b->skirt == 3 ? g->r * 0.35f + 10 : 0;
 g->panH = b->cycle ? 0 : (30 + b->bore * 0.5f) * (b->lube == 2 ? 0.45f : 1.f);
 float fw = FLY[b->flywheel].magneto ? 0.85f : (FLY[b->flywheel].mul > 1.5f ? 1.15f : 1.f);
 g->flyR = (b->stroke * 0.9f + 70) * fw;
 g->flyT = 18 * FLY[b->flywheel].mul + 8;
 for (int i = 0; i < n; i++) {
  float a = g->a[i];
  float is = (a > 0.01f) ? -1.f : 1.f;
  if (custom && b->cFlip[i]) is = -is;
  float vp = g->deck + g->headH * 0.48f, ve = g->deck + g->headH * (b->cycle ? 0.4f : 0.3f);
  g->inP[i] = BP(a, g->x[i], vp, is * g->headW * 0.5f);
  g->inD[i] = BD(a, 0, is);
  g->exP[i] = BP(a, g->x[i], ve, -is * g->headW * 0.5f);
  g->exD[i] = BD(a, 0, -is);
  g->is[i] = is;
 }
 g->lowY = 1e9f; g->stTop = -1e9f;
 if (custom) for (int i = 0; i < n; i++) for (int sd = -1; sd <= 1; sd += 2) g->lowY = fminf(g->lowY, BP(g->a[i], 0, g->deck + g->headH + 14, sd * g->headW * 0.5f).y);
 if (custom && b->lube == 2 && g->lowY < g->ccBot - g->panH) g->panH = 0;
 if (custom) {
  // bank = which side the exhaust exits, so collectors stay on one side each
  int s0 = 0, s1 = 0; float a0 = 0, a1 = 0;
  for (int i = 0; i < n; i++) { g->bank[i] = g->exD[i].z >= 0 ? 0 : 1; if (g->bank[i]) { s1++; a1 += g->a[i]; } else { s0++; a0 += g->a[i]; } }
  if (!s0) for (int i = 0; i < n; i++) g->bank[i] = 0;
  g->nb = (s0 && s1) ? 2 : 1;
  g->bankA[0] = s0 ? a0 / s0 : a1 / fmaxf(s1, 1); g->bankA[1] = s1 ? a1 / s1 : 0;
  for (int bk = 0; bk < 2; bk++) { g->bx0[bk] = 1e9f; g->bx1[bk] = -1e9f; }
  for (int i = 0; i < n; i++) { int bk = g->bank[i]; g->bx0[bk] = fminf(g->bx0[bk], g->x[i] - g->pitch * 0.5f); g->bx1[bk] = fmaxf(g->bx1[bk], g->x[i] + g->pitch * 0.5f); }
 }
}

static void DrawFilter(int type, Vector3 at, Vector3 dir, float s) {
 int sg = gGroup; gGroup = G_FILTER;
 Vector3 e = Vector3Add(at, Vector3Scale(dir, 6));
 switch (type) {
 case 0: Cyl(at, Vector3Add(at, Vector3Scale(dir, 42 * s)), 22 * s, C_CHROME, 0.9f); ConeP(Vector3Add(at, Vector3Scale(dir, 60 * s)), Vector3Add(at, Vector3Scale(dir, 30 * s)), 34 * s, C_CHROME, 0.9f); break;
 case 1: Cyl(e, Vector3Add(e, Vector3Scale(dir, 55 * s)), 105 * s, C_BLACK, 0.4f);
         Cyl(Vector3Add(e, Vector3Scale(dir, 50 * s)), Vector3Add(e, Vector3Scale(dir, 60 * s)), 60 * s, C_CHROME, 0.9f); break;
 case 2: Cyl(at, e, 22 * s, C_DARK, 0.3f); SphE(Vector3Add(e, Vector3Scale(dir, 45 * s)), (Vector3){55 * s, 55 * s, 55 * s}, (Color){230, 120, 30, 255}, 0.0f, gNoEmis); break;
 case 3: Cyl(at, Vector3Add(e, Vector3Scale(dir, 14 * s)), 50 * s, C_CHROME, 0.9f);
         ConeP(Vector3Add(e, Vector3Scale(dir, 12 * s)), Vector3Add(e, Vector3Scale(dir, 150 * s)), 62 * s, (Color){190, 30, 35, 255}, 0.1f); break;
 case 4: Cyl(at, Vector3Add(e, Vector3Scale(dir, 150 * s)), 70 * s, C_BLACK, 0.3f);
         Sph(Vector3Add(e, Vector3Scale(dir, 150 * s)), 70 * s, C_BLACK, 0.3f); break;
 }
 gGroup = sg;
}

static void DrawTurbo(Vector3 c, float k, Vector3 *compOut, Vector3 *compIn, Vector3 *turbIn, Vector3 *turbOut) {
 SphE(c, (Vector3){45 * k, 62 * k, 62 * k}, (Color){90, 80, 72, 255}, 0.5f, gNoEmis);
 Cyl((Vector3){c.x + 20 * k, c.y, c.z}, (Vector3){c.x + 62 * k, c.y, c.z}, 26 * k, C_STEEL, 0.6f);
 Cyl((Vector3){c.x + 60 * k, c.y, c.z}, (Vector3){c.x + 112 * k, c.y, c.z}, 60 * k, (Color){200, 202, 206, 255}, 0.8f);
 Cyl((Vector3){c.x + 110 * k, c.y, c.z}, (Vector3){c.x + 135 * k, c.y, c.z}, 36 * k, (Color){200, 202, 206, 255}, 0.8f);
 *compIn = (Vector3){c.x + 133 * k, c.y, c.z};
 *compOut = (Vector3){c.x + 86 * k, c.y + 56 * k, c.z};
 *turbIn = (Vector3){c.x, c.y - 30 * k, c.z};
 *turbOut = (Vector3){c.x - 40 * k, c.y, c.z};
}

static void DrawOil(const Build *b, const Spec *s, float crankDeg, const DrawState *ds);
static void DrawSpinOnce(const Build *b, float crankDeg, const DrawState *ds, unsigned char al) {
 Geo *g = &G;
 float rr = g->r, frontX = -g->L * 0.5f, rearX = g->L * 0.5f;
 Color steel = C_STEEL, chrome = C_CHROME, pist = C_PISTON, rod = {170, 172, 178, 255}, white = {240, 240, 240, 255}, blk = C_BLACK;
 steel.a = chrome.a = pist.a = rod.a = white.a = blk.a = al;
 Color steel2 = Shade(C_STEEL, 0.85f); steel2.a = al;
 if (ds->cut) {
  gGroup = G_ROT;
  Cyl((Vector3){frontX + 3, 0, 0}, (Vector3){rearX - 3, 0, 0}, fmaxf(rr * 0.45f, 9), steel, 0.8f);
  for (int i = 0; i < g->n; i++) {
   float a = g->a[i];
   float beta = (crankDeg - g->tdc[i]) * DEG2RAD + a;
   float th = beta - a;
   Vector3 pin = {g->x[i], rr * cosf(beta), rr * sinf(beta)};
   float sv = rr * cosf(th) + sqrtf(fmaxf(g->l * g->l - rr * rr * sinf(th) * sinf(th), 1));
   Vector3 pp = BP(a, g->x[i], sv, 0);
   float cw = g->nb == 2 ? fminf(b->bore * 0.22f, g->pitch * 0.105f) : b->bore * 0.22f;
   Matrix Rc = MatrixRotateX(beta);
   Box(BP(beta, g->x[i] - cw * 0.78f, rr * 0.4f, 0), (Vector3){cw * 0.45f, rr * 1.9f, rr * 0.9f}, Rc, steel, 0.7f);
   Box(BP(beta, g->x[i] + cw * 0.78f, rr * 0.4f, 0), (Vector3){cw * 0.45f, rr * 1.9f, rr * 0.9f}, Rc, steel, 0.7f);
   Box(BP(beta, g->x[i], -rr * 0.8f, 0), (Vector3){cw * 1.6f, rr * 0.7f, rr * 1.7f}, Rc, steel2, 0.7f);
   Cyl((Vector3){g->x[i] - cw * 0.93f, pin.y, pin.z}, (Vector3){g->x[i] + cw * 0.93f, pin.y, pin.z}, fmaxf(rr * 0.32f, 6), chrome, 0.9f);
   Cyl(pin, pp, fmaxf(b->bore * 0.075f, 4), rod, 0.8f);
   Cyl(BP(a, g->x[i], sv - b->bore * 0.3f, 0), BP(a, g->x[i], sv + b->bore * 0.25f, 0), b->bore * 0.49f, pist, 0.75f);
  }
 }
 gGroup = G_FRONT;
 if (g->pulR > 0) {
  Matrix Rs = MatrixRotateX(crankDeg * DEG2RAD);
  Box(BP(crankDeg * DEG2RAD, g->pulX - 1.f, g->pulR * 0.75f, 0), (Vector3){6, g->pulR * 0.35f, 5}, Rs, white, 0);
 }
 gGroup = G_FLY;
 float fx0 = rearX + 12;
 for (int k = 0; k < 6; k++) {
  float an = crankDeg * DEG2RAD + k * PI / 3;
  Vector3 bp = {fx0 + g->flyT + 1.5f, cosf(an) * g->flyR * 0.6f, sinf(an) * g->flyR * 0.6f};
  Cyl((Vector3){bp.x - 3, bp.y, bp.z}, bp, 6, chrome, 0.9f);
 }
 Box((Vector3){fx0 + g->flyT + 0.5f, cosf(crankDeg * DEG2RAD) * g->flyR * 0.85f, sinf(crankDeg * DEG2RAD) * g->flyR * 0.85f}, (Vector3){5, 8, 8}, MatrixRotateX(crankDeg * DEG2RAD), white, 0);
 if (b->cooling > 0 && g->fanR > 0) {
  gGroup = G_COOL;
  Vector3 fc = g->fanC; float fr = g->fanR;
  for (int k = 0; k < 5; k++) {
   float an = crankDeg * DEG2RAD * 0.9f + k * 2 * PI / 5;
   Matrix rb = MatrixMultiply(MatrixRotateY(0.35f), MatrixRotateX(an));
   Box(Vector3Add(fc, (Vector3){0, cosf(an) * fr * 0.55f, sinf(an) * fr * 0.55f}), (Vector3){4, fr * 0.8f, fr * 0.28f}, rb, blk, 0.3f);
  }
 }
}
static void DrawSpinning(const Build *b, float crankDeg, const DrawState *ds) {
 float span = fminf(ds->span, 720.f);
 if (span < 18.f || gSolving) { DrawSpinOnce(b, crankDeg, ds, 255); return; }
 int n = (int)ceilf(span / 10.f); if (n > 24) n = 24; if (n < 3) n = 3;
 float a = 1 - powf(0.12f, 1.f / n);
 rlDisableDepthMask();
 for (int s = 0; s < n; s++) DrawSpinOnce(b, crankDeg - span * s / n, ds, (unsigned char)(a * 255));
 rlEnableDepthMask();
}

static float G_drainVis = 1;

// ===== oil flow view =====
// pressure-fed: particles ride gallery paths; splash: droplets bounce inside the crankcase and film back down
#define NOILP 420
typedef struct { int path, state; float t, spd, life; Vector3 p, v; } OilP;
static OilP gOil[NOILP];
enum { OP_FEED, OP_MAIN, OP_SQUIRT, OP_RISE, OP_DRAIN };
static Vector3 gOilPath[28][8]; static int gOilPathN[28], gOilKind[28], gNOilPaths = 0;
static Vector3 gOilFilter;
static float OilPoolY(const Build *b, const Spec *s, float sump) {
 Geo *g = &G;
 float cap = fmaxf(s->oilMax, 0.1f), lvl = Clampf(sump / cap, 0, 1);
 if (g->panH > 0) return g->ccBot - g->panH + 3 + (g->panH - 6) * lvl;
 return g->ccBot + 2 + g->r * 0.7f * lvl;
}
static void AddOilPath(int kind, Vector3 *pts, int n) { if (gNOilPaths >= 28) return; memcpy(gOilPath[gNOilPaths], pts, sizeof(Vector3) * n); gOilPathN[gNOilPaths] = n; gOilKind[gNOilPaths++] = kind; }
static void OilPathsBuild(const Build *b) {
 Geo *g = &G; float frontX = -g->L * 0.5f, rearX = g->L * 0.5f, rr = g->r;
 gNOilPaths = 0;
 memset(gOil, 0, sizeof gOil);
 if (b->lube == 0 || b->cycle) return;
 float pickY = g->ccBot - g->panH * 0.75f, galZ = -g->ccW * 0.36f, galY = g->ccBot * 0.35f;
 gOilFilter = (Vector3){frontX + g->L * 0.25f, galY - 10, -g->ccW * 0.5f - 40};
 Vector3 feed[6] = {{0, pickY, 0}, {frontX + 30, pickY, 0}, {frontX + 30, g->ccBot - 8, 0}, {frontX + 30, galY, galZ}, {gOilFilter.x, galY - 10, -g->ccW * 0.5f - 18}, {gOilFilter.x + 30, galY, galZ}};
 AddOilPath(OP_FEED, feed, 6);
 Vector3 gal[2] = {{gOilFilter.x + 30, galY, galZ}, {rearX - 24, galY, galZ}};
 AddOilPath(OP_FEED, gal, 2);
 for (int i = 0; i < g->n; i += (g->nb == 2 ? 2 : 1)) {
  float x = g->x[i];
  Vector3 mn[3] = {{x, galY, galZ}, {x, -rr * 0.35f, -rr * 0.4f}, {x, 0, 0}};
  AddOilPath(OP_MAIN, mn, 3);
  Vector3 sq[2] = {{x, galY, galZ}, {x, galY + 6, galZ * 0.5f}};
  AddOilPath(OP_SQUIRT, sq, 2);
 }
 for (int bk = 0; bk < g->nb; bk++) {
  float a = g->bankA[bk], x0 = g->bx0[bk] + 14, x1 = g->bx1[bk] - 14, side = (a > 0.01f) ? 1.f : -1.f;
  Vector3 up[5] = {{x0, galY, galZ}, BP(a, x0, g->v0 + 6, -side * g->jw * 0.42f), BP(a, x0, g->deck + g->headH * 0.55f, -side * g->headW * 0.3f), BP(a, (x0 + x1) * 0.5f, g->deck + g->headH * 0.6f, -side * g->headW * 0.3f), BP(a, x1, g->deck + g->headH * 0.6f, -side * g->headW * 0.3f)};
  AddOilPath(OP_RISE, up, 5);
  Vector3 dr[4] = {BP(a, x1, g->deck + g->headH * 0.6f, side * g->headW * 0.3f), BP(a, x1, g->deck + 4, side * g->headW * 0.38f), BP(a, x1, g->v0, side * g->jw * 0.42f), {x1, g->ccBot - g->panH * 0.5f, side * g->ccW * 0.3f}};
  AddOilPath(OP_DRAIN, dr, 4);
 }
}
static float PathLen(int pi) { float L = 0; for (int k = 0; k + 1 < gOilPathN[pi]; k++) L += Vector3Distance(gOilPath[pi][k], gOilPath[pi][k + 1]); return L; }
static Vector3 OilPathAt(int pi, float t) {
 int n = gOilPathN[pi]; Vector3 *p = gOilPath[pi];
 float d = t * PathLen(pi);
 for (int i = 0; i + 1 < n; i++) { float l = Vector3Distance(p[i], p[i + 1]); if (d <= l) return Vector3Lerp(p[i], p[i + 1], d / fmaxf(l, 1e-3f)); d -= l; }
 return p[n - 1];
}
static float Rnd(void) { return (float)GetRandomValue(0, 10000) / 10000.f; }
static int InsideCase(Vector3 p, float poolY) {
 Geo *g = &G;
 if (p.x > -g->L * 0.5f + 4 && p.x < g->L * 0.5f - 4 && fabsf(p.z) < g->ccW * 0.5f - 4 && p.y > poolY && p.y < g->ccTop - 3) return 1;
 for (int i = 0; i < g->n; i++) {
  Vector3 d = Vector3Subtract(p, (Vector3){g->x[i], 0, 0});
  float a = g->a[i];
  float v = d.y * cosf(a) + d.z * sinf(a), w = -d.y * sinf(a) + d.z * cosf(a);
  if (v > g->v0 - 10 && v < g->deck && sqrtf(d.x * d.x + w * w) < 0.48f * (G.headW / 1.35f)) return 2;
 }
 return 0;
}
static void OilUpdate(const Build *b, const Spec *s, const DrawState *ds, float rpm, float crankDeg, float dt) {
 Geo *g = &G;
 float poolY = OilPoolY(b, s, ds->oilSump);
 if (b->lube == 0 || b->cycle) {
  // splash: dipper on each rod scoops the pool and flings droplets around inside the case
  float reach = b->cycle ? 1 : ds->oilSupply;
  float rate = rpm / 60.f * g->n * (b->cycle ? 3.f : 7.f) * reach * dt;
  int spawn = (int)rate + (Rnd() < rate - (int)rate ? 1 : 0);
  for (int i = 0; i < NOILP && spawn > 0; i++) {
   OilP *o = &gOil[i];
   if (o->life > 0) continue;
   int c = GetRandomValue(0, g->n - 1);
   float beta = (crankDeg - g->tdc[c]) * DEG2RAD + g->a[c];
   float an = beta + PI + (Rnd() - 0.5f) * 0.6f;
   o->p = (Vector3){g->x[c] + (Rnd() - 0.5f) * 10, fmaxf(g->r * cosf(an) * 0.9f, poolY + 2), g->r * sinf(an) * 0.9f};
   float sp = Clampf(rpm * 0.18f, 150, 900);
   Vector3 tang = {0, -sinf(an), cosf(an)};
   o->v = Vector3Add(Vector3Scale(tang, sp * (Rnd() > 0.5f ? 1 : -1)), (Vector3){(Rnd() - 0.5f) * 120, sp * 0.6f, 0});
   o->life = 3; o->state = 0; spawn--;
  }
  for (int i = 0; i < NOILP; i++) {
   OilP *o = &gOil[i];
   if (o->life <= 0) continue;
   o->life -= dt;
   if (o->state == 0) {
    o->v.y -= 3000 * dt;
    Vector3 np = Vector3Add(o->p, Vector3Scale(o->v, dt));
    if (!InsideCase(np, poolY - 1)) { o->state = 1; o->v = (Vector3){0, -40 - Rnd() * 50, 0}; if (!InsideCase(o->p, poolY - 1)) o->life = 0; }
    else o->p = np;
   } else {
    Vector3 np = Vector3Add(o->p, Vector3Scale(o->v, dt));
    if (InsideCase(np, poolY - 1)) o->p = np; else o->life = 0;
    if (o->p.y <= poolY + 1) o->life = 0;
   }
  }
  return;
 }
 if (gNOilPaths == 0) return;
 float spd = Clampf(ds->oilFlow * 6.f, 0, 2.2f);
 for (int i = 0; i < NOILP; i++) {
  OilP *o = &gOil[i];
  if (o->life <= 0) {
   o->path = i % gNOilPaths; o->t = Rnd(); o->life = 1; o->spd = 0.75f + 0.5f * Rnd(); o->state = 0;
  }
  int kind = gOilKind[o->path];
  float mul = kind == OP_DRAIN ? Clampf(G_drainVis, 0.03f, 1) * 0.6f : (kind == OP_SQUIRT ? 2.2f : 1.f);
  if (kind == OP_DRAIN && spd < 0.05f && rpm < 50) mul = 0.15f * Clampf(G_drainVis, 0.03f, 1);
  o->t += (kind == OP_DRAIN ? fmaxf(spd, 0.25f) : spd) * mul * o->spd * 260.f / fmaxf(PathLen(o->path), 1) * dt;
  if (o->t >= 1) o->t -= 1;
  o->p = OilPathAt(o->path, o->t);
  if (kind == OP_SQUIRT) { float k = o->t; o->p.y += k * k * 60; }
 }
}
static void DrawOil(const Build *b, const Spec *s, float crankDeg, const DrawState *ds) {
 Geo *g = &G;
 int sg = gGroup; gGroup = G_ROT;
 Color amber = {226, 158, 34, 255}, deep = {150, 92, 18, 255}, tube = {120, 78, 20, 255}, air = {240, 240, 245, 255};
 float hot = Clampf((ds->oilT - 40) / 100.f, 0, 1);
 amber = (Color){(unsigned char)Lerpf(226, 160, hot), (unsigned char)Lerpf(158, 90, hot), (unsigned char)Lerpf(34, 20, hot), 255};
 float poolY = OilPoolY(b, s, ds->oilSump);
 float floorY = g->panH > 0 ? g->ccBot - g->panH + 3 : g->ccBot + 2;
 if (poolY - floorY > 0.5f) {
  float w = g->panH > 0 ? g->ccW * 0.78f : g->ccW * 0.9f, l = g->panH > 0 ? g->L * 0.8f : g->L * 0.94f;
  Box((Vector3){0, (poolY + floorY) * 0.5f, 0}, (Vector3){l, poolY - floorY, w}, MatrixIdentity(), deep, 0.25f);
 }
 if (b->lube > 0 && !b->cycle) {
  for (int p = 0; p < gNOilPaths; p++) for (int k = 0; k + 1 < gOilPathN[p]; k++)
   Cyl(gOilPath[p][k], gOilPath[p][k + 1], gOilKind[p] == OP_DRAIN ? 5.f : 3.5f, tube, 0.3f);
  for (int p = 0; p < gNOilPaths; p++) for (int k = 1; k + 1 < gOilPathN[p]; k++) Sph(gOilPath[p][k], gOilKind[p] == OP_DRAIN ? 5.f : 3.5f, tube, 0.3f);
  Cyl((Vector3){g->L * -0.5f + 30, g->ccBot - 18, -20}, (Vector3){g->L * -0.5f + 30, g->ccBot + 14, -20}, 22, (Color){90, 92, 98, 255}, 0.6f);
  Cyl((Vector3){0, g->ccBot - g->panH + 5, 0}, (Vector3){0, g->ccBot - g->panH * 0.75f, 0}, 16, ds->oilSupply < 0.7f ? (Color){235, 70, 55, 255} : (Color){80, 82, 88, 255}, 0.5f);
 }
 if (ds->oilHead > 0.01f && b->lube > 0 && !b->cycle) {
  float hf = Clampf(ds->oilHead / fmaxf(s->oilHeadCap, 0.05f), 0, 1);
  for (int bk = 0; bk < g->nb; bk++) {
   float a = g->bankA[bk], xm = (g->bx0[bk] + g->bx1[bk]) * 0.5f, len = g->bx1[bk] - g->bx0[bk];
   float hh = g->headH * 0.8f * hf;
   if (hh > 1) Box(BP(a, xm, g->deck + 2 + hh * 0.5f, 0), (Vector3){len - 8, hh, g->headW * 0.8f}, MatrixRotateX(a), deep, 0.3f);
  }
 }
 if (b->lube == 0 && !b->cycle) {
  for (int i = 0; i < g->n; i++) {
   float a = g->a[i], beta = (crankDeg - g->tdc[i]) * DEG2RAD + a;
   Vector3 pin = {g->x[i] + b->bore * 0.12f, g->r * cosf(beta), g->r * sinf(beta)};
   Vector3 dir = Vector3Normalize((Vector3){0, cosf(beta), sinf(beta)});
   Cyl(pin, Vector3Add(pin, Vector3Scale(dir, g->r * 0.75f)), 3.5f, (Color){170, 172, 178, 255}, 0.8f);
  }
 }
// coverage heat: green well oiled, orange marginal, red dry
 {
#define COV(v) ((v) > 0.75f ? (Color){70, 210, 100, 255} : ((v) > 0.4f ? (Color){245, 150, 40, 255} : (Color){235, 55, 45, 255}))
  float rr = g->r;
  for (int i = 0; i < g->n; i += (g->nb == 2 ? 2 : 1)) {
   float x = g->x[i] + (g->nb == 2 ? g->pitch * 0.12f : 0), cw = b->bore * 0.12f;
   Cyl((Vector3){x - cw, 0, 0}, (Vector3){x + cw, 0, 0}, fmaxf(rr * 0.45f, 9) + 3, COV(ds->covMain), 0.2f);
  }
  for (int i = 0; i < g->n; i++) {
   float a = g->a[i], beta = (crankDeg - g->tdc[i]) * DEG2RAD + a, th = beta - a;
   Vector3 pin = {g->x[i], rr * cosf(beta), rr * sinf(beta)};
   float cw = g->nb == 2 ? fminf(b->bore * 0.22f, g->pitch * 0.105f) : b->bore * 0.22f;
   Cyl((Vector3){pin.x - cw * 0.5f, pin.y, pin.z}, (Vector3){pin.x + cw * 0.5f, pin.y, pin.z}, fmaxf(rr * 0.32f, 6) + 3, COV(ds->covRod), 0.2f);
   float sv = rr * cosf(th) + sqrtf(fmaxf(g->l * g->l - rr * rr * sinf(th) * sinf(th), 1));
   Cyl(BP(a, g->x[i], sv - b->bore * 0.28f, 0), BP(a, g->x[i], sv - b->bore * 0.05f, 0), b->bore * 0.5f, COV(ds->covWall), 0.2f);
  }
  if (!b->cycle) for (int bk = 0; bk < g->nb; bk++) {
   float a = g->bankA[bk], xm = (g->bx0[bk] + g->bx1[bk]) * 0.5f, len = g->bx1[bk] - g->bx0[bk];
   Box(BP(a, xm, g->deck + g->headH * 0.75f, 0), (Vector3){len - 12, g->headH * 0.35f, g->headW * 0.5f}, MatrixRotateX(a), COV(ds->covHead), 0.2f);
  }
  if (g->hasTurbo) { Vector3 c = g->turboPos; float k = g->turboK; Cyl((Vector3){c.x + 18 * k, c.y, c.z}, (Vector3){c.x + 64 * k, c.y, c.z}, 30 * k, COV(ds->covTurbo), 0.2f); }
#undef COV
 }
 for (int i = 0; i < NOILP; i++) {
  OilP *o = &gOil[i];
  if (o->life <= 0) continue;
  int bubbly = b->lube > 0 && !b->cycle && ds->oilSupply < 0.75f && (i % 5) < (int)((1 - ds->oilSupply) * 7);
  float rad = (b->lube == 0 || b->cycle) ? (o->state ? 2.2f : 2.8f) : 3.2f;
  Sph(o->p, rad, bubbly ? air : amber, 0.5f);
 }
 gGroup = sg;
}

static void DrawEngine(const Build *b, const Spec *s, float crankDeg, DrawState *ds) {
 Geo *g = &G;
 Color blk = BLOCKS[b->block].col;
 Color headC = Shade(blk, b->cooling == 0 ? 1.05f : 1.08f);
 Color cover = C_BLACK;
 if (!b->cycle) cover = b->valvetrain == 3 ? (Color){175, 28, 30, 255} : (b->valvetrain == 2 ? (Color){30, 32, 36, 255} : (b->valvetrain == 1 ? (Color){38, 72, 160, 255} : Shade(blk, 1.0f)));
 float glowT = Clampf((ds->glow - 320) / 520.f, 0, 1);
 Vector3 glow = {glowT * glowT * 0.95f, glowT * glowT * 0.28f, glowT * glowT * 0.05f};
 Color pipeC = Shade((Color){120, 112, 104, 255}, 1 - glowT * 0.4f);
 float frontX = -g->L * 0.5f, rearX = g->L * 0.5f;
 float rr = g->r;
 const StarterDef *st = &STARTERS[b->starter];
 int beltAlt = b->alternator >= 3;
 // ---- block
 gGroup = G_BLOCK;
 if (!ds->cut) Box((Vector3){0, (g->ccTop + g->ccBot) * 0.5f, 0}, (Vector3){g->L, g->ccTop - g->ccBot, g->ccW}, MatrixIdentity(), blk, 0.35f);
 else Box((Vector3){0, (g->ccTop + g->ccBot) * 0.5f, -g->ccW * 0.3f}, (Vector3){g->L, g->ccTop - g->ccBot, g->ccW * 0.4f}, MatrixIdentity(), blk, 0.35f);
 if ((b->layout == 0 || b->cycle) && !ds->cut) {
  float rr2 = fminf(g->ccTop - g->ccBot, g->ccW) * 0.5f;
  Cyl((Vector3){frontX - 6, (g->ccTop + g->ccBot) * 0.5f, 0}, (Vector3){rearX + 6, (g->ccTop + g->ccBot) * 0.5f, 0}, rr2 * 0.92f, Shade(blk, 1.1f), 0.5f);
 }
 if (g->bedH > 0 && !ds->cut) Box((Vector3){0, g->ccBot - g->bedH * 0.5f + 2, 0}, (Vector3){g->L - 6, g->bedH + 4, g->ccW - 6}, MatrixIdentity(), Shade(blk, 0.82f), 0.45f);
 if (g->panH > 0) Box((Vector3){0, g->ccBot - g->bedH - g->panH * 0.5f + 2, 0}, (Vector3){g->L * 0.82f, g->panH + 4, g->ccW * 0.8f}, MatrixIdentity(), b->block == 0 ? C_DARK : Shade(blk, 0.95f), 0.4f);
 for (int bk = 0; bk < g->nb && !g->custom; bk++) {
  float a = g->bankA[bk];
  Matrix R = MatrixRotateX(a);
  float x0 = g->bx0[bk], x1 = g->bx1[bk], xm = (x0 + x1) * 0.5f, len = x1 - x0;
  if (b->cooling > 0) {
   gGroup = G_BLOCK;
   if (!ds->cut) Box(BP(a, xm, (g->v0 + g->deck) * 0.5f, 0), (Vector3){len + 0.12f * g->pitch, g->deck - g->v0, g->jw}, R, blk, 0.35f);
   gGroup = G_HEAD;
   Box(BP(a, xm, g->deck + g->headH * 0.5f - 1, 0), (Vector3){len + 0.12f * g->pitch + 12, g->headH + 2, g->headW}, R, headC, 0.45f);
   if (!b->cycle && b->valvetrain > 0) {
    float ch = b->bore * (b->valvetrain == 1 ? 0.2f : 0.26f);
    float cl = len + 0.12f * g->pitch - 20;
    if (b->valvetrain == 3) {
     for (int sd = -1; sd <= 1; sd += 2)
      Box(BP(a, xm, g->deck + g->headH + ch * 0.5f - 2, sd * b->bore * 0.3f), (Vector3){cl, ch, b->bore * 0.42f}, R, cover, 0.35f);
    } else { float is0 = (a > 0.01f) ? -1.f : 1.f; Box(BP(a, xm, g->deck + g->headH + ch * 0.5f - 2, is0 * b->bore * 0.2f), (Vector3){cl, ch, b->bore * 0.6f}, R, cover, 0.35f); }
   }
  }
 }
 for (int i = 0; i < g->n; i++) {
  float a = g->a[i], x = g->x[i];
  Matrix R = MatrixRotateX(a);
  if (b->cooling > 0 && g->custom) {
   float pw = fminf(g->pitch * 0.92f, b->bore * 1.3f);
   gGroup = G_BLOCK;
   if (!ds->cut) Box(BP(a, x, (g->v0 + g->deck) * 0.5f, 0), (Vector3){pw, g->deck - g->v0, g->jw}, R, blk, 0.35f);
   gGroup = G_HEAD;
   Box(BP(a, x, g->deck + g->headH * 0.5f - 1, 0), (Vector3){pw + 8, g->headH + 2, g->headW}, R, headC, 0.45f);
   if (!b->cycle && b->valvetrain > 0) { float ch = b->bore * 0.24f, is0 = g->is[i]; Box(BP(a, x, g->deck + g->headH + ch * 0.5f - 2, b->valvetrain == 3 ? 0 : is0 * b->bore * 0.2f), (Vector3){pw - 10, ch, b->valvetrain == 3 ? b->bore * 0.8f : b->bore * 0.6f}, R, cover, 0.35f); }
  }
  if (b->cooling == 0) {
   gGroup = G_BLOCK;
   if (!ds->cut) {
    Cyl(BP(a, x, g->v0, 0), BP(a, x, g->deck, 0), b->bore * 0.56f, Shade(blk, 0.9f), 0.4f);
    float fr = fminf(b->bore * 0.8f, g->pitch * 0.62f);
    float off = (i & 1) ? 3.5f : 0;
    for (float v = g->v0 + 10 + off; v < g->deck - 3; v += 7) Cyl(BP(a, x, v, 0), BP(a, x, v + 2.4f, 0), fr, blk, 0.35f);
   }
   gGroup = G_HEAD;
   Box(BP(a, x, g->deck + g->headH * 0.5f - 1, 0), (Vector3){g->pitch * 0.86f, g->headH + 2, g->headW * 0.95f}, R, headC, 0.45f);
   for (float v = g->deck + 5; v < g->deck + g->headH - 2; v += 7) Box(BP(a, x, v, 0), (Vector3){g->pitch * 0.94f, 2.2f, g->headW * 1.28f}, R, Shade(blk, 1.0f), 0.35f);
   if (!b->cycle && b->valvetrain > 0) { float ch = b->bore * 0.2f, is0 = g->is[i]; Box(BP(a, x, g->deck + g->headH + ch * 0.5f - 2, b->valvetrain == 3 ? 0 : is0 * b->bore * 0.2f), (Vector3){g->pitch * 0.7f, ch, b->valvetrain == 3 ? b->bore * 0.8f : b->bore * 0.6f}, R, cover, 0.35f); }
  }
  if (!FUELS[b->fuel].diesel) {
   gGroup = G_PLUG;
   Vector3 p0, p1;
   {
    float top = g->deck + g->headH, is = g->is[i];
    float w = (b->cycle || b->valvetrain == 3 || b->valvetrain == 0) ? 0 : -is * b->bore * 0.36f;
    p0 = BP(a, x, top - 8, w); p1 = BP(a, x, top + 28, w);
   }
   Cyl(p0, p1, 6.5f, (Color){235, 235, 230, 255}, 0.1f);
   Cyl(p1, Vector3Add(p1, Vector3Scale(Vector3Normalize(Vector3Subtract(p1, p0)), 10)), 4.5f, C_STEEL, 0.7f);
  }
 }
 float topY = -1e9f;
 for (int bk = 0; bk < g->nb; bk++) for (int sd = -1; sd <= 1; sd += 2) topY = fmaxf(topY, BP(g->bankA[bk], 0, g->deck + g->headH + b->bore * 0.3f + 30, sd * g->headW * 0.5f).y);
 if (ds->cut) {
  gGroup = G_BLOCK;
  float xs[18]; int nxs = 0;
  for (int i = 0; i < g->n; i++) { int dup = 0; for (int k = 0; k < nxs; k++) if (fabsf(xs[k] - g->x[i]) < g->pitch * 0.4f) dup = 1; if (!dup && nxs < 16) xs[nxs++] = g->x[i]; }
  for (int i = 0; i < nxs; i++) for (int j = i + 1; j < nxs; j++) if (xs[j] < xs[i]) { float t = xs[i]; xs[i] = xs[j]; xs[j] = t; }
  float caps[20]; int nc = 0;
  caps[nc++] = -g->L * 0.5f + 12;
  for (int k = 0; k + 1 < nxs; k++) caps[nc++] = (xs[k] + xs[k + 1]) * 0.5f;
  caps[nc++] = g->L * 0.5f - 12;
  float jr = fmaxf(rr * 0.45f, 9), cw = 14 + b->caps * 3, cd = g->r * 1.9f + 30;
  int nbolt = b->caps == 0 ? 2 : 4;
  for (int k = 0; k < nc; k++) {
   Box((Vector3){caps[k], -jr - 13, 0}, (Vector3){cw, 26, cd}, MatrixIdentity(), Shade(blk, 0.75f), 0.5f);
   for (int q = 0; q < nbolt; q++) { float zz = (q - (nbolt - 1) * 0.5f) * (cd - 24) / fmaxf(nbolt - 1, 1); Cyl((Vector3){caps[k], -jr - 34, zz}, (Vector3){caps[k], -jr - 24, zz}, 4.5f, C_CHROME, 0.9f); }
   if (b->caps == 2) for (int sd = -1; sd <= 1; sd += 2) Cyl((Vector3){caps[k], -jr - 13, sd * (cd * 0.5f - 4)}, (Vector3){caps[k], -jr - 13, sd * (cd * 0.5f + 8)}, 4.5f, C_CHROME, 0.9f);
  }
 }
 // timing cover + ignition hardware
 if (!b->cycle && b->valvetrain > 0) {
  gGroup = G_FRONT;
  Color tcC = b->camDrive == 2 ? (Color){28, 28, 30, 255} : Shade(blk, 1.05f);
  float tw = b->camDrive == 1 ? b->bore * 0.95f : b->bore * 0.8f;
  for (int bk = 0; bk < g->nb && !g->custom; bk++) {
   float a = g->bankA[bk], top = g->deck + g->headH;
   float x0 = fminf(g->bx0[bk] - g->pitch * 0.06f - 6, frontX + 2);
   Box(BP(a, x0 - 4, top * 0.5f, 0), (Vector3){10, top, tw}, MatrixRotateX(a), tcC, 0.4f);
  }
 }
 if (!FUELS[b->fuel].diesel) {
  int ig = b->ignition ? b->ignition : (FLY[b->flywheel].magneto ? 3 : 2);
  if (ig == 2 && !b->cycle) {
   gGroup = G_HEAD;
   float a = g->bankA[0];
   Vector3 d0 = BP(a, g->bx1[0] - 10, g->deck + g->headH + 4, g->is[0] * g->headW * 0.3f);
   Vector3 d1 = Vector3Add(d0, (Vector3){0, 46, 0});
   Cyl(d0, d1, 15, C_DARK, 0.3f); Cyl(d1, Vector3Add(d1, (Vector3){0, 18, 0}), 26, (Color){30, 30, 32, 255}, 0.2f);
  }
  if (ig == 4) {
   gGroup = G_PLUG;
   for (int i = 0; i < g->n; i++) {
    float a = g->a[i], w = (b->cycle || b->valvetrain == 3 || b->valvetrain == 0) ? 0 : -g->is[i] * b->bore * 0.36f;
    Box(BP(a, g->x[i], g->deck + g->headH + 44, w), (Vector3){24, 34, 24}, MatrixRotateX(a), (Color){30, 30, 34, 255}, 0.3f);
   }
  }
 }
 // ---- front: pulley, belts, alternator, recoil
 gGroup = G_FRONT;
 float px0 = frontX - 30, px1 = frontX - 10;
 float pulR = rr * 0.8f + 28;
 int needPulley = beltAlt || b->cooling > 0 || b->forced == 4;
 g->pulR = needPulley ? pulR : 0; g->pulX = px0;
 if (needPulley) Cyl((Vector3){px1 + 6, 0, 0}, (Vector3){px0, 0, 0}, pulR, C_DARK, 0.6f);
 if (!st->elec) {
  float rx0 = needPulley ? px0 - 4 : frontX - 4;
  float rcR = fmaxf(pulR * 1.1f, 48 + b->stroke * 0.5f);
  Cyl((Vector3){rx0 + 2, 0, 0}, (Vector3){rx0 - 38, 0, 0}, rcR, (Color){200, 40, 35, 255}, 0.3f);
  Cyl((Vector3){rx0 - 36, 0, 0}, (Vector3){rx0 - 44, 0, 0}, rcR * 0.4f, C_DARK, 0.5f);
  float ext = ds->pullT > 0 ? 260 * sinf(Clampf(ds->pullT, 0, 1) * PI) : 0;
  Vector3 hole = {rx0 - 38, rcR * 0.62f, rcR * 0.42f};
  Vector3 hnd = {hole.x - 14 - ext * 0.6f, hole.y + ext * 0.45f, hole.z + ext * 0.4f};
  Cyl((Vector3){hole.x + 2, hole.y, hole.z}, hnd, 2.0f, (Color){230, 220, 190, 255}, 0);
  Cyl((Vector3){hnd.x - 6, hnd.y - 28, hnd.z}, (Vector3){hnd.x - 6, hnd.y + 28, hnd.z}, 9, C_BLACK, 0.2f);
  Cyl((Vector3){hnd.x + 1, hnd.y, hnd.z}, (Vector3){hnd.x - 7, hnd.y, hnd.z}, 4, C_BLACK, 0.2f);
 }
 if (beltAlt) {
  gGroup = G_ALT;
  float side = g->inD[0].z >= 0 ? 1.f : -1.f;
  Vector3 base = g->nb == 2 || b->layout == 4 || g->custom ? (Vector3){0, g->ccTop + 70, 0} : (Vector3){0, g->ccTop * 0.35f + rr * 0.3f, side * (g->ccW * 0.5f + 64)};
  Vector3 altC = Vector3Add(base, g->offAlt);
  if (b->placeMan) {
   float an = b->altAng * DEG2RAD, dy = cosf(an), dz = sinf(an), R0 = pulR + 70, Rf = R0;
   for (float R2 = R0; R2 < 900; R2 += 5) {
    float y = dy * R2, z = dz * R2; int hit = 0;
    if (fabsf(z) < g->ccW * 0.5f + 59 && y < g->ccTop + 59 && y > g->ccBot - g->bedH - g->panH - 59) hit = 1;
    for (int bk = 0; bk < g->nb && !hit; bk++) {
     float ba = g->bankA[bk], v = y * cosf(ba) + z * sinf(ba), w = -y * sinf(ba) + z * cosf(ba);
     if (v > g->v0 - 59 && v < g->deck + g->headH + 90 && fabsf(w) < g->headW * 0.5f + 59) hit = 1;
    }
    if (!hit) { Rf = R2; break; }
    Rf = R2;
   }
   altC = (Vector3){0, dy * Rf, dz * Rf};
   if (Rf > 420) snprintf(g->attachErr, 96, "Alternator at %.0f deg is %.0f mm out: no belt reaches that far.", b->altAng, Rf);
   Vector3 br0 = {frontX + 30, altC.y - dy * 50, altC.z - dz * 50}, br1 = {frontX + 30, altC.y - dy * 72, altC.z - dz * 72};
   Cyl(br0, br1, 9, C_STEEL, 0.6f);
  }
  float altPR = 26;
  Cyl((Vector3){frontX - 8 + g->offAlt.x, altC.y, altC.z}, (Vector3){frontX + 75 + g->offAlt.x, altC.y, altC.z}, 55, (Color){165, 168, 172, 255}, 0.7f);
  if (g->offAlt.x < -20) Cyl((Vector3){px0, altC.y, altC.z}, (Vector3){frontX - 6 + g->offAlt.x, altC.y, altC.z}, 8, C_STEEL, 0.8f);
  gGroup = G_FRONT;
  Cyl((Vector3){px1, altC.y, altC.z}, (Vector3){px0, altC.y, altC.z}, altPR, C_DARK, 0.6f);
  float dy = altC.y, dz = altC.z, d = fmaxf(sqrtf(dy * dy + dz * dz), 1);
  float ny = -dz / d, nz = dy / d, bx = (px0 + px1) * 0.5f;
  for (int sd = -1; sd <= 1; sd += 2) Cyl((Vector3){bx, ny * pulR * sd, nz * pulR * sd}, (Vector3){bx, altC.y + ny * altPR * sd, altC.z + nz * altPR * sd}, 3.5f, C_RUBBER, 0.1f);
 }
 // ---- flywheel + starter
 gGroup = G_FLY;
 float fx0 = rearX + 12;
 Cyl((Vector3){fx0, 0, 0}, (Vector3){fx0 + g->flyT, 0, 0}, g->flyR, (Color){110, 112, 118, 255}, 0.8f);
 if (FLY[b->flywheel].ring) Cyl((Vector3){fx0 + g->flyT * 0.25f, 0, 0}, (Vector3){fx0 + g->flyT * 0.75f, 0, 0}, g->flyR + 6, (Color){80, 82, 88, 255}, 0.7f);
 else for (int k = 0; k < 12; k++) { float an = k * PI / 6; Box((Vector3){fx0 + g->flyT * 0.5f, cosf(an) * g->flyR * 0.9f, sinf(an) * g->flyR * 0.9f}, (Vector3){g->flyT * 0.8f, 4, g->flyR * 0.18f}, MatrixRotateX(an), (Color){130, 132, 138, 255}, 0.7f); }
 gGroup = G_ROT;
 Cyl((Vector3){rearX - 2, 0, 0}, (Vector3){fx0 + 2, 0, 0}, fmaxf(rr * 0.4f, 10), C_STEEL, 0.8f);
 gGroup = G_FLY;
 if (st->elec) {
  gGroup = G_STARTER;
  float sr = 20 + 12 * sqrtf(st->power);
  Vector3 sc = Vector3Add((Vector3){rearX - 40, -g->flyR * 0.55f, -(g->flyR * 0.72f + 10)}, g->offStarter);
  if (b->placeMan) { float an = b->stAng * DEG2RAD, Rr = g->flyR + sr - 4; sc = (Vector3){rearX - 40, cosf(an) * Rr, sinf(an) * Rr}; }
  Cyl((Vector3){sc.x - 50, sc.y, sc.z}, (Vector3){sc.x + 55, sc.y, sc.z}, sr, C_BLACK, 0.4f);
  g->stTop = sc.z < 0 ? sc.y + sr + 40 : -1e9f;
  Cyl((Vector3){sc.x - 30, sc.y + sr + 14, sc.z}, (Vector3){sc.x + 30, sc.y + sr + 14, sc.z}, 16, C_STEEL, 0.6f);
  Box((Vector3){sc.x, sc.y + sr + 4, sc.z}, (Vector3){40, 22, 14}, MatrixIdentity(), C_STEEL, 0.6f);
 }
 // ---- intake
 gGroup = G_INTAKE;
 int vtop = g->nb == 2 || b->layout == 4 || g->custom;
 Vector3 E, D; int itb = 0;
 float pr = fmaxf(b->bore * 0.16f, 9);
 Color plenC = b->block == 0 ? (Color){150, 154, 160, 255} : Shade(blk, 1.1f);
 if (g->n == 1) {
  E = Vector3Add(g->inP[0], Vector3Scale(g->inD[0], 30)); D = g->inD[0];
  Cyl(Vector3Subtract(g->inP[0], Vector3Scale(g->inD[0], 4)), E, pr * 1.3f, plenC, 0.5f);
 } else if (!vtop) {
  float z = g->inP[0].z + g->inD[0].z * 80, y = g->inP[0].y + 10;
  for (int i = 0; i < g->n; i++) { Vector3 pts[3] = {Vector3Subtract(g->inP[i], Vector3Scale(g->inD[i], 4)), {g->x[i], g->inP[i].y + 5, g->inP[i].z + g->inD[i].z * 40}, {g->x[i], y, z}}; Pipe(pts, 3, pr, plenC, 0.5f, gNoEmis); }
  float pl = g->L * 0.9f;
  Box((Vector3){0, y, z}, (Vector3){pl, 72, 64}, MatrixIdentity(), plenC, 0.5f);
  E = (Vector3){-pl * 0.5f, y, z}; D = (Vector3){-1, 0, 0};
 } else if (g->custom) {
  // plenum goes on whichever side most intake ports face; runners bolt to every port
  float sy = 0, sz = 0;
  for (int i = 0; i < g->n; i++) { Vector3 ax = Vector3Subtract(BP(g->a[i], 0, 1, 0), BP(g->a[i], 0, 0, 0)); sy += g->inD[i].y + 0.6f * ax.y; sz += g->inD[i].z + 0.6f * ax.z; }
  Vector3 dir = {0, 1, 0};
  float sa[16]; for (int i = 0; i < g->n; i++) sa[i] = atan2f(sinf(g->a[i]), cosf(g->a[i]));
  for (int i = 1; i < g->n; i++) for (int k = i; k > 0 && sa[k] < sa[k - 1]; k--) { float t = sa[k]; sa[k] = sa[k - 1]; sa[k - 1] = t; }
  float gap = sa[0] + 2 * PI - sa[g->n - 1];
  for (int i = 1; i < g->n; i++) gap = fmaxf(gap, sa[i] - sa[i - 1]);
  itb = gap < PI * 0.95f && b->forced == 0; g->itb = itb;
  if (fabsf(sy) > 0.3f * g->n || fabsf(sz) > 0.3f * g->n) dir = fabsf(sy) >= fabsf(sz) ? (Vector3){0, sy > 0 ? 1.f : -1.f, 0} : (Vector3){0, 0, sz > 0 ? 1.f : -1.f};
  if (dir.y < -0.5f) dir = (Vector3){0, 0, sz >= 0 ? 1.f : -1.f};
  Vector3 perp = {0, -dir.z, dir.y};
  float reach = -1e9f;
  for (int i = 0; i < g->n; i++) {
   reach = fmaxf(reach, Vector3DotProduct(g->inP[i], dir) + 55);
   for (int sd = -1; sd <= 1; sd += 2) reach = fmaxf(reach, Vector3DotProduct(BP(g->a[i], 0, g->deck + g->headH + 40, sd * g->headW * 0.5f), dir) + 34);
  }
  if (dir.z != 0) reach = fmaxf(reach, g->ccW * 0.5f + 60);
  Vector3 c = Vector3Scale(dir, reach);
  float fsI = Clampf(sqrtf(s->dispL / g->n) * 0.75f, 0.35f, 0.9f);
  // spread-out layouts with no common side: one carb/throttle + filter per cylinder, straight off each port
  if (itb) for (int i = 0; i < g->n; i++) {
   Vector3 a0 = Vector3Subtract(g->inP[i], Vector3Scale(g->inD[i], 4)), a1 = Vector3Add(g->inP[i], Vector3Scale(g->inD[i], 40));
   Cyl(a0, a1, pr, plenC, 0.5f);
   Vector3 t1 = Vector3Add(a1, Vector3Scale(g->inD[i], 34));
   Cyl(Vector3Subtract(a1, Vector3Scale(g->inD[i], 2)), t1, pr * 1.45f, b->delivery == 0 ? (Color){200, 175, 95, 255} : (Color){170, 172, 178, 255}, 0.6f);
   DrawFilter(b->filter == 4 ? 3 : b->filter, Vector3Subtract(t1, Vector3Scale(g->inD[i], 2)), g->inD[i], fsI);
  }
  if (!itb) for (int i = 0; i < g->n; i++) {
   Vector3 m = Vector3Add(g->inP[i], Vector3Scale(g->inD[i], 25));
   float sg2 = Vector3DotProduct(g->inP[i], perp) >= 0 ? 1.f : -1.f;
   Vector3 e2 = Vector3Add(Vector3Add(c, Vector3Scale(dir, -20)), Vector3Scale(perp, sg2 * 28));
   e2.x = g->x[i];
   Vector3 pts[3] = {Vector3Subtract(g->inP[i], Vector3Scale(g->inD[i], 4)), m, e2};
   Pipe(pts, 3, pr, plenC, 0.5f, gNoEmis);
  }
  float pl = fmaxf(g->L * 0.82f, 120);
  if (!itb) Box(c, dir.y != 0 ? (Vector3){pl, 60, 120} : (Vector3){pl, 120, 60}, MatrixIdentity(), plenC, 0.5f);
  E = Vector3Add(c, Vector3Scale(dir, 30)); D = dir;
  g->inSide = itb ? 0 : (dir.z > 0.5f ? 1 : (dir.z < -0.5f ? -1 : 0));
 } else {
  float top = -1e9f;
  for (int i = 0; i < g->n; i++) top = fmaxf(top, g->inP[i].y);
  float topH = -1e9f;
  for (int bk = 0; bk < g->nb; bk++) for (int sd = -1; sd <= 1; sd += 2) topH = fmaxf(topH, BP(g->bankA[bk], 0, g->deck + g->headH + 40, sd * g->headW * 0.5f).y);
  float y = fmaxf(top + 55, topH + 34);
  for (int i = 0; i < g->n; i++) {
   Vector3 m = Vector3Add(g->inP[i], Vector3Scale(g->inD[i], 25));
   Vector3 pts[3] = {Vector3Subtract(g->inP[i], Vector3Scale(g->inD[i], 4)), m, {g->x[i], y - 20, g->inP[i].z > 0 ? 28.f : -28.f}};
   Pipe(pts, 3, pr, plenC, 0.5f, gNoEmis);
  }
  float pl = g->L * 0.82f;
  Box((Vector3){0, y, 0}, (Vector3){pl, 60, 120}, MatrixIdentity(), plenC, 0.5f);
  E = (Vector3){0, y + 30, 0}; D = (Vector3){0, 1, 0};
 }
 if (b->forced == 4) {
  gGroup = G_SC;
  Vector3 bc = Vector3Add(Vector3Add(E, Vector3Scale(D, 55)), g->offSC);
  float bl = fmaxf(g->L * 0.55f, 160);
  if (D.y > 0.5f) {
   Box(bc, (Vector3){bl, 110, 150}, MatrixIdentity(), (Color){190, 192, 198, 255}, 0.8f);
   for (int k = -3; k <= 3; k++) Box((Vector3){bc.x + k * bl / 8.f, bc.y + 56, bc.z}, (Vector3){6, 6, 150}, MatrixIdentity(), C_CHROME, 0.9f);
   Vector3 sp = {-bl * 0.5f - 18, bc.y, bc.z};
   Cyl((Vector3){sp.x + 12, sp.y, sp.z}, (Vector3){sp.x - 8, sp.y, sp.z}, 34, C_DARK, 0.6f);
   gGroup = G_FRONT;
   if (frontX - 20 < sp.x) {
    Cyl((Vector3){px0 + 10, pulR, 0}, (Vector3){px0 + 10, sp.y + 34, 0}, 4, C_RUBBER, 0);
   }
   E = Vector3Add(bc, (Vector3){0, 57, 0});
  } else {
   Box(Vector3Add(bc, Vector3Scale(D, 20)), (Vector3){120, 100, 100}, MatrixIdentity(), (Color){190, 192, 198, 255}, 0.8f);
   E = Vector3Add(bc, Vector3Scale(D, 72));
  }
  gGroup = G_INTAKE;
 }
 Vector3 tbEnd = Vector3Add(E, Vector3Scale(D, 48));
 Color tbC = b->delivery == 0 ? (Color){200, 175, 95, 255} : (Color){170, 172, 178, 255};
 if (itb) {} else if (b->delivery == 0) Box(Vector3Add(E, Vector3Scale(D, 24)), (Vector3){D.x != 0 ? 48 : 70, D.y != 0 ? 48 : 60, D.z != 0 ? 48 : 64}, MatrixIdentity(), tbC, 0.6f);
 else Cyl(Vector3Subtract(E, Vector3Scale(D, 2)), tbEnd, fmaxf(b->bore * 0.3f, 18), tbC, 0.7f);
 float fs = Clampf(sqrtf(s->dispL) * 0.75f, 0.45f, 1.5f);
 int hasT = b->forced >= 1 && b->forced <= 3;
 if (!hasT && !itb) {
  Vector3 fe = Vector3Add(tbEnd, Vector3Scale(D, Vector3Length(g->offFilter)));
  if (Vector3Length(g->offFilter) > 0) { gGroup = G_INTAKE; Cyl(Vector3Subtract(tbEnd, Vector3Scale(D, 2)), fe, fmaxf(b->bore * 0.28f, 16), tbC, 0.6f); }
  DrawFilter(b->filter, fe, D, fs);
 }
 // ---- exhaust: headers to an outboard lane, down the side, past the flywheel to the muffler
 gGroup = G_EXH;
 float pipeR = fmaxf(b->bore * 0.15f, 9);
 int exp2t = b->cycle && b->exhaust == 3;
 float sideExt = fmaxf(g->ccW * 0.5f, g->flyR) + 30;
 for (int i = 0; i < g->n; i++) { Vector3 tip = Vector3Add(g->exP[i], Vector3Scale(g->exD[i], 40)); sideExt = fmaxf(sideExt, fabsf(tip.z)); }
 Vector3 lanePt[2], hp1[16];
 float laneY = g->ccBot - g->panH * 0.5f;
 float bankLow[2] = {1e9f, 1e9f};
 for (int i = 0; i < g->n; i++) {
  Vector3 p1 = Vector3Add(g->exP[i], Vector3Scale(g->exD[i], 40));
  for (int k = 0; k < 60; k++) {
   int inBox = fabsf(p1.z) < g->ccW * 0.5f + pipeR + 6 && p1.y < g->ccTop + pipeR + 6 && p1.y > g->ccBot - g->panH - pipeR - 6;
   if (!inBox) break;
   p1 = Vector3Add(p1, Vector3Scale(g->exD[i], 10));
  }
  hp1[i] = p1;
  bankLow[g->bank[i]] = fminf(bankLow[g->bank[i]], p1.y);
  sideExt = fmaxf(sideExt, fabsf(p1.z));
 }
 g->nOut = 0; g->hasTurbo = 0;
 for (int bk = 0; bk < g->nb; bk++) {
  int first = -1;
  for (int i = 0; i < g->n; i++) if (g->bank[i] == bk) { first = i; break; }
  float sz = g->exP[first].z + g->exD[first].z * 10 >= 0 ? 1.f : -1.f;
  if (fabsf(g->exD[first].z) < 0.3f && fabsf(g->exP[first].z) < 1) sz = bk ? 1.f : -1.f;
  g->laneSide[bk] = sz;
  lanePt[bk] = Vector3Add((Vector3){(g->bx0[bk] + g->bx1[bk]) * 0.5f + g->L * 0.15f, fminf(laneY, bankLow[bk] - pipeR * 2 - 10), sz * (sideExt + pipeR + 25)}, g->offLane[bk]);
 }
 if (!exp2t) {
  for (int i = 0; i < g->n; i++) {
   int bk = g->bank[i];
   Vector3 p1 = hp1[i];
   Vector3 p2 = {p1.x, p1.y, lanePt[bk].z};
   Vector3 p3 = {p1.x + (lanePt[bk].x - p1.x) * 0.25f, lanePt[bk].y, lanePt[bk].z};
   Vector3 pts[5] = {Vector3Subtract(g->exP[i], Vector3Scale(g->exD[i], 4)), p1, p2, p3, lanePt[bk]};
   Pipe(pts, 5, pipeR, pipeC, 0.4f, glow);
  }
 }
 float tailX = fx0 + g->flyT + 70;
 if (exp2t) {
  Vector3 d = g->exD[0];
  Vector3 p0 = g->exP[0], p1 = Vector3Add(p0, Vector3Scale(d, 40));
  float sz = lanePt[0].z >= 0 ? 1.f : -1.f;
  Vector3 p2 = {p1.x, laneY + 30, lanePt[0].z};
  Vector3 p3 = {p2.x + 120, laneY, lanePt[0].z};
  Vector3 p4 = {tailX + 120, laneY, lanePt[0].z};
  Vector3 p5 = {p4.x + 120, laneY + 10, lanePt[0].z};
  Vector3 pts[3] = {Vector3Subtract(p0, Vector3Scale(d, 4)), p1, p2};
  Pipe(pts, 3, pipeR, pipeC, 0.4f, glow);
  Sph(p2, pipeR, pipeC, 0.4f);
  Taper(p2, p3, pipeR, pipeR * 2.4f, pipeC, 0.4f, glow);
  Sph(p3, pipeR * 2.4f, pipeC, 0.4f);
  Taper(p3, p4, pipeR * 2.4f, pipeR * 2.4f, pipeC, 0.4f, gNoEmis);
  Taper(p4, p5, pipeR * 2.4f, pipeR * 0.8f, pipeC, 0.4f, gNoEmis);
  Vector3 p6 = {p5.x + 120, p5.y, p5.z};
  Cyl(p5, p6, pipeR * 1.6f, (Color){60, 62, 66, 255}, 0.5f);
  g->exhOut[0] = p6; g->nOut = 1; (void)sz;
 } else {
  Vector3 runFrom[2] = {lanePt[0], lanePt[1]};
  if (hasT) {
   float k = (b->forced == 2 ? 1.3f : (b->forced == 3 ? 0.85f : 0.8f)) * Clampf(sqrtf(s->dispL / 2.f), 0.6f, 1.6f);
   int twins = b->forced == 3 ? 2 : 1;
   int rearT = (g->nb == 2 && twins == 1) || (b->turboSide == 3 && twins == 1);
   if (b->turboSide == 1 || b->turboSide == 2) {
    float want = b->turboSide == 1 ? -1.f : 1.f;
    int ok = 0; for (int bk2 = 0; bk2 < g->nb; bk2++) if ((lanePt[bk2].z >= 0 ? 1.f : -1.f) == want) ok = 1;
    if (!ok) snprintf(g->attachErr, 96, "Turbo on the %s can't reach the exhaust manifold (exhaust is on the other side).", want < 0 ? "left" : "right");
    else if (g->nb == 2 && twins == 1) snprintf(g->attachErr, 96, "A single turbo on a V/split-bank engine mounts at the rear so both banks reach it.");
   }
   for (int t = 0; t < twins; t++) {
    int bk = g->nb == 2 ? t : 0;
    float sz = lanePt[bk].z >= 0 ? 1.f : -1.f;
    Vector3 tc;
    if (rearT) tc = Vector3Add((Vector3){tailX + 60, 30, 0}, g->offTurbo);
    else tc = Vector3Add((Vector3){lanePt[bk].x - 30 + (g->nb == 1 && twins == 2 ? (t ? 1 : -1) * g->L * 0.25f : 0), lanePt[bk].y + 120 * k, lanePt[bk].z + sz * 70 * k}, g->offTurbo);
    Vector3 cOut, cIn, tIn, tOut;
    gGroup = G_TURBO;
    if (t == 0) { g->hasTurbo = 1; g->turboPos = tc; g->turboK = k; }
    DrawTurbo(tc, k, &cOut, &cIn, &tIn, &tOut);
    gGroup = G_EXH;
    if (rearT) {
     for (int bb = 0; bb < g->nb; bb++) { Vector3 q[3] = {lanePt[bb], {tailX - 20, lanePt[bb].y, lanePt[bb].z}, {tailX + 10, tIn.y - 20, tIn.z * 0.5f + lanePt[bb].z * 0.1f}}; Pipe(q, 3, pipeR * 1.1f, pipeC, 0.4f, glow); Vector3 q2[2] = {q[2], tIn}; Pipe(q2, 2, pipeR * 1.1f, pipeC, 0.4f, glow); }
    } else { Vector3 q[3] = {lanePt[bk], {tIn.x, lanePt[bk].y + 10, tIn.z}, tIn}; Pipe(q, 3, pipeR * 1.2f, pipeC, 0.4f, glow); }
    Vector3 dp[3] = {tOut, {tOut.x - 10, laneY, tOut.z}, {tOut.x, laneY, tOut.z}};
    Pipe(dp, 2, pipeR * 1.3f, pipeC, 0.4f, glow);
    runFrom[t] = dp[1];
    if (rearT) { runFrom[0] = (Vector3){tc.x + 150 * k, laneY, tc.z}; Vector3 dq[3] = {tOut, {tOut.x, laneY, tOut.z}, runFrom[0]}; Pipe(dq, 3, pipeR * 1.3f, pipeC, 0.4f, glow); }
    { gGroup = G_FILTER; Vector3 el = {cIn.x + 34 * k, cIn.y, cIn.z}; Vector3 fp[3] = {cIn, el, {el.x, el.y, el.z + sz * 40}}; Pipe(fp, 3, 24 * k, C_CHROME, 0.9f, gNoEmis); DrawFilter(b->filter, fp[2], (Vector3){0, 0, sz}, fs * 0.8f); gGroup = G_EXH; }
    // charge pipe up and over to the throttle (top-mount intercooler)
    gGroup = G_CHARGE;
    Vector3 dst = Vector3Add(tbEnd, Vector3Scale(D, 4));
    float yT = topY + 70 + g->offCharge.y;
    if (t == 0) {
     if (b->intercooler) {
      Vector3 ic = {0, yT + 30, 0};
      Box(ic, (Vector3){fmaxf(g->L * 0.5f, 220), 50, 240}, MatrixIdentity(), (Color){160, 165, 170, 255}, 0.7f);
      for (int k2 = -4; k2 <= 4; k2++) Box((Vector3){ic.x + k2 * 22.f, ic.y + 26, 0}, (Vector3){3, 4, 236}, MatrixIdentity(), (Color){120, 124, 130, 255}, 0.6f);
      Vector3 q1[4] = {cOut, {cOut.x, yT + 30, cOut.z}, {g->L * 0.25f + 40, yT + 30, cOut.z * 0.6f}, {g->L * 0.25f + 10, yT + 30, 0}};
      Pipe(q1, 4, 22, C_CHROME, 0.9f, gNoEmis);
      Vector3 q2[4] = {{-g->L * 0.25f - 10, yT + 30, 0}, {dst.x - 30 * D.x, yT + 30, dst.z}, {dst.x - 30 * D.x, dst.y + (D.y > 0.5f ? 0 : 0), dst.z}, dst};
      if (D.y > 0.5f) { q2[1] = (Vector3){-g->L * 0.25f - 40, yT + 30, dst.z}; q2[2] = (Vector3){-g->L * 0.25f - 40, dst.y + 40, dst.z}; q2[3] = Vector3Add(dst, (Vector3){0, 2, 0}); }
      Pipe(q2, 4, 22, C_CHROME, 0.9f, gNoEmis);
     } else {
      Vector3 q1[4] = {cOut, {cOut.x, yT, cOut.z}, {dst.x - 40 * D.x, yT, dst.z}, dst};
      if (D.y > 0.5f) q1[2] = (Vector3){dst.x, yT, dst.z};
      Pipe(q1, 4, 22, C_CHROME, 0.9f, gNoEmis);
     }
    } else {
     Vector3 q1[3] = {cOut, {cOut.x, yT, cOut.z}, {cOut.x, yT, 0}};
     Pipe(q1, 3, 20, C_CHROME, 0.9f, gNoEmis);
    }
    gGroup = G_EXH;
   }
  }
  int nOut = (g->nb == 2 && !(hasT && b->forced != 3)) ? 2 : 1;
  for (int o = 0; o < nOut; o++) {
   Vector3 src = runFrom[o];
   Vector3 m0 = {fmaxf(tailX, src.x + 40), src.y, src.z};
   Vector3 run[2] = {src, m0};
   Pipe(run, 2, pipeR * 1.2f, pipeC, 0.4f, gNoEmis);
   float mr = (b->exhaust == 0 ? 70 : 48) * Clampf(sqrtf(s->dispL / nOut) * 0.8f, 0.5f, 1.4f);
   float ml = b->exhaust == 0 ? 320 : 210;
   if (b->exhaust != 2) {
    Cyl((Vector3){m0.x - 2, m0.y, m0.z}, (Vector3){m0.x + ml, m0.y, m0.z}, mr, b->exhaust == 0 ? (Color){70, 72, 76, 255} : C_CHROME, 0.8f);
    Vector3 tp = {m0.x + ml + 60, m0.y, m0.z};
    Cyl((Vector3){m0.x + ml - 5, m0.y, m0.z}, tp, pipeR * 1.1f, C_CHROME, 0.9f);
    g->exhOut[o] = tp;
   } else {
    Vector3 tp = {m0.x + 140, m0.y, m0.z};
    Cyl(m0, tp, pipeR * 1.2f, pipeC, 0.4f);
    Taper(tp, (Vector3){tp.x + 30, tp.y, tp.z}, pipeR * 1.2f, pipeR * 1.5f, C_CHROME, 0.9f, gNoEmis);
    g->exhOut[o] = (Vector3){tp.x + 30, tp.y, tp.z};
   }
  }
  g->nOut = nOut;
 }
 // ---- lubrication hardware
 if (!b->cycle) {
  gGroup = G_OILSYS;
  float sideO = -1.f;
  for (int i = 0; i < g->n; i++) if (g->exD[i].z < -0.3f) sideO = 1.f;
  if (g->oilFlip) sideO = -sideO;
  if (b->lube == 0) {
   Vector3 dp = Vector3Add((Vector3){frontX + g->L * 0.2f, g->ccTop - 6, sideO * g->ccW * 0.3f}, g->offOil);
   Cyl(dp, Vector3Add(dp, (Vector3){0, 34, sideO * 10}), 7, C_DARK, 0.4f);
   Cyl(Vector3Add(dp, (Vector3){0, 32, sideO * 9}), Vector3Add(dp, (Vector3){0, 44, sideO * 13}), 12, (Color){240, 200, 30, 255}, 0.1f);
  } else {
   float pR = rr * 0.8f + 28;
   Vector3 fp = Vector3Add((Vector3){frontX + 4, -(pR + 46), g->ccW * 0.18f * sideO}, g->offOil);
   if (b->filtPos == 1 || b->filtPos == 2) {
    float sz2 = b->filtPos == 1 ? -1.f : 1.f;
    fp = (Vector3){frontX + g->L * 0.25f, g->ccBot * 0.45f, sz2 * (g->ccW * 0.5f + 4)};
    Cyl((Vector3){fp.x, fp.y, fp.z - sz2 * 8}, (Vector3){fp.x, fp.y, fp.z + sz2 * 16}, 30, C_STEEL, 0.6f);
    Cyl((Vector3){fp.x, fp.y, fp.z + sz2 * 14}, (Vector3){fp.x, fp.y - 6, fp.z + sz2 * 92}, 36, (Color){30, 60, 160, 255}, 0.3f);
   } else {
    Cyl((Vector3){fp.x + 6, fp.y, fp.z}, (Vector3){fp.x - 16, fp.y, fp.z}, 30, C_STEEL, 0.6f);
    Cyl((Vector3){fp.x - 14, fp.y, fp.z}, (Vector3){fp.x - 92, fp.y, fp.z}, 36, (Color){30, 60, 160, 255}, 0.3f);
   }
   if (b->lube == 2) {
    float fxe = rearX + 12 + g->flyT;
    Vector3 tk = Vector3Add((Vector3){fxe + 80, g->flyR * 0.15f, 0}, (Vector3){fmaxf(g->offOil.x, 0), 0, 0});
    Cyl((Vector3){tk.x, tk.y - 120, tk.z}, (Vector3){tk.x, tk.y + 110, tk.z}, 55, (Color){60, 62, 68, 255}, 0.7f);
    g->tankP = tk;
    Cyl((Vector3){tk.x, tk.y + 108, tk.z}, (Vector3){tk.x, tk.y + 126, tk.z}, 20, C_CHROME, 0.9f);
    float ly = g->ccBot - g->panH - 34;
    if (g->panH <= 0) {
     // scavenge straight off the crankcase side, over the flywheel, into the tank top
     int sd = g->inSide ? -g->inSide : 1;
     float sz = g->ccW * 0.5f, oy = fmaxf(g->flyR + 16, tk.y + 150);
     for (int k = 0; k < 2; k++) {
      float xx = k ? fp.x - 50 : g->L * 0.3f, zz = sd * (sz + 14 + k * 20);
      Vector3 l[5] = {{xx, 0, sd * (sz - 4)}, {xx, 0, zz}, {xx, oy, zz}, {tk.x - 30 + k * 60, oy, zz}, {tk.x - 30 + k * 60, tk.y + 100, (k ? -18.f : 18.f)}};
      Pipe(l, 5, 8, C_RUBBER, 0.2f, gNoEmis);
     }
    } else {
    Vector3 l1[4] = {{g->L * 0.3f, g->ccBot - g->panH + 4, 18}, {g->L * 0.3f, ly, 18}, {tk.x - 30, ly, 18}, {tk.x - 30, tk.y - 118, 18}};
    Pipe(l1, 4, 8, C_RUBBER, 0.2f, gNoEmis);
    Vector3 l2[4] = {{tk.x - 30, tk.y - 118, -18}, {tk.x - 30, ly, -18}, {fp.x - 50, ly, -18}, {fp.x - 50, fp.y - 34, -18}};
    Pipe(l2, 4, 8, C_RUBBER, 0.2f, gNoEmis);
    }
   }
   if (b->oilCooler) {
    if (g->panH <= 0 && g->custom) {
     // panless dry sump: cooler stands beside the tank, plumbed straight into it
     Vector3 tk = g->tankP, oc = {tk.x, tk.y, 150};
     Box(oc, (Vector3){22, 170, 150}, MatrixIdentity(), (Color){50, 52, 56, 255}, 0.5f);
     for (int k = -3; k <= 3; k++) Box((Vector3){oc.x + 12, oc.y + k * 22.f, oc.z}, (Vector3){4, 4, 144}, MatrixIdentity(), (Color){80, 82, 86, 255}, 0.4f);
     for (int k = -1; k <= 1; k += 2) { Vector3 c1[2] = {{tk.x, tk.y + k * 60, 40}, {tk.x, tk.y + k * 60, oc.z - 70}}; Pipe(c1, 2, 9, C_RUBBER, 0.2f, gNoEmis); }
    } else {
    Vector3 oc = {frontX - 40, fp.y - 95, 0};
    Box(oc, (Vector3){150, 22, 170}, MatrixIdentity(), (Color){50, 52, 56, 255}, 0.5f);
    for (int k = -3; k <= 3; k++) Box((Vector3){oc.x + k * 20.f, oc.y - 12, 0}, (Vector3){4, 4, 164}, MatrixIdentity(), (Color){80, 82, 86, 255}, 0.4f);
    Vector3 c1[2] = {{fp.x - 30, fp.y - 30, fp.z + 14}, {fp.x - 30, oc.y + 8, fp.z + 14}};
    Pipe(c1, 2, 6, C_RUBBER, 0.2f, gNoEmis);
    Vector3 c2[2] = {{fp.x - 70, fp.y - 30, fp.z - 14}, {fp.x - 70, oc.y + 8, fp.z - 14}};
    Pipe(c2, 2, 6, C_RUBBER, 0.2f, gNoEmis);
    }
   }
  }
 }
 // ---- radiator + fan
 g->fanR = 0;
 if (b->cooling > 0) {
  gGroup = G_COOL;
  float rh = (240 + 60 * sqrtf(s->dispL)) * (b->cooling == 2 ? 1.25f : 1.f);
  float rw = (280 + 90 * sqrtf(s->dispL)) * (b->cooling == 2 ? 1.15f : 1.f);
  float rx = frontX - 170 - (st->elec ? 0 : 60) + g->offRad.x;
  Vector3 rc = {rx, g->ccTop * 0.4f, 0};
  Box(rc, (Vector3){34, rh, rw}, MatrixIdentity(), (Color){48, 50, 54, 255}, 0.5f);
  Box((Vector3){rx, rc.y + rh * 0.5f + 11, 0}, (Vector3){44, 24, rw * 0.8f}, MatrixIdentity(), C_DARK, 0.5f);
  for (int k = -6; k <= 6; k++) Box((Vector3){rx - 18, rc.y, k * rw / 14.f}, (Vector3){4, rh * 0.95f, 3}, MatrixIdentity(), (Color){70, 72, 76, 255}, 0.4f);
  float fr = fminf(rh, rw) * 0.4f;
  Vector3 fc = {rx + 40, rc.y, 0};
  Cyl((Vector3){fc.x - 6, fc.y, 0}, (Vector3){fc.x + 6, fc.y, 0}, fr * 0.18f, C_DARK, 0.5f);
  Cyl((Vector3){rx + 16, fc.y, 0}, (Vector3){fc.x - 5, fc.y, 0}, fr * 0.06f, C_DARK, 0.5f);
  g->fanC = fc; g->fanR = fr;
  float ys[6] = {g->nb == 1 ? g->deck - 10 : g->ccTop - 12, g->ccTop - 12, g->ccTop * 0.55f, rr * 0.4f, g->ccBot + 22, (g->ccTop + g->ccBot) * 0.5f};
  float zs[5] = {0, g->ccW * 0.3f, -g->ccW * 0.3f, g->ccW * 0.42f, -g->ccW * 0.42f};
  for (int hsel = 0; hsel < 2; hsel++) {
   float sy = hsel ? -1.f : 1.f;
   Vector3 st0 = {rx + 18, rc.y + sy * rh * 0.4f, sy * rw * 0.3f};
   int best = g->hoseSel[hsel];
   if (gSolving) {
    best = -1; int bestHits = 1 << 30;
    for (int c = 0; c < 30; c++) {
     Vector3 tgt = {frontX + 8, ys[c % 6], zs[c / 6]};
     Vector3 hp[4] = {st0, {rx + 30, st0.y, st0.z}, {rx + 30, tgt.y, tgt.z}, tgt};
     int start = gNPrim;
     Pipe(hp, 4, 16, C_RUBBER, 0.1f, gNoEmis);
     int h = NewConflicts(start);
     gNPrim = start;
     if (h < bestHits) { bestHits = h; best = c; }
     if (!h) break;
    }
    g->hoseSel[hsel] = best;
   }
   if (best < 0) best = 0;
   Vector3 tgt = {frontX + 8, ys[best % 6], zs[best / 6]};
   Vector3 hp[4] = {st0, {rx + 30, st0.y, st0.z}, {rx + 30, tgt.y, tgt.z}, tgt};
   Pipe(hp, 4, 16, C_RUBBER, 0.1f, gNoEmis);
  }
 }
 g->intakeTop = tbEnd;
 DrawSpinning(b, crankDeg, ds);
 if (ds->oilView && !gSolving) DrawOil(b, s, crankDeg, ds);
}

static void DrawStand(const Geo *g) {
 Matrix save = gBase;
 int sg = gGroup; gGroup = G_STAND;
 gBase = MatrixScale(0.01f, 0.01f, 0.01f);
 float H = g->H * 100;
 float bot = H + fminf(g->ccBot - g->panH, g->lowY) + 6;
 float pw = g->panH > 0 ? g->ccW * 0.8f : g->ccW;
 Color sc = {200, 90, 30, 255};
 float px = g->L * 0.28f, pz = pw * 0.28f;
 for (int sx = -1; sx <= 1; sx += 2) {
  Box((Vector3){sx * px, 12, 0}, (Vector3){60, 34, pw + 220}, MatrixIdentity(), sc, 0.3f);
  for (int sz = -1; sz <= 1; sz += 2) Box((Vector3){sx * px, (bot + 14) * 0.5f - 7, sz * pz}, (Vector3){40, bot + 14 + 14, 40}, MatrixIdentity(), sc, 0.3f);
  Box((Vector3){sx * px, bot - 4, 0}, (Vector3){56, 20, pz * 2 + 50}, MatrixIdentity(), Shade(sc, 0.9f), 0.3f);
 }
 Box((Vector3){0, 12, 0}, (Vector3){px * 2 - 30, 30, 50}, MatrixIdentity(), Shade(sc, 0.85f), 0.3f);
 gBase = save; gGroup = sg;
}

// layout solver: draw invisibly, find group overlaps, push movable parts out
static int MoverOf(int a, int b) {
 static const int rank[G_N] = {0, 0, 0, 0, 0, 0, 6, 6, 0, 4, 2, 4, 4, 2, 7, 0, 6};
 if (a == G_COOL || b == G_COOL) return G_COOL;
 if (G.itb && ((a == G_FILTER && b == G_EXH) || (a == G_EXH && b == G_FILTER) || (a == G_INTAKE && b == G_EXH) || (a == G_EXH && b == G_INTAKE))) return G_EXH;
 return rank[a] >= rank[b] ? a : b;
}
static int gConf[G_N][G_N];
static void SolveLayout(const Build *b, const Spec *s) {
 Geo *g = &G;
 DrawState ds = {0};
 Matrix save = gBase;
 gBase = MatrixScale(0.01f, 0.01f, 0.01f);
 gSolving = 1; gRecord = 1;
 int it;
 for (it = 0; it < 40; it++) {
  gNPrim = 0;
  DrawEngine(b, s, 0, &ds);
  int tot = Conflicts(gConf);
  if (!tot) break;
  int moved = 0;
  for (int a = 0; a < G_N; a++) for (int c = 0; c < G_N; c++) {
   if (!gConf[a][c]) continue;
   int mv = MoverOf(a, c);
   const float st = 14.f;
   switch (mv) {
   case G_ALT: if (b->placeMan) break; if (g->offAlt.x > -110) { g->offAlt.x -= st; moved = 1; break; } { Vector3 d = {0, 0.4f, g->offAlt.z + (g->nb == 2 ? 0 : (g->inD[0].z >= 0 ? 1 : -1))}; if (g->nb == 2) d = (Vector3){0, 1, 0}; g->offAlt = Vector3Add(g->offAlt, Vector3Scale(Vector3Normalize(d), st)); moved = 1; } break;
   case G_STARTER: if (b->placeMan) break; g->offStarter = Vector3Add(g->offStarter, (Vector3){0, -st * 0.6f, -st * 0.8f}); moved = 1; break;
   case G_TURBO: case G_FILTER: if (mv == G_FILTER && !(b->forced >= 1 && b->forced <= 3)) g->offFilter.x += st; else { g->offTurbo.y += st * 0.7f; g->offTurbo.z += (g->offTurbo.z >= 0 ? 1 : -1) * st * 0.7f; } moved = 1; break;
   case G_CHARGE: g->offCharge.y += st; moved = 1; break;
   case G_SC: g->offSC.y += st; moved = 1; break;
   case G_COOL: g->offRad.x -= st; moved = 1; break;
   case G_OILSYS: if (!g->oilFlip && g->offOil.x > 50) { g->oilFlip = 1; g->offOil = (Vector3){0}; } else { g->offOil.y -= st * 0.5f; g->offOil.x += st * 0.6f; } moved = 1; break;
   case G_EXH: { static int flip = 0; flip ^= 1; for (int bk = 0; bk < 2; bk++) { if (flip) g->offLane[bk].y -= st; else g->offLane[bk].z += (g->laneSide[bk] >= 0 ? 1 : -1) * st; } moved = 1; } break;
   default: break;
   }
  }
  g->offLane[0].z = Clampf(g->offLane[0].z, -120, 120); g->offLane[1].z = Clampf(g->offLane[1].z, -120, 120);
  g->offLane[0].y = fmaxf(g->offLane[0].y, -160); g->offLane[1].y = fmaxf(g->offLane[1].y, -160);
  g->offTurbo.y = fminf(g->offTurbo.y, 200); g->offTurbo.z = Clampf(g->offTurbo.z, -200, 200);
  g->offFilter.x = fminf(g->offFilter.x, 200); g->offRad.x = fmaxf(g->offRad.x, -300);
  g->offCharge.y = fminf(g->offCharge.y, 200); g->offSC.y = fminf(g->offSC.y, 160);
  g->offStarter.y = fmaxf(g->offStarter.y, -150); g->offStarter.z = fmaxf(g->offStarter.z, -200);
  g->offOil.y = fmaxf(g->offOil.y, -200); g->offOil.x = fminf(g->offOil.x, 300);
  g->offAlt.y = Clampf(g->offAlt.y, -150, 150); g->offAlt.z = Clampf(g->offAlt.z, -150, 150);
  if (!moved) break;
 }
 g->solveIters = it;
 gNPrim = 0;
 DrawEngine(b, s, 0, &ds);
 g->solveLeft = Conflicts(gConf);
 float lowest = 1e9f;
 for (int i = 0; i < gNPrim; i++) lowest = fminf(lowest, gPrim[i].lo.y);
 g->H = -lowest + 0.35f;
 gSolving = 0; gRecord = 0;
 gBase = save;
}

static void GeoCompute(const Build *b, const Spec *s) {
 GeoBase(b);
 SolveLayout(b, s);
}
static void DrawFloor(Vector3 bg, Vector3 ctr, float r) {
 SetShaderValue(gFloorSh, locBg, &bg, SHADER_UNIFORM_VEC3);
 SetShaderValue(gFloorSh, locCtr, &ctr, SHADER_UNIFORM_VEC3);
 SetShaderValue(gFloorSh, locShR, &r, SHADER_UNIFORM_FLOAT);
 DrawMesh(mPlane, gFloorMat, MatrixIdentity());
}

// smoke
typedef struct { Vector3 p, v; float life, max, size; Color c; } Puff;
#define NPUFF 2600
static Puff gPuff[NPUFF];
static void PuffAdd(Vector3 p, Vector3 v, float life, float size, Color c) {
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life <= 0) { gPuff[i] = (Puff){p, v, life, life, size, c}; return; }
}
static void PuffUpdate(float dt) {
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life > 0) {
  Puff *p = &gPuff[i];
  p->life -= dt; p->p = Vector3Add(p->p, Vector3Scale(p->v, dt));
  p->v.y += 0.35f * dt; p->v = Vector3Scale(p->v, 1 - 0.6f * dt);
 }
}
static void PuffDraw(void) {
 rlDisableDepthMask();
 Matrix save = gBase; gBase = MatrixIdentity();
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life > 0) {
  Puff *p = &gPuff[i];
  float t = 1 - p->life / p->max;
  Color c = p->c; c.a = (unsigned char)(c.a * (1 - t) * (1 - t));
  float rs = p->size * (0.4f + 2.2f * t);
  Put(mPuff, MatrixMultiply(MatrixScale(rs, rs, rs), MatrixTranslate(p->p.x, p->p.y, p->p.z)), c, 0, gNoEmis);
 }
 gBase = save;
 rlEnableDepthMask();
}
