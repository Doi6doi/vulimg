#version 450

#include "vulimg_comp.h"

#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types: require

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   Vig_DiffParams p;
};

layout (binding = 0 ) readonly buffer A {
   uint8_t a[];
};

layout (binding = 1 ) readonly buffer B {
   uint8_t b[];
};

layout( binding = 2 ) writeonly buffer Dest {
   uint8_t dest[];
};

uint8_t calc( uint8_t aa, uint8_t bb, bool alp ) {
   if ( aa < bb )
      return alp ? bb : bb-aa;
      else return alp ? aa : aa-bb;
}
         
void main() {
   uint y = gl_GlobalInvocationID.y;
   if ( p.img.height <= y ) return;
   uint x = gl_GlobalInvocationID.x;
   if ( p.img.width <= x ) return;
   uint i = y * p.img.stride + x;
   dest[i] = calc( a[i], b[i], p.alpha == x % 4 );
}
   
