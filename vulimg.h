#ifndef VULIMGH
#define VULIMGH

#include <vulcmp.h>

typedef uint32_t VigCoord;
typedef enum Vig_Pixel { vix_Unknown, vix_8, vix_g8, vix_rgb24, vix_rgba32, vix_ybr24 } VigPixel;
typedef enum Vig_Plane { vpl_Unknown, vpl_R, vpl_G, vpl_B, vpl_Y, vpl_Cb, vpl_Cr } VigPlane;

typedef struct Vig__Image * VigImage;

#define VIG_SUCCESS    VCP_SUCCESS
#define VIG_HOSTMEM    VCP_HOSTMEM
#define VIG_INITERR    -11001
#define VIG_STORAGEERR -11004
#define VIG_COORDERR   -11005
#define VIG_PIXELERR   -11006
#define VIG_TASKERR    -11007

/// vig last error code
int vig_error();
/// check result and fail if not ok
void vig_check_fail();
/// initialize image operations
bool vig_init( VcpVulcomp v );
/// terminate image operations
void vig_done();
/// pixel size in bytes
uint32_t vig_pixel_size( VigPixel pixel );
/// create new image
VigImage vig_image_create( VigCoord width, VigCoord height, VigPixel pixel );
/// get image width
VigCoord vig_image_width( VigImage );
/// get image height
VigCoord vig_image_height( VigImage );
/// get image stride
VigCoord vig_image_stride( VigImage );
/// get image pixel format
VigPixel vig_image_pixel( VigImage );
/// get image data
void * vig_image_address( VigImage );
/// get image storage
VcpStorage vig_image_storage( VigImage ); 
/// destroy image
void vig_image_free( VigImage );
/// copy image part
bool vig_image_copy( VigImage src, VigCoord srcx, VigCoord srcy,
   VigImage dst, VigCoord dstx, VigCoord dsty, VigCoord width, VigCoord height );
/// add a plane to image
bool vig_image_join( VigImage dst, VigImage src, VigPlane plane );
/// extract plane from image
VigImage vig_image_plane( VigImage src, VigPlane plane );
/// scale image
bool vig_image_scale( VigImage src, VigImage dst );

#endif // VULIMGH
