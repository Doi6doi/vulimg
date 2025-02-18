#version 450

#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=1, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigRectParams p;
};

layout (binding = 0 ) buffer Data {
   uint data[];
};

uint i;
int ps;
uint right, bottom;

// vízszintes vonalak
void horz( uint y ) {
   if ( y < 0 || p.img.height <= y ) return;
   uint ix = p.left*ps/32 + i;
   if ( ix < 0 || p.img.stride <= ix ) return;
   uint j = ix*32/ps;
   uint idx = y*p.img.stride + ix;
   uint v = data[ idx ];
   for (int at=0; at<32; at += ps ) {
      if ( p.left <= j && j <= right ) 
         v = bitfieldInsert( v, p.pixVal, at, ps );
      ++j;
   }
   data[idx] = v;
}
   
// függőleges vonalak
void vert( uint x ) {
   if ( 0 == i || p.height -1 <= i ) return;
   if ( x < 0 || p.img.width <= x ) return;
   uint y = p.top + i;
   if ( y < 0 || p.img.height <= y ) return;
   uint idx = y*p.img.stride + x*p.pixSize/32;
   int at = int( x*ps % 32 );
   data[idx] = bitfieldInsert( data[idx], p.pixVal, at, ps );
   at += ps;
   if ( 32 < at ) {
      at -= 32;
      int done = ps-at;
      ++idx;
      data[idx] = bitfieldInsert( data[idx], p.pixVal >> done, 0, at );
   }
} 
   
         
void main() {
   i = gl_GlobalInvocationID.x;
   ps = int( p.pixSize );
   right = p.left+p.width-1;
   bottom = p.top+p.height-1;
   horz( p.top );
   horz( bottom );
   vert( p.left );
   vert( right );
}
   


