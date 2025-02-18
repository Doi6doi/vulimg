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
      vtl_die("Could not allocate memory");
}

VigImage * frame( FrameData d ) {
   if ( d->imgs )
      return d->imgs;
   return & d->out;
}

/// egy delta kiírása
void dumpd( int i ) {
   Delta d = data.ds+i;
   vtl_ewrite( "%d: %d %d,%d", i, d->kind, d->dx, d->dy );
}

/// ds-ek kiírása
void dumpds() {
   for (int i=0; i<data.count; ++i)
      dumpd(i);
   vtl_ewrite("");
}

bool arg( FrameData d, int argc, char ** argv, int * at ) {
   if ( argc <= *at ) return false;
   VcpStr s = argv[(*at)++];
   if ( vtl_same( s, "-n" ))
      return vfp_nat_arg( argc, argv, at, & d->count );
   --*at;
   return vfp_arg( d, argc, argv, at );
}

void init( int argc, char ** argv ) {
   data.count = 0;
   data.imgs = NULL;
   vfp_init( "deshake", VCP_VALIDATION, &data, &proc, argc, argv );
   uint32_t n = data.count;
   if ( ! n ) vtl_die( "Count missing (-n)" );
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

/// kép összeállítása
void compose( int i, int u ) {
   // összegzett elmozdulás
   int sx=0, sy=0;
   for (int j=i; u<=j; --j) {
      sx += data.ds[j].dx;
      sy += data.ds[j].dy;
   }
   int ex = sx / (i-u);
   int ey = sy / (i-u);
vtl_ewrite("ex:%d ey:%d", ex, ey );   
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
   struct VtlRect r = { .left = 0, .top = 0, 
      .width = width()-ax, .height = height()-ay };
   struct VtlRect q = { .left = bx, .top = by, 
      .width = bx, .height = height()-by };
   int cx = 0, cy = 0;
   if ( 0 < fx ) {
      r.left = ax;
      ax = 0;
      q.left = 0;
      cx = bx;
   }
   if ( 0 < fy ) {
      r.top = ay;
      ay = 0;
      q.top = 0;
      cy = by;
   }
   vig_image_copy_part( data.imgs[i], data.out, &r, ax, ay );
   // előző kocka függőleges rész
   vig_image_copy_part( data.prev, data.out, &q, cx, cy );
   // előző kocka vízszintes rész
   r.width = width()-bx;
   r.height = by;
   vig_image_copy_part( data.prev, data.out, &q, cx, cy );
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
   vig_raw_write( data.out, stdout, vtl_fwrite, false );
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
vtl_ewrite("flushTill %d %d", i, start );
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
vtl_ewrite("\nMOVE %d %d\n", ds->dx, ds->dy );
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


