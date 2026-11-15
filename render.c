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

static void RenderInit(void) {
 gSh = LoadShaderFromMemory(VS_LIT, FS_LIT);
 locEmis = GetShaderLocation(gSh, "emissive"); locMetal = GetShaderLocation(gSh, "metal"); locView = GetShaderLocation(gSh, "viewPos");
 gFloorSh = LoadShaderFromMemory(VS_FLOOR, FS_FLOOR);
 locBg = GetShaderLocation(gFloorSh, "bg"); locCtr = GetShaderLocation(gFloorSh, "ctr"); locShR = GetShaderLocation(gFloorSh, "shR");
 gMat = LoadMaterialDefault(); gMat.shader = gSh;
 gFloorMat = LoadMaterialDefault(); gFloorMat.shader = gFloorSh;
 mCube = GenMeshCube(1, 1, 1); mCyl = GenMeshCylinder(1, 1, 28); mSph = GenMeshSphere(1, 18, 24); mCone = GenMeshCone(1, 1, 28);
 mPlane = GenMeshPlane(200, 200, 1, 1);
}

static void Put(Mesh m, Matrix local, Color c, float metal, Vector3 emis) {
 gMat.maps[MATERIAL_MAP_DIFFUSE].color = c;
 SetShaderValue(gSh, locMetal, &metal, SHADER_UNIFORM_FLOAT);
 SetShaderValue(gSh, locEmis, &emis, SHADER_UNIFORM_VEC3);
 DrawMesh(m, gMat, MatrixMultiply(local, gBase));
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
// frustum: two cones would be hacky; build tapered pipe from short cylinders
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
 Vector3 exhOut, intakeTop;
} Geo;
static Geo G;

static Vector3 BP(float a, float x, float v, float w) { return (Vector3){x, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }
static Vector3 BD(float a, float v, float w) { return (Vector3){0, v * cosf(a) - w * sinf(a), v * sinf(a) + w * cosf(a)}; }

static void GeoCompute(const Build *b) {
 Geo *g = &G;
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
 float cyc = b->cycle ? 360.f : 720.f;
 for (int i = 0; i < n; i++) {
  if (g->nb == 2) {
   int bk = i & 1, j = i >> 1;
   g->bank[i] = bk; g->a[i] = (bk ? -half : half) + tlt;
   g->x[i] = (j - (g->m - 1) * 0.5f) * g->pitch + (bk ? 0.12f : -0.12f) * g->pitch;
  } else { g->bank[i] = 0; g->a[i] = tlt; g->x[i] = (i - (n - 1) * 0.5f) * g->pitch; }
  g->tdc[i] = AU.fire[i];
  (void)cyc;
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
 float lowest = g->ccBot - g->panH;
 if (b->layout == 4) lowest = fminf(lowest, -(g->headW * 0.5f + 40));
 for (int bk = 0; bk < g->nb; bk++) { float a2 = g->bankA[bk]; for (int sd = -1; sd <= 1; sd += 2) { Vector3 p = BP(a2, 0, g->deck + g->headH, sd * g->headW * 0.5f); lowest = fminf(lowest, p.y - 30); } }
 g->H = (-lowest + 300) / 100.f;
 float fw = b->flywheel == 0 ? 0.85f : (b->flywheel == 2 ? 1.15f : 1.f);
 g->flyR = fminf((b->stroke * 0.9f + 70) * fw, -lowest + 250);
 g->flyT = 22 * FLY[b->flywheel].mul + 6;
 for (int i = 0; i < n; i++) {
  float a = g->a[i];
  float is = (a > 0.01f) ? -1.f : 1.f;
  float vp = g->deck + g->headH * 0.45f;
  g->inP[i] = BP(a, g->x[i], vp, is * g->headW * 0.5f);
  g->inD[i] = BD(a, 0, is);
  g->exP[i] = BP(a, g->x[i], vp, -is * g->headW * 0.5f);
  g->exD[i] = BD(a, 0, -is);
 }
}

static Color Shade(Color c, float k) { return (Color){(unsigned char)Clampf(c.r * k, 0, 255), (unsigned char)Clampf(c.g * k, 0, 255), (unsigned char)Clampf(c.b * k, 0, 255), c.a}; }

static const Color C_STEEL = {150, 152, 158, 255}, C_DARK = {38, 40, 44, 255}, C_BLACK = {24, 25, 28, 255}, C_CHROME = {225, 228, 235, 255};
static const Color C_RUBBER = {20, 20, 22, 255}, C_COPPER = {184, 115, 60, 255}, C_PISTON = {200, 200, 205, 255};

static void DrawFilter(int type, Vector3 at, Vector3 dir, float s) {
 Vector3 e = Vector3Add(at, Vector3Scale(dir, 6));
 switch (type) {
 case 0: ConeP(Vector3Add(at, Vector3Scale(dir, 40 * s)), at, 34 * s, C_CHROME, 0.9f); Cyl(at, Vector3Add(at, Vector3Scale(dir, 42 * s)), 22 * s, C_CHROME, 0.9f); break;
 case 1: Cyl(e, Vector3Add(e, Vector3Scale(dir, 55 * s)), 105 * s, C_BLACK, 0.4f);
         Cyl(Vector3Add(e, Vector3Scale(dir, 54 * s)), Vector3Add(e, Vector3Scale(dir, 60 * s)), 60 * s, C_CHROME, 0.9f); break;
 case 2: SphE(Vector3Add(e, Vector3Scale(dir, 45 * s)), (Vector3){55 * s, 55 * s, 55 * s}, (Color){230, 120, 30, 255}, 0.0f, gNoEmis); break;
 case 3: Cyl(e, Vector3Add(e, Vector3Scale(dir, 14 * s)), 50 * s, C_CHROME, 0.9f);
         ConeP(Vector3Add(e, Vector3Scale(dir, 12 * s)), Vector3Add(e, Vector3Scale(dir, 150 * s)), 62 * s, (Color){190, 30, 35, 255}, 0.1f); break;
 case 4: Cyl(e, Vector3Add(e, Vector3Scale(dir, 150 * s)), 70 * s, C_BLACK, 0.3f);
         Sph(Vector3Add(e, Vector3Scale(dir, 150 * s)), 70 * s, C_BLACK, 0.3f); break;
 }
}

static void DrawTurbo(Vector3 c, float k, Vector3 *compOut, Vector3 *compIn, Vector3 *turbIn, Vector3 *turbOut) {
 SphE(c, (Vector3){45 * k, 62 * k, 62 * k}, (Color){90, 80, 72, 255}, 0.5f, gNoEmis);
 Cyl((Vector3){c.x + 20 * k, c.y, c.z}, (Vector3){c.x + 62 * k, c.y, c.z}, 26 * k, C_STEEL, 0.6f);
 Cyl((Vector3){c.x + 60 * k, c.y, c.z}, (Vector3){c.x + 112 * k, c.y, c.z}, 60 * k, (Color){200, 202, 206, 255}, 0.8f);
 Cyl((Vector3){c.x + 110 * k, c.y, c.z}, (Vector3){c.x + 135 * k, c.y, c.z}, 36 * k, (Color){200, 202, 206, 255}, 0.8f);
 *compIn = (Vector3){c.x + 135 * k, c.y, c.z};
 *compOut = (Vector3){c.x + 86 * k, c.y + 58 * k, c.z};
 *turbIn = (Vector3){c.x, c.y + 40 * k, c.z};
 *turbOut = (Vector3){c.x - 20 * k, c.y - 50 * k, c.z};
}

typedef struct { int cut; float fanAng; float vis; float glow; float pullT; } DrawState;

static void DrawEngine(const Build *b, const Spec *s, float crankDeg, DrawState *ds) {
 Geo *g = &G;
 Color blk = BLOCKS[b->block].col;
 Color headC = b->cooling == 0 ? Shade(blk, 1.05f) : Shade(blk, 1.08f);
 Color cover = C_BLACK;
 if (!b->cycle) cover = b->valvetrain == 3 ? (Color){175, 28, 30, 255} : (b->valvetrain == 2 ? (Color){30, 32, 36, 255} : (b->valvetrain == 1 ? (Color){38, 72, 160, 255} : Shade(blk, 1.0f)));
 float glowT = Clampf((ds->glow - 320) / 520.f, 0, 1);
 Vector3 glow = {glowT * glowT * 0.95f, glowT * glowT * 0.28f, glowT * glowT * 0.05f};
 Color pipeC = Shade((Color){120, 112, 104, 255}, 1 - glowT * 0.4f);
 float frontX = -g->L * 0.5f, rearX = g->L * 0.5f;
 // crankcase
 if (!ds->cut) Box((Vector3){0, (g->ccTop + g->ccBot) * 0.5f, 0}, (Vector3){g->L, g->ccTop - g->ccBot, g->ccW}, MatrixIdentity(), blk, 0.35f);
 else Box((Vector3){0, (g->ccTop + g->ccBot) * 0.5f, -g->ccW * 0.3f}, (Vector3){g->L, g->ccTop - g->ccBot, g->ccW * 0.4f}, MatrixIdentity(), blk, 0.35f);
 if (b->layout == 0 || b->cycle) {
  float rr = fminf(g->ccTop - g->ccBot, g->ccW) * 0.5f;
  Vector3 cc = {0, (g->ccTop + g->ccBot) * 0.5f, 0};
  if (!ds->cut) Cyl((Vector3){frontX - 6, cc.y, 0}, (Vector3){rearX + 6, cc.y, 0}, rr * 0.92f, Shade(blk, 1.1f), 0.5f);
 }
 if (g->panH > 0) Box((Vector3){0, g->ccBot - g->panH * 0.5f + 2, 0}, (Vector3){g->L * 0.82f, g->panH + 4, g->ccW * 0.86f}, MatrixIdentity(), b->block == 0 ? C_DARK : Shade(blk, 0.95f), 0.4f);
 // banks
 for (int bk = 0; bk < g->nb; bk++) {
  float a = g->bankA[bk];
  Matrix R = MatrixRotateX(a);
  float x0 = g->bx0[bk], x1 = g->bx1[bk], xm = (x0 + x1) * 0.5f, len = x1 - x0;
  if (b->cooling > 0 && !ds->cut) Box(BP(a, xm, (g->v0 + g->deck) * 0.5f, 0), (Vector3){len + 0.12f * g->pitch, g->deck - g->v0, g->jw}, R, blk, 0.35f);
  if (b->cooling > 0) {
   Box(BP(a, xm, g->deck + g->headH * 0.5f - 1, 0), (Vector3){len + 0.12f * g->pitch + 12, g->headH + 2, g->headW}, R, headC, 0.45f);
   if (!b->cycle && b->valvetrain > 0) {
    float ch = b->bore * (b->valvetrain == 1 ? 0.2f : 0.26f);
    float cl = len + 0.12f * g->pitch - 20;
    if (b->valvetrain == 3) {
     for (int sd = -1; sd <= 1; sd += 2)
      Box(BP(a, xm, g->deck + g->headH + ch * 0.5f - 2, sd * b->bore * 0.3f), (Vector3){cl, ch, b->bore * 0.42f}, R, cover, 0.35f);
    } else Box(BP(a, xm, g->deck + g->headH + ch * 0.5f - 2, 0), (Vector3){cl, ch, b->bore * (b->valvetrain == 1 ? 1.1f : 0.8f)}, R, cover, 0.35f);
   }
  }
 }
 // cylinders (air-cooled barrels + finned heads)
 for (int i = 0; i < g->n; i++) {
  float a = g->a[i], x = g->x[i];
  Matrix R = MatrixRotateX(a);
  if (b->cooling == 0) {
   if (!ds->cut) {
    Cyl(BP(a, x, g->v0, 0), BP(a, x, g->deck, 0), b->bore * 0.56f, Shade(blk, 0.9f), 0.4f);
    float fr = fminf(b->bore * 0.8f, g->pitch * 0.62f);
    float off = (i & 1) ? 3.5f : 0;
    for (float v = g->v0 + 10 + off; v < g->deck - 3; v += 7)
     Cyl(BP(a, x, v, 0), BP(a, x, v + 2.4f, 0), fr, blk, 0.35f);
   }
   Box(BP(a, x, g->deck + g->headH * 0.5f - 1, 0), (Vector3){g->pitch * 0.86f, g->headH + 2, g->headW * 0.95f}, R, headC, 0.45f);
   for (float v = g->deck + 5; v < g->deck + g->headH - 2; v += 7)
    Box(BP(a, x, v, 0), (Vector3){g->pitch * 0.94f, 2.2f, g->headW * 1.28f}, R, Shade(blk, 1.0f), 0.35f);
   if (!b->cycle && b->valvetrain > 0) {
    float ch = b->bore * 0.2f;
    Box(BP(a, x, g->deck + g->headH + ch * 0.5f - 2, 0), (Vector3){g->pitch * 0.7f, ch, b->bore * 0.8f}, R, cover, 0.35f);
   }
  }
  // spark plug
  if (!FUELS[b->fuel].diesel) {
   Vector3 p0, p1;
   if (b->cycle || b->valvetrain == 3 || b->valvetrain == 0) {
    float top = g->deck + g->headH + (b->valvetrain == 3 ? 0 : 0);
    p0 = BP(a, x, top - 8, 0); p1 = BP(a, x, top + 28, 0);
   } else {
    float is = (a > 0.01f) ? -1.f : 1.f;
    p0 = BP(a, x, g->deck + g->headH * 0.4f, -is * g->headW * 0.42f);
    p1 = BP(a, x, g->deck + g->headH * 0.4f + 16, -is * (g->headW * 0.5f + 22));
   }
   Cyl(p0, p1, 6.5f, (Color){235, 235, 230, 255}, 0.1f);
   Cyl(p1, Vector3Add(p1, Vector3Scale(Vector3Normalize(Vector3Subtract(p1, p0)), 10)), 4.5f, C_STEEL, 0.7f);
  }
 }
 // rotating assembly
 float rr = g->r;
 if (ds->cut) {
  Cyl((Vector3){frontX - 4, 0, 0}, (Vector3){rearX + 4, 0, 0}, fmaxf(rr * 0.45f, 9), C_STEEL, 0.8f);
  for (int i = 0; i < g->n; i++) {
   float a = g->a[i];
   float beta = (crankDeg - g->tdc[i]) * DEG2RAD + a;
   float th = beta - a;
   Vector3 pin = {g->x[i], rr * cosf(beta), rr * sinf(beta)};
   float sv = rr * cosf(th) + sqrtf(fmaxf(g->l * g->l - rr * rr * sinf(th) * sinf(th), 1));
   Vector3 pp = BP(a, g->x[i], sv, 0);
   float cw = b->bore * 0.22f;
   Matrix Rc = MatrixRotateX(beta);
   Box(BP(beta, g->x[i] - cw * 0.75f, rr * 0.4f, 0), (Vector3){cw * 0.5f, rr * 1.9f, rr * 0.9f}, Rc, C_STEEL, 0.7f);
   Box(BP(beta, g->x[i] + cw * 0.75f, rr * 0.4f, 0), (Vector3){cw * 0.5f, rr * 1.9f, rr * 0.9f}, Rc, C_STEEL, 0.7f);
   Box(BP(beta, g->x[i], -rr * 0.8f, 0), (Vector3){cw * 2.2f, rr * 0.7f, rr * 1.7f}, Rc, Shade(C_STEEL, 0.85f), 0.7f);
   Cyl((Vector3){g->x[i] - cw, pin.y, pin.z}, (Vector3){g->x[i] + cw, pin.y, pin.z}, fmaxf(rr * 0.32f, 6), C_CHROME, 0.9f);
   Cyl(pin, pp, fmaxf(b->bore * 0.075f, 4), (Color){170, 172, 178, 255}, 0.8f);
   Cyl(BP(a, g->x[i], sv - b->bore * 0.3f, 0), BP(a, g->x[i], sv + b->bore * 0.25f, 0), b->bore * 0.49f, C_PISTON, 0.75f);
   Cyl(BP(a, g->x[i], sv + b->bore * 0.18f, 0), BP(a, g->x[i], sv + b->bore * 0.2f, 0), b->bore * 0.495f, C_DARK, 0.5f);
  }
 }
 // front: pulley / recoil
 float px0 = frontX - 30, px1 = frontX - 10;
 float pulR = rr * 0.8f + 28;
 Matrix Rspin = MatrixRotateX(crankDeg * DEG2RAD);
 if (b->alternator || b->starter == 1 || b->cooling > 0 || b->forced == 4) {
  Cyl((Vector3){px1 + 6, 0, 0}, (Vector3){px0, 0, 0}, pulR, C_DARK, 0.6f);
  Box(BP(crankDeg * DEG2RAD, px0 - 0.5f, pulR * 0.75f, 0), (Vector3){3, pulR * 0.35f, 5}, Rspin, (Color){240, 240, 240, 255}, 0);
 }
 if (b->starter == 0) {
  float rx0 = (b->alternator || b->cooling > 0) ? px0 - 6 : frontX - 6;
  float rcR = fmaxf(pulR * 1.1f, 48 + b->stroke * 0.5f);
  Cyl((Vector3){rx0, 0, 0}, (Vector3){rx0 - 38, 0, 0}, rcR, (Color){200, 40, 35, 255}, 0.3f);
  Cyl((Vector3){rx0 - 38, 0, 0}, (Vector3){rx0 - 44, 0, 0}, rcR * 0.4f, C_DARK, 0.5f);
  float ext = ds->pullT > 0 ? 260 * sinf(Clampf(ds->pullT, 0, 1) * PI) : 0;
  Vector3 hole = {rx0 - 20, rcR * 0.55f, rcR * 0.85f};
  Vector3 hnd = {hole.x - 8 - ext * 0.3f, hole.y + 12 + ext * 0.5f, hole.z + 18 + ext * 0.7f};
  if (ext > 1) Cyl(hole, hnd, 2.0f, (Color){230, 220, 190, 255}, 0);
  Cyl((Vector3){hnd.x - 28, hnd.y, hnd.z}, (Vector3){hnd.x + 28, hnd.y, hnd.z}, 9, C_BLACK, 0.2f);
 }
 // alternator + belt
 Vector3 altC = {frontX + 30, g->ccTop * 0.35f + rr * 0.3f, -(g->ccW * 0.5f + 62)};
 if (b->layout == 4) altC = (Vector3){frontX + 30, g->ccTop + 70, -60};
 float altPR = 26;
 if (b->alternator) {
  Cyl((Vector3){frontX - 8, altC.y, altC.z}, (Vector3){frontX + 75, altC.y, altC.z}, 55, (Color){165, 168, 172, 255}, 0.7f);
  Cyl((Vector3){px1, altC.y, altC.z}, (Vector3){px0, altC.y, altC.z}, altPR, C_DARK, 0.6f);
  float dy = altC.y, dz = altC.z, d = sqrtf(dy * dy + dz * dz);
  float ny = -dz / d, nz = dy / d;
  float bx = (px0 + px1) * 0.5f;
  for (int sd = -1; sd <= 1; sd += 2) {
   Vector3 p0 = {bx, ny * pulR * sd, nz * pulR * sd}, p1 = {bx, altC.y + ny * altPR * sd, altC.z + nz * altPR * sd};
   Cyl(p0, p1, 3.5f, C_RUBBER, 0.1f);
  }
 }
 // flywheel & starter
 float fx0 = rearX + 12;
 Cyl((Vector3){fx0, 0, 0}, (Vector3){fx0 + g->flyT, 0, 0}, g->flyR, (Color){110, 112, 118, 255}, 0.8f);
 Cyl((Vector3){fx0 + g->flyT * 0.25f, 0, 0}, (Vector3){fx0 + g->flyT * 0.75f, 0, 0}, g->flyR + 6, (Color){80, 82, 88, 255}, 0.7f);
 for (int k = 0; k < 6; k++) {
  float an = crankDeg * DEG2RAD + k * PI / 3;
  Vector3 bp = {fx0 + g->flyT + 1.5f, cosf(an) * g->flyR * 0.6f, sinf(an) * g->flyR * 0.6f};
  Cyl((Vector3){bp.x - 3, bp.y, bp.z}, bp, 6, C_CHROME, 0.9f);
 }
 if (b->starter == 1) {
  Vector3 sc = {rearX - 40, -g->flyR * 0.55f, -g->flyR * 0.72f - 10};
  Cyl((Vector3){sc.x - 50, sc.y, sc.z}, (Vector3){sc.x + 55, sc.y, sc.z}, 36, C_BLACK, 0.4f);
  Cyl((Vector3){sc.x - 30, sc.y + 40, sc.z}, (Vector3){sc.x + 30, sc.y + 40, sc.z}, 18, C_STEEL, 0.6f);
 }
 // intake
 int vtop = g->nb == 2;
 Vector3 E, D;
 float pr = fmaxf(b->bore * 0.16f, 9);
 Color plenC = b->block == 0 ? (Color){150, 154, 160, 255} : Shade(blk, 1.1f);
 if (g->n == 1) {
  E = Vector3Add(g->inP[0], Vector3Scale(g->inD[0], 30)); D = g->inD[0];
  Cyl(g->inP[0], E, pr * 1.3f, plenC, 0.5f);
 } else if (!vtop && b->layout != 4) {
  float z = g->inP[0].z + g->inD[0].z * 80, y = g->inP[0].y + 10;
  for (int i = 0; i < g->n; i++) { Vector3 pts[3] = {g->inP[i], {g->x[i], g->inP[i].y + 5, g->inP[i].z + g->inD[i].z * 40}, {g->x[i], y, z}}; Pipe(pts, 3, pr, plenC, 0.5f, gNoEmis); }
  float pl = g->L * 0.9f;
  Box((Vector3){0, y, z}, (Vector3){pl, 72, 64}, MatrixIdentity(), plenC, 0.5f);
  E = (Vector3){-pl * 0.5f, y, z}; D = (Vector3){-1, 0, 0};
 } else {
  float top = -1e9f;
  for (int i = 0; i < g->n; i++) top = fmaxf(top, g->inP[i].y);
  float y = top + 55;
  for (int i = 0; i < g->n; i++) {
   Vector3 m = Vector3Add(g->inP[i], Vector3Scale(g->inD[i], 25));
   Vector3 pts[3] = {g->inP[i], m, {g->x[i], y - 20, g->inP[i].z > 0 ? 28.f : -28.f}};
   Pipe(pts, 3, pr, plenC, 0.5f, gNoEmis);
  }
  float pl = g->L * 0.82f;
  Box((Vector3){0, y, 0}, (Vector3){pl, 60, 120}, MatrixIdentity(), plenC, 0.5f);
  E = (Vector3){0, y + 30, 0}; D = (Vector3){0, 1, 0};
 }
 if (b->forced == 4) {
  Vector3 bc = Vector3Add(E, Vector3Scale(D, 55));
  float bl = fmaxf(g->L * 0.55f, 160);
  if (D.y > 0.5f) {
   Box(bc, (Vector3){bl, 110, 150}, MatrixIdentity(), (Color){190, 192, 198, 255}, 0.8f);
   for (int k = -3; k <= 3; k++) Box((Vector3){bc.x + k * bl / 8.f, bc.y + 56, bc.z}, (Vector3){6, 6, 150}, MatrixIdentity(), C_CHROME, 0.9f);
   Vector3 sp = {-bl * 0.5f - 18, bc.y, bc.z};
   Cyl((Vector3){sp.x + 12, sp.y, sp.z}, (Vector3){px0, sp.y, sp.z}, 34, C_DARK, 0.6f);
   float bx = (px0 + px1) * 0.5f;
   Cyl((Vector3){bx, pulR, 0}, (Vector3){bx, sp.y + 34, 0}, 4, C_RUBBER, 0); Cyl((Vector3){bx, -pulR, 0}, (Vector3){bx, sp.y - 34, 0}, 4, C_RUBBER, 0);
   (void)sp;
  } else {
   Box(Vector3Add(bc, Vector3Scale(D, 20)), (Vector3){120, 100, 100}, MatrixIdentity(), (Color){190, 192, 198, 255}, 0.8f);
  }
  E = Vector3Add(E, Vector3Scale(D, 112));
 }
 Vector3 tbEnd = Vector3Add(E, Vector3Scale(D, 48));
 Color tbC = b->delivery == 0 ? (Color){200, 175, 95, 255} : (Color){170, 172, 178, 255};
 if (b->delivery == 0) Box(Vector3Add(E, Vector3Scale(D, 24)), (Vector3){D.x != 0 ? 48 : 70, D.y != 0 ? 48 : 60, D.z != 0 ? 48 : 64}, MatrixIdentity(), tbC, 0.6f);
 else Cyl(E, tbEnd, fmaxf(b->bore * 0.3f, 18), tbC, 0.7f);
 float fs = Clampf(sqrtf(s->dispL) * 0.75f, 0.45f, 1.5f);
 // exhaust
 Vector3 col[2];
 float pipeR = fmaxf(b->bore * 0.15f, 9);
 float yLow = fminf(g->ccBot - g->panH - 45, -g->headW * 0.6f - 50);
 if (b->layout == 4) yLow = -g->headW * 0.6f - 60;
 for (int bk = 0; bk < g->nb; bk++) {
  int first = -1;
  for (int i = 0; i < g->n; i++) if (g->bank[i] == bk) { first = i; break; }
  if (first < 0) { col[bk] = (Vector3){0, yLow, 0}; continue; }
  Vector3 d0 = g->exD[first];
  float cx = g->bx1[bk] - g->pitch * 0.3f;
  col[bk] = (Vector3){cx + 40, g->exP[first].y + (d0.y < -0.5f ? -95 : -110), g->exP[first].z + d0.z * 105};
  if (b->layout == 4) col[bk] = (Vector3){cx + 40, g->exP[first].y - 70, g->exP[first].z * 0.75f};
 }
 int exp2t = b->cycle && b->exhaust == 3;
 for (int i = 0; i < g->n; i++) {
  if (exp2t) break;
  int bk = g->bank[i];
  Vector3 p1 = Vector3Add(g->exP[i], Vector3Scale(g->exD[i], 45));
  Vector3 pts[3] = {Vector3Subtract(g->exP[i], Vector3Scale(g->exD[i], 4)), p1, col[bk]};
  Pipe(pts, 3, pipeR, pipeC, 0.4f, glow);
 }
 Vector3 outEnd;
 if (exp2t) {
  Vector3 d = g->exD[0];
  Vector3 p0 = g->exP[0], p1 = Vector3Add(p0, Vector3Scale(d, 40));
  Vector3 p2 = {p1.x - 20, p1.y - 90, p1.z + d.z * 50};
  Vector3 p3 = {rearX + 60, yLow + 30, p2.z};
  Vector3 p4 = {p3.x + 240, p3.y + 10, p3.z};
  Vector3 p5 = {p4.x + 120, p4.y + 30, p4.z};
  Vector3 pts[3] = {Vector3Subtract(p0, Vector3Scale(d, 4)), p1, p2};
  Pipe(pts, 3, pipeR, pipeC, 0.4f, glow);
  Sph(p2, pipeR, pipeC, 0.4f);
  Taper(p2, p3, pipeR, pipeR * 2.6f, pipeC, 0.4f, glow);
  Sph(p3, pipeR * 2.6f, pipeC, 0.4f);
  Taper(p3, p4, pipeR * 2.6f, pipeR * 2.6f, pipeC, 0.4f, gNoEmis);
  Taper(p4, p5, pipeR * 2.6f, pipeR * 0.8f, pipeC, 0.4f, gNoEmis);
  Vector3 p6 = {p5.x + 120, p5.y, p5.z};
  Cyl(p5, p6, pipeR * 1.6f, (Color){60, 62, 66, 255}, 0.5f);
  outEnd = p6;
 } else {
  Vector3 src = col[0];
  Vector3 cIn = {0}, cOut = {0}, tIn, tOut;
  int hasT = b->forced >= 1 && b->forced <= 3;
  if (hasT) {
   float k = b->forced == 2 ? 1.3f : (b->forced == 3 ? 0.85f : 0.8f);
   k *= Clampf(sqrtf(s->dispL / 2.f), 0.6f, 1.6f);
   int twins = b->forced == 3 ? 2 : 1;
   Vector3 tc[2];
   for (int t = 0; t < twins; t++) {
    int bk = (g->nb == 2) ? t : 0;
    Vector3 cb = col[bk];
    float side = cb.z >= 0 ? 1.f : -1.f;
    if (g->nb == 1 && twins == 2) cb.x += (t ? 1 : -1) * g->L * 0.22f;
    tc[t] = (Vector3){cb.x - 60, cb.y + 10, cb.z + side * 110};
    DrawTurbo(tc[t], k, &cOut, &cIn, &tIn, &tOut);
    Vector3 pa[2] = {col[bk], tIn};
    Pipe(pa, 2, pipeR * 1.2f, pipeC, 0.4f, glow);
    DrawFilter(b->filter, cIn, (Vector3){1, 0, 0}, fs * 0.8f);
    Vector3 dst = Vector3Add(tbEnd, Vector3Scale(D, 4));
    if (b->intercooler) {
     Vector3 ic = {frontX - (b->cooling ? 300 : 200), -g->r * 0.6f, 0};
     Vector3 ici = {ic.x, ic.y + 40, side * 140}, ico = {ic.x, ic.y + 40, -side * 140};
     Vector3 q1[4] = {cOut, {cOut.x, cOut.y + 40, cOut.z}, {ic.x + 60, cOut.y + 40, ici.z}, ici};
     Pipe(q1, 4, 22, C_CHROME, 0.9f, gNoEmis);
     if (t == 0) {
      Box(ic, (Vector3){55, 150 + 30 * sqrtf(s->dispL), 340}, MatrixIdentity(), (Color){160, 165, 170, 255}, 0.7f);
      Vector3 q2[4] = {ico, {ic.x + 60, ico.y + 30, ico.z}, {dst.x - 80 * D.x, dst.y + 80, dst.z}, dst};
      if (D.y > 0.5f) { q2[2] = (Vector3){dst.x - 80, dst.y + 70, dst.z}; q2[3] = Vector3Add(dst, (Vector3){0, 30, 0}); }
      Pipe(q2, 4, 22, C_CHROME, 0.9f, gNoEmis);
     }
    } else if (t == 0) {
     Vector3 q1[4] = {cOut, {cOut.x, cOut.y + 50, cOut.z}, {dst.x - 60 * D.x, dst.y + 60 * (1 - D.y), dst.z + D.z * 0}, dst};
     if (D.y > 0.5f) q1[3] = Vector3Add(dst, (Vector3){0, 30, 0});
     Pipe(q1, 4, 22, C_CHROME, 0.9f, gNoEmis);
    }
   }
   if (g->nb == 2 && twins == 1) {
    Vector3 xo[3] = {col[1], {col[1].x + 40, yLow + 20, col[1].z}, {col[0].x + 40, yLow + 20, col[0].z}};
    Pipe(xo, 3, pipeR, pipeC, 0.4f, glow);
   }
   src = (Vector3){tc[0].x - 20 * k, tc[0].y - 50 * k, tc[0].z};
   Vector3 dp[2] = {src, {src.x + 40, yLow, src.z}};
   Pipe(dp, 2, pipeR * 1.3f, pipeC, 0.4f, glow);
   src = dp[1];
  } else {
   DrawFilter(b->filter, tbEnd, D, fs);
   Vector3 dp[2] = {col[0], {col[0].x + 30, yLow, col[0].z}};
   Pipe(dp, 2, pipeR * 1.2f, pipeC, 0.4f, glow);
   if (g->nb == 2) {
    Vector3 dq[3] = {col[1], {col[1].x + 30, yLow, col[1].z}, {col[0].x + 80, yLow, 0}};
    Pipe(dq, 3, pipeR * 1.2f, pipeC, 0.4f, glow);
   }
   src = dp[1];
  }
  float mz = g->nb == 2 && !hasT ? 0 : src.z;
  Vector3 m0 = {rearX + 80, yLow, mz};
  Vector3 run[3] = {src, {src.x + 50, yLow, (src.z + mz) * 0.5f}, m0};
  Pipe(run, 3, pipeR * 1.2f, pipeC, 0.4f, gNoEmis);
  float mr = (b->exhaust == 0 ? 70 : 48) * Clampf(sqrtf(s->dispL) * 0.8f, 0.5f, 1.4f);
  float ml = b->exhaust == 0 ? 320 : 210;
  if (b->exhaust <= 1 || b->exhaust == 3) {
   Cyl(m0, (Vector3){m0.x + ml, m0.y, m0.z}, mr, b->exhaust == 0 ? (Color){70, 72, 76, 255} : C_CHROME, 0.8f);
   Vector3 tp = {m0.x + ml + 60, m0.y, m0.z};
   Cyl((Vector3){m0.x + ml - 5, m0.y, m0.z}, tp, pipeR * 1.1f, C_CHROME, 0.9f);
   outEnd = tp;
  } else {
   Vector3 tp = {m0.x + 140, m0.y + 10, m0.z};
   Cyl(m0, tp, pipeR * 1.2f, pipeC, 0.4f);
   Taper(tp, (Vector3){tp.x + 30, tp.y, tp.z}, pipeR * 1.2f, pipeR * 1.5f, C_CHROME, 0.9f, gNoEmis);
   outEnd = (Vector3){tp.x + 30, tp.y, tp.z};
  }
 }
 g->exhOut = outEnd;
 // radiator + fan
 if (b->cooling > 0) {
  float rh = (240 + 60 * sqrtf(s->dispL)) * (b->cooling == 2 ? 1.25f : 1.f);
  float rw = (280 + 90 * sqrtf(s->dispL)) * (b->cooling == 2 ? 1.15f : 1.f);
  float rx = frontX - 200 - (b->starter == 0 ? 50 : 0);
  Vector3 rc = {rx, g->ccTop * 0.4f, 0};
  Box(rc, (Vector3){34, rh, rw}, MatrixIdentity(), (Color){48, 50, 54, 255}, 0.5f);
  Box((Vector3){rx, rc.y + rh * 0.5f + 12, 0}, (Vector3){44, 24, rw * 0.8f}, MatrixIdentity(), C_DARK, 0.5f);
  for (int k = -6; k <= 6; k++) Box((Vector3){rx - 18, rc.y, k * rw / 14.f}, (Vector3){2, rh * 0.95f, 3}, MatrixIdentity(), (Color){70, 72, 76, 255}, 0.4f);
  float fr = fminf(rh, rw) * 0.4f;
  Vector3 fc = {rx + 40, rc.y, 0};
  Cyl((Vector3){fc.x - 6, fc.y, 0}, (Vector3){fc.x + 6, fc.y, 0}, fr * 0.18f, C_DARK, 0.5f);
  for (int k = 0; k < 5; k++) {
   float an = ds->fanAng + k * 2 * PI / 5;
   Matrix rb = MatrixMultiply(MatrixRotateY(0.35f), MatrixRotateX(an));
   Box(Vector3Add(fc, (Vector3){0, cosf(an) * fr * 0.55f, sinf(an) * fr * 0.55f}), (Vector3){4, fr * 0.8f, fr * 0.28f}, rb, C_BLACK, 0.3f);
  }
  Vector3 h1[3] = {{rx + 18, rc.y + rh * 0.4f, rw * 0.25f}, {frontX - 60, rc.y + rh * 0.4f, rw * 0.2f}, {frontX + 10, g->ccTop, 0}};
  if (g->nb == 1) h1[2] = (Vector3){frontX + 10, g->deck, 0};
  Pipe(h1, 3, 16, C_RUBBER, 0.1f, gNoEmis);
  Vector3 h2[3] = {{rx + 18, rc.y - rh * 0.4f, -rw * 0.25f}, {frontX - 60, rc.y - rh * 0.35f, -rw * 0.2f}, {frontX + 10, 0 + rr * 0.6f, -g->ccW * 0.3f}};
  Pipe(h2, 3, 16, C_RUBBER, 0.1f, gNoEmis);
 }
 g->intakeTop = tbEnd;
}

static void DrawStand(const Geo *g) {
 Matrix save = gBase;
 gBase = MatrixScale(0.01f, 0.01f, 0.01f);
 float H = g->H * 100;
 float bot = H + g->ccBot - g->panH * 0.4f;
 Color sc = {200, 90, 30, 255};
 for (int sd = -1; sd <= 1; sd += 2) {
  float x = sd * (g->L * 0.5f - 20);
  Box((Vector3){x, 12, 0}, (Vector3){60, 34, g->ccW + 260}, MatrixIdentity(), sc, 0.3f);
  Box((Vector3){x, (bot + 10) * 0.5f, -g->ccW * 0.5f - 60}, (Vector3){44, bot + 10, 44}, MatrixIdentity(), sc, 0.3f);
  Box((Vector3){x, bot, -g->ccW * 0.5f - 26}, (Vector3){44, 40, 86}, MatrixIdentity(), sc, 0.3f);
 }
 Box((Vector3){0, 12, -g->ccW * 0.5f - 60}, (Vector3){g->L - 30, 30, 50}, MatrixIdentity(), Shade(sc, 0.85f), 0.3f);
 gBase = save;
}

static void DrawFloor(Vector3 bg, Vector3 ctr, float r) {
 SetShaderValue(gFloorSh, locBg, &bg, SHADER_UNIFORM_VEC3);
 SetShaderValue(gFloorSh, locCtr, &ctr, SHADER_UNIFORM_VEC3);
 SetShaderValue(gFloorSh, locShR, &r, SHADER_UNIFORM_FLOAT);
 DrawMesh(mPlane, gFloorMat, MatrixIdentity());
}

// smoke
typedef struct { Vector3 p, v; float life, max, size; Color c; } Puff;
#define NPUFF 300
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
