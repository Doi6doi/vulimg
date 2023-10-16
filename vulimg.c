#include "vulimg.h"

#include <stdlib.h>
#include <stdio.h>

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )
#define DIVC( a, b ) (((a)+(b-1))/(b))
#define TICK 1000
#define DEBUG( fmt, ... ) fprintf( stderr, fmt "\n", __VA_ARGS__ ); fflush( stderr )

#define COPY_GX 16
#define COPY_GY 16
#define GROW_GX 16
#define GROW_GY 16
#define JOIN_GX 16
#define JOIN_GY 16

typedef struct {
   VcpVulcomp vulcomp;
   uint32_t nimg;
   VigImage * imgs;
   bool started;
   VcpTask copy;
   VcpTask grow1;
   VcpTask join3;
} Vig_Vulimg;

typedef struct Vig__Image {
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   VigPixel pixel;
   VcpStorage stor;
} Vig_Image;


typedef struct {
   uint32_t srcoff;
   uint32_t srcwidth;
   uint32_t dstoff;
   uint32_t dstwidth;
   uint32_t width;
   uint32_t height;
} Vig_CopyParams;


typedef struct {
   uint32_t srcstride;
   uint32_t dstwidth;
   uint32_t dstheight;
   uint32_t dststride;
   uint32_t xscale;
   uint32_t yscale;
   uint32_t xrest;
   uint32_t yrest;
} Vig_ScaleParams;
typedef Vig_ScaleParams * VigScaleParams;	

 
typedef struct {
   uint32_t width;
   uint32_t height;
   uint32_t srcstride;
   uint32_t dststride;
   uint32_t index;
} Vig_JoinParams;
typedef Vig_JoinParams * VigJoinParams;	
 
Vig_Vulimg vulimg = { .started=false };

int vigResult = VIG_SUCCESS;

#include "copy.inc"
#include "grow1.inc"
#include "join3.inc"

bool vig_run( VcpTask t ) {
   vcp_task_start( t );
   while ( ! vcp_task_wait( t, TICK ))
      ;
   return ! vcp_error();
}

bool vig_init( VcpVulcomp v ) {
   vigResult = VIG_INITERR;
   if ( ! v ) return false;
   vigResult = VIG_SUCCESS;
   if ( vulimg.started ) return true;
   vulimg.vulcomp = v;
   vulimg.nimg = 0;
   vulimg.imgs = NULL;
   vulimg.copy = NULL;
   vulimg.grow1 = NULL;
   vulimg.started = true;
   return true;
}


/// is vulimg inited
bool vig_inited() {
   if ( ! vulimg.started ) {
	  vigResult = VIG_INITERR;
	  return false;
   } 	
   return true;
}

uint32_t vig_image_width( VigImage img ) {
   return img->width;
}

uint32_t vig_image_height( VigImage img ) {
   return img->height;
}

uint32_t vig_image_stride( VigImage img ) {
   return img->stride;	
}

VigPixel vig_image_pixel( VigImage img ) {
   return img->pixel;
}

void * vig_image_address( VigImage img ) {
   return vcp_storage_address( img->stor );
}

VcpStorage vig_image_storage( VigImage img ) {
   return img->stor;
}	

uint32_t vig_pixel_size( VigPixel pix ) {
   switch (pix) {
	  case vix_8: case vix_g8: return 1;
	  case vix_rgb24: case vix_ybr24: return 3;
	  case vix_rgba32: return 4;
	  default: return 0; 
   }
}

VigImage vig_image_create( VigCoord width, VigCoord height, VigPixel pixel ) {
   vigResult = VIG_HOSTMEM;
   VigImage ret = REALLOC( NULL, Vig_Image, 1 );
   if ( ! ret ) return NULL;
   ret->width = width;
   ret->height = height;
   ret->stride = 4*DIVC( vig_pixel_size( pixel )*width, 4 );
   ret->pixel = pixel;
   uint64_t sz = height * ret->stride;
   vigResult = VIG_STORAGEERR;
   if ( ! ( ret->stor = vcp_storage_create( vulimg.vulcomp, sz )))
      return NULL;
   vigResult = VIG_HOSTMEM;
   VigImage * imgs = REALLOC( vulimg.imgs, VigImage, vulimg.nimg+1 );
   if ( ! imgs ) return NULL;
   imgs[ vulimg.nimg++ ] = ret;
   vulimg.imgs = imgs;
   vigResult = VIG_SUCCESS;
   return ret;
}


bool vig_image_copy( VigImage src, VigCoord srcx, VigCoord srcy,
   VigImage dst, VigCoord dstx, VigCoord dsty, VigCoord width, VigCoord height )
{
   	if ( ! vig_inited() ) return false;
   	vigResult = VIG_PIXELERR;
   	if ( src->pixel != dst->pixel ) return false;   
   	int ps = vig_pixel_size( src->pixel );
   	if ( 0 != ps % 4 ) return false;
	vigResult = VIG_COORDERR;
   	if (src->width < srcx+width) return false;
   	if (src->height < srcy+height) return false;
   	if (dst->width < dstx+width) return false;
   	if (dst->height < dsty+height) return false;
    vigResult = VIG_TASKERR;
   	VcpTask t;
	if ( ! (t = vulimg.copy) ) {
	   if ( ! (t=vulimg.copy= vcp_task_create( vulimg.vulcomp,
	         copy_spv, copy_spv_len, "main", 2, sizeof(Vig_CopyParams))))
	      return false;
	}
	VcpStorage ss[2] = { src->stor, dst->stor };
	int pw = ps/4;
	Vig_CopyParams pars = {
       .srcoff = (srcy*src->width + srcx)*pw,
       .srcwidth = src->width * pw,
       .srcwidth = src->width * pw, 
       .dstoff = (dsty*dst->width + dstx)*pw,
       .dstwidth = dst->width * pw,
       .width = width*pw,
       .height = height
    }; 
	vcp_task_setup( t, ss, DIVC( pars.width, COPY_GX ),
	   DIVC( pars.height, COPY_GY ), 1, & pars );
	return vig_run( t );
}


void vig_done() {
   if ( ! vulimg.started ) return;
   for ( ; 0<= vulimg.nimg; --vulimg.nimg)
      vig_image_free( vulimg.imgs[ vulimg.nimg-1 ] );
   vulimg.imgs = REALLOC( vulimg.imgs, VigImage, 0 );
   if (( vulimg.copy )) {
	  vcp_task_free( vulimg.copy );
	  vulimg.copy = NULL;
   }
   vulimg.started = false;
}


/// get multiplier and remainder of coords
static bool vig_scale_get( uint32_t src, uint32_t dst, uint32_t * scale,
   uint32_t * rest )
{
	*rest = 0;
	*scale = 0;
	if ( src == dst ) {
	   *scale = 1;
	   return true;
	} else if ( 0 == src ) {
	   return false;
	} else if ( src < dst ) {
	   *scale = DIVC( dst, src );
	   *rest = dst % *scale;
	   return *rest < *scale;
	} else if ( 0 == dst ) {
	   return true;
	} else {
	   *scale = - DIVC( src, dst );
	   *rest = src % *scale;
	   return *rest < -*scale;
	}
}

/// task for scale
static VcpTask vig_scale_task( VigScaleParams pars, uint32_t pixelsize, uint32_t * unitsize ) {
   uint32_t csz = sizeof( pars );
   if ( 0 > pars->xscale * pars->yscale ) return NULL;
   switch ( pixelsize ) {
	  case 1: case 3: *unitsize = 4; break;
	  case 4: *unitsize = 1; break;
	  default: return NULL;
   }
   if ( 0 < pars->xscale ) {
      switch ( pixelsize ) {
	     case 1:
            if ( ! vulimg.grow1 )
	           vulimg.grow1 = vcp_task_create( vulimg.vulcomp,
	              grow1_spv, grow1_spv_len, "main", 2, csz );
	     return vulimg.grow1;
	  }
   }
   return NULL;
}


bool vig_image_scale( VigImage src, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( src->pixel != dst->pixel ) return false;
   vigResult = VIG_COORDERR;
   Vig_ScaleParams pars = {
	  .srcstride = src->stride,
	  .dstwidth = dst->width,
	  .dstheight = dst->height,
	  .dststride = dst->stride
   };
   if ( ! vig_scale_get( src->width, dst->width, & pars.xscale, & pars.xrest ))
      return false;
   if ( ! vig_scale_get( src->width, dst->width, & pars.yscale, & pars.yrest ))
      return false;
   if ( 1 == pars.xscale && 1 == pars.yscale )
      return vig_image_copy( src, 0, 0, dst, 0, 0, dst->width, dst->height );
   uint32_t ps = vig_pixel_size( dst->pixel );
   uint32_t us;
   VcpTask t = vig_scale_task( & pars, ps, & us );
   if ( ! t ) return false;
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, DIVC( DIVC( pars.dstwidth, us), GROW_GX ),
	   DIVC( pars.dstheight, GROW_GY ), 1, & pars );
   return vig_run( t );
}


/// join3 task
static VcpTask vig_join3() {
   if ( ! vulimg.join3 ) {
      vulimg.join3 = vcp_task_create( vulimg.vulcomp,
         join3_spv, join3_spv_len, "main", 2, sizeof( Vig_JoinParams ));
   }
   return vulimg.join3;
}


/// task for join
static VcpTask vig_join_task( VigPixel dpix, VigPixel spix, 
   VigPlane plane, VigJoinParams pars, uint32_t * unitsize ) 
{
DEBUG("vig_join_task %d %d\n", 1, spix );
   switch ( spix ) {
	  case vix_8:
	  case vix_g8:
	  break;
	  default:
	     return NULL;
   }
DEBUG("vig_join_task %d %d %d\n", 2, dpix, vix_ybr24 );
   switch ( dpix ) {
	  case vix_ybr24:
	     switch ( plane ) {
			case vpl_Y: pars->index = 0; break;
			case vpl_Cb: pars->index = 1; break;
			case vpl_Cr: pars->index = 2; break;
			default: return NULL;
	     }
	     *unitsize = 4;
	     return vig_join3();
	  case vix_rgb24:
	     switch (plane) {
			case vpl_R: pars->index = 0; break;
			case vpl_G: pars->index = 1; break;
			case vpl_B: pars->index = 2; break;
			default: return NULL;
	     }
	     *unitsize = 4;
	     return vig_join3();
	  default:
	     return NULL;
   }
}



bool vig_image_join( VigImage dst, VigImage src, VigPlane plane ) {
DEBUG("vig_image_join %d\n", 1 );	
   if ( ! vig_inited() ) return false;
DEBUG("vig_image_join %d %d %d %d %d\n", 2, src->width, dst->width, src->height, dst->height );	
   vigResult = VIG_COORDERR;
   if ( src->width < dst->width || src->height < dst->height )
      return false;
DEBUG("vig_image_join %d\n", 3 );	
   Vig_JoinParams pars = {
	  .width = src->width,
	  .height = src->height,
	  .srcstride = src->stride,
	  .dststride = src->stride
   };
   vigResult = VIG_PIXELERR;
   uint32_t us;
DEBUG("vig_image_join %d\n", 4 );	
   VcpTask t = vig_join_task( dst->pixel, src->pixel, plane, & pars, &us );
   if ( ! t ) return false;
DEBUG("vig_image_join %d\n", 5 );	
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, DIVC( DIVC( pars.width, us ), JOIN_GX ),
	   DIVC( pars.height, JOIN_GY ), 1, & pars );
DEBUG("vig_image_join %d\n", 6 );	
   return vig_run( t );
}


void vig_image_free( VigImage img ) {
   VigImage * imgs = vulimg.imgs;
   int n = vulimg.nimg;
   for ( int i=n-1; 0 <=i; --i ) {
	  if ( imgs[i] == img ) {
		 imgs[i] = imgs[n-1];
		 vulimg.imgs = REALLOC( imgs, VigImage, n-1 );
		 -- vulimg.nimg;
	     return;
	  }
   }
   vcp_storage_free( img->stor );
   img = REALLOC( img, Vig_Image, 0 );
}

