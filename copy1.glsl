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
   
void edgePixel( uint x, uint y ) {
   int sm = int( p.sleft ) % 32;
   int rest = int(sm+p.width) - int( x*32 );
   if ( 32 <= rest ) return;
   int left = 0;
   if ( 0 == x ) {
      left = int( p.dleft ) % 32;
      rest -= left;
   }
   uint si = (y+p.stop)*p.src.stride + p.sleft/32 + x;
   uint v;
   if ( left+rest <= 32 ) {
      v = bitfieldExtract( source[si], left, rest );
   } else {
      int a = 32-left;
      v = bitfieldExtract( source[si], left, a )
         | bitfieldExtract( source[si+1], 0, rest-a ) << a;
   }
   uint di = (y+p.dtop)*p.dst.stride + p.dleft/32 + x;
   dest[ di ] = bitfieldInsert( dest[di], v, left, rest );
}
   

void midPixel( uint x, uint y ) {
   int sm = int( p.sleft ) % 32;
   int dm = int( p.dleft ) % 32;
   uint si = (y+p.stop)*p.src.stride + p.sleft/32 +x;
   uint v;
   if ( sm <= dm ) {
      int a = sm+32-dm;
      int b = 32-a;
      v = bitfieldExtract( source[si-1], b, a )
         | bitfieldExtract( source[si], 0, b ) << a;
   } else { 
      int a = sm - dm;
      int b = 32-a;
      v = bitfieldExtract( source[si], a, b )
         | bitfieldExtract( source[si+1], 0, a ) << b;
   }
   dest[ (y+p.dtop)*p.dst.stride + p.dleft/32 + x ] = v;
}
   
   
void main() {
   uint y = gl_GlobalInvocationID.y;
   if ( p.height <= y ) return;
   uint x = gl_GlobalInvocationID.x;
   uint xl = (p.width+31)/32;
   if ( 0 == x || xl <= x )
      edgePixel( x, y );
      else midPixel( x, y );
}
