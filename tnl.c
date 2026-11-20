#include "tnl.h"

/* =========================================================================
   État
   ========================================================================= */

/* Matrice 3 x 4 : rotation/échelle (3 x 3) et translation (colonne 3).
   La quatrième ligne d'une matrice OpenGL (0 0 0 1) est implicite. */
typedef GLfixed Matrix[3][4];

static Matrix modelview = {
    {FIXED_ONE, 0, 0, 0}, {0, FIXED_ONE, 0, 0}, {0, 0, FIXED_ONE, 0}};
static Matrix stack[GL_MAX_MODELVIEW_STACK_DEPTH];
static int stackDepth = 0;

/* Fenêtre (pixels) */
static int viewX = 0, viewY = 0, viewWidth = 320, viewHeight = 224;

/* Pyramide de vue */
static GLfixed frustumLeft = -FIXED_ONE, frustumRight = FIXED_ONE;
static GLfixed frustumBottom = -FIXED_ONE, frustumTop = FIXED_ONE;
static GLfixed frustumNear = FIXED_ONE;

/* Pixels par unité du plan proche, déduits de la fenêtre et de la
   pyramide (recalculés quand l'une ou l'autre change) */
static GLfixed scaleX, scaleY;

/* Primitive en cours */
static int vertexCount = 0;
static GrVertex triangle[3];

extern const Fixed tnl_sinTable[1025];

static void multiplyRight(const Matrix r);
static void updateScale(void);

/* =========================================================================
   Fenêtre et projection
   ========================================================================= */

void glViewport(int x, int y, int width, int height) {
  viewX = x;
  viewY = y;
  viewWidth = width;
  viewHeight = height;
  updateScale();
}

void glFrustum(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
                GLfixed near, GLfixed far) {
  (void)far; /* pas encore de plan lointain */

  frustumLeft = left;
  frustumRight = right;
  frustumBottom = bottom;
  frustumTop = top;
  frustumNear = near;
  updateScale();
}

/* Le plan proche, de left à right et de bottom à top, couvre la fenêtre */
static void updateScale(void) {
  scaleX = fixed_div(FIXED(viewWidth), frustumRight - frustumLeft);
  scaleY = fixed_div(FIXED(viewHeight), frustumTop - frustumBottom);
}

/* Projette un point du repère de l'œil vers l'écran (GrVertex) */
static void project(GLfixed x, GLfixed y, GLfixed z, GrVertex *out) {
  /* Distance devant l'œil (qui regarde vers les z négatifs). Sans
     découpage, un point derrière l'œil donnerait n'importe quoi : on se
     contente d'éviter la division par zéro. */
  int64_t d = -(int64_t)z;
  if (d < 1)
    d = 1;

  /* Point sur le plan proche (une division par coordonnée) */
  const GLfixed xn = (GLfixed)(((int64_t)frustumNear * x) / d);
  const GLfixed yn = (GLfixed)(((int64_t)frustumNear * y) / d);

  /* Puis dans la fenêtre ; y descend à l'écran */
  out->x = FIXED(viewX) + fixed_mul(xn - frustumLeft, scaleX);
  out->y = FIXED(viewY) + fixed_mul(frustumTop - yn, scaleY);

  /* ooz = 65535 * near / d, en 16.16, borné devant le plan proche */
  int64_t ooz = ((int64_t)65535 * frustumNear * FIXED_ONE) / d;
  if (ooz > (int64_t)GR_DEPTH(65535))
    ooz = GR_DEPTH(65535);
  out->ooz = (GrDepth)ooz;
}

/* =========================================================================
   Matrice de modélisation
   ========================================================================= */

void glLoadIdentity(void) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 4; j++)
      modelview[i][j] = (i == j) ? FIXED_ONE : 0;
}

void glTranslate(GLfixed x, GLfixed y, GLfixed z) {
  const Matrix t = {
      {FIXED_ONE, 0, 0, x}, {0, FIXED_ONE, 0, y}, {0, 0, FIXED_ONE, z}};
  multiplyRight(t);
}

void glScale(GLfixed x, GLfixed y, GLfixed z) {
  const Matrix s = {{x, 0, 0, 0}, {0, y, 0, 0}, {0, 0, z, 0}};
  multiplyRight(s);
}

/* Racine carrée entière (partie entière), par la méthode bit à bit */
static uint64_t isqrt64(uint64_t v) {
  uint64_t result = 0;
  uint64_t bit = (uint64_t)1 << 62;

  while (bit > v)
    bit >>= 2;
  while (bit != 0) {
    if (v >= result + bit) {
      v -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }
  return result;
}

void glRotate(GLangle angle, GLfixed x, GLfixed y, GLfixed z) {
  /* Axe normé : longueur = racine de (x² + y² + z²), en 16.16 */
  const uint64_t sq = (uint64_t)((int64_t)x * x) + (uint64_t)((int64_t)y * y) +
                      (uint64_t)((int64_t)z * z);
  const GLfixed length = (GLfixed)isqrt64(sq);
  if (length == 0)
    return; /* pas d'axe : OpenGL ne définit rien, on ne fait rien */

  x = fixed_div(x, length);
  y = fixed_div(y, length);
  z = fixed_div(z, length);

  const GLfixed c = tnl_cos(angle);
  const GLfixed s = tnl_sin(angle);
  const GLfixed t = FIXED_ONE - c;

  /* Matrice de rotation d'OpenGL (formule de Rodrigues) */
  const Matrix r = {
      {fixed_mul(fixed_mul(x, x), t) + c, fixed_mul(fixed_mul(x, y), t) - fixed_mul(z, s),
       fixed_mul(fixed_mul(x, z), t) + fixed_mul(y, s), 0},
      {fixed_mul(fixed_mul(y, x), t) + fixed_mul(z, s), fixed_mul(fixed_mul(y, y), t) + c,
       fixed_mul(fixed_mul(y, z), t) - fixed_mul(x, s), 0},
      {fixed_mul(fixed_mul(x, z), t) - fixed_mul(y, s),
       fixed_mul(fixed_mul(y, z), t) + fixed_mul(x, s), fixed_mul(fixed_mul(z, z), t) + c, 0}};
  multiplyRight(r);
}

void glPushMatrix(void) {
  if (stackDepth >= GL_MAX_MODELVIEW_STACK_DEPTH)
    return; /* OpenGL : GL_STACK_OVERFLOW, la commande est ignorée */

  GLfixed *dst = &stack[stackDepth++][0][0];
  const GLfixed *src = &modelview[0][0];
  for (int i = 0; i < 12; i++)
    *dst++ = *src++;
}

void glPopMatrix(void) {
  if (stackDepth == 0)
    return; /* OpenGL : GL_STACK_UNDERFLOW, la commande est ignorée */

  const GLfixed *src = &stack[--stackDepth][0][0];
  GLfixed *dst = &modelview[0][0];
  for (int i = 0; i < 12; i++)
    *dst++ = *src++;
}

/* modelview = modelview * r (r : matrice affine 3 x 4) */
static void multiplyRight(const Matrix r) {
  Matrix result;

  for (int i = 0; i < 3; i++) {
    const GLfixed *m = modelview[i];
    for (int j = 0; j < 4; j++) {
      /* Somme en 64 bits, un seul décalage : moins d'arrondis */
      int64_t sum = (int64_t)m[0] * r[0][j] + (int64_t)m[1] * r[1][j] +
                    (int64_t)m[2] * r[2][j];
      sum >>= 16;
      if (j == 3)
        sum += m[3]; /* la translation de modelview se garde telle quelle */
      result[i][j] = (GLfixed)sum;
    }
  }

  GLfixed *dst = &modelview[0][0];
  const GLfixed *src = &result[0][0];
  for (int i = 0; i < 12; i++)
    *dst++ = *src++;
}

/* =========================================================================
   Sommets
   ========================================================================= */

void glIndexub(GLubyte index) { grConstantColorValue(index); }

void glBegin(GLenum mode) {
  (void)mode; /* GL_TRIANGLES seulement */
  vertexCount = 0;
}

void glVertex3(GLfixed x, GLfixed y, GLfixed z) {
  const GLfixed *m0 = modelview[0];
  const GLfixed *m1 = modelview[1];
  const GLfixed *m2 = modelview[2];

  /* Transformation : repère de l'objet -> repère de l'œil */
  const GLfixed ex = (GLfixed)((((int64_t)m0[0] * x + (int64_t)m0[1] * y +
                                 (int64_t)m0[2] * z) >> 16) + m0[3]);
  const GLfixed ey = (GLfixed)((((int64_t)m1[0] * x + (int64_t)m1[1] * y +
                                 (int64_t)m1[2] * z) >> 16) + m1[3]);
  const GLfixed ez = (GLfixed)((((int64_t)m2[0] * x + (int64_t)m2[1] * y +
                                 (int64_t)m2[2] * z) >> 16) + m2[3]);

  project(ex, ey, ez, &triangle[vertexCount]);

  if (++vertexCount == 3) {
    grDrawTriangle(&triangle[0], &triangle[1], &triangle[2]);
    vertexCount = 0;
  }
}

void glEnd(void) { vertexCount = 0; /* un triangle incomplet est oublié */ }

/* =========================================================================
   Trigonométrie
   ========================================================================= */

GLfixed tnl_sin(GLangle angle) {
  /* 65536 unités par tour -> 4096 pas (arrondi au plus proche) */
  const int step = ((angle + 8) >> 4) & 4095;
  const int i = step & 1023;

  switch (step >> 10) { /* quart de tour */
  case 0:  return tnl_sinTable[i];
  case 1:  return tnl_sinTable[1024 - i];
  case 2:  return -tnl_sinTable[i];
  default: return -tnl_sinTable[1024 - i];
  }
}

GLfixed tnl_cos(GLangle angle) { return tnl_sin(angle + 16384); }
