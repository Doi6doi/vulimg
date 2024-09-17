#ifndef VULIMGH
#define VULIMGH

#include <vulcmp.h>
#include <vultools.h>

typedef uint32_t VigCoord;
typedef enum VigPixel { vix_Unknown, vix_1, vix_8, vix_g8, vix_rgb24, vix_rgba32, vix_ybr24 } VigPixel;
typedef enum VigPlane { vpl_Unknown, vpl_R, vpl_G, vpl_B, vpl_Y, vpl_Cb, vpl_Cr } VigPlane;

typedef struct VigImage * VigImage;

typedef struct VigTransform {
   float sx, ry, rx, sy, dx, dy;
} * VigTransform;

#define VIG_SUCCESS    VCP_SUCCESS
#define VIG_HOSTMEM    VCP_HOSTMEM
#define VIG_STREAMERR  VTL_STREAMERR
#define VIG_INITERR    -11001
#define VIG_STORAGEERR -11002
#define VIG_COORDERR   -11003
#define VIG_PIXELERR   -11004
#define VIG_TASKERR    -11005
#define VIG_NOIMG      -11006
#define VIG_BMPERR     -11008

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
VigPixel vig_image_pixel( VigImage );
/// get image width
VigCoord vig_image_width( VigImage );
/// get image height
VigCoord vig_image_height( VigImage );
/// get image data address
void * vig_image_address( VigImage );
/// get image stride in dwords
uint32_t vig_image_stride( VigImage );
/// get image storage
VcpStorage vig_image_storage( VigImage );
/// destroy image
void vig_image_free( VigImage );

/// copy image
bool vig_image_copy( VigImage src, VigImage dst, VtlRect rect, 
   VigCoord dstLeft, VigCoord dstTop );
/// add a plane to image
bool vig_image_join( VigImage dst, VigImage src, VigPlane plane );
/// extract plane from image
bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst );
/// transform image
bool vig_image_transform( VigImage src, VigImage dst, VigTransform t );
/// difference of two images
bool vig_image_diff( VigImage a, VigImage b, VigImage dst );
/// sum of difference
bool vig_image_diffsum( VigImage a, VtlRect rect, VigImage b,
   uint32_t bx, uint32_t by, uint64_t * diff );
/// create "pyramid" of an image: /2, /4, ... scaled images
bool vig_image_pyramid( VigImage src, VigImage dst );
/// get rects of interest
bool vig_white_rects( VigImage img, float limit, VtlRect rects, uint32_t * count );
/// reads raw image
bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad );
/// writes raw image
bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad );


/// reads bmp
VigImage vig_bmp_read( void * stream, VtlStreamOp read );
/// write bmp
bool vig_bmp_write( VigImage src, void * stream, VtlStreamOp write );


#endif // VULIMGH
