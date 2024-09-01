#ifndef VULIMGH
#define VULIMGH

#include <vulcmp.h>
#include <vultools.h>

typedef uint32_t VigCoord;
typedef enum Vig_Pixel { vix_Unknown, vix_8, vix_g8, vix_rgb24, vix_rgba32, vix_ybr24 } VigPixel;
typedef enum Vig_Plane { vpl_Unknown, vpl_R, vpl_G, vpl_B, vpl_Y, vpl_Cb, vpl_Cr } VigPlane;

typedef struct VigImage * VigImage;

#define VIG_SUCCESS    VCP_SUCCESS
#define VIG_HOSTMEM    VCP_HOSTMEM
#define VIG_STREAMERR  VTL_STREAMERR
#define VIG_INITERR    -11001
#define VIG_STORAGEERR -11002
#define VIG_COORDERR   -11003
#define VIG_PIXELERR   -11004
#define VIG_TASKERR    -11005
#define VIG_NOIMG      -11006
#define VIG_SHIFTERR   -11007

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

/// is image full or a part
bool vig_image_full( VigImage );
/// get image data address
void * vig_image_address( VigImage );
/// get image start in bits relative to address
uint32_t vig_image_start( VigImage );
/// get image stride in dwords
uint32_t vig_image_stride( VigImage );
/// get image storage
VcpStorage vig_image_storage( VigImage ); 
/// destroy image
void vig_image_free( VigImage );
/// part of an image
VigImage vig_image_part( VigImage img, VigCoord left, VigCoord top,
   VigCoord width, VigCoord height );
/// copy image
bool vig_image_copy( VigImage src, VigImage dst );
/// add a plane to image
bool vig_image_join( VigImage dst, VigImage src, VigPlane plane );
/// extract plane from image
bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst );
/// scale image
bool vig_image_scale( VigImage src, VigImage dst );
/* get average pixel value of image
bool vig_image_avg( VigImage img, uint32_t * avg );
/// pixel value differences between images
bool vig_image_diff( VigImage a, VigImage b, uint32_t * diff );
*/
/// reads bmp
VigImage vig_bmp_read( void * stream, VtlStreamOp read );
/// write bmp
bool vig_bmp_write( VigImage src, void * stream, VtlStreamOp write );
/// reads raw image
bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad );
/// writes raw image
bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad );

#endif // VULIMGH
