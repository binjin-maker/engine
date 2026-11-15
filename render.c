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
static Mesh mCube, mCyl, mSph, mCone, mPlane;
static Matrix gBase;
static Vector3 gNoEmis = {0, 0, 0};

// part groups, for hiding and for the collision solver
enum { G_BLOCK, G_HEAD, G_PLUG, G_ROT, G_FRONT, G_FLY, G_STARTER, G_ALT, G_INTAKE, G_FILTER, G_SC, G_TURBO, G_CHARGE, G_EXH, G_COOL, G_STAND, G_N };
static const char *GROUP_NAME[] = {"block","head","plug","crank","front","flywheel","starter","alternator","intake","filter","blower","turbo","charge pipe","exhaust","cooling","stand"};
static int gGroup = 0, gSolving = 0, gHide = 0;
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
 AllowInit();
}

static void AllowPair(int a, int b) { gAllow[a] |= 1u << b; gAllow[b] |= 1u << a; }
static void AllowInit(void) {
 for (int i = 0; i < G_N; i++) gAllow[i] = 1u << i;
 AllowPair(G_BLOCK, G_HEAD); AllowPair(G_BLOCK, G_ROT); AllowPair(G_BLOCK, G_STAND); AllowPair(G_BLOCK, G_INTAKE);
 AllowPair(G_BLOCK, G_COOL); AllowPair(G_HEAD, G_PLUG); AllowPair(G_HEAD, G_INTAKE); AllowPair(G_HEAD, G_EXH); AllowPair(G_HEAD, G_COOL);
 AllowPair(G_ROT, G_FRONT); AllowPair(G_ROT, G_FLY); AllowPair(G_FRONT, G_ALT); AllowPair(G_FRONT, G_SC); AllowPair(G_FRONT, G_BLOCK);
 AllowPair(G_INTAKE, G_FILTER); AllowPair(G_INTAKE, G_SC); AllowPair(G_INTAKE, G_CHARGE); AllowPair(G_FILTER, G_TURBO); AllowPair(G_FILTER, G_SC);
 AllowPair(G_TURBO, G_EXH); AllowPair(G_TURBO, G_CHARGE); AllowPair(G_STARTER, G_FLY); AllowPair(G_STARTER, G_BLOCK); AllowPair(G_ALT, G_BLOCK);
}

static int MeshId(Mesh m) { return m.vaoId == mCube.vaoId ? 0 : m.vaoId == mCyl.vaoId ? 1 : m.vaoId == mSph.vaoId ? 2 : 3; }
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
 // solver offsets for movable groups
 Vector3 offAlt, offStarter, offTurbo, offFilter, offRad, offLane[2], offSC, offCharge;
 int solveIters, solveLeft;
} Geo;
static Geo G;

static Vector3 BP(float a, float x, float v, float w) { return (Vector3){x, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }
static Vector3 BD(float a, float v, float w) { return (Vector3){0, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }
static Color Shade(Color c, float k) { return (Color){(unsigned char)Clampf(c.r * k, 0, 255), (unsigned char)Clampf(c.g * k, 0, 255), (unsigned char)Clampf(c.b * k, 0, 255), c.a}; }

static const Color C_STEEL = {150, 152, 158, 255}, C_DARK = {38, 40, 44, 255}, C_BLACK = {24, 25, 28, 255}, C_CHROME = {225, 228, 235, 255};
static const Color C_RUBBER = {20, 20, 22, 255}, C_PISTON = {200, 200, 205, 255};

typedef struct { int cut; float fanAng; float vis; float glow; float pullT; float span; int oilView; float oilSump, oilHead, oilNeed, oilSupply, oilFlow, oilT; } DrawState;

static void GeoBase(const Build *b) {
 Geo *g = &G;
 Vector3 keep[8] = {g->offAlt, g->offStarter, g->offTurbo, g->offFilter, g->offRad, g->offLane[0], g->offLane[1], g->offSC};
 (void)keep;
 memset(g, 0, sizeof *g);
 int n = b->cyl;
 g->n = n; g->nb = b->layout >= 2 ? 2 : 1; g->m = g->nb == 2 ? (n + 1) / 2 : n;
 float half = LAYOUT_HALF[b->layout] * DEG2RAD;
 float tlt = b->tilt * DEG2RAD;
 g->r = b->stroke / 2; g->l = 1.75f * b->stroke;
 g->pitch = b->bore * 1.18f + 10;
 g->deck = g->r + g->l + 0.25f * b->bore + 1;
 g->v0 = g->r * 1.25f + 10;
 g->headH = (b->cycle ? 0.42f : VT4[b->valvetrain].hgt) * b->bore;
 g->headW = b->bore * 1.35f;
 g->jw = b->bore * 1.28f;
 g->L = g->m * g->pitch + 0.45f * g->pitch + (g->nb == 2 ? 0.25f * g->pitch : 0);
 for (int i = 0; i < n; i++) {
  if (g->nb == 2) {
   int bk = i & 1, j = i >> 1;
   g->bank[i] = bk; g->a[i] = (bk ? -half : half) + tlt;
   g->x[i] = (j - (g->m - 1) * 0.5f) * g->pitch + (bk ? 0.12f : -0.12f) * g->pitch;
  } else { g->bank[i] = 0; g->a[i] = tlt; g->x[i] = (i - (n - 1) * 0.5f) * g->pitch; }
  g->tdc[i] = AU.fire[i];
 }
 g->bankA[0] = (g->nb == 2 ? half : 0) + tlt; g->bankA[1] = -half + tlt;
 for (int bk = 0; bk < g->nb; bk++) { g->bx0[bk] = 1e9f; g->bx1[bk] = -1e9f; }
 for (int i = 0; i < n; i++) { int bk = g->bank[i]; g->bx0[bk] = fminf(g->bx0[bk], g->x[i] - g->pitch * 0.5f); g->bx1[bk] = fmaxf(g->bx1[bk], g->x[i] + g->pitch * 0.5f); }
 float ccWi = 2 * g->r + b->bore * 0.9f + 20;
 if (b->layout <= 1) { g->ccW = fmaxf(ccWi, g->jw + 14) + 2 * fabsf(sinf(tlt)) * g->v0 * 0.8f; g->ccTop = g->v0 * fmaxf(cosf(tlt), 0.35f) + 10 + fabsf(sinf(tlt)) * b->bore * 0.3f; }
 else if (b->layout <= 3) { g->ccW = fmaxf(2 * sinf(half) * g->v0 + b->bore * 1.25f, ccWi); g->ccTop = g->v0 * cosf(half) + b->bore * 0.35f; }
 else { g->ccW = 2 * g->v0 + 24; g->ccTop = g->r * 1.3f + b->bore * 0.45f; }
 g->ccBot = -(g->r * 1.35f + 12);
 g->panH = b->cycle ? 0 : 30 + b->bore * 0.5f;
 float fw = FLY[b->flywheel].magneto ? 0.85f : (FLY[b->flywheel].mul > 1.5f ? 1.15f : 1.f);
 g->flyR = (b->stroke * 0.9f + 70) * fw;
 g->flyT = 18 * FLY[b->flywheel].mul + 8;
 for (int i = 0; i < n; i++) {
  float a = g->a[i];
  float is = (a > 0.01f) ? -1.f : 1.f;
  float vp = g->deck + g->headH * 0.48f, ve = g->deck + g->headH * (b->cycle ? 0.4f : 0.3f);
  g->inP[i] = BP(a, g->x[i], vp, is * g->headW * 0.5f);
  g->inD[i] = BD(a, 0, is);
  g->exP[i] = BP(a, g->x[i], ve, -is * g->headW * 0.5f);
  g->exD[i] = BD(a, 0, -is);
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
#define NOILP 260
typedef struct { int path; float t, spd, life; Vector3 p, v; } OilP;
static OilP gOil[NOILP];
static float gOilClock = 0;
static Vector3 gOilPath[24][8]; static int gOilPathN[24], gNOilPaths = 0;
static void OilPathsBuild(const Build *b) {
 Geo *g = &G; float frontX = -g->L * 0.5f, rearX = g->L * 0.5f, rr = g->r;
 gNOilPaths = 0;
 float poolY = g->panH > 0 ? g->ccBot - g->panH * 0.6f : g->ccBot + 8;
 Vector3 pick = {0, poolY, 0}, pump = {frontX + 28, g->ccBot - 4, 0};
 Vector3 g0 = {frontX + 28, -rr * 0.15f, -g->ccW * 0.32f}, g1 = {rearX - 28, -rr * 0.15f, -g->ccW * 0.32f};
 if (b->lube == 0) return;
 // main gallery feed
 Vector3 *p = gOilPath[gNOilPaths]; int n = 0;
 p[n++] = pick; p[n++] = pump; p[n++] = g0; p[n++] = g1; gOilPathN[gNOilPaths++] = n;
 // main bearings
 for (int i = 0; i < g->n && gNOilPaths < 18; i += (g->nb == 2 ? 2 : 1)) {
  p = gOilPath[gNOilPaths]; n = 0;
  float x = g->x[i];
  p[n++] = (Vector3){x, -rr * 0.15f, -g->ccW * 0.32f}; p[n++] = (Vector3){x, 0, 0}; p[n++] = (Vector3){x, g->ccBot + 6, g->ccW * 0.2f}; p[n++] = (Vector3){x, poolY, g->ccW * 0.15f};
  gOilPathN[gNOilPaths++] = n;
 }
 // up to each head, along it, drain back
 for (int bk = 0; bk < g->nb && gNOilPaths < 24; bk++) {
  float a = g->bankA[bk], x0 = g->bx0[bk] + 10, x1 = g->bx1[bk] - 10;
  p = gOilPath[gNOilPaths]; n = 0;
  p[n++] = (Vector3){x0, -rr * 0.15f, -g->ccW * 0.32f};
  p[n++] = BP(a, x0, g->v0, -g->jw * 0.42f);
  p[n++] = BP(a, x0, g->deck + g->headH * 0.6f, -g->headW * 0.3f);
  p[n++] = BP(a, x1, g->deck + g->headH * 0.6f, -g->headW * 0.3f);
  p[n++] = BP(a, x1, g->deck + g->headH * 0.2f, g->headW * 0.35f);
  p[n++] = BP(a, x1, g->v0, g->jw * 0.42f);
  p[n++] = (Vector3){x1, poolY, 0};
  gOilPathN[gNOilPaths++] = n;
 }
}
static Vector3 OilPathAt(int pi, float t, float *segOut) {
 int n = gOilPathN[pi]; Vector3 *p = gOilPath[pi];
 float tot = 0, len[8];
 for (int i = 0; i + 1 < n; i++) { len[i] = Vector3Distance(p[i], p[i + 1]); tot += len[i]; }
 float d = t * tot;
 for (int i = 0; i + 1 < n; i++) { if (d <= len[i]) { *segOut = (float)i / (n - 1); return Vector3Lerp(p[i], p[i + 1], d / fmaxf(len[i], 1e-3f)); } d -= len[i]; }
 *segOut = 1; return p[n - 1];
}
static void OilUpdate(const Build *b, const DrawState *ds, float rpm, float dt) {
 gOilClock += dt;
 float spd = Clampf(ds->oilFlow * 2.5f, 0, 1.2f);
 for (int i = 0; i < NOILP; i++) {
  OilP *o = &gOil[i];
  if (b->lube == 0 || b->cycle) {
   // splash: droplets flung off the crank, fall back
   if (o->life <= 0) {
    if (rpm < 60 || (float)GetRandomValue(0, 1000) / 1000.f > ds->oilSupply * 0.6f + 0.1f) continue;
    int c = GetRandomValue(0, G.n - 1);
    float an = (float)GetRandomValue(0, 628) / 100.f, sp = 300 + rpm * 0.12f;
    o->p = (Vector3){G.x[c], -G.r * 0.6f, 0}; o->v = (Vector3){0, sinf(an) * sp * 0.6f + sp * 0.5f, cosf(an) * sp * 0.6f}; o->life = 0.6f;
   }
   o->v.y -= 2400 * dt; o->p = Vector3Add(o->p, Vector3Scale(o->v, dt)); o->life -= dt;
   if (o->p.y < G.ccBot - G.panH * 0.5f) o->life = 0;
  } else {
   if (gNOilPaths == 0) continue;
   if (o->life <= 0) { o->path = GetRandomValue(0, gNOilPaths - 1); o->t = (float)GetRandomValue(0, 1000) / 1000.f; o->life = 1; o->spd = 0.6f + 0.6f * GetRandomValue(0, 100) / 100.f; }
   float L = 0; for (int k = 0; k + 1 < gOilPathN[o->path]; k++) L += Vector3Distance(gOilPath[o->path][k], gOilPath[o->path][k + 1]);
   float slow = 1;
   if (gOilPathN[o->path] == 7) { float seg; OilPathAt(o->path, o->t, &seg); if (seg > 0.55f) slow = Clampf(G_drainVis, 0.03f, 1); }
   o->t += spd * slow * o->spd * 220.f / fmaxf(L, 1) * dt;
   if (o->t >= 1) o->t -= 1;
   float seg; o->p = OilPathAt(o->path, o->t, &seg);
  }
 }
}
static void DrawOil(const Build *b, const Spec *s, float crankDeg, const DrawState *ds) {
 Geo *g = &G;
 int sg = gGroup; gGroup = G_ROT;
 Color amber = {214, 150, 32, 255}, dark = {150, 95, 20, 235}, air = {235, 235, 240, 255};
 float frontX = -g->L * 0.5f;
 // sump pool (surface stays level)
 float cap = fmaxf(s->oilMax, 0.1f);
 float lvl = Clampf(ds->oilSump / cap, 0, 1);
 if (g->panH > 0) {
  float h = (g->panH - 6) * lvl;
  if (h > 1) Box((Vector3){0, g->ccBot - g->panH + 3 + h * 0.5f, 0}, (Vector3){g->L * 0.8f, h, g->ccW * 0.78f}, MatrixIdentity(), dark, 0.2f);
  float mh = (g->panH - 6) * (s->oilNeed * 0.55f / cap);
  Box((Vector3){frontX + g->L * 0.09f + 1, g->ccBot - g->panH + 3 + mh, g->ccW * 0.39f + 2}, (Vector3){6, 2, 6}, MatrixIdentity(), (Color){235, 70, 55, 255}, 0);
 } else if (!b->cycle) {
  float h = fmaxf((g->r * 0.6f) * lvl, 0);
  if (h > 1) Box((Vector3){0, g->ccBot + h * 0.5f + 2, -g->ccW * 0.2f}, (Vector3){g->L * 0.85f, h, g->ccW * 0.35f}, MatrixIdentity(), dark, 0.2f);
 }
 // pickup + pump
 if (b->lube > 0) {
  float poolY = g->ccBot - g->panH * 0.6f;
  Cyl((Vector3){0, poolY, 0}, (Vector3){frontX + 28, g->ccBot - 4, 0}, 6, ds->oilSupply < 0.7f ? (Color){235, 70, 55, 255} : C_STEEL, 0.6f);
  Box((Vector3){frontX + 28, g->ccBot + 6, 0}, (Vector3){30, 24, 40}, MatrixIdentity(), C_STEEL, 0.6f);
 }
 // oil held up in the heads
 if (ds->oilHead > 0.01f && b->lube > 0) {
  float hf = Clampf(ds->oilHead / fmaxf(s->oilHeadCap, 0.05f), 0, 1);
  for (int bk = 0; bk < g->nb; bk++) {
   float a = g->bankA[bk], xm = (g->bx0[bk] + g->bx1[bk]) * 0.5f, len = g->bx1[bk] - g->bx0[bk];
   float hh = g->headH * 0.8f * hf;
   if (hh > 1) Box(BP(a, xm, g->deck + 2 + hh * 0.5f, 0), (Vector3){len, hh, g->headW * 0.85f}, MatrixRotateX(a), amber, 0.3f);
  }
 }
 // dry sump tank
 if (b->lube == 2) {
  Vector3 tk = {0, g->ccTop * 0.3f, -(g->ccW * 0.5f + 110)};
  Cyl((Vector3){tk.x, tk.y - 120, tk.z}, (Vector3){tk.x, tk.y + 120, tk.z}, 55, (Color){60, 62, 68, 255}, 0.7f);
  Vector3 l1[3] = {{0, g->ccBot - g->panH + 8, -g->ccW * 0.38f}, {0, g->ccBot - g->panH + 8, tk.z + 40}, {0, tk.y - 100, tk.z + 40}};
  Pipe(l1, 3, 7, (Color){60, 60, 66, 255}, 0.4f, gNoEmis);
 }
 // particles
 for (int i = 0; i < NOILP; i++) {
  OilP *o = &gOil[i];
  if (o->life <= 0 && (b->lube == 0 || b->cycle)) continue;
  if (b->lube > 0 && !b->cycle && gNOilPaths == 0) continue;
  int bubbly = b->lube > 0 && ds->oilSupply < 0.75f && (i % 4) < (int)((1 - ds->oilSupply) * 6);
  Sph(o->p, b->lube == 0 || b->cycle ? 3.f : 4.5f, bubbly ? air : amber, 0.4f);
 }
 gGroup = sg;
 (void)crankDeg;
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
 if (g->panH > 0) Box((Vector3){0, g->ccBot - g->panH * 0.5f + 2, 0}, (Vector3){g->L * 0.82f, g->panH + 4, g->ccW * 0.8f}, MatrixIdentity(), b->block == 0 ? C_DARK : Shade(blk, 0.95f), 0.4f);
 for (int bk = 0; bk < g->nb; bk++) {
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
   if (!b->cycle && b->valvetrain > 0) { float ch = b->bore * 0.2f, is0 = (a > 0.01f) ? -1.f : 1.f; Box(BP(a, x, g->deck + g->headH + ch * 0.5f - 2, b->valvetrain == 3 ? 0 : is0 * b->bore * 0.2f), (Vector3){g->pitch * 0.7f, ch, b->valvetrain == 3 ? b->bore * 0.8f : b->bore * 0.6f}, R, cover, 0.35f); }
  }
  if (!FUELS[b->fuel].diesel) {
   gGroup = G_PLUG;
   Vector3 p0, p1;
   {
    float top = g->deck + g->headH, is = (a > 0.01f) ? -1.f : 1.f;
    float w = (b->cycle || b->valvetrain == 3 || b->valvetrain == 0) ? 0 : -is * b->bore * 0.36f;
    p0 = BP(a, x, top - 8, w); p1 = BP(a, x, top + 28, w);
   }
   Cyl(p0, p1, 6.5f, (Color){235, 235, 230, 255}, 0.1f);
   Cyl(p1, Vector3Add(p1, Vector3Scale(Vector3Normalize(Vector3Subtract(p1, p0)), 10)), 4.5f, C_STEEL, 0.7f);
  }
 }
 float topY = -1e9f;
 for (int bk = 0; bk < g->nb; bk++) for (int sd = -1; sd <= 1; sd += 2) topY = fmaxf(topY, BP(g->bankA[bk], 0, g->deck + g->headH + b->bore * 0.3f + 30, sd * g->headW * 0.5f).y);
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
  Vector3 base = g->nb == 2 || b->layout == 4 ? (Vector3){0, g->ccTop + 70, 0} : (Vector3){0, g->ccTop * 0.35f + rr * 0.3f, side * (g->ccW * 0.5f + 64)};
  Vector3 altC = Vector3Add(base, g->offAlt);
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
  float sr = 22 + 7 * st->power;
  Vector3 sc = Vector3Add((Vector3){rearX - 40, -g->flyR * 0.55f, -(g->flyR * 0.72f + 10)}, g->offStarter);
  Cyl((Vector3){sc.x - 50, sc.y, sc.z}, (Vector3){sc.x + 55, sc.y, sc.z}, sr, C_BLACK, 0.4f);
  Cyl((Vector3){sc.x - 30, sc.y + sr + 14, sc.z}, (Vector3){sc.x + 30, sc.y + sr + 14, sc.z}, 16, C_STEEL, 0.6f);
  Box((Vector3){sc.x, sc.y + sr + 4, sc.z}, (Vector3){40, 22, 14}, MatrixIdentity(), C_STEEL, 0.6f);
 }
 // ---- intake
 gGroup = G_INTAKE;
 int vtop = g->nb == 2 || b->layout == 4;
 Vector3 E, D;
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
 if (b->delivery == 0) Box(Vector3Add(E, Vector3Scale(D, 24)), (Vector3){D.x != 0 ? 48 : 70, D.y != 0 ? 48 : 60, D.z != 0 ? 48 : 64}, MatrixIdentity(), tbC, 0.6f);
 else Cyl(Vector3Subtract(E, Vector3Scale(D, 2)), tbEnd, fmaxf(b->bore * 0.3f, 18), tbC, 0.7f);
 float fs = Clampf(sqrtf(s->dispL) * 0.75f, 0.45f, 1.5f);
 int hasT = b->forced >= 1 && b->forced <= 3;
 if (!hasT) {
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
 g->nOut = 0;
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
   for (int t = 0; t < twins; t++) {
    int bk = g->nb == 2 ? t : 0;
    float sz = lanePt[bk].z >= 0 ? 1.f : -1.f;
    Vector3 tc;
    if (g->nb == 2 && twins == 1) tc = Vector3Add((Vector3){tailX + 60, 30, 0}, g->offTurbo);
    else tc = Vector3Add((Vector3){lanePt[bk].x - 30 + (g->nb == 1 && twins == 2 ? (t ? 1 : -1) * g->L * 0.25f : 0), lanePt[bk].y + 120 * k, lanePt[bk].z + sz * 70 * k}, g->offTurbo);
    Vector3 cOut, cIn, tIn, tOut;
    gGroup = G_TURBO;
    DrawTurbo(tc, k, &cOut, &cIn, &tIn, &tOut);
    gGroup = G_EXH;
    if (g->nb == 2 && twins == 1) {
     for (int bb = 0; bb < 2; bb++) { Vector3 q[3] = {lanePt[bb], {tailX - 20, lanePt[bb].y, lanePt[bb].z}, {tailX + 10, tIn.y - 20, tIn.z * 0.5f + lanePt[bb].z * 0.1f}}; Pipe(q, 3, pipeR * 1.1f, pipeC, 0.4f, glow); Vector3 q2[2] = {q[2], tIn}; Pipe(q2, 2, pipeR * 1.1f, pipeC, 0.4f, glow); }
    } else { Vector3 q[3] = {lanePt[bk], {tIn.x, lanePt[bk].y + 10, tIn.z}, tIn}; Pipe(q, 3, pipeR * 1.2f, pipeC, 0.4f, glow); }
    Vector3 dp[3] = {tOut, {tOut.x - 10, laneY, tOut.z}, {tOut.x, laneY, tOut.z}};
    Pipe(dp, 2, pipeR * 1.3f, pipeC, 0.4f, glow);
    runFrom[t] = dp[1];
    if (g->nb == 2 && twins == 1) { runFrom[0] = (Vector3){tc.x + 150 * k, laneY, tc.z}; Vector3 dq[3] = {tOut, {tOut.x, laneY, tOut.z}, runFrom[0]}; Pipe(dq, 3, pipeR * 1.3f, pipeC, 0.4f, glow); }
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
 float bot = H + g->ccBot - g->panH + 6;
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
 static const int rank[G_N] = {0, 0, 0, 0, 0, 0, 6, 6, 0, 4, 2, 4, 4, 2, 7, 0};
 if (a == G_COOL || b == G_COOL) return G_COOL;
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
   case G_ALT: if (g->offAlt.x > -110) { g->offAlt.x -= st; moved = 1; break; } { Vector3 d = {0, 0.4f, g->offAlt.z + (g->nb == 2 ? 0 : (g->inD[0].z >= 0 ? 1 : -1))}; if (g->nb == 2) d = (Vector3){0, 1, 0}; g->offAlt = Vector3Add(g->offAlt, Vector3Scale(Vector3Normalize(d), st)); moved = 1; } break;
   case G_STARTER: g->offStarter = Vector3Add(g->offStarter, (Vector3){0, -st * 0.6f, -st * 0.8f}); moved = 1; break;
   case G_TURBO: case G_FILTER: if (mv == G_FILTER && !(b->forced >= 1 && b->forced <= 3)) g->offFilter.x += st; else { g->offTurbo.y += st * 0.7f; g->offTurbo.z += (g->offTurbo.z >= 0 ? 1 : -1) * st * 0.7f; } moved = 1; break;
   case G_CHARGE: g->offCharge.y += st; moved = 1; break;
   case G_SC: g->offSC.y += st; moved = 1; break;
   case G_COOL: g->offRad.x -= st; moved = 1; break;
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
#define NPUFF 900
static Puff gPuff[NPUFF];
static void PuffAdd(Vector3 p, Vector3 v, float life, float size, Color c) {
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life <= 0) { gPuff[i] = (Puff){p, v, life, life, size, c}; return; }
}
static void PuffUpdate(float dt) {
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life > 0) {
  Puff *p = &gPuff[i];
  p->life -= dt; p->p = Vector3Add(p->p, Vector3Scale(p->v, dt));
  p->v.y += 0.25f * dt; p->v = Vector3Scale(p->v, 1 - 0.8f * dt);
 }
}
static void PuffDraw(void) {
 rlDisableDepthMask();
 Matrix save = gBase; gBase = MatrixIdentity();
 for (int i = 0; i < NPUFF; i++) if (gPuff[i].life > 0) {
  Puff *p = &gPuff[i];
  float t = 1 - p->life / p->max;
  Color c = p->c; c.a = (unsigned char)(c.a * (1 - t) * (1 - t));
  Sph(p->p, p->size * (0.4f + 1.6f * t), c, 0);
 }
 gBase = save;
 rlEnableDepthMask();
}
