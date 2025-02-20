#include <vulimg.h>
#include <stdio.h>
#include <stdlib.h>
#include "frameproc.h"

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )

typedef enum Kind { NONE, DELTA, MUCH, WROTE } Kind;

typedef struct Delta {
   Kind kind;
   int32_t dx, dy;
} * Delta;

struct FrameData {
   uint32_t count;
   struct Delta * ds;
   VigImage * imgs;
   VigImage * pyrs;
   VigImage out;
   VigImage prev;
};

struct FrameData data;

VigImage * frame( FrameData d );
bool arg( FrameData d, int argc, char ** argv, int * at );
VigImage next( FrameData d );

struct FrameProc proc = { .frame = frame, 
  .arg = arg, .next = next };

void * check( void * p ) {
   if (! p)
      vyt_die("Could not allocate memory");
}

VigImage * frame( FrameData d ) {
   if ( d->imgs )
      return d->imgs;
   return & d->out;
}

/// egy delta kiírása
void dumpd( int i ) {
   Delta d = data.ds+i;
   vyt_ewrite( "%d: %d %d,%d", i, d->kind, d->dx, d->dy );
}

/// ds-ek kiírása
void dumpds() {
   for (int i=0; i<data.count; ++i)
      dumpd(i);
   vyt_ewrite("");
}

bool arg( FrameData d, int argc, char ** argv, int * at ) {
   if ( argc <= *at ) return false;
   VcpStr s = argv[(*at)++];
   if ( vyt_same( s, "-n" ))
      return vfp_nat_arg( argc, argv, at, & d->count );
   --*at;
   return vfp_arg( d, argc, argv, at );
}

void init( int argc, char ** argv ) {
   data.count = 0;
   data.imgs = NULL;
   vfp_init( "deshake", VCP_VALIDATION, &data, &proc, argc, argv );
   uint32_t n = data.count;
   if ( ! n ) vyt_die( "Count missing (-n)" );
   data.ds = check( REALLOC( NULL, struct Delta, n ));
   data.imgs = check( REALLOC( NULL, VigImage, n ));
   data.pyrs = check( REALLOC( NULL, VigImage, n ));
   VigCoord w = vig_image_width( data.out );
   VigCoord h = vig_image_height( data.out );
   VigPixel x = vig_image_pixel( data.out );
   data.prev = vig_image_create( w, h, x );
   for ( int i=0; i < data.count; ++i) {
      data.ds[i].kind = NONE;
      data.imgs[i] = vig_image_create( w, h, x );
      data.pyrs[i] = vig_image_create( w/2, h, x );
      vig_check_fail();
   }
}

VigCoord width() {
   return vig_image_width( data.out );
}

VigCoord height() {
   return vig_image_height( data.out );
}


void wri( VigImage img, VcpStr fname ) {
   FILE * fh = fopen( fname, "w" );
   vig_bmp_write( img, fh, vyt_fwrite );
   fclose(fh);
}

void fill( VytURect prt, int cx, int cy, uint32_t v ) {
   prt->left = cx;
   prt->top = cy;
   vig_part_fill( data.out, prt, v );
}

/// kép összeállítása
void compose( int i, int u ) {
static int gq=0;
++gq;   
   // összegzett elmozdulás
   int sx=0, sy=0;
   for (int j=i; u<=j; --j) {
      sx += data.ds[j].dx;
      sy += data.ds[j].dy;
   }
   int ex = sx / (i-u);
   int ey = sy / (i-u);
   Delta di = data.ds+i;
   if ( 0 == ex && 0 == ey ) {
      vig_image_copy( data.imgs[i], data.out );
      return;
   }
   int fx = di->dx - ex;
   int fy = di->dy - ey;
   int bx = abs(ex);
   int by = abs(ey);
   int ax = abs(fx);
   int ay = abs(fy);
   // új kép
   struct Vyt_URect r = { .left = 0, .top = 0, 
      .width = width()-ax, .height = height()-ay };
   struct Vyt_UVec2 h = { .x = ax, .y = ay };
   struct Vyt_URect q = { .left = bx, .top = by, 
      .width = ax, .height = height()-ay };
   struct Vyt_UVec2 c = { .x = 0, .y = 0 };
//   struct VigPart q = { .img=data.prev, .left = bx, .top = by, 
//      .width = ax, .height = height()-ay };
   if ( 0 < fx ) {
      r.left = ax;
      h.x = 0;
      q.left = width()-ax-bx;
      c.x = width()-ax;
   }
   if ( 0 < fy ) {
      r.top = ay;
      h.y = 0;
      q.top = 0;
      c.y = by;
   }
   vig_part_copy( data.imgs[i], &r, data.out, &h );
   // előző kocka függőleges rész
   vig_part_copy( data.prev, &q, data.out, &c );
//   fill( &q, cx, cy, 0 );

vyt_ewrite("gq:%d dx:%d dy:%d ex:%d ey:%d fx:%d xy:%d bx:%d by:%d", gq, di->dx, di->dy, ex, ey, fx, fy, bx, by );   
vyt_ewrite("V: qt:%d ql:%d qw:%d qh:%d cx:%d cy:%d", q.top, q.left, q.width, q.height, c.x, c.y );
   // előző kocka vízszintes rész
   q.width = width()-ax;
   q.height = ay;
   c.x = c.y = 0;
   q.left = bx;
   q.top = by;
   if ( 0 < fx ) {
      q.left = 0;
      c.x = bx;
   }
   if ( 0 < fy ) {
      q.top = height()-ay-by;
      c.y = height()-ay;
   }
   vig_part_copy( data.prev, &q, data.out, &c );
//    fill( &q, cx, cy, 0 );
vyt_ewrite("H: qt:%d ql:%d qw:%d qh:%d cx:%d cy:%d", q.top, q.left, q.width, q.height, c.x, c.y );
if ( 167 == gq ) {
   wri( data.imgs[i], "o1.bmp" );
   wri( data.prev, "o2.bmp" );
   wri( data.out, "o3.bmp" );
}
   if ( 0 < i ) {
      data.ds[i-1].dx += fx;
      data.ds[i-1].dy += fy;
   }
}   

/// két kép csere
void swap( VigImage * a, VigImage * b ) {
   VigImage c = *a;
   *a = *b;
   *b = c;
}

/// egy kiírása
void flushOne( int i, int u ) {
   Kind k = data.ds[i].kind;
   if ( NONE == k || WROTE == k )
      return;
   if ( i == u || MUCH == k )
      vig_image_copy( data.imgs[i], data.out );
   else
      compose(i,u);
   vig_raw_write( data.out, stdout, vyt_fwrite, false );
   swap( &data.out, &data.prev );
   data.ds[i].kind = WROTE;
}


/// összes kép kiírása i-ig bezárólag
void flushTill( int i ) {
   int start = 0;
   // első kiírandó megkeresése
   for (int j = data.count-1; i <= j; --j) {
      Kind k = data.ds[j].kind;
      if ( WROTE != k && NONE != k ) {
         start = j;
         break;
      }
   }
   // utolsó referencia megkeresése
   int u = start;
   while ( 0 < u && DELTA == data.ds[u].kind )
      --u;
vyt_ewrite("flushTill %d %d", i, start );
   // képek kiírása
   for (int j=start; i <= j; --j )
      flushOne( j, u );
}

/// szükséges képek kiírása
void flush( bool all ) {
   int u = data.count-1;
   if ( MUCH == data.ds[0].kind )
      flushTill( 1 );
   else if ( all )
      flushTill( 0 );
   else if ( DELTA == data.ds[u].kind )
      flushTill( u );
}


/// grögetés
void roll() {
   int n = data.count;
   VigImage img = data.imgs[n-1];
   VigImage pyr = data.pyrs[n-1];
   struct Delta d = data.ds[n-1];
   for (int i=n-2; 0 <= i; --i)  {
      data.imgs[i+1] = data.imgs[i];
      data.pyrs[i+1] = data.pyrs[i];
      data.ds[i+1] = data.ds[i];
   }
   data.imgs[0] = img;
   data.pyrs[0] = pyr;
   data.ds[0] = d;
}

/// utolsó két képkocka összehasonlítása
void compare() {
   Delta ds = data.ds;
   if ( NONE == data.ds[1].kind ) {
      ds->kind = MUCH;
   } else {
      vig_pyr_delta( data.imgs[1], data.imgs[0], data.pyrs[1], data.pyrs[0],
         0.2, &ds->dx, &ds->dy );
vyt_ewrite("\nMOVE %d %d\n", ds->dx, ds->dy );
      if ( VIG_MUCH == ds->dx )
         ds->kind = MUCH;
         else ds->kind = DELTA; 
   }
}

/// új képkocka feldolgozása
VigImage next( FrameData d ) {
// static int gq=0;
// if ( 20 == ++gq ) exit(1);   
   vig_pyr_create( d->imgs[0], d->pyrs[0] );
   compare();
   flush(false);
   roll();
   return NULL;
}

/// befejezés
void done() {
   flush(true);
   int n = data.count;
   for (int i=0; i<n; ++i) {
      vig_image_free( data.imgs[i] );
      vig_image_free( data.pyrs[i] );
   }
   data.ds = REALLOC( data.ds, struct Delta, 0 );
   data.pyrs = REALLOC( data.pyrs, VigImage, 0 );
   data.imgs = REALLOC( data.ds, VigImage, 0 );
   vfp_done( &data, &proc );
}

/// paraméterek értelmezése és bemenet feldolgozása
int main( int argc, char ** argv ) {
   init( argc, argv );
   vfp_process( &data, &proc  );
   done();
   return 0;
}


