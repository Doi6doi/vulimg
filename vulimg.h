#ifndef VULIMGH
#define VULIMGH

/**
# vulimg.h

[Vulimg](index) is a *C* and *C++* library for
low-level image manipulation through Vulkan.

The `vulimg.h` header is for the *C* part.
It contains the [#VigImage] type, some simple
auxiliary types and correspondent functions.

If compiled with a C++ compiler, all the declarations are in
the `vigc` namespace
*/

/** ## Contents
\toc */

/** ## Details */

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

/// Image x or y coordinate
typedef VytU VigCoord;

/// Pixel value (all components)
typedef VytU VigValue;

/// Pixel kind
typedef enum Vig_Pixel {
   /// Unkown
   vix_Unknown,
   /// Black and White 1 bit
   vix_1,
   /// Custom 8 bit data
   vix_8,
   /// Grayscale 8 bit
   vix_g8,
   /// Custom signed 8-bit data
   vix_s8,
   /// RGB 24 bit
   vix_rgb24,
   /// YCbCr 24 bit
   vix_ybr24,
   /// RGBA 32 bit
   vix_rgba32,
   /// ARGB 32 bit
   vix_argb32
} VigPixel;

/// An image plane
typedef enum Vig_Plane {
   /// Unknown
   vpl_Unknown,
   /// Red plane of RGB
   vpl_R,
   /// Green plane of RGB
   vpl_G,
   /// Blue plane of RGB
   vpl_B,
   /// Y plane of YCbCr
   vpl_Y,
   /// Cb plane of YCbCr
   vpl_Cb,
   /// Cr plane of YCbCr
   vpl_Cr
} VigPlane;

/// Histogram data of an image
typedef VytF * VigHist;

/// Vulimg system
typedef struct Vig_Vulimg * VigVulimg;

/// Image usable by GPU
typedef struct Vig_Image * VigImage;

/// 2d Affine transformation
typedef VytFTrans2 VigTrans;

/// Vulimg error values
typedef enum Vig_Result {
   /// Successful execution
   VIG_SUCCESS = VCP_SUCCESS,
   /// Not enough host memory
   VIG_HOSTMEM = VCP_HOSTMEM,
   /// Initialization error
   VIG_INITERR = -11001,
   /// Storage error
   VIG_STORAGEERR = -11002,
   /// Coordinate error or mismatch
   VIG_COORDERR = -11003,
   /// Pixel error or mismatch
   VIG_PIXELERR = -11004,
   /// Task error
   VIG_TASKERR = -11005,
   /// No image given
   VIG_NOIMG = -11006,
   /// BMP error
   VIG_BMPERR = -11008,
   /// Stream error
   VIG_STREAMERR = -11009
}  VigResult;

/// Too large value
#define VIG_MUCH 1000000000

/** Last error code
\return Code */
VYT_EXPORT VigResult vig_error();

/// Check result and fail if not ok
VYT_EXPORT void vig_check_fail();

/** initialize image operations
\param v Vulcmp subsystem
\return `true` on success */
VYT_EXPORT bool vig_init( VcpVulcomp v );

/// terminate image operations
VYT_EXPORT void vig_done();

/** Pixel size in bits
\param pixel Pixel kind
\return Size in bits */
VYT_EXPORT uint32_t vig_pixel_size( VigPixel pixel );

/** Create new image
\param width Width of image in pixels
\param heiht Height of image in pixels
\param pixel Pixel kind
\return Image handle or NULL on error */
VYT_EXPORT VigImage vig_image_create( VigCoord width, VigCoord height, VigPixel pixel );

/** Get image pixel kind
\param img Handle
\return Pixel kind */
VYT_EXPORT VigPixel vig_image_pixel( VigImage img );

/** Get image width
\param img Handle
\return Width in pixels */
VYT_EXPORT VigCoord vig_image_width( VigImage );

/** Get image height
\param img Handle
\return Height in pixels */
VYT_EXPORT VigCoord vig_image_height( VigImage );

/** Get image data address
\param img Handle
\return CPU-address of image data */
VYT_EXPORT void * vig_image_address( VigImage img );

/** Get image stride in bytes
Image rows are stored 4-byte padded for GPU performance.
Stride is the difference in bytes between two rows in memory
\param img Handle
\return Stride in bytes */
VYT_EXPORT VigCoord vig_image_stride( VigImage img );

/** Get image storage
\param img Handle
\return Vulcmp storage handle of image */
VYT_EXPORT VcpStorage vig_image_storage( VigImage img );

/** Destroy image
Usually not needed as [#vig_done] frees up all resources.
You can call it to save GPU memory if the image is no longer needed.
\param img Handle */
VYT_EXPORT void vig_image_free( VigImage img );

/** Copy whole image
Copies `src` into `dst`.
Sizes must match and pixel kinds must be compatible.
\param src Source image
\param dst Destination image
\return `true` on success */
VYT_EXPORT bool vig_image_copy( VigImage src, VigImage dst );

/** Extract plane from image
Extracts `plane` from `src` and copies it into `dst`.
Sizes must match and pixel kinds must be compatible.
\param src Source image
\param plane Plane to extract
\param dst Destination image
\return `true` on success */
VYT_EXPORT bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst );

/** Set plane of an image
Sets `src` as `plane` plane of `dst`
Sizes must match and pixel kinds must be compatible.
\param src Source image
\param plane Plane to set
\param dst Destination image
\return `true` on success */
VYT_EXPORT bool vig_image_join( VigImage dst, VigImage src, VigPlane plane );

/** Transform image
Do an affine (scale, rotate, skew, shift) transformation.
Pixel kinds must be compatible.
\param src Source image
\param dst Destination image
\param trans Transformation
\return `true` on success */
VYT_EXPORT bool vig_image_transform( VigImage src, VigImage dst, VigTrans trans );

/** Add pixel values
Adds `pixel` to all pixels of `src` and outputs `dst`.
Results are clamped on each component.
Sizes must match and pixel kinds must be compatible.
\param src Source image
\param pixel Pixel value to add
\param dst Destination image
\return `true` on success */
VYT_EXPORT bool vig_image_add( VigImage src, VigValue pixel, VigImage dst );

/** difference of two images
Component-wise difference of `a` and `b` resulting `dst`.
Results are component-wise absolute value by pixels.
Sizes must match and pixel kinds must be compatible.
\param a First image
\param b Second image
\param dst Destination image
\return `true` on success */
VYT_EXPORT bool vig_image_diff( VigImage a, VigImage b, VigImage dst );

/** Average pixel
Returns component-wise average of all pixels in `img`
\param img Handle
\param pix Return value
\return `true` on success */
VYT_EXPORT bool vig_image_avg( VigImage img, VigValue * pix );

/** Copy image part
Copies `prt` part of `src` into `loc` location of `dst`.
Pixel kinds must be compatible, `prt` must be inside `img` and the
resulting are must be inside `dst`
\param src Source image
\param prt Part of `src` to be copied
\param dst Destination image
\param loc Top left corner of copied area in `dst`
\return `true` on success */
VYT_EXPORT bool vig_part_copy( VigImage src, VytURect prt, VigImage dst, VytUVec2 loc );

/** Sum of difference between parts of images
Sum of all the differences between component values of
`prt` area in `a` and `prt`+`loc` area in `b`.
Areas must be inside images and pixel kinds must be compatible
\param a First image
\param prt Area of `a` to be compared
\param b Second image
\param `loc` Top left corner of area of `b` to be compared
\param diff Return value, sum of all component-differences
\return `true` on success */
VYT_EXPORT bool vig_part_diffsum( VigImage a, VytURect prt, VigImage b,
   VytUVec2 loc, VytZ * diff );

/** Fill area with pixel
\param img Image handle
\param prt Area to fill. Must be inside `img`
\param pix Pixel value
\return `true` on success */
VYT_EXPORT bool vig_part_fill( VigImage img, VytURect prt, VigValue pix );

/** Create pyramid of an image
A pyramid is a /2, a /4, ... etc. scaled versions below each other
Width of `pyr` must be at least width of `img`
Height of `pyr` must be at least height of `img`
Pixel kinds must be compatible.
\param img Source image
\param pyr Destination image for the pyramid
\return `true` on success */
VYT_EXPORT bool vig_pyr_create( VigImage img, VigImage pyr );

/** Calculate delta (move) between two images using their pyramids
\param a First image
\param b Second image
\param pyra Pyramid of a
\param pyrb Pyramid of b
\param limit Fraction of differences between images to give up (e.g 0.1 = 10%)
\param dx Return value: X shift or [#VIG_MUCH] if images are too different
\param dy Return value: Y shift or [#VIG_MUCH] if images are too different
\return `true` on success */
VYT_EXPORT bool vig_pyr_delta( VigImage a, VigImage b, VigImage pyra, VigImage pyrb,
   float limit, int32_t * dx, int32_t * dy );

/** Create horizontal and vertical histogram of an image
The histogram is an averaged value for every row or column of the image
\param img Source image
\param horz Return value: Horizontal histogram. Must have img.width elements
\param vert Return value: Vertical histogram. Must have img.height elements
\param norm On `true` histograms will have noramlized (0-1) values
\return `true` on success */
VYT_EXPORT bool vig_hist_create( VigImage img, VigHist horz, VigHist vert, bool norm );

VYT_EXPORT bool vig_white_rects( VigImage img, float limit,
   float density, uint32_t minSize, uint32_t maxDist,
   VytURect rects, uint32_t * count );

/** Draw rectangle
Draws a rectangle frame
\param img Image to draw to
\param part The rectangle
\param pixel Pixel value to draw rectangle with
\return `true` on success */
VYT_EXPORT bool vig_draw_rect( VigImage img, VytURect part, VigValue pixel );

/** Read raw image
Read raw image from `stream` containing data in pixel format of `dst`
\param dst Destitaion image
\param stream Handle of stream containing data
\param read Read operation on `stream`
\param pad Is data 4-byte padded in `stream`
\return `true` on success */
VYT_EXPORT bool vig_raw_read( VigImage dst, void * stream, VytStreamOp read, bool pad );

/** Write raw image
Write raw image data to `stream`
\param src Source image
\param stream Handle of stream to write to
\param write Write operation on `stream`
\param pad Write each row 4-byte padded
\return `true` on success */
bool vig_raw_write( VigImage src, void * stream, VytStreamOp write, bool pad );

/** Read BMP image
Read an uncompressed BMP from a stream
\param stream Handle of stream to read from
\param read Read operation on `stream`
\return Image read or NULL on error */
VigImage vig_bmp_read( void * stream, VytStreamOp read );

/** Write BMP image
Write image as uncompressed BMP to `stream`
Pixel kind must be BMP-compatible
\param img Image handle
\param stream Handle of stream to write to
\param write Write operation on `stream`
\return `true` on success */
bool vig_bmp_write( VigImage img, void * stream, VytStreamOp write );

void vig_drawallrects( VigImage img, uint32_t n );
// void vig_drawallclouds( VigImage img, uint32_t n );

VIG_NEND()

#endif // VULIMGH
