#include "vulimg.h"

#include <stdlib.h>
#include <stdio.h>
#include <endian.h>

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )
#define DIVC( a, b ) (((a)+(b-1))/(b))
#define TICK 1000
#define DEBUG( fmt, ... ) fprintf( stderr, fmt "\n", __VA_ARGS__ ); fflush( stderr )
#define FAIL( ... ) { DEBUG( __VA_ARGS__ ); exit(1); }
#define MIN(x,y) ((x)<(y)?(x):(y))
#define L16(x) htole16(x)
#define L32(x) htole32(x)


typedef struct VigVulimg {
   VcpVulcomp vulcomp;
   uint32_t nimg;
   VigImage * imgs;
   bool started;
   VcpTask copy1;
   VcpTask copy32;
   VcpTask grow1;
   VcpTask join3;
   VcpTask plane3;
   VcpTask trans;
   VcpTask diff;
} * VigVulimg;

struct VigImage {
   VigPixel pixel;
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   VcpStorage stor;
};

#pragma pack(push,1)

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
 
typedef struct VigImgParam {
   uint32_t width;
   uint32_t height;
   uint32_t stride;
} * VigImgParam;

typedef struct VigCopyParams {
   struct VigImgParam src;
   struct VigImgParam dst;
} * VigCopyParams;

typedef struct VigDiffParams {
   struct VigImgParam img;
   int32_t compBits;
} * VigDiffParams;

/*
typedef struct VigScaleParams {
   uint32_t srcstride;
   uint32_t dstwidth;
   uint32_t dstheight;
   uint32_t dststride;
   uint32_t xscale;
   uint32_t yscale;
   uint32_t xrest;
   uint32_t yrest;
} * VigScaleParams;
*/

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
   vulimg.copy1 = NULL;
   vulimg.copy32 = NULL;
   vulimg.grow1 = NULL;
   vulimg.join3 = NULL;
   vulimg.plane3 = NULL;
   vulimg.trans = NULL;
   vulimg.diff = NULL;
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

/// copy32 task
static VcpTask vig_copy32() {
   if ( ! vulimg.copy32 ) {
	  vulimg.copy32 = vcp_task_create( vulimg.vulcomp,
         copy32_spv, copy32_spv_len, "main", 2, sizeof( struct VigCopyParams ) );
   }
   return vulimg.copy32;
}

/// task for copy
static VcpTask vig_copy_task( VigCopyParams pars, VigPixel spix, VigPixel dpix ) {
   switch ( dpix ) {
      case vix_8: case vix_g8:
         switch ( spix ) {
            case vix_8: case vix_g8:
               pars->dst.width *= 8;
               return vig_copy1();
            break;
            default:
         }
      break;
      case vix_rgba32:
         if ( spix == dpix )
            return vig_copy32();
      break;
      case vix_rgb24: case vix_ybr24:
         if ( spix == dpix ) {
            pars->dst.width *= 24;
            return vig_copy1();
         }
      break;
      default:
   }
   return NULL;
}

/*
/// átfedő intervallumok
static bool vig_overlap_iv( VigCoord a, VigCoord al, VigCoord b, VigCoord bl ) {
   return (a <= b && b < a+al)
      || (b <= a && a < b+bl);
}
*/

static void vig_imgpars( VigImage i, VigImgParam p ) {
   p->width = i->width;
   p->height = i->height;
   p->stride = i->stride;
}   

bool vig_image_copy( VigImage src, VigImage dst ) {
   if ( src == dst ) return true;
   if ( ! vig_inited() ) return false;
	vigResult = VIG_COORDERR;
   if ( src->width != dst->width ) return false;
   if ( src->height != dst->height ) return false;
	struct VigCopyParams pars;
   vig_imgpars( src, & pars.src );
   vig_imgpars( dst, & pars.dst );
   uint nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*COPY_GX );
   vigResult = VIG_PIXELERR;
   VcpTask t = vig_copy_task( & pars, src->pixel, dst->pixel );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[2] = { src->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, COPY_GY ), 1, & pars );
	return vig_run( t );
}

static bool vig_same_pixel( VigPixel a, VigPixel b ) {
   switch ( a ) {
      case vix_Unknown: return false;
      case vix_8: case vix_g8: return vix_8 == b || vix_g8 == b;
      default: return a == b;
   }
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
   uint nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*TRANS_GX );
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
	struct VigDiffParams pars;
   vig_imgpars( a, & pars.img );
   pars.compBits = vig_pixel_size( dst->pixel ) / vig_pixel_comps( dst->pixel );
   uint nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*DIFF_GX );
   VcpTask t = vig_diff();
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	VcpStorage ss[3] = { a->stor, b->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, DIFF_GY ), 1, & pars );
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
   for (int i=vulimg.nimg-1; 0 <= i; --i)
      vig_image_free( vulimg.imgs[i] );
   vulimg.imgs = REALLOC( vulimg.imgs, VigImage, 0 );
   vig_done_task( & vulimg.copy1 );
   vig_done_task( & vulimg.copy32 );
   vig_done_task( & vulimg.grow1 );
   vig_done_task( & vulimg.join3 );
   vig_done_task( & vulimg.plane3 );
   vig_done_task( & vulimg.trans );
   vig_done_task( & vulimg.diff );
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
	     return vig_image_copy( src, dst );
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
      .magic = L16( 0x4d42 ),
      .size = L32( hsz + isz ),
      .reserved = 0,
      .address = L32( hsz )
   };
   vigResult = VIG_STREAMERR;
   if ( ! vtl_write_block( stream, write, &bfh, sizeof(bfh))) return false;
   struct VigBmpInfoHeader bih = {
	  .size = L32( sizeof( struct VigBmpInfoHeader )),
	  .width = L32( img->width ),
	  .height = L32( img->height ),
	  .planes = L16( 1 ),
	  .bpp = vig_pixel_size( img->pixel ),
	  .compression = 0,
	  .imgsize = L32( isz ),
	  .ppmx = L32( 2835 ),
	  .ppmy = L32( 2835 ),
	  .colors = 0,
	  .impcols = 0
   };
   if ( ! vtl_write_block( stream, write, &bih, sizeof(bih))) return false;
   void * data = vig_image_address( img );
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
   void * data = vig_image_address(ret);
   for (int r=ret->height-1; 0 <=r; --r) {
      if ( ! vtl_read_block( stream, read, data+r*stride, stride )) return false;
   }
   vigResult = VIG_SUCCESS;
   return ret;
}



bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad ) {
   void * data = vig_image_address( img );
   
   if ( ! data ) return false;
   int height = img->height;
   int stride = vig_image_stride( img );
   vigResult = VIG_SUCCESS;
   if ( pad )
      return vtl_write_block( stream, write, data, height * stride );
   int w = DIVC( img->width * vig_pixel_size( img->pixel ), 8 );
   for (int r=height; 0 < r; --r) {
      if ( ! vtl_write_block( stream, write, data, w ))
         return false;
      data += stride;
   }
   return true;
}

bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad ) {
   void * data = vig_image_address( img );
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



