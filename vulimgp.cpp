#include "vulimg.hpp"

#include "vulimg.h"
 
using namespace vig;
using namespace vigc;

VcpVulcomp vulcmp = NULL;

/// refcounted handle
struct HRefData: RefData {
   void * handle;
   HRefData( void * handle ) : handle(handle) {}
};

/// stream művelet
VytU streamOp( void * stream, void * mem, VytU size ) {
   return ((Stream *)stream)->op( mem, size );
}

/// hívás után kivétel
inline void check( bool ret ) {
   if ( ! ret )
      throw vig::Exc( vig_error() );
}

/// pointer eredményű hívás után kivétel
inline void * checkp( void * ret ) {
   if ( ! ret )
      throw vig::Exc( vig_error() );
   return ret;
}   

/// transzformáció konverzió
inline Vyt_FTrans2 fTrans( const FTrans2 & t ) {
   return { .sx=t.sx, .rx=t.sy, 
      .ry=t.ry, .sy=t.sy, .dx=t.dx, .dy=t.dy };
}
   
/// téglalap konverzió
inline Vyt_URect fRect( const URect & r ) {
   return { .left=r.left, .top=r.top, 
      .width=r.width, .height=r.height };
}
   
/// vektor konverzió
static inline Vyt_UVec2 fVec( const UVec2 & v ) {
   return { .x=v.x, .y=v.y };
}

/// nyél
static inline VigImage fImg( const Image & img ) {
   return (VigImage)img.imp();
}

static VigPixel fPixel( Pixel p ) {
   switch( p ) {
      case Pixel::B1: return vix_1;
      case Pixel::B8: return vix_8;
      case Pixel::G8: return vix_g8;
      case Pixel::S8: return vix_s8;
      case Pixel::RGB24: return vix_rgb24;
      case Pixel::YBR24: return vix_ybr24;
      case Pixel::RGBA32: return vix_rgba32;
      case Pixel::ARGB32: return vix_argb32;
      default: return vix_Unknown;
   }
}
   
static Pixel tPixel( VigPixel p ) {
   switch( p ) {
      case vix_1: return Pixel::B1;
      case vix_8: return Pixel::B8;
      case vix_g8: return Pixel::G8;
      case vix_s8: return Pixel::S8;
      case vix_rgb24: return Pixel::RGB24;
      case vix_ybr24: return Pixel::YBR24;
      case vix_rgba32: return Pixel::RGBA32;
      case vix_argb32: return Pixel::ARGB32;
      default: return Pixel::UNKNOWN;
   }
}
   

Vulimg::Vulimg( CString name, Uint flags ) {
   if ( vulcmp ) 
      throw vig::Exc( VIG_INITERR );
   if ( ! (vulcmp = vcp_init( name, flags ))) 
      throw vig::Exc( VIG_INITERR );
   check( vig_init( vulcmp ) );
} 
     
Vulimg::~Vulimg() {
   vig_done();
   vcp_done( vulcmp );
   vulcmp = NULL;
}

Image::Image( UVec2 dims, Pixel pix ) {
   VigImage ret = vig_image_create( dims.x, dims.y, fPixel( pix ) );
   checkp( ret );
   ref( new HRefData( ret ) );
}

Image::Image( Ptr p ) {
   ref( new HRefData( p ) );
}

Image::Image( Image img, bool withData ) 
: Image( img.dims(), img.pixel() ) 
{
   if ( withData ) Draw::copy( img, *this );
}

void Image::destroy() {
   vig_image_free( (VigImage)imp() );
}

Ptr Image::imp() const {
   return ((HRefData *)rd)->handle;
}

UVec2 Image::dims() const {
   VigImage img = (VigImage)imp();
   return UVec2( vig_image_width(img), vig_image_height(img) );
}

Pixel Image::pixel() const {
   return tPixel( vig_image_pixel( (VigImage)imp() ));
}

void Image::write( Stream s, bool pad ) const {
   check( vig_raw_write( (VigImage)imp(), &s, streamOp, pad ));
}

void Draw::copy( Image src, Image dst ) {
   check( vig_image_copy( fImg(src), fImg(dst) ));
}

void Draw::copy( Image src, const URect & prt, Image dst, const UVec2 & loc ) {
   Vyt_URect rr = fRect( prt );
   Vyt_UVec2 ll = fVec( loc );
   check( vig_part_copy( fImg(src), &rr, fImg(dst), &ll ));
}


void Draw::transform( Image src, const FTrans2 & t, Image dst ) {
   Vyt_FTrans2 tt = fTrans( t );
   check( vig_image_transform( fImg(src), fImg(dst), &tt ));
}

Image Bmp::read( Stream s ) {
   return Image( checkp( vig_bmp_read( &s, streamOp )));
}

void Bmp::write( Image img, Stream s ) {
   check( vig_bmp_write( fImg(img), &s, streamOp ));
}

vig::Exc::Exc( Int code ) : code(code) {}
