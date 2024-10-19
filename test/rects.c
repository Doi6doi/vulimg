#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>
#include "frameproc.h"

#define NRECT 20

/// következő képkocka
VigImage next( FrameProc p ) {
   struct VtlRect r[NRECT];
   uint32_t n = NRECT;
   vig_white_rects( p->frame, 0.3, 0.2, 100, 20, r, &n );
   vig_check_fail();
   for ( int i=0; i<n; ++i)
      vig_draw_rect( p->frame, r+i, 0xff );
/*
   struct VtlRect r = {
	 .left = 100, .top = 100, .height = 500, .width = 500 };
   vig_draw_rect( p->frame, & r, 0xffffffff );
   */
   return p->frame;
}

/// paraméterek értelmezése és bemenet feldolgozása
int main( int argc, char ** argv ) {
   struct FrameProc p;
   vfp_init( "rects", VCP_VALIDATION, & p, argc, argv, vfp_next_arg );
   vfp_process( & p, next );
   vfp_done( & p );
   return 0;
}




