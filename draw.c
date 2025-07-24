#include "vulimg_impl.h"

VIG_NBEGIN()

#include "rect.inc"
#include "fill.inc"

TASK( rect, 1, struct Vig_RectParams );
TASK( fill, 1, struct Vig_RectParams );

/// draw rectangle
bool vig_draw_rect( VigImage img, VytURect prt, VigValue pixel ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_SUCCESS;
   if ( 0 == prt->width * prt->height ) return true;
   vigResult = VIG_PIXELERR;
   struct Vig_RectParams pars = { 
	  .pixSize = vig_pixel_size( img->pixel ),
     .pixVal = pixel
   };
   vig_imgpar( img, & pars.img );
   pars.left = prt->left;
   pars.top = prt->top;
   pars.width = prt->width;
   pars.height = prt->height;
   VcpTask t = vig_rect();
   uint32_t nx = DIVC( prt->width * pars.pixSize, 32 );
   uint32_t n = MAX( nx, prt->height );
   if ( ! t ) return false;
   vcp_task_setup( t, & img->stor, DIVC( n, UGR ), 1, 1, & pars );
   return vig_run( t );
}

bool vig_part_fill( VigImage img, VytURect prt, VigValue pixel ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_SUCCESS;
   if ( 0 == prt->width * prt->height ) return true;
   vigResult = VIG_PIXELERR;
   struct Vig_RectParams pars = {  
	  .pixSize = vig_pixel_size( img->pixel ),
     .pixVal = pixel 
   };
   vig_imgpar( img, & pars.img );
   pars.left = prt->left;
   pars.top = prt->top;
   pars.width = prt->width;
   pars.height = prt->height;
   VcpTask t = vig_fill();
   if ( ! t ) return false;
   uint32_t gx = DIVC( prt->width * pars.pixSize, 32*UGR );
   uint32_t gy = DIVC( prt->height, UGR );
   vcp_task_setup( t, & img->stor, gx, gy, 1, & pars );
   return vig_run( t );
}

VIG_NEND()
