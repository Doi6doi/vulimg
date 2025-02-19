#include "vulimg_impl.h"

#include "rect.inc"
#include "fill.inc"

TASK( rect, 1, struct VigRectParams );
TASK( fill, 1, struct VigRectParams );

/// draw rectangle
bool vig_draw_rect( VigPart prt, VigValue pixel ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_SUCCESS;
   if ( 0 == prt->width * prt->height ) return true;
   vigResult = VIG_PIXELERR;
   struct VigRectParams pars = { .pixVal = pixel, 
	  .pixSize = vig_pixel_size( prt->img->pixel ) };
   vig_imgpar( prt->img, & pars.img );
   pars.left = prt->left;
   pars.top = prt->top;
   pars.width = prt->width;
   pars.height = prt->height;
   VcpTask t = vig_rect();
   uint32_t nx = DIVC( prt->width * pars.pixSize, 32 );
   uint32_t n = MAX( nx, prt->height );
   if ( ! t ) return false;
   vcp_task_setup( t, & prt->img->stor, DIVC( n, UGR ), 1, 1, & pars );
   return vig_run( t );
}

bool vig_part_fill( VigPart prt, VigValue pixel ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_SUCCESS;
   if ( 0 == prt->width * prt->height ) return true;
   vigResult = VIG_PIXELERR;
   struct VigRectParams pars = { .pixVal = pixel, 
	  .pixSize = vig_pixel_size( prt->img->pixel ) };
   vig_imgpar( prt->img, & pars.img );
   pars.left = prt->left;
   pars.top = prt->top;
   pars.width = prt->width;
   pars.height = prt->height;
   VcpTask t = vig_fill();
   if ( ! t ) return false;
   uint32_t gx = DIVC( prt->width * pars.pixSize, 32*UGR );
   uint32_t gy = DIVC( prt->height, UGR );
   vcp_task_setup( t, & prt->img->stor, gx, gy, 1, & pars );
   return vig_run( t );
}
