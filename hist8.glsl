#version 450

#include "vulimg_comp.h"

#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types: require

layout (local_size_x=UGR, local_size_y=1, local_size_z=1) in;

layout (push_constant) uniform Constants {
   Vig_HistParams p;
};

layout (binding = 0 ) readonly buffer A {
   uint8_t a[];
};

layout( binding = 1 ) writeonly buffer Dest {
   uint dest[];
};

uint sum( uint idx, uint n, uint step ) {
   uint ret = 0;
   for (uint i=idx; 0<n; --n ) {
      ret += uint( a[i] );
      i += step;
   }
   return ret;
}

void main() {
   uint x = gl_GlobalInvocationID.x;
   uint idx = x*p.mul+p.rem;
   if ( x < p.img.width )
      dest[x] = sum( idx, p.img.height, p.img.stride );
   idx = x*p.img.stride+p.rem;
   if ( x < p.img.height )
      dest[x+p.img.width] = sum( idx, p.img.width, p.mul );
}
   
