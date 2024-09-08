#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>
#include "frameproc.h"

typedef struct Diff {
   struct FrameProc fp;
   VigImage last;
   VigImage diff;
} * Diff;

/// következő képkocka
VigImage next( Diff d ) {
   vfp_flip( & d->fp.frame, & d->last );
   if ( ! vig_image_diff( d->last, d->fp.frame, d->diff ))
      vtl_die("Could not create frame");
   return d->diff;
}

void init( Diff d, int argc, char ** argv ) {
   vfp_init( "diff", VCP_VALIDATION, & d->fp, argc, argv, vfp_next_arg );
   VigImage f = d->fp.frame;
   uint32_t w = vig_image_width( f );
   uint32_t h = vig_image_height( f );
   VigPixel p = vig_image_pixel( f );
   d->last = vig_image_create( w, h, p );
   d->diff = vig_image_create( w, h, p );
   if ( ! ( d->last && d->diff ))
      vtl_die("Could not create images");
}

/// memória felszabadítás
int done( Diff d ) {
   vig_image_free( d->last );
   vig_image_free( d->diff );
   vfp_done( & d->fp );
   return 0;
}

/// paraméterek értelmezése és bemenet feldolgozása
int main( int argc, char ** argv ) {
   struct Diff d;
   init( & d, argc, argv );
   vfp_process( & d.fp, (vfp_next_proc)next );
   return done( & d );
}




