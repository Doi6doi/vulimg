#include "vulimg_impl.h"

#include "rect.inc"

TASK( rect, 1, struct VigRectParams );

/// draw rectangle
bool vig_draw_rect( VigImage img, VtlRect rect, VigValue pixel ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_SUCCESS;
   if ( 0 == rect->width * rect->height ) return true;
   vigResult = VIG_PIXELERR;
   struct VigRectParams pars = { .pixVal = pixel, 
	  .pixSize = vig_pixel_size( img->pixel ) };
   vig_imgpar( img, & pars.img );
   pars.rect = *rect;
   VcpTask t = vig_rect();
   uint32_t nx = DIVC( rect->width * pars.pixSize, 32 );
   uint32_t n = MAX( nx, rect->height );
   if ( ! t ) return false;
   vcp_task_setup( t, & img->stor, DIVC( n, UGR ), 1, 1, & pars );
   return vig_run( t );
}

