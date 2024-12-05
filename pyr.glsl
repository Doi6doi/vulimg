#version 450
#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigPyrParams p;
};

layout (binding = 0 ) readonly buffer Source {
   uint source[];
};

layout (binding = 1 ) buffer Dest {
   uint dest[];
};


uint calcComp( int xPix, int y, int iComp ) {
   int x2 = xPix * 2;
   int x2Comp = x2*p.compCount+iComp;
   int x2Bit = x2Comp * p.compBits;
   int m1 = x2Bit % 32;
   int m2 = m1 + p.compBits*p.compCount % 32;
   if (0 == p.row) {
      uint si = y*2*p.src.stride + x2Bit / 32;
      uint v = bitfieldExtract( source[si], m1, p.compBits )
         + bitfieldExtract( source[si+p.src.stride], m1, p.compBits );
      if ( m1 >= m2 )
         ++si;
      v += bitfieldExtract( source[si], m2, p.compBits )
         + bitfieldExtract( source[si+p.src.stride], m2, p.compBits );
      return (v+2)/4;
   } else {
      uint dy = p.row-2*(p.height-y);
      uint di = dy*p.dst.stride + x2Bit / 32;
      uint v = bitfieldExtract( dest[di], m1, p.compBits )
         + bitfieldExtract( dest[di+p.dst.stride], m1, p.compBits );
      if ( m1 >= m2 )
         ++di;
      v += bitfieldExtract( dest[di], m2, p.compBits )
         + bitfieldExtract( dest[di+p.dst.stride], m2, p.compBits );
      return (v+2)/4;
   }
}


void main() {
   uint y = gl_GlobalInvocationID.y;
   if ( p.height <= y ) return;
   uint x = gl_GlobalInvocationID.x;
   int xComp = 32*int(x) / p.compBits;
   int xPix = xComp / p.compCount;
   if ( p.width <= xPix ) return;
   int iComp = xComp % p.compCount;
   uint ret = 0;
   for (int i=0; i<32; i += p.compBits) {
      uint v = calcComp( xPix, int(y), iComp );
      ret = bitfieldInsert( ret, v, i, p.compBits );
      if ( ++iComp == p.compCount ) {
         ++xPix;
         iComp = 0;
      }
   }
   dest[ (y+p.row) * p.dst.stride + x ] = ret;
}
   
