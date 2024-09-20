#version 450

#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants { 
   VigCopyParams p;
};

layout (binding = 0 ) readonly buffer Source {
   uint source[];
};

layout( binding = 1 ) writeonly buffer Dest {
   uint dest[];
};

void main() {
   uint y = gl_GlobalInvocationID.y;
   if ( p.height <= y ) return;
   uint x = gl_GlobalInvocationID.x;
   if ( p.width <= x ) return;
   dest[ (y+p.dtop)*p.dst.stride + (x+p.dleft) ]
      = source[ (y+p.stop)*p.src.stride + (x+p.sleft) ];
}
