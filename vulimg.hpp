#ifndef VULIMGHPP
#define VULIMGHPP

#include <vytools.hpp>
 
using namespace vyt;   

namespace vig {

/// pixel value
typedef Uint Value;
     
/// pixel kind
enum class Pixel { UNKNOWN, B1, B8, G8, S8, RGB24, YBR24, RGBA32, ARGB32 };

/// plane
enum class Plane { UNKNOWN, R, G, B, Y, Cb, Cr };   

/// module class
class Vulimg: public NoAssign {
public:
   /// too large displacement
   static constexpr Int MUCH = 1000000000;
   /// pixel size in bits
   static Uint pixelSize( Pixel );
public:
   Vulimg( CString name = "", Uint flags = 0 );
   ~Vulimg();
};

/// image
class Image: public HRefCount {
protected:
   void destroy();
public:
   /// by properties
   Image( UVec2 dims, Pixel );
   /// by other image
   Image( Image other, bool withData );
   /// by implementation
   Image( Ptr );
   /// get image pixel format
   Pixel pixel() const;
   /// get dimensions
   UVec2 dims() const;
   /// get image data address
   Ptr address(); 
   /// get image implementation
   Ptr imp() const;
   /// get image stride in bytes
   Uint stride() const;
   /// get vulcmp storage pointer
   Ptr storage();
   /// extract plane from image
   void plane( Plane plane, Image dst );
   /// add a plane to image
   void join( Plane plane, Image src );
   /// difference of two images
   void diff( Image other, Image dst );
   /// average pixel
   Value average();
   /// sum of difference
   Zint diffsum( URect part, Image other, UVec2 dstLoc );
   /// reads raw image
   void read( Stream, bool pad );
   /// writes raw image
   void write( Stream, bool pad ) const;
};

/// drawing methods
class Draw: public NoCreate {
public:
   /// fill part with color
   static void fill( Image img, const URect & part, Value value );
   /// copy whole image
   static void copy( Image src, Image dst );
   /// copy image part
   static void copy( Image src, const URect & part, Image dst, const UVec2 & dstLoc );
   /// add image pixel values
   static void add( Image img, Value pixel );
   /// transform image
   static void transform( Image src, const FTrans2 & trans, Image dst );
   /// draw rectangle
   static bool rect( Image img, const URect & part, Value value );
};

/// create "pyramid" of an image: /2, /4, ... scaled images
class Pyr: public RefCount {
public:
   Pyr( Image );
   ~Pyr();
public:
   /// calculate delta (move) between two images using their pyramids
   static IVec2 delta( Image a, Image b, Pyr pa, Pyr pb, Float limit );
};


/// Bmp read/write
class Bmp: public NoCreate {
public:
   static Image read( Stream );
   static void write( Image, Stream );
};

/// kivétel
class Exc: public vyt::Exc {
public:
   Int code;
   Exc( Int code );
};

}

#endif // VULIMGHPP
