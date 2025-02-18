#include <vulimg.h>
#include <stdio.h>
#include <stdlib.h>
#include "frameproc.h"

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )

typedef enum Kind { NONE, DELTA, MUCH, WROTE } Kind;

typedef struct Delta {
   Kind kind;
   int32_t dx, dy, ex, ey;
} * Delta;

struct FrameData {
   uint32_t count;
   struct Delta * ds;
   VigImage * imgs;
   VigImage * pyrs;
   VigImage out;
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
   vtl_ewrite( "%d: %d %d,%d %d,%d", i, d->kind, d->dx, d->dy, d->ex, d->ey );
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


/// vízszintes rész másolása
void compose_horz( Delta d ) {
   if ( 0 == d->ey ) return;
   VigCoord x = abs(d->ex);
   VigCoord y = abs(d->ey);
   struct VtlRect r = { .left = 0, .top = 0, 
      .width = width()-x, .height = y };
   if ( 0 < d->ex ) {
      r.left = x;
      x = 0;
   }
   if ( 0 < d->ey ) {
      r.top = y;
      y = 0;
   } else {
      r.top = height()-2*y;
      y = height()-y;
   }
   vig_image_copy( data.out, data.out, &r, x, y );
}

/// függőleges rész másolása
void compose_vert( Delta d ) {
   if ( 0 == d->ex ) return;
   VigCoord x = abs(d->ex);
   VigCoord y = abs(d->ey);
   struct VtlRect r = { .left = 0, .top = 0, 
      .width = x, .height = height()-2*y };
   if ( 0 < d->ex ) {
      r.left = x;
      x = 0;
   } else {
      r.left = width()-2*x;
      x = width()-x;
   }
   if ( 0 < d->ey ) {
      r.top = 2*y;
   } else {
      r.top = y;
      y = 0;
   }
   vig_image_copy( data.out, data.out, &r, x, y );
}

/// új kép másolása
void compose_new( Delta d, int i ) {
   VigCoord ax = abs(d->ex);
   VigCoord ay = abs(d->ey);
   struct VtlRect r = { .left = 0, .top = 0, 
      .width = width()-ax, .height = height()-ay };
   if ( 0 > d->ex ) {
      r.left = ax;
      ax = 0;
   }
   if ( 0 > d->ey ) {
      r.top = ay;
      ay = 0;
   }
vtl_ewrite( "compose_new %d,%d:%d,%d %d,%d", r.left, r.top, r.width, r.height, ax, ay );
   vig_image_copy( data.imgs[i], data.out, &r, ax, ay );
   
}

/// kép készítése az előzőből és az újból
void compose( int i ) {
   Delta d = data.ds+i;
   d->ex = d->dx - d->ex;
   d->ey = d->dy - d->ey;
//   d->ex -= d->dx;
//   d->ey -= d->dy;
   if ( 0 < i ) {
      data.ds[i-1].dx += d->ex;
      data.ds[i-1].dy += d->ey;
   }
vtl_ewrite("compose %d,%d", d->ex, d->ey );   
//   compose_horz( d );
//   compose_vert( d );
   compose_new( d, i );
   vig_check_fail();
}

/// kép kiírása
void write( VigImage img, int i ) {
   static int gq=0;
   vig_raw_write( img, stdout, vtl_fwrite, false );
//   if ( ++ gq < 20 ) 
//      wri( img, gq, 0 );
   data.ds[i].kind = WROTE;
}

/// megkeresni, ameddig ki lehet írni
int find_flush( bool all ) {
   // a végén mindet
   if ( all ) return 0;
   // az utolsó ugrásig
   for (int i=0; i<data.count; ++i) {
      if ( MUCH == data.ds[i].kind )
         return i;
   }
   // a legkorábbi
   return data.count-1;
}


/// azon képek kiírása, amik után nagy eltérés volt
void flush( bool all ) {
   int till = find_flush( all );
vtl_ewrite("till: %d", till);   
   for (int i = data.count-1; till <= i; --i ) {
      switch ( data.ds[i].kind ) {
         case WROTE: case NONE:
         break;
         case DELTA:
            compose(i);
            write( data.out, i );
         break;
         default:
            write( data.imgs[i], i );
      }
   }
}



/// rész simítása
void smooth_part( int last, int unt, int ex, int ey ) {
   int n = last-unt;
   for (int i=1; i<=n; ++i) {
      Delta ds = data.ds+last-i;
      ds->ex = ex * i / n;
      ds->ey = ey * i / n;
   }
}

/// simított változások
void smooth() {
   int ex = 0;
   int ey = 0;
   int last = data.count-1;
   for ( int i=last-1; 0 <= i; --i ) {
      Delta ds = data.ds+i;
      switch ( ds->kind ) {
         case NONE: case WROTE:
            ex = ey = 0;
            last = i;
         break;
         case DELTA:
            ex += ds->dx;
            ey += ds->dy;
         break;
         case MUCH:
            smooth_part( last, i-1, ex, ey );
            ex = ey = 0;
            last = i;
         break;
      }
      smooth_part( last, 0, ex, ey );
   }
   dumpds();
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

/// kiírás
void wri( VigImage img, int gq, int s ) {
   VcpStr fname = vtl_sprintf("ds%d_%d.bmp", gq, s);
   FILE * fh = fopen( fname, "w" );
   vig_bmp_write( img, fh, vtl_fwrite );
   fclose( fh );
}

/// utolsó két képkocka összehasonlítása
void compare() {
   Delta ds = data.ds;
   if ( NONE == data.ds[1].kind ) {
      ds->kind = MUCH;
   } else {
      vig_pyr_delta( data.imgs[0], data.imgs[1], data.pyrs[0], data.pyrs[1],
         0.2, &ds->dx, &ds->dy );
vtl_ewrite("\nMOVE %d %d", ds->dx, ds->dy );
      if ( VIG_MUCH == ds->dx )
         ds->kind = MUCH;
         else ds->kind = DELTA;
   }
}

/// új képkocka feldolgozása
VigImage next( FrameData d ) {
   static int gq = 0;
//   if ( 200 <= ++gq ) exit(1);
   Delta ds = d->ds;
   vig_pyr_create( d->imgs[0], d->pyrs[0] );
   compare();
   smooth();
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


