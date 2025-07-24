#ifndef VULIMGH
#define VULIMGH

#include <vulcmp.h>
#include <vytools.h>

#ifdef __cplusplus
#define VIG_NBEGIN() \
   using namespace vcpc; \
   using namespace vytc; \
   namespace vigc {
#define VIG_NEND() }
#else
#define VIG_NBEGIN()
#define VIG_NEND()
#endif

VIG_NBEGIN()

/// image x or y coordinate
typedef VytU VigCoord;

/// pixel value (all components)
typedef VytU VigValue;

/// pixel kind
typedef enum VigPixel { vix_Unknown, vix_1, vix_8, vix_g8, vix_s8,
   vix_rgb24, vix_ybr24, vix_rgba32, vix_argb32 } VigPixel;

/// one image plane
typedef enum VigPlane { vpl_Unknown, vpl_R, vpl_G, vpl_B, vpl_Y, vpl_Cb, vpl_Cr } VigPlane;

typedef VytF * VigHist;

/// gpu image system
typedef struct Vig_Vulimg * VigVulimg;

/// gpu stored image
typedef struct Vig_Image * VigImage;

/// transformation
typedef VytFTrans2 VigTrans;

#define VIG_SUCCESS    VCP_SUCCESS
#define VIG_HOSTMEM    VCP_HOSTMEM
#define VIG_INITERR    -11001
#define VIG_STORAGEERR -11002
#define VIG_COORDERR   -11003
#define VIG_PIXELERR   -11004
#define VIG_TASKERR    -11005
#define VIG_NOIMG      -11006
#define VIG_BMPERR     -11008
#define VIG_STREAMERR  -11009

#define VIG_MUCH 1000000000

/// vig last error code
int vig_error();
/// check result and fail if not ok
void vig_check_fail();

/// initialize image operations
bool vig_init( VcpVulcomp v );
/// terminate image operations
void vig_done();

/// pixel size in bits
uint32_t vig_pixel_size( VigPixel pixel );

/// create new image
VigImage vig_image_create( VigCoord width, VigCoord height, VigPixel pixel );
/// get image pixel format
VigPixel vig_image_pixel( VigImage img );
/// get image width
VigCoord vig_image_width( VigImage );
/// get image height
VigCoord vig_image_height( VigImage );
/// get image data address
void * vig_image_address( VigImage );
/// get image stride in bytes
VigCoord vig_image_stride( VigImage );
/// get image storage
VcpStorage vig_image_storage( VigImage );
/// destroy image
void vig_image_free( VigImage );

/// copy whole image
bool vig_image_copy( VigImage src, VigImage dst );

/// extract plane from image
bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst );
/// add a plane to image
bool vig_image_join( VigImage dst, VigImage src, VigPlane plane );
/// transform image
bool vig_image_transform( VigImage src, VigImage dst, VigTrans trans );
/// add image pixel values
bool vig_image_add( VigImage src, VigValue pixel, VigImage dst );
/// difference of two images
bool vig_image_diff( VigImage a, VigImage b, VigImage dst );
/// average pixel
bool vig_image_avg( VigImage img, VigValue * pix );

/// copy image part
bool vig_part_copy( VigImage img, VytURect prt, VigImage dst, VytUVec2 loc );
/// sum of difference
bool vig_part_diffsum( VigImage img, VytURect prt, VigImage b,
   VytUVec2 loc, VytZ * diff );
/// fill with value
bool vig_part_fill( VigImage img, VytURect prt, VigValue pix );

/// create "pyramid" of an image: /2, /4, ... scaled images
bool vig_pyr_create( VigImage img, VigImage pyr );
/// calculate delta (move) between two images using their pyramids
bool vig_pyr_delta( VigImage a, VigImage b, VigImage pyra, VigImage pyrb,
   float limit, int32_t * dx, int32_t * dy );

/// create horizontal nad vertical histogram of an image
bool vig_hist_create( VigImage img, VigHist horz, VigHist vert, bool norm );

/// get rects of interest
bool vig_white_rects( VigImage img, float limit,
   float density, uint32_t minSize, uint32_t maxDist,
   VytURect rects, uint32_t * count );

/// draw rectangle
bool vig_draw_rect( VigImage img, VytURect part, VigValue pixel );

/// reads raw image
bool vig_raw_read( VigImage img, void * stream, VytStreamOp read, bool pad );
/// writes raw image
bool vig_raw_write( VigImage img, void * stream, VytStreamOp write, bool pad );

/// reads bmp
VigImage vig_bmp_read( void * stream, VytStreamOp read );
/// write bmp
bool vig_bmp_write( VigImage img, void * stream, VytStreamOp write );

void vig_drawallrects( VigImage img, uint32_t n );
// void vig_drawallclouds( VigImage img, uint32_t n );

VIG_NEND()

#endif // VULIMGH
