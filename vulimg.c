#include "vulimg.h"
#include <vultools.h>

#include <stdlib.h>
#include <stdio.h>

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )
#define DIVC( a, b ) (((a)+(b-1))/(b))
#define TICK 1000
#define DEBUG( fmt, ... ) fprintf( stderr, fmt "\n", __VA_ARGS__ ); fflush( stderr )
#define FAIL( fmt, ... ) { DEBUG( fmt, __VA_ARGS__ ); exit(1); }
#define MIN(x,y) ((x)<(y)?(x):(y))

typedef struct VigImgParam {
   uint32_t width;
   uint32_t height;
   uint32_t stride;
} * VigImgParam;

typedef struct VigPyrParams {
   struct VigImgParam src;
   struct VigImgParam dst;
   int32_t compBits;
   int32_t compCount;
   uint32_t width;
   uint32_t height;
   uint32_t row;
} * VigPyrParams;


typedef struct VigWhiteParams {
   struct VigImgParam img;
   float limit;
   uint32_t phase;
   uint32_t count;
} * VigWhiteParams;


typedef struct VigVulimg {
   VcpVulcomp vulcomp;
   uint32_t nimg;
   VigImage * imgs;
   VigImage temp8;
   uint32_t npyrs;
   VigPyrParams pyrs;
   uint32_t nwhites;
   VigWhiteParams whites;
   bool started;
   VcpTask copy1;
   VcpTask copy32;
   VcpTask grow1;
   VcpTask join3;
   VcpTask plane3;
   VcpTask trans;
   VcpTask diff;
   VcpTask pyr;
   VcpTask white8;
   VcpTask dsum;
} * VigVulimg;


struct VigImage {
   VigPixel pixel;
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   VcpStorage stor;
};

#define COPY_GX 16
#define COPY_GY 16
#define GROW_GX 16
#define GROW_GY 16
#define JOIN_GX 16
#define JOIN_GY 16
#define TRANS_GX 16
#define TRANS_GY 16
#define DIFF_GX 16
#define DIFF_GY 16
#define PYR_GX 16
#define PYR_GY 16
#define DSUM_GX 16

#pragma pack(push,1)

typedef struct VigRect {
   uint16_t dx;
   uint16_t dy;
   uint32_t weight;
   uint16_t left;
   uint16_t top;
   uint16_t width;
   uint16_t height;
} * VigRect;
 

typedef struct VigCopyParams {
   struct VigImgParam src;
   struct VigImgParam dst;
   uint32_t sleft;
   uint32_t stop;
   uint32_t width;
   uint32_t height;
   uint32_t dleft;
   uint32_t dtop;
} * VigCopyParams;

typedef struct VigDiffParams {
   struct VigImgParam img;
   int32_t compBits;
} * VigDiffParams;

struct VigTransParams {
   struct VigImgParam src;
   struct VigImgParam dst;
   struct VigTransform trans;
   int32_t compBits;
   int32_t compCount;
};
 
typedef struct VigJoinParams {
   uint32_t width;
   uint32_t height;
   uint32_t srcstride;
   uint32_t dststride;
   uint32_t index;
} * VigJoinParams;

typedef struct VigDSumParams {
   uint32_t astride;
   uint32_t bstride;
   uint32_t aleft;
   uint32_t atop;
   uint32_t bleft;
   uint32_t btop;
   uint32_t width;
   uint32_t height;
} * VigDSumParams;

typedef struct VigBmpFileHeader {
   uint16_t magic;
   uint32_t size;
   uint32_t reserved;
   uint32_t address;
} * VigBmpFileHeader;

typedef struct VigBmpInfoHeader {
   uint32_t size;
   uint32_t width;
   uint32_t height;
   uint16_t planes;
   uint16_t bpp;
   uint32_t compression;
   uint32_t imgsize;
   int32_t ppmx;
   int32_t ppmy;
   uint32_t colors;
   uint32_t impcols;
} * VigBmpInfoHeader;

#pragma pack(pop)
 
struct VigVulimg vulimg = { .started=false };

int vigResult = VIG_SUCCESS;

#include "copy1.inc"
#include "copy32.inc"
#include "grow1.inc"
#include "join3.inc"
#include "plane3.inc"
#include "trans.inc"
#include "diff.inc"
#include "pyr.inc"
#include "white8.inc"
#include "dsum.inc"

/*
static void ewrite( VcpStr msg ) {
   fprintf( stderr, "%s\n", msg );
   fflush( stderr );
}
*/

int vig_error() { return vigResult; }

void vig_check_fail() {
   if ( VIG_SUCCESS != vigResult )
      FAIL( "Error: vulimg %d\n", vigResult );
}
   

bool vig_run( VcpTask t ) {
   vcp_task_start( t );
   if (( vigResult = vcp_error() )) return false;
   while ( ! vcp_task_wait( t, TICK )) {
      ;
   }
   if (( vigResult = vcp_error() )) return false;
   return true;
}

bool vig_init( VcpVulcomp v ) {
   vigResult = VIG_SUCCESS;
   if ( vulimg.started ) {
	  if ( v != vulimg.vulcomp ) {
		 vigResult = VIG_INITERR;
		 return false;
	  }
	  return true;
   }
   vulimg.vulcomp = v;
   vulimg.nimg = 0;
   vulimg.imgs = NULL;
   vulimg.temp8 = NULL;
   vulimg.npyrs = 0;
   vulimg.pyrs = NULL;
   vulimg.nwhites = 0;
   vulimg.whites = NULL;
   vulimg.copy1 = NULL;
   vulimg.copy32 = NULL;
   vulimg.grow1 = NULL;
   vulimg.join3 = NULL;
   vulimg.plane3 = NULL;
   vulimg.trans = NULL;
   vulimg.diff = NULL;
   vulimg.pyr = NULL;
   vulimg.white8 = NULL;
   vulimg.dsum = NULL;
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

/// is image present
bool vig_isimage( VigImage img ) {
   if ( ! img ) {
	  vigResult = VIG_NOIMG;
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
   return 4*img->stride;	
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
	  case vix_8: case vix_g8: return 8;
	  case vix_rgb24: case vix_ybr24: return 24;
	  case vix_rgba32: return 32;
	  default: return 0; 
   }
}

/// number of components
static uint32_t vig_pixel_comps( VigPixel pix ) {
   switch (pix) {
	  case vix_8: case vix_g8: return 1;
	  case vix_rgb24: case vix_ybr24: return 3;
	  case vix_rgba32: return 4;
	  default: return 0; 
   }
}

VigImage vig_image_create( VigCoord width, VigCoord height, VigPixel pixel ) {
   vigResult = VIG_HOSTMEM;
   VigImage ret = REALLOC( NULL, struct VigImage, 1 );
   if ( ! ret ) return NULL;
   ret->pixel = pixel;
   ret->width = width;
   ret->height = height;
   ret->stride = DIVC( vig_pixel_size( pixel )*width, 32 );
   uint64_t sz = height * 4 * ret->stride;
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

/// copy1 task
static VcpTask vig_copy1() {
   if ( ! vulimg.copy1 ) {
	  vulimg.copy1 = vcp_task_create( vulimg.vulcomp,
         copy1_spv, copy1_spv_len, "main", 2, sizeof( struct VigCopyParams ) );
   }
   return vulimg.copy1;
}

/// trans task
static VcpTask vig_trans() {
   if ( ! vulimg.trans ) {
	  vulimg.trans = vcp_task_create( vulimg.vulcomp,
         trans_spv, trans_spv_len, "main", 2, sizeof( struct VigTransParams ) );
   }
   return vulimg.trans;
}

/// diff task
static VcpTask vig_diff() {
   if ( ! vulimg.diff ) {
	  vulimg.diff = vcp_task_create( vulimg.vulcomp,
         diff_spv, diff_spv_len, "main", 3, sizeof( struct VigDiffParams ) );
   }
   return vulimg.diff;
}

/// dsum task
static VcpTask vig_dsum() {
   if ( ! vulimg.dsum ) {
	  vulimg.dsum = vcp_task_create( vulimg.vulcomp,
         dsum_spv, dsum_spv_len, "main", 3, sizeof( struct VigDSumParams ) );
   }
   return vulimg.dsum;
}

/// copy32 task
static VcpTask vig_copy32() {
   if ( ! vulimg.copy32 ) {
	  vulimg.copy32 = vcp_task_create( vulimg.vulcomp,
         copy32_spv, copy32_spv_len, "main", 2, sizeof( struct VigCopyParams ) );
   }
   return vulimg.copy32;
}

/// pyr task
static VcpTask vig_pyr() {
   if ( ! vulimg.pyr ) {
	  vulimg.pyr = vcp_task_create( vulimg.vulcomp,
         pyr_spv, pyr_spv_len, "main", 2, sizeof( struct VigPyrParams ) );
   }
   return vulimg.pyr;
}

/// white task
static VcpTask vig_white() {
   if ( ! vulimg.white8 ) {
	  vulimg.white8 = vcp_task_create( vulimg.vulcomp,
         white8_spv, white8_spv_len, "main", 2, sizeof( struct VigWhiteParams ) );
   }
   return vulimg.white8;
}


/// task for copy
static VcpTask vig_copy_task( VigCopyParams pars, VigPixel pix ) {
   uint32_t pxs = vig_pixel_size( pix );
   if ( 0 == pars->sleft * pxs % 32
      && 0 == pars->width % 32
      && 0 == pars->dleft % 32 )
   {
      pars->sleft = pars->sleft * pxs / 32;
      pars->width = pars->width * pxs / 32;
      pars->dleft = pars->dleft * pxs / 32;
      return vig_copy32();
   }
   pars->sleft *= pxs;
   pars->width *= pxs;
   pars->dleft *= pxs;
   return vig_copy1();
}

/// kép paraméterek másolása
static void vig_imgpars( VigImage i, VigImgParam p ) {
   p->width = i->width;
   p->height = i->height;
   p->stride = i->stride;
}   

/// egyező felépítésű pixelek
static bool vig_same_pixel( VigPixel a, VigPixel b ) {
   switch ( a ) {
      case vix_Unknown: return false;
      case vix_8: case vix_g8: return vix_8 == b || vix_g8 == b;
      default: return a == b;
   }
}

bool vig_image_copy( VigImage src, VigImage dst, VtlRect rect,
   VigCoord dstLeft, VigCoord dstTop ) 
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( src->pixel, dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   uint32_t rw = rect->width;
   uint32_t rh = rect->height;
   if ( src->width < rect->left + rw ) return false;
   if ( src->height < rect->top + rh ) return false;
   if ( dst->width < dstLeft + rw ) return false;
   if ( dst->height < dstTop + rh ) return false;
	struct VigCopyParams pars;
   vig_imgpars( src, & pars.src );
   vig_imgpars( dst, & pars.dst );
   pars.sleft = rect->left;
   pars.stop = rect->top;
   pars.width = rect->width;
   pars.height = rect->height;
   pars.dleft = dstLeft;
   pars.dtop = dstTop;
   uint32_t nx = DIVC( rect->width * vig_pixel_size( dst->pixel ), 32*COPY_GX );
   VcpTask t = vig_copy_task( & pars, dst->pixel );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[2] = { src->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, COPY_GY ), 1, & pars );
	return vig_run( t );
}

static bool vig_inv_transform( VigTransform src, VigTransform dst ) {
   float a=src->sx, b=src->ry, c=src->rx, d=src->sy, e=src->dx, f=src->dy;
   float det = a*d - b*c;
   if ( 0 == det ) return false;
   dst->sx = d / det;
   dst->ry = -b / det;
   dst->rx = -c / det;
   dst->sy = a / det;
   dst->dx = c*f - d*e;
   dst->dy = b*e - a*f;
   return true;
}

bool vig_image_transform( VigImage src, VigImage dst, VigTransform trans ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( src->pixel, dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   if ( src == dst ) return false;
	struct VigTransParams pars;
   vig_imgpars( src, & pars.src );
   vig_imgpars( dst, & pars.dst );
   if ( ! vig_inv_transform( trans, & pars.trans )) return false;
   pars.compCount = vig_pixel_comps( dst->pixel );
   pars.compBits = vig_pixel_size( dst->pixel ) / pars.compCount;
   uint32_t nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*TRANS_GX );
   VcpTask t = vig_trans();
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[2] = { src->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, TRANS_GY ), 1, & pars );
	return vig_run( t );
}

bool vig_image_diff( VigImage a, VigImage b, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( a->pixel, b->pixel )) return false;
   if ( ! vig_same_pixel( a->pixel, dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   uint32_t w = vig_image_width(a);
   if ( w != vig_image_width(b) || w != vig_image_width(dst) ) return false;
   uint32_t h = vig_image_height(a);
   if ( h != vig_image_height(b) || h != vig_image_height(dst) ) return false;
   if ( a == dst || b == dst ) return false;
   uint32_t nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*DIFF_GX );
   struct VigDiffParams pars;
   vig_imgpars( a, & pars.img );
   pars.compBits = vig_pixel_size( dst->pixel ) / vig_pixel_comps( dst->pixel );
   VcpTask t = vig_diff();
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[3] = { a->stor, b->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, DIFF_GY ), 1, & pars );
	return vig_run( t );
}

static bool vig_pyrs_grow( uint32_t n ) {
   if ( vulimg.npyrs >= n ) return true;
   vigResult = VIG_HOSTMEM;
   VigPyrParams ret = REALLOC( vulimg.pyrs, struct VigPyrParams, n );
   if ( !ret ) return false;
   vulimg.pyrs = ret;
   vulimg.npyrs = n;
   vigResult = VIG_SUCCESS;
   return ret;
}

static bool vig_whites_grow( uint32_t n ) {
   if ( vulimg.nwhites >= n ) return true;
   vigResult = VIG_HOSTMEM;
   VigWhiteParams ret = REALLOC( vulimg.whites, struct VigWhiteParams, n );
   if ( !ret ) return false;
   vulimg.whites = ret;
   vulimg.nwhites = n;
   vigResult = VIG_SUCCESS;
   return ret;
}

static bool vig_temp8_grow( uint32_t w, uint32_t h ) {
   if ( vulimg.temp8 ) {
	  uint32_t tw = vulimg.temp8->width;
	  uint32_t th = vulimg.temp8->height;
	  if ( w <= tw && h <= th )
	     return true;
	  if ( w*h <= tw * th ) {
	     vulimg.temp8->width = w;
	     vulimg.temp8->height = tw * th / w;
         return true;
	  }
   }
   vig_image_free( vulimg.temp8 );
   vulimg.temp8 = vig_image_create( w, h, vix_8 );
   return NULL != vulimg.temp8;
}

static VcpTask vig_pyr_setup( VigImage src, VigImage dst ) {
   VcpStorage ss [2] = { src->stor, dst->stor };
   // lépésszám meghatározás
   uint32_t nrows = vig_image_height(src);
   uint32_t ncols = vig_image_width(src);
   int d = MIN(nrows, ncols );
   int n = 0;
   while ( 1 < d ) {
      ++n;
      d /= 2;
   }
   uint32_t ps = vig_pixel_size( dst->pixel );
   uint32_t compCount = vig_pixel_comps( dst->pixel );
   uint32_t compBits = ps / compCount;
   // konfiguráció kitöltése
   if ( ! vig_pyrs_grow( n ))
      return NULL;
   vigResult = VIG_TASKERR;
   VcpTask ret = vig_pyr();
   if ( ! ret ) return NULL;
   vcp_task_setup( ret, ss, 0, 0, 0, NULL );
   VcpPart prs = vcp_task_parts( ret, n );
   vcp_check_fail();   
   uint32_t row = 0;
   for ( int i=0; i<n; ++i ) {
      VigPyrParams py = vulimg.pyrs+i;
      vig_imgpars( src, & py->src );
      vig_imgpars( dst, & py->dst );
      py->compBits = compBits;
      py->compCount = compCount;
      py->height = (nrows /= 2);
      py->width = (ncols /= 2); 
      py->row = row;
      row += nrows;
      VcpPart pr = prs+i;
      pr->baseX = pr->baseY = pr->baseZ = 0;
      pr->countX = DIVC( ncols * ps, 32*PYR_GX );
      pr->countY = DIVC( nrows, PYR_GY );
      pr->countZ = 1;
      pr->constants = py;
   }
   vigResult = VIG_SUCCESS;
   return ret;
}


bool vig_image_pyramid( VigImage src, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( src->pixel, dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   if ( src == dst ) return false;
   uint32_t w = vig_image_width(src);
   uint32_t h = vig_image_height(src);
   if ( 2 > w || 2 > h ) return false;
   if ( w/2 > vig_image_width(dst)) return false;
   if ( h > vig_image_height(dst)) return false;
   struct VigPyrParams pars;
   vig_imgpars( src, & pars.src );
   vig_imgpars( dst, & pars.dst );
   uint32_t pxs = vig_pixel_size( dst->pixel );
   pars.compBits = pxs / vig_pixel_comps( dst->pixel );
   VcpTask t = vig_pyr_setup( src, dst );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	return vig_run( t );
}

/*
bool vig_image_diff( VigImage a, VigImage b, uint32_t * diff ) {
   if ( ! vig_inited() ) return false;
	vigResult = VIG_COORDERR;
   if ( a->width != b->width ) return false;
   if ( a->height != b->height ) return false;
   struct VigDiffParams pars = {
      .aoff = a->offset,
      .ashift = a->shift,
      .astride = a->stride,
      .boff = b->offset,
      .bshift = b->shift,
      .bstride = b->stride
   };
   vigResult = VIG_PIXELERR;
DEBUG("vig_image_copy %d", 3 );   	
   VcpTask t = vig_diff_task( pars, src->pixel, dst->pixel );
DEBUG("vig_image_copy %d %p", 4, t );   	
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[2] = { src->stor, dst->stor };
	vcp_task_setup( t, ss, DIVC( pars.width * vig_pixel_size( dst->, COPY_GX ),
	DIVC( pars.height, COPY_GY ), 1, & pars );
	return vig_run( t );
}
*/

static void vig_done_task( VcpTask * t ) {
   if ( *t ) {
      vcp_task_free( *t );
      *t = NULL;
   }
}
   

void vig_done() {
   if ( ! vulimg.started ) return;
   vig_image_free( vulimg.temp8 );
   for (int i=vulimg.nimg-1; 0 <= i; --i)
      vig_image_free( vulimg.imgs[i] );
   vulimg.imgs = REALLOC( vulimg.imgs, VigImage, 0 );
   vulimg.pyrs = REALLOC( vulimg.pyrs, struct VigPyrParams, 0 );
   vulimg.whites = REALLOC( vulimg.whites, struct VigWhiteParams, 0 );
   vig_done_task( & vulimg.copy1 );
   vig_done_task( & vulimg.copy32 );
   vig_done_task( & vulimg.grow1 );
   vig_done_task( & vulimg.join3 );
   vig_done_task( & vulimg.plane3 );
   vig_done_task( & vulimg.trans );
   vig_done_task( & vulimg.diff );
   vig_done_task( & vulimg.pyr );
   vig_done_task( & vulimg.white8 );
   vig_done_task( & vulimg.dsum );
   vulimg.started = false;
}

/*
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
*/

/*
/// task for scale
static VcpTask vig_scale_task( VigScaleParams pars, uint32_t pixelsize, uint32_t * unitsize ) {
   if ( 0 > pars->xscale * pars->yscale ) return NULL;
   switch ( pixelsize ) {
	  case 8: case 24: *unitsize = 4; break;
	  case 32: *unitsize = 1; break;
	  default: return NULL;
   }
   if ( 0 < pars->xscale ) {
      switch ( pixelsize ) {
	     case 1:
            if ( ! vulimg.grow1 )
	           vulimg.grow1 = vcp_task_create( vulimg.vulcomp,
	              grow1_spv, grow1_spv_len, "main", 2, sizeof( struct VigScaleParams ) );
	     return vulimg.grow1;
	  }
   }
   return NULL;
}
*/

/*bool vig_image_scale( VigImage src, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( src->pixel != dst->pixel ) return false;
   vigResult = VIG_COORDERR;
   struct VigScaleParams pars = {
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
      return vig_image_copy( src, dst );
   uint32_t ps = vig_pixel_size( dst->pixel );
   uint32_t us;
   VcpTask t = vig_scale_task( & pars, ps, & us );
   if ( ! t ) return false;
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, DIVC( DIVC( pars.dstwidth, us), GROW_GX ),
	   DIVC( pars.dstheight, GROW_GY ), 1, & pars );
   bool ret = vig_run( t );
   return ret;
}
*/

/// join3 task
static VcpTask vig_join3() {
   if ( ! vulimg.join3 ) {
      vulimg.join3 = vcp_task_create( vulimg.vulcomp,
         join3_spv, join3_spv_len, "main", 2, sizeof( struct VigJoinParams ));
   }
   return vulimg.join3;
}

/// plane3 task
static VcpTask vig_plane3() {
   if ( ! vulimg.plane3 ) {
	  vulimg.plane3 = vcp_task_create( vulimg.vulcomp,
	     plane3_spv, plane3_spv_len, "main", 2, sizeof( struct VigJoinParams ));
   }
   return vulimg.plane3;
}


/// task for join
static VcpTask vig_join_task( VigPixel dpix, VigPixel spix, 
   VigPlane plane, VigJoinParams pars, uint32_t * unitsize ) 
{
   switch ( spix ) {
	  case vix_8:
	  case vix_g8:
	  break;
	  default:
	     return NULL;
   }
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

/// task for plane
static VcpTask vig_plane_task( VigPixel spix, VigPixel dpix, 
   VigPlane plane, VigJoinParams pars, uint32_t * unitsize ) 
{
   switch ( dpix ) {
	  case vix_8:
	  case vix_g8:
	  break;
	  default:
	     return NULL;
   }
   switch ( spix ) {
	  case vix_ybr24:
	     switch ( plane ) {
			case vpl_Y: pars->index = 0; break;
			case vpl_Cb: pars->index = 1; break;
			case vpl_Cr: pars->index = 2; break;
			default: return NULL;
	     }
	     *unitsize = 4;
	     return vig_plane3();
	  case vix_rgb24:
	     switch (plane) {
			case vpl_R: pars->index = 0; break;
			case vpl_G: pars->index = 1; break;
			case vpl_B: pars->index = 2; break;
			default: return NULL;
	     }
	     *unitsize = 4;
	     return vig_plane3();
	  default:
	     return NULL;
   }
}


bool vig_image_join( VigImage dst, VigImage src, VigPlane plane ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_COORDERR;
   if ( src->width < dst->width || src->height < dst->height )
      return false;
   struct VigJoinParams pars = {
	  .width = src->width,
	  .height = src->height,
	  .srcstride = vig_image_stride( src ),
	  .dststride = vig_image_stride( dst )
   };
   vigResult = VIG_PIXELERR;
   uint32_t us;
   VcpTask t = vig_join_task( dst->pixel, src->pixel, plane, & pars, &us );
   if ( ! t ) return false;
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, DIVC( DIVC( pars.width, us ), JOIN_GX ),
	   DIVC( pars.height, JOIN_GY ), 1, & pars );
   return vig_run( t );
}


bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   struct VigJoinParams pars = {
	  .width = MIN( src->width, dst->width ),
	  .height = MIN( src->height, dst->height ),
	  .srcstride = vig_image_stride( src ),
	  .dststride = vig_image_stride( dst )
   };
   vigResult = VIG_PIXELERR;
   switch ( src->pixel ) {
	  case vix_8: case vix_g8:
        struct VtlRect r = {.left=0, .top=0, .width=pars.width, .height=pars.height };
	     return vig_image_copy( src, dst, & r, 0, 0 );
	  default: ;
   }
   uint32_t us;
   VcpTask t = vig_plane_task( src->pixel, dst->pixel, plane, & pars, &us );
   if ( ! t ) return false;
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, DIVC( DIVC( pars.width, us ), JOIN_GX ),
	   DIVC( pars.height, JOIN_GY ), 1, & pars );
   return vig_run( t );
}


void vig_image_free( VigImage img ) {
   if ( ! img ) return;
   VigImage * imgs = vulimg.imgs;
   int n = vulimg.nimg;
   for ( int i=n-1; 0<=i; --i ) {
	  if ( imgs[i] == img ) {
		 imgs[i] = imgs[n-1];
		 vulimg.imgs = REALLOC( imgs, VigImage, n-1 );
		 -- vulimg.nimg;
	     break;
	  }
   }
   vcp_storage_free( img->stor );
   img = REALLOC( img, struct VigImage, 0 );
}


bool vig_bmp_write( VigImage img, void * stream, VtlStreamOp write ) {
   if ( ! vig_isimage( img )) return false;
   uint32_t hsz = sizeof( struct VigBmpFileHeader )+sizeof( struct VigBmpInfoHeader );
   uint32_t isz = vig_image_stride(img) * img->height;
   struct VigBmpFileHeader bfh = {
      .magic = VT_L16( 0x4d42 ),
      .size = VT_L32( hsz + isz ),
      .reserved = 0,
      .address = VT_L32( hsz )
   };
   vigResult = VIG_STREAMERR;
   if ( ! vtl_write_block( stream, write, &bfh, sizeof(bfh))) return false;
   struct VigBmpInfoHeader bih = {
	  .size = VT_L32( sizeof( struct VigBmpInfoHeader )),
	  .width = VT_L32( img->width ),
	  .height = VT_L32( img->height ),
	  .planes = VT_L16( 1 ),
	  .bpp = vig_pixel_size( img->pixel ),
	  .compression = 0,
	  .imgsize = VT_L32( isz ),
	  .ppmx = VT_L32( 2835 ),
	  .ppmy = VT_L32( 2835 ),
	  .colors = 0,
	  .impcols = 0
   };
   if ( ! vtl_write_block( stream, write, &bih, sizeof(bih))) return false;
   char * data = vig_image_address( img );
   int stride = vig_image_stride(img);
   for (int r=img->height-1; 0 <=r; --r) {
      if ( ! vtl_write_block( stream, write, data+r*stride, stride )) return false;
   }
   vigResult = VIG_SUCCESS;
   return true;
}

static VigPixel vig_bmp_pixel( int bpp ) {
   switch ( bpp ) {
      case 1: return vix_1;
      case 8: return vix_g8;
      case 24: return vix_rgb24;
      case 32: return vix_rgba32;
      default: return vix_Unknown;
   }
}

VigImage vig_bmp_read( void * stream, VtlStreamOp read ) {
   vigResult = VIG_BMPERR;
   struct VigBmpFileHeader bfh;
   if ( ! vtl_read_block( stream, read, &bfh, sizeof(bfh))) return false;
   if ( 0x4d42 != bfh.magic ) return NULL;
   struct VigBmpInfoHeader bih;
   if ( ! vtl_read_block( stream, read, &bih, sizeof(bih))) return false;
   if ( sizeof(bih) > bih.size ) return NULL;
   if ( 1 != bih.planes ) return NULL;
   switch (bih.compression) {
      case 0: case 3: break;
      default: return NULL;
   }
   VigPixel pix = vig_bmp_pixel( bih.bpp );
   if ( vix_Unknown == pix ) return NULL;
   int rest = bfh.address - (sizeof(bfh)+sizeof(bih));
   if ( 0 > rest ) return NULL;
   if ( ! vtl_read_skip( stream, read, rest )) return NULL;
   VigImage ret = vig_image_create( bih.width, bih.height, pix );
   if ( ! ret ) return NULL;
   int stride = vig_image_stride(ret);
   char * data = vig_image_address(ret);
   for (int r=ret->height-1; 0 <=r; --r) {
      if ( ! vtl_read_block( stream, read, data+r*stride, stride )) return false;
   }
   vigResult = VIG_SUCCESS;
   return ret;
}



bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad ) {
   char * data = vig_image_address( img );
   if ( ! data ) return false;
   int width = img->width;
   int height = img->height;
   int stride = vig_image_stride( img );
   int wps = width * vig_pixel_size( img->pixel );
   vigResult = VIG_SUCCESS;
   if ( pad || stride * 32 == wps )
      return vtl_write_block( stream, write, data, height * stride * 4 );
   int w = DIVC( wps, 8 );
   for (int r=height; 0 < r; --r) {
      if ( ! vtl_write_block( stream, write, data, w ))
         return false;
      data += stride;
   }
   return true;
}

bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad ) {
   char * data = vig_image_address( img );
   if ( ! data ) return false;
   int height = img->height;
   int stride = vig_image_stride( img );
   vigResult = VIG_SUCCESS;
   if ( pad )
      return vtl_read_block( stream, read, data, height * stride );
   int w = DIVC( img->width * vig_pixel_size( img->pixel ), 8 );
   for (int r=img->height; 0 < r; --r) {
      if ( ! vtl_read_block( stream, read, data, w ))
         return false;
      data += stride;
   }
   return true;
}


/// white config beállítás
static VcpTask vig_white_setup( VigImage img, float limit, uint32_t count ) {
   uint32_t n = 1;
   uint32_t w = img->width;
   uint32_t h = img->height;
   uint32_t n4 = 4;
   while ( n4 < w || n4 < h ) {
	  ++n;
	  n4 *= 4;
   }
   if ( ! vig_whites_grow( n )) return false;
   if ( ! vig_temp8_grow( w, h )) return false;
   VcpTask ret = vig_white();
   if ( ! ret ) return false;
   VcpStorage ss[2] = { img->stor, vulimg.temp8->stor };
   vcp_task_setup( ret, ss, 0, 0, 0, NULL );
   VcpPart ps = vcp_task_parts( ret, n );
   n4 = 4;
   for ( int i=0; i<n; ++i ) {
      VigWhiteParams pr = vulimg.whites+i;
      vig_imgpars( img, & pr->img );
      pr->phase = i;
      pr->limit = limit;
      pr->count = count;
	  VcpPart p = ps+i;
	  p->baseX = p->baseY = p->baseZ = 0;
	  p->countX = DIVC( img->width * 8, 32 );
	  p->countY = DIVC( img->height, n4 );
      p->countZ = 1;
      p->constants = pr;
   }
   return false;
}

/// egy rect kiolvasása az eredményből
static void * vig_white_result( void * ptr, VtlRect rects, uint32_t stride ) {
   VigRect r = ptr;
   if ( ! r->weight ) return false;
   rects->left = r->left;
   rects->top = r->top;
   rects->width = r->width;
   rects->height = r->height;
   if ( 0 == r->dx && 0 == r->dy )
      return NULL;
   return ((uint32_t *)ptr)+stride*r->dy*4 + r->dx;
}


bool vig_white_rects( VigImage img, float limit, VtlRect rects, uint32_t * count ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( vix_g8, img->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( 0 >= *count ) return false;
   if ( 0 > limit || 1 < limit ) return false;
   VcpTask t = vig_white_setup( img, limit, *count );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
   if ( ! vig_run( t )) return false;
   uint32_t stride = vulimg.temp8->stride;
   void * p = vig_image_address( vulimg.temp8 );
   for ( ; 0 < *count; --*count ) {
	  if ( ! vig_white_result( p, rects, stride ))
	     break;
	  ++ rects;
   }
   return true;
}

bool vig_image_diffsum( VigImage a, VtlRect r, VigImage b, 
   VigCoord bx, VigCoord by, uint64_t * diff ) 
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_same_pixel( a->pixel, b->pixel )) return false;
   uint32_t comps = vig_pixel_comps( a->pixel );
   if ( 8 != vig_pixel_size( a->pixel ) / comps ) return false;
   vigResult = VIG_COORDERR;
   if ( a->width < r->left + r->width ) return false;
   if ( a->height < r->top + r->height ) return false;
   if ( b->width < bx + r->width ) return false;
   if ( b->height < by + r->height ) return false;
   struct VigDSumParams pars = {
	   .astride = a->stride,
	   .bstride = b->stride,
	   .aleft = r->left * comps,
	   .atop = r->top,
	   .bleft = bx * comps,
	   .btop = by,
	   .width = r->width * comps,
	   .height = r->height
   };
   if ( ! vig_temp8_grow( pars.width*4, 1 )) return false;
   VcpTask t = vig_dsum();
   if ( ! t ) return false;
   VcpStorage ss[3] = { a->stor, b->stor, vulimg.temp8->stor };
   vcp_task_setup( t, ss, DIVC( pars.width, DSUM_GX ), 1, 1, & pars );
   if ( ! vig_run( t )) return false;
   uint32_t * p = vig_image_address( vulimg.temp8 );
   *diff = 0;
   for ( int i=pars.width; 0<i; --i )
      *diff += *(p++);
   return true;
}   



