#version 450
#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigCopyParams p;
};

layout (binding = 0 ) readonly buffer Source {
   uint source[];
};

layout( binding = 1 ) buffer Dest {
   uint dest[];
};
   
uint x, y;   
   
uint bits(int b, int l) {
   int sm = int( p.sleft ) % 32;
   b += sm;
   uint si = (y+p.stop)*p.src.stride + p.sleft/32 + x;
   if ( 0 > b ) {
      b = -b;
      return bitfieldExtract( source[si-1], 32-b, b )
         | bitfieldExtract( source[si], 0, l-b ) << b;
   } else if ( 32 < b+l ) {
      b = 32-b;
      return bitfieldExtract( source[si], 32-b, b )
         | bitfieldExtract( source[si+1], 0, l-b ) << b;
   } else {
      return bitfieldExtract( source[si], b, l );
   }
}   
   
   
void main() {
   y = gl_GlobalInvocationID.y;
   if ( p.height <= y ) return;
   x = gl_GlobalInvocationID.x;
   int dm = int( p.dleft ) % 32;
   int rest = int( dm+p.width )-int(x*32);
   if ( 0 >= rest ) return;
   uint di = (y+p.dtop)*p.dst.stride + p.dleft/32 + x;
   uint v;
   int l;
   if ( 0 == x ) {
      // bal pixel
      l = min( rest, 32-dm );
      v = bitfieldInsert( dest[di], bits(0,l), dm, l );
   } else if ( dm+rest < 32 ) {
      // jobb pixel
      l = min( 32, dm+rest );
      v = bitfieldInsert( dest[di], bits(-dm,l), 0, l );
   } else {
      // középső pixel
      v = bits(-dm,32);
   }
   dest[di] = v;
}
