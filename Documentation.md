# Vulimg documentation

Vulimg is a simple C library with image handling functions using GPU. 
Works much faster than a CPU solution.

- [Important types](#important-types)
- [Important functions](#important-functions)
- [Less important functions](#less-important-functions)
- [Constant values](#constant-values)
- [Example code](#example-code)

## Important types

- `VigImage`: opaque type handle for an image handled by this library.
- `VigCoord`: image x or y coordinate (unsigned 32 bit integer).
- `VigPixel`: image pixel type. Then number means pixel size in bits.
- `VigPlane`: a plane (component) of an image.
- `VigTransform`: an affine transformation which can be applied to an image

For detailed values see the [constants](#constant-values)

Some types are used from the [vulcmp](https://github.com/Doi6doi/vulcmp/blob/main/Documentation.md) 
and [vultools](https://github.com/Doi6doi/vultools/blob/main/Documentation.md) libraries

## Important functions

```c
    int vig_error()
```
Returns [error code](#error-codes) of last call. 0 means no error.

---
```c
    void vig_init( VcpVulcomp v ) 
```
Initialize vulimg system.
- `v`: already initialized vulcmp instance
---
```c
void vig_done()
```
End of vulimg usage. Also frees all created images.

---
```c
VigImage vig_image_create( uint32_t width, uint32_t height, VigPixel pixel )
```
Create new image.
- `width`: image width in pixels
- `height`: image heightin pixels
- `pixel`: image pixel type
- *returns* handle to new image or NULL if an error occurs

---
```c
void * vig_image_address( VigImage img )
```
Address of image pixels. This function is needed to be called before 
reading or modifying pixels directly. It syncs GPU memory with host memory.
- `img`: image handle
- *returns* address of pixels to read or write


---
```c
bool vig_image_copy( VigImage src, VigImage dst, VtlRect rect, 
   VigCoord dstLeft, VigCoord dstTop )
```
Copy part of image to other. Areas must fit in images, and piyel type smust match.
- `src`: source image
- `dst`: destination image
- `rect`: position and size of source area in `src`
- `dstLeft`: x coordinate of destination area in `dst`
- `dstTop`: y coordinate of destination area in `dst`
- *returns* true if copy is successful

---
```c
bool vig_image_plane( VigImage src, VigPlane plane, VigImage dst )
```
Extract a plane (component) from an image. 
Dimensions and pixel types must match.
- `src`: source image
- `plane`: the plane to extract
- `dst`: destination image
- *returns* true if extraction is successful

---
```c
bool vig_image_join( VigImage dst, VigImage src, VigPlane plane )
```
Updates (writes) a plane (component) of an image. 
Dimensions and pixel types must match.
- `dst`: destination image (which will get a plane updated)
- `src`: source image (containing one compoment data)
- `plane`: the plane to update
- *returns* true if update is successful

---
```c
bool vig_image_transform( VigImage src, VigImage dst, VigTransform trans )
```
Do an affine transformation (move, rotate, scale) on an image. 
Pixel types must match. 
- `src`: source image (to be transformed)
- `dst`: destination image (the result). Must be other than `src`
- `trans`: the transformation matrix. Must be invertible.
- *returns* true if transformation is successful

---
```c
bool vig_image_delta( VigImage src, uint32_t pixel, VigImage dst )
```
Modify each pixel value of an image. The components of `pixel` are added
to them. Result is clamped to pixel type boundaries.
- `src`: source image (to be modified)
- `pixel`: the modification pixel value. Has the same type as `src`.
- `dst`: destination image (the result).
- *returns* true if delta is successful

---
```c
bool vig_image_diff( VigImage a, VigImage b, VigImage dst );
```
Difference between two images. Difference will be pixel-component-wise.
Image dimensions and pixel types must match.
- `a`: first image
- `b`: second image
- `dst`: difference image (the result).
- *returns* true if diff is successful

---
```c
bool vig_image_diffsum( VigImage a, VtlRect rect, VigImage b,
   VigCoord bLeft, VigCoord bTop, uint64_t * diff )
```
Sum of pixel-component differences between parts of two images. 
- `a`: first image
- `rect`: the area to compare in `a`. Must fit inside `a`
- `b`: second image. Can be `a` as well.
- `bLeft`: x coordinate of area in `dst`
- `bTop`: y coordinate of area in `dst`
- `diff`: the sum of pixel-component differences between the areas.
- *returns* true if diffsum is successful

---
```c
bool vig_image_avg( VigImage img, uint32_t * pix )
```
Component-wise average of all pixels in the image.
- `img`: the image
- `pix`: the component-wise pixel value as an average of all pixels of `img`
- *returns* true if averaging is successful

---
```c
bool vig_raw_read( VigImage img, void * stream, VtlStreamOp read, bool pad )
```
Read raw pixel data from stream.
- `img`: the image to fill with pixel data
- `stream`: the stream to read from
- `read`: stream reader function
- `pad`: is pixel data stored in 4-byte padded lines in stream
- *returns* true if raw read is successful

---
```c
bool vig_raw_write( VigImage img, void * stream, VtlStreamOp write, bool pad )
```
Write raw pixel data to stream.
- `img`: the image containing pixel data
- `stream`: the stream to write to
- `write`: stream writer function
- `pad`: should 4-byte padding be written after each line
- *returns* true if raw write is successful

---
```c
VigImage vig_bmp_read( void * stream, VtlStreamOp read );
```
Read image from bmp file. It can only read uncompressed bmp files.
- `stream`: the stream to read bmp from
- `read`: stream reader function
- *returns* the image or NULL if there was an error

---
```c
bool vig_bmp_write( VigImage src, void * stream, VtlStreamOp write )
```
Write image to uncompressed bmp file. Pixel type must be bmp compatible.
- `img`: the image to write
- `stream`: the stream to write bmp data
- `write`: stream writer function
- *returns* true if write is successful

## Less important functions

```c
void vig_check_fail()
```
Checks last error code (*vig_error*) and terminates program with an error message if it was not *VIG_SUCCESS*.

---
```c
void vig_pixel_size( VigPixel pixel )
```
Returns pixel type size in bits.
- `pixel`: pixel type
- *returns* size of pixel in bits

---
```c
VigPixel vig_image_pixel( VigImage img )
```
Returns image pixel type.
- `img`: image handle
- *returns* pixel type of image

---
```c
VigCoord vig_image_width( VigImage img )
```
Returns width of image.
- `img`: image handle
- *returns* width of image in pixels

---
```c
VigCoord vig_image_height( VigImage img )
```
Height width of image.
- `img`: image handle
- *returns* height of image in pixels

---
```c
VigCoord vig_image_stride( VigImage img )
```
Stride of image in bytes, distance between two lines in memory.
It can be a bit more than width * pixel size, because all lines
are 4-bit aligned.
- `img`: image handle
- *returns* stride of image in bytes

---
```c
bool vig_image_pyramid( VigImage src, VigImage dst )
```
Create "pyramid" of image. It means a half-width * half-height, 
then below a quarter-width * quarter-height, etc.. sized shrinked image of
the original. It can be used to calculate shifts between two frames.
Pixel types must match.
- `src`: original image
- `dst`: destination image (where pyramid will go). 
Must be at least width/2 * height.
- *returns* true if pyramid is successful.

---
```c
bool vig_white_rects( VigImage img, float limit, 
   float dist, VtlRect rects, uint32_t * count );
```
Get rectangular parts of image where there are light pixel values.
Those will be an area of interest (e.g after edge detection or diff detection)
pixel type must be `vix_8` or `vix_g8`
- `img`: image handle
- `limit`: must be between 0 and 1. 
Pixels darker than that (<256*limit) won't be considered light.
- `dist`: maximum distance between two detected rects to join. 
If they are farther than that, they will be returned separately.
- `rects`: the address where detected rect coordinates will be returned.
- `count`: the number of rects returned. 
The input value is the maximum number of rects returned.
- *returns* true if rect detection is successful

---
```c
VcpStorage vig_image_storage( VigImage img )
```
Vulcmp storage object of image. Usually not needed to use.
- `img`: image handle
- *returns* vulcmp handle of storage

---
```c
void vig_image_free( VigImage img )
```
Frees resources used by `img`. You usually don't need to call this as vulimg frees up everything on `vig_done`. 
It is only needed if you wish to save memory earlier.
- `img`: image to dispose

## Constant values

### Error codes

- `VIG_SUCCESS`: last operation terminated successfully.
- `VIG_HOSTMEM`: host memory allocation error
- `VIG_STREAMERR`: stream read or write error
- `VIG_INITERR`: could not initialize vulimg sytsem
- `VIG_STORAGEERR`: could not allocate storage for image
- `VIG_COORDERR`: non-matching or overlapping image coordinates
- `VIG_PIXELERR`: non-matching image pixel types
- `VIG_TASKERR`: error during execution of vulcmp task
- `VIG_NOIMG`: null image handle passed
- `VIG_BMPERR`: error during bmp file read (unknown format)

### Pixel types

- `vix_Unknown`: dummy empty value. An image cannot have this.
- `vix_1`: 1-bit pixel (black/white)
- `vix_8`: 8-bit value (usually a component)
- `vix_g8`: grayscale 8-bit pixel
- `vix_s8`: signed 8-bit value (-128..127)
- `vix_rgb24`: 8-8-8 bit red, green, blue values
- `vix_ybr24`: YCbCr 8-8-8 bit luminance, blue-diff, red-diff values
- `vix_rgba32`: 8-8-8-8 bit red, green, blue, alpha values
- `vix_argb32`: 8-8-8-8 bit alpha, red, green, blue values

### Pixel components (planes)

- `vpl_Unknown`: dummy empty value. A pixel cannot have this.
- `vpl_R`: red plane
- `vpl_G`: green plane
- `vpl_B`: blue plane
- `vpl_Y`: luminance plane
- `vpl_Cr`: red-diff chroma plane
- `vpl_Cb`: blue-diff chroma plane

### Transformation matrix

- `sx`: x coordinate scale
- `sy`: y coordinate scale
- `rx`: x coordinate rotation or skew
- `ry`: y coordinate rotation or skew
- `dx`: x coordinate shift
- `dy`: y coordinate shift

## Example code

```c
// initialize GPU
VcpVulcomp v = vcp_init( "vigtest", VCP_VALIDATION );
// initialize vulimg
vig_init( v );
// load bmp image
FILE * fh = fopen( "test.bmp", "rb" );
VigImage i1 = vig_bmp_read( fh, vtl_fread );
fclose( fh );
// create other image
VigImage i2 = vig_image_create( vig_image_width(i1),
   vig_image_height(i1), vig_image_pixel(i1) );
// rotate image with 30 degrees
struct VigTransform tr = {
   .sx=0.866, .rx=-0.5,  .dx=0,
   .ry=0.5,   .sy=0.866, .dy=0
};
vig_image_transform( i1, i2, &tr );
// write as bmp
FILE * gh = fopen("test2.bmp", "wb" );
vig_bmp_write( i2, gh, vtl_fwrite );
// finalize
vig_done();
vcp_done(v);
```

