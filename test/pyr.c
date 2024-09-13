#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>
#include "frameproc.h"

typedef struct Pyr {
   struct FrameProc fp;
   VigImage pyr;
} * Pyr;

/// következő képkocka
VigImage next( Pyr p ) {
   if ( ! vig_image_pyramid( p->fp.frame, p->pyr ) ) {
      vtl_die("Could not create frame");
   }
   return p->pyr;
}

void init( Pyr p, int argc, char ** argv ) {
   vfp_init( "pyr", VCP_VALIDATION, & p->fp, argc, argv, vfp_next_arg );
   VigImage f = p->fp.frame;
   uint32_t w = vig_image_width( f );
   uint32_t h = vig_image_height( f );
   VigPixel x = vig_image_pixel( f );
   p->pyr = vig_image_create( w/2, h, x );
   if ( ! ( p->pyr ))
      vtl_die("Could not create image");
}

/// memória felszabadítás
int done( Pyr p ) {
   vig_image_free( p->pyr );
   vfp_done( & p->fp );
   return 0;
}

/// paraméterek értelmezése és bemenet feldolgozása
int main( int argc, char ** argv ) {
   struct Pyr p;
   init( & p, argc, argv );
   vfp_process( & p.fp, (vfp_next_proc)next );
   return done( & p );
}




