#ifndef TNL_H
#define TNL_H

#include "fixed.h"
#include "glide.h"

/* =========================================================================
   TnL (Transform and Lighting) : un sous-ensemble d'OpenGL 1.x, au-dessus
   de Glide, comme MiniGL de 3Dfx (écrit pour faire tourner GLQuake).
   Bibliothèque du jeu, C99, sans thread. Elle tourne sur le master, comme
   l'application sur le CPU hôte d'un PC équipé d'une Voodoo.

   Écarts volontaires avec OpenGL 1.x :
   - virgule fixe 16.16 partout (GLfixed). Les fonctions n'ont donc pas
     de suffixe de type : glVertex3, glRotate, glTranslate... au lieu de
     glVertex3f, glRotatef, glTranslatef ;
   - les angles sont en unités binaires, 65536 par tour, et non en
     degrés. GL_DEG fait la traduction ;
   - les couleurs sont des index de palette (mode « color index »
     d'OpenGL : glIndexub) ;
   - une seule matrice de modélisation (modelview) ; la projection est
     fixée par glFrustum, sans matrice (pas de glMatrixMode) ;
   - glViewport compte y depuis le HAUT de l'écran ;
   - pas encore de découpage par le plan proche : tout sommet doit rester
     au-delà du plan proche (-z >= near). Le plan lointain n'est pas
     utilisé.
   Pour l'effacement, le tampon de profondeur et l'échange de buffers, le
   jeu appelle directement Glide.

   Repère de l'œil, comme OpenGL : x vers la droite, y vers le haut,
   l'œil regarde vers les z NÉGATIFS.
   ========================================================================= */

typedef Fixed GLfixed;
typedef uint8_t GLubyte;
typedef int GLenum;

/* Angle : 65536 unités par tour (comme ANGLE dans la SGL de la Saturn) */
typedef int32_t GLangle;

/* Traduction degrés -> unités binaires, pour un nombre entier de degrés
   (GL_DEG(90) = 16384), calculée à la compilation pour une constante */
#define GL_DEG(d) ((GLangle)((d) * 65536 / 360))

/* Primitives de glBegin */
#define GL_TRIANGLES 0x0004

/* Profondeur de la pile de matrices (OpenGL en garantit au moins 32) */
#define GL_MAX_MODELVIEW_STACK_DEPTH 8

/* ---- Fenêtre et projection --------------------------------------------- */

/* Zone de l'écran où l'on dessine, en pixels (y depuis le haut). */
void glViewport(int x, int y, int width, int height);

/* Pyramide de vue, comme le glFrustum d'OpenGL : left, right, bottom, top sont les
   bords de la fenêtre sur le plan proche, à la distance near de l'œil.
   far est mémorisé mais pas encore utilisé.
   Le z-buffer reçoit ooz = 65535 * near / distance : 65535 sur le plan
   proche. Pour la précision, placer near aussi loin que la scène le
   permet. */
void glFrustum(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
                GLfixed near, GLfixed far);

/* ---- Matrice de modélisation -------------------------------------------
   Chaque fonction multiplie la matrice courante PAR LA DROITE : sur les
   sommets, le dernier appel s'applique en premier. Pour tourner autour
   d'un axe qui passe par le point P :
       glTranslate(P);  glRotate(...);  glTranslate(-P);           */

void glLoadIdentity(void);
void glTranslate(GLfixed x, GLfixed y, GLfixed z);
void glScale(GLfixed x, GLfixed y, GLfixed z);

/* Rotation de angle autour de l'axe (x, y, z), qui n'a pas besoin d'être
   normé. Sens direct : de y vers z autour de x, de z vers x autour de y,
   de x vers y autour de z, comme le glRotatef d'OpenGL. */
void glRotate(GLangle angle, GLfixed x, GLfixed y, GLfixed z);

void glPushMatrix(void);
void glPopMatrix(void);

/* ---- Sommets ------------------------------------------------------------- */

/* Couleur des primitives suivantes : index de palette */
void glIndexub(GLubyte index);

/* Seul GL_TRIANGLES est prévu : chaque groupe de trois sommets donne un
   triangle, transformé, projeté, puis envoyé à grDrawTriangle(). */
void glBegin(GLenum mode);
void glVertex3(GLfixed x, GLfixed y, GLfixed z);
void glEnd(void);

/* ---- Trigonométrie --------------------------------------------------------
   Table d'un quart de tour, 1024 pas (précision : 1/4096 de tour). */

GLfixed tnl_sin(GLangle angle);
GLfixed tnl_cos(GLangle angle);

#endif
