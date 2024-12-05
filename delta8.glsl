#version 450
#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigDeltaParams p;
};

layout (binding = 0 ) readonly buffer Source {
   uint source[];
};

layout( binding = 2 ) writeonly buffer Dest {
   uint dest[];
};

uint x, v, d;

uint d8comp( int i ) {
   uint old = bitfieldExtract( v, 8*i, 8 );
   uint pv = bitfieldExtract( p.pixel, int(8*(i+d)%4), 8 );
   if ( 0 == pv )
      return old;
   return clamp( int(old)+int(pv), p.min, p.max );
}

void main() {
   x = 4*gl_GlobalInvocationID.x;
   if ( p.img.width <= x*4 ) return;
   d = 4*x % p.compCount;
   uint y = gl_GlobalInvocationID.x;
   if ( p.img.height <= y ) return;
   v = source[ y*p.img.stride + x ];
   uint ret = 0;
   for (int i=3; 0<=i; --i)
      ret = ret << 8 | d8comp( i );
   dest[ y * p.img.stride + x ] = ret;
}
   
