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


