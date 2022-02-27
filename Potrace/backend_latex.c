
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>


#include "potracelib.h"
#include "curve.h"
#include "main_potrace.h"
#include "lists.h"

#include "backend_latex.h"

#define UNDEF ((double)(1e30))   /* a value to represent "undefined" */
#define SCALE 0.1


struct shapeList shapelist;
static point_t cur;
static char lastop = 0;
static int column = 0;
static int newline = 1;


static void addShape(struct shape inShape) {
	if (shapelist.size == shapelist.currentIndex) {
		shapelist.size *= 2;
		shapelist.shapes = (struct shape*)realloc(shapelist.shapes, shapelist.size * sizeof(struct shape));
	}

	shapelist.shapes[shapelist.currentIndex++] = inShape;
}

/* coordinate quantization */
static inline point_t unit(dpoint_t p) {
  point_t q;

  q.x = (long)(floor(p.x*info.unit+.5));
  q.y = (long)(floor(p.y*info.unit+.5));
  return q;
}

static void svg_moveto(dpoint_t p) {
  cur = unit(p);
  lastop = 'M';
}

static void svg_rmoveto(dpoint_t p) {
  point_t q;
  q = unit(p);
  cur = q;

  lastop = 'm';
}

static void svg_lineto(dpoint_t p) {

  struct shape shape;
  shape.isLine = 1;


  point_t q;
  q = unit(p);

  shape.line[0].x = cur.x*SCALE;
  shape.line[0].y = cur.y*SCALE;
  shape.line[1].x = q.x*SCALE;
  shape.line[1].y = q.y*SCALE;
  addShape(shape);


  cur = q;
  lastop = 'l';

}

static void svg_curveto(dpoint_t p1, dpoint_t p2, dpoint_t p3) {
  point_t q1, q2, q3;

  q1 = unit(p1);
  q2 = unit(p2);
  q3 = unit(p3);

  struct shape shape;
  shape.isLine = 0;
  shape.curve[0].x = cur.x*SCALE;
  shape.curve[0].y = cur.y*SCALE;
  shape.curve[1].x = q1.x*SCALE;
  shape.curve[1].y = q1.y*SCALE;
  shape.curve[2].x = q2.x*SCALE;
  shape.curve[2].y = q2.y*SCALE;
  shape.curve[3].x = q3.x*SCALE;
  shape.curve[3].y = q3.y*SCALE;
  //shape.curve[1] = q1;
  //shape.curve[2] = q2;
  //shape.curve[3] = q3;
  addShape(shape);

  cur = q3;
  lastop = 'c';
}

static int svg_path(potrace_curve_t *curve, int abs) {
  int i;
  dpoint_t *c;
  int m = curve->n;

  c = curve->c[m-1];
  if (abs) {
    svg_moveto(c[2]);
  } else {
    svg_rmoveto(c[2]);
  }

  for (i=0; i<m; i++) {
    c = curve->c[i];
    switch (curve->tag[i]) {
    case POTRACE_CORNER:
      svg_lineto(c[1]);
      svg_lineto(c[2]);
      break;
    case POTRACE_CURVETO:
      svg_curveto(c[0], c[1], c[2]);
      break;
    }
  }
  return 0;
}


static void write_paths_transparent_rec(potrace_path_t *tree) {
  potrace_path_t *p, *q;

  for (p=tree; p; p=p->sibling) {
    if (info.grouping != 0) {
      newline = 1;
      lastop = 0;
    }
    svg_path(&p->curve, 1);

    for (q=p->childlist; q; q=q->sibling) {
		svg_path(&q->curve, 0);
    }

    for (q=p->childlist; q; q=q->sibling) {
      write_paths_transparent_rec(q->childlist);
    }
  }
}

static void write_paths_transparent(potrace_path_t *tree) {
  if (info.grouping == 0) {
    newline = 1;
    lastop = 0;
  }
  write_paths_transparent_rec(tree);
}

struct shapeList pageLatex(potrace_path_t *plist, imginfo_t *imginfo) {

	/* defaults */
	info.debug = 0;
	info.width_d.x = UNDEF;
	info.height_d.x = UNDEF;
	info.rx = UNDEF;
	info.ry = UNDEF;
	info.sx = UNDEF;
	info.sy = UNDEF;
	info.stretch = 1;
	info.lmar_d.x = UNDEF;
	info.rmar_d.x = UNDEF;
	info.tmar_d.x = UNDEF;
	info.bmar_d.x = UNDEF;
	info.angle = 0;
	info.paperwidth = DEFAULT_PAPERWIDTH;
	info.paperheight = DEFAULT_PAPERHEIGHT;
	info.tight = 0;
	info.unit = 10;
	info.compress = 1;
	info.pslevel = 2;
	info.color = 0x000000;
	info.gamma = 2.2;
	info.param = potrace_param_default();
	if (!info.param) {
		fprintf(stderr, "" POTRACE ": %s\n", strerror(errno));
		exit(2);
	}
	info.longcoding = 0;
	info.outfile = NULL;
	info.blacklevel = 0.5;
	info.invert = 0;
	info.opaque = 0;
	info.grouping = 1;
	info.fillcolor = 0xffffff;
	info.progress = 0;
	info.progress_bar = DEFAULT_PROGRESS_BAR;

	double bboxx = imginfo->trans.bb[0] + imginfo->lmar + imginfo->rmar;
	double bboxy = imginfo->trans.bb[1] + imginfo->tmar + imginfo->bmar;
	double origx = imginfo->trans.orig[0] + imginfo->lmar;
	double origy = bboxy - imginfo->trans.orig[1] - imginfo->bmar;
	double scalex = imginfo->trans.scalex / info.unit;
	double scaley = -imginfo->trans.scaley / info.unit;

	shapelist.shapes = (struct shape*)malloc(30*sizeof(struct shape));
	shapelist.size = 30;
	shapelist.currentIndex = 0;
	



	write_paths_transparent (plist);



	return shapelist;

}
