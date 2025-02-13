#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>
#include "frameproc.h"

struct FrameData {
   VigImage frame;
   VigImage pyr;
} data;

VigImage * frame( FrameData d );
bool arg( FrameData d, int argc, char ** argv, int * at );
VigImage next( FrameData d );

struct FrameProc proc = { .frame = frame, 
  .arg = vfp_arg, .next = next };

VigImage * frame( FrameData d ) {
   return &d->frame;
}

/// következő képkocka
VigImage next( FrameData d ) {
   if ( ! vig_pyr_create( d->frame, d->pyr ) ) {
      vtl_die("Could not create frame");
   }
   return d->pyr;
}

void init( int argc, char ** argv ) {
   vfp_init( "pyr", VCP_VALIDATION, & data, & proc, argc, argv );
   uint32_t w = vig_image_width( data.frame );
   uint32_t h = vig_image_height( data.frame );
   VigPixel x = vig_image_pixel( data.frame );
   data.pyr = vig_image_create( w/2, h, x );
   vig_check_fail();
}

/// memória felszabadítás
void done() {
   vig_image_free( data.pyr );
   vfp_done( & data, & proc );
}

/// paraméterek értelmezése és bemenet feldolgozása
int main( int argc, char ** argv ) {
   init( argc, argv );
   vfp_process( & data, & proc );
   done();
   return 0;
}




