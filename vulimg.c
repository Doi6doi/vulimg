#include "vulimg_impl.h"

#pragma pack(push,1)

typedef struct VigDiffParams {
   struct VigImgParam img;
   int32_t compBits;
} * VigDiffParams;

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
#include "join3.inc"
#include "plane3.inc"
#include "trans.inc"
#include "diff.inc"
#include "dsum.inc"
#include "add8.inc"

int vig_error() { return vigResult; }

void vig_check_fail() {
   if ( VIG_SUCCESS != vigResult )
      FAIL( "Error: vulimg %d\n", vigResult );
}

bool vig_run( VcpTask t ) {
   vigResult = VIG_TASKERR;
// vtl_ewrite("vigrun %p", t );   
   vcp_task_start( t );
// vtl_ewrite("vigstart %d", vcp_error() );   
   if ( vcp_error() ) return false;
   while ( ! vcp_task_wait( t, TICK )) {
      ;
   }
// vtl_ewrite("vigrun %d", vcp_error() );   
   if (( vigResult = vcp_error() )) return false;
   vigResult = VIG_SUCCESS;
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
   vulimg.temp = NULL;
   vulimg.npyrs = 0;
   vulimg.pyrs = NULL;
   vulimg.nwhites = 0;
   vulimg.whites = NULL;
   vulimg.nwclouds = 0;
   vulimg.wclouds = NULL;
   vulimg.copy1 = NULL;
   vulimg.copy32 = NULL;
   vulimg.join3 = NULL;
   vulimg.plane3 = NULL;
   vulimg.trans = NULL;
   vulimg.diff = NULL;
   vulimg.pyr = NULL;
   vulimg.white8 = NULL;
   vulimg.dsum = NULL;
   vulimg.add8 = NULL;
   vulimg.rect = NULL;
   vulimg.wcloud8 = NULL;
   vulimg.delta8 = NULL;
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

VigCoord vig_image_width( VigImage img ) {
   return img->width;
}

VigCoord vig_image_height( VigImage img ) {
   return img->height;
}

VigCoord vig_image_stride( VigImage img ) {
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
	  case vix_8: case vix_g8: case vix_s8: return 8;
	  case vix_rgb24: case vix_ybr24: return 24;
	  case vix_rgba32: case vix_argb32: return 32;
	  default: return 0; 
   }
}

/// number of components
uint32_t vig_pixel_comps( VigPixel pix ) {
   switch (pix) {
	  case vix_8: case vix_g8: return 1;
	  case vix_rgb24: case vix_ybr24: return 3;
	  case vix_rgba32: case vix_argb32: return 4;
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

TASK( copy1, 2, struct VigCopyParams );
TASK( trans, 2, struct VigTransParams );
TASK( diff, 3, struct VigDiffParams );
TASK( dsum, 3, struct VigDSumParams );
TASK( add8, 2, struct VigAddParams );
TASK( copy32, 2, struct VigCopyParams );

/// task for copy
static VcpTask vig_copy_task( VigCopyParams pars, VigPixel pix,
   uint32_t * nx 
) {
   uint32_t pxs = vig_pixel_size( pix );
   if ( 0 == pars->sleft * pxs % 32
      && 0 == pars->width * pxs % 32
      && 0 == pars->dleft *pxs % 32 )
   {
      pars->sleft = pars->sleft * pxs / 32;
      pars->width = pars->width * pxs / 32;
      pars->dleft = pars->dleft * pxs / 32;
      *nx = DIVC( pars->width, UGR );
      return vig_copy32();
   }
   pars->sleft *= pxs;
   pars->width *= pxs;
   pars->dleft *= pxs;
   *nx = DIVC( pars->width, UGR*32 );
   return vig_copy1();
}

/// kép paraméterek másolása
void vig_imgpar( VigImage i, VigImgParam p ) {
   p->width = i->width;
   p->height = i->height;
   p->stride = i->stride;
}   

/// egyező felépítésű pixelek
bool vig_pixel_same( VigPixel a, VigPixel b ) {
   switch ( a ) {
      case vix_Unknown: return false;
      case vix_8: case vix_g8: return vix_8 == b || vix_g8 == b;
      default: return a == b;
   }
}

/// előjeles pixelek
bool vig_pixel_signed( VigPixel pix ) {
   switch (pix) {
	  case vix_s8: return true;
	  default: return false;
   }
}

bool vig_image_copy( VigImage src, VigImage dst, VtlRect rect,
   VigCoord dstLeft, VigCoord dstTop ) 
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( src->pixel, dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   uint32_t rw = rect->width;
   uint32_t rh = rect->height;
   if ( src->width < rect->left + rw ) return false;
   if ( src->height < rect->top + rh ) return false;
   if ( dst->width < dstLeft + rw ) return false;
   if ( dst->height < dstTop + rh ) return false;
	struct VigCopyParams pars;
   vig_imgpar( src, &pars.src );
   vig_imgpar( dst, &pars.dst );
   pars.sleft = rect->left;
   pars.stop = rect->top;
   pars.width = rect->width;
   pars.height = rect->height;
   pars.dleft = dstLeft;
   pars.dtop = dstTop;
   uint32_t nx;
   VcpTask t = vig_copy_task( & pars, dst->pixel, &nx );
   if ( ! t ) return false;
	VcpStorage ss[2] = { src->stor, dst->stor };
   uint32_t ny = DIVC( pars.height, UGR );
	vcp_task_setup( t, ss, nx, ny, 1, & pars );
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
   if ( ! vig_pixel_same( src->pixel, dst->pixel )) return false;
   if ( vig_pixel_signed( src->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( src == dst ) return false;
	struct VigTransParams pars;
   vig_imgpar( src, & pars.src );
   vig_imgpar( dst, & pars.dst );
   if ( ! vig_inv_transform( trans, & pars.trans )) return false;
   pars.compCount = vig_pixel_comps( dst->pixel );
   pars.compBits = vig_pixel_size( dst->pixel ) / pars.compCount;
   uint32_t nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*UGR );
   VcpTask t = vig_trans();
   if ( ! t ) return false;
	VcpStorage ss[2] = { src->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, UGR ), 1, & pars );
	return vig_run( t );
}

bool vig_image_diff( VigImage a, VigImage b, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( a->pixel, b->pixel )) return false;
   if ( ! vig_pixel_same( a->pixel, dst->pixel )) return false;
   if ( vig_pixel_signed( dst->pixel )) return false;
	vigResult = VIG_COORDERR;
   uint32_t w = vig_image_width(a);
   if ( w != vig_image_width(b) || w != vig_image_width(dst) ) return false;
   uint32_t h = vig_image_height(a);
   if ( h != vig_image_height(b) || h != vig_image_height(dst) ) return false;
   if ( a == dst || b == dst ) return false;
   uint32_t nx = DIVC( dst->width * vig_pixel_size( dst->pixel ), 32*UGR );
   struct VigDiffParams pars;
   vig_imgpar( a, & pars.img );
   pars.compBits = vig_pixel_size( dst->pixel ) / vig_pixel_comps( dst->pixel );
   VcpTask t = vig_diff();
   if ( ! t ) return false;
	VcpStorage ss[3] = { a->stor, b->stor, dst->stor };
	vcp_task_setup( t, ss, nx, DIVC( dst->height, UGR ), 1, & pars );
	return vig_run( t );
}

bool vig_temp_grow( uint64_t size ) {
   if ( vulimg.temp ) {
      if ( size <= vcp_storage_size( vulimg.temp ))
         return true;
   }
   vcp_storage_free( vulimg.temp );
   vulimg.temp = vcp_storage_create( vulimg.vulcomp, size );
   return NULL != vulimg.temp;
}

static void vig_done_task( VcpTask * t ) {
   if ( *t ) {
      vcp_task_free( *t );
      *t = NULL;
   }
}
   

void vig_done() {
   if ( ! vulimg.started ) return;
   vcp_storage_free( vulimg.temp );
   for (int i=vulimg.nimg-1; 0 <= i; --i)
      vig_image_free( vulimg.imgs[i] );
   vulimg.imgs = REALLOC( vulimg.imgs, VigImage, 0 );
   vulimg.pyrs = REALLOC( vulimg.pyrs, struct VigPyrParams, 0 );
   vulimg.whites = REALLOC( vulimg.whites, struct VigWhiteParams, 0 );
   vulimg.wclouds = REALLOC( vulimg.wclouds, struct VigWCloudParams, 0 );
   vig_done_task( & vulimg.copy1 );
   vig_done_task( & vulimg.copy32 );
   vig_done_task( & vulimg.join3 );
   vig_done_task( & vulimg.plane3 );
   vig_done_task( & vulimg.trans );
   vig_done_task( & vulimg.diff );
   vig_done_task( & vulimg.white8 );
   vig_done_task( & vulimg.dsum );
   vig_done_task( & vulimg.add8 );
   vig_done_task( & vulimg.wcloud8 );
   vig_done_task( & vulimg.pyr );
   vig_done_task( & vulimg.delta8 );
   vulimg.started = false;
}

TASK( join3, 2, struct VigJoinParams );
TASK( plane3, 2, struct VigJoinParams );

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
   vcp_task_setup( t, ss, DIVC( DIVC( pars.width, us ), UGR ),
	   DIVC( pars.height, UGR ), 1, & pars );
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
   vcp_task_setup( t, ss, DIVC( DIVC( pars.width, us ), UGR ),
	   DIVC( pars.height, UGR ), 1, & pars );
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

/// bmp formátum pixele
static VigPixel vig_bmp_pixel( int bpp ) {
   switch ( bpp ) {
      case 1: return vix_1;
      case 8: return vix_g8;
      case 24: return vix_rgb24;
      case 32: return vix_argb32;
      default: return vix_Unknown;
   }
}

/// a pixel bmp mérete
static int vig_pixel_bmpbits( VigPixel pix ) {
   switch (pix) {
	  case vix_1: return 1;
	  case vix_g8: case vix_8: return 8;
	  case vix_rgb24: return 24;
	  case vix_argb32: return 32;
	  default: return -1;
   }
}

static bool vig_bmp_write_pal8( void * stream, VtlStreamOp write ) {
   static bool first = true;
   static uint32_t cols[256];
   if ( first ) {
      for (int i=0; i<256; ++i)
         cols[i] = i << 16 | i << 8 | i;
      first = false;
   } 
   return vtl_write_block( stream, write, cols, 256*4 );
}


bool vig_bmp_write( VigImage img, void * stream, VtlStreamOp write ) {
   if ( ! vig_inited() ) return false;
   if ( ! vig_isimage( img )) return false;
   int bits = vig_pixel_bmpbits( img->pixel );
   if ( 0 > bits ) return false;
   uint32_t hsz = sizeof( struct VigBmpFileHeader )+sizeof( struct VigBmpInfoHeader );
   uint32_t psz = 8 == bits ? 256*4 : 0;
   uint32_t isz = vig_image_stride(img) * img->height;
   struct VigBmpFileHeader bfh = {
      .magic = VT_L16( 0x4d42 ),
      .size = VT_L32( hsz + psz + isz ),
      .reserved = 0,
      .address = VT_L32( hsz + psz )
   };
   vigResult = VIG_STREAMERR;
   if ( ! vtl_write_block( stream, write, &bfh, sizeof(bfh))) return false;
   struct VigBmpInfoHeader bih = {
	  .size = VT_L32( sizeof( struct VigBmpInfoHeader )),
	  .width = VT_L32( img->width ),
	  .height = VT_L32( img->height ),
	  .planes = VT_L16( 1 ),
	  .bpp = bits,
	  .compression = 0,
	  .imgsize = VT_L32( isz ),
	  .ppmx = VT_L32( 2835 ),
	  .ppmy = VT_L32( 2835 ),
	  .colors = 0,
	  .impcols = 0
   };
   if ( ! vtl_write_block( stream, write, &bih, sizeof(bih))) return false;
   if ( 0 < psz && ! vig_bmp_write_pal8( stream, write )) return false;
   char * data = vig_image_address( img );
   int stride = vig_image_stride(img);
   for (int r=img->height-1; 0 <=r; --r) {
      if ( ! vtl_write_block( stream, write, data+r*stride, stride )) return false;
   }
   vigResult = VIG_SUCCESS;
   return true;
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

bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad ) {
   char * data = vig_image_address( img );
   if ( ! data ) return false;
   int width = img->width;
   int height = img->height;
   int stride = vig_image_stride( img );
   int wps = width * vig_pixel_size( img->pixel );
   vigResult = VIG_SUCCESS;
   if ( pad || wps == stride * 8 )
      return vtl_read_block( stream, read, data, height * stride );
   int w = DIVC( wps, 8 );
   for (int r=img->height; 0 < r; --r) {
      if ( ! vtl_read_block( stream, read, data, w ))
         return false;
      data += stride;
   }
   return true;
}

bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad ) {
   char * data = vig_image_address( img );
   if ( ! data ) return false;
   int width = img->width;
   int height = img->height;
   int stride = vig_image_stride( img );
   int wps = width * vig_pixel_size( img->pixel );
   if ( pad || wps == stride * 8 )
      return vtl_write_block( stream, write, data, height * stride );
   int w = DIVC( wps, 8 );
   for (int r=height; 0 < r; --r) {
      if ( ! vtl_write_block( stream, write, data, w ))
         return false;
      data += stride;
   }
   return true;
}

bool vig_image_diffsum( VigImage a, VtlRect r, VigImage b, 
   VigCoord bx, VigCoord by, uint64_t * diff ) 
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( a->pixel, b->pixel )) return false;
   uint32_t comps = vig_pixel_comps( a->pixel );
   if ( 8 != vig_pixel_size( a->pixel ) / comps ) return false;
   vigResult = VIG_COORDERR;
   if ( a->width < r->left + r->width ) return false;
   if ( a->height < r->top + r->height ) return false;
   if ( b->width < bx + r->width ) return false;
   if ( b->height < by + r->height ) return false;
   bool sgn = vig_pixel_signed( a->pixel );
   struct VigDSumParams pars = {
	   .mode = sgn ? 4 : 3,
	   .astride = a->stride,
	   .bstride = b->stride,
	   .aleft = r->left * comps,
	   .atop = r->top,
	   .bleft = bx * comps,
	   .btop = by,
	   .width = r->width * comps,
	   .height = r->height
   };
   if ( ! vig_temp_grow( pars.width*4 )) return false;
   VcpTask t = vig_dsum();
   if ( ! t ) return false;
   VcpStorage ss[3] = { a->stor, b->stor, vulimg.temp };
   vcp_task_setup( t, ss, DIVC( pars.width, UGR ), 1, 1, & pars );
   if ( ! vig_run( t )) return false;
   uint32_t * p = vcp_storage_address( vulimg.temp );
   *diff = 0;
   for ( int i=pars.width; 0<i; --i )
      *diff += *(p++);
   return true;
}   


bool vig_image_avg( VigImage img, VigValue * pix ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   bool sgn = vig_pixel_signed( img->pixel );
   uint32_t comps = vig_pixel_comps( img->pixel );
   if ( 8 != vig_pixel_size( img->pixel ) / comps ) return false;
   struct VigDSumParams pars = {
	   .mode = sgn ? 2 : 1,
	   .astride = img->stride,
	   .bstride = 0,
	   .aleft = 0,
	   .atop = 0,
	   .bleft = 0,
	   .btop = 0,
	   .width = img->width * comps,
	   .height = img->height
   };
   if ( ! vig_temp_grow( pars.width*4 )) return false;
   VcpTask t = vig_dsum();
   if ( ! t ) return false;
   VcpStorage ss[3] = { img->stor, img->stor, vulimg.temp };
   vcp_task_setup( t, ss, DIVC( pars.width, UGR ), 1, 1, & pars );
   if ( ! vig_run( t )) return false;
   int32_t cvals[4] = {0,0,0,0};
   int32_t * p = vcp_storage_address( vulimg.temp );
   for ( int i=0; i < pars.width; ++i )
	  cvals[ i % comps ] += *(p++);
   *pix=0;	  
   uint32_t sz = img->width * img->height;
   for (int i=0; i < comps; ++i) {
	  uint32_t cval = 0xff & (cvals[comps-1-i] / sz);
      *pix = (*pix) << 8 | cval;
   }
   return true;
}   


bool vig_image_add( VigImage src, VigValue pixel, VigImage dst ) {
   if ( ! vig_inited() ) return false;
   if ( ! vig_isimage(src)) return false;
   if ( ! vig_isimage(dst)) return false;
   vigResult = VIG_PIXELERR;
   VigPixel sp = src->pixel;
   VigPixel dp = dst->pixel;
   uint32_t psz = vig_pixel_size(dp);
   uint32_t compCount = vig_pixel_comps(dp);
   if ( ! vig_pixel_same( sp, dp ))
      if ( ! (8==vig_pixel_size(sp) && 8 == psz))
         return false;
   if ( 8 != psz / compCount) return false;
   bool sgn = vig_pixel_signed( dp );
   vigResult = VIG_COORDERR;
   if ( src->width != dst->width ) return false;
   if ( src->height != dst->height ) return false;
   struct VigAddParams pars = {
	  .compCount = compCount, 
      .min = sgn ? -128 : 0,
      .max = sgn ? 127 : 255,
      .pixel = pixel
   };
   vig_imgpar( src, & pars.img );
   pars.img.width *= vig_pixel_comps(dp);
   uint32_t nx = DIVC( pars.img.width, 4*UGR );
   VcpTask t = vig_add8();
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
   VcpStorage ss[2] = { src->stor, dst->stor };
   vcp_task_setup( t, ss, nx, DIVC( dst->height, UGR ), 1, & pars );
   return vig_run( t );
}

/*static void cdraw8( VigImage img, VtlRect rect, VigValue pixel ) {
   if ( 8 != vig_pixel_size( img->pixel )) return;
   uint8_t * p = vig_image_address( img );
   uint32_t s = vig_image_stride( img );
   for ( int y = 0; y < rect->height; ++y ) {
      p[ (rect->top+y)*s + rect->left ] = pixel;
      p[ (rect->top+y)*s + rect->left + rect->width-1 ] = pixel;
   }
   for ( int x = 0; x < rect->width; ++x ) {
      p[ rect->top*s + rect->left+x ] = pixel;
      p[ (rect->top+rect->height-1)*s + rect->left+x ] = pixel;
   }
}   
*/




