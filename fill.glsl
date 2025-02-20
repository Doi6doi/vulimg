#version 450

#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   Vig_RectParams p;
};

layout (binding = 0 ) buffer Data {
   uint data[];
};

uint i, x, y, ps;
uint v;
int m;

void rotate() {
   if ( 0 == m ) return;
   uint q = bitfieldExtract( v, 0, m );
   v >>= m;
   v = bitfieldInsert( v, q, int(p.pixSize)-m, m );
}
         
void main() {
   y = gl_GlobalInvocationID.y;
   if ( p.height <= y ) return;
   x = gl_GlobalInvocationID.x;
   ps = p.pixSize;
   int m = int(p.left * ps) % 32;
   int rest = int( m+p.width*ps )-int(x*32);
   if ( 0 >= rest ) return;
   uint i = (y+p.top)*p.img.stride + (p.left*ps)/32 + x;
   int l;
   v = p.pixVal;
   rotate();
   if ( 0 == x ) {
      // bal pixel
      l = min( rest, 32-m );
      v = bitfieldInsert( data[i], v, m, l );
   } else if ( m+rest < 32 ) {
      // jobb pixel
      l = min( 32, m+rest );
      v = bitfieldInsert( data[i], v, 0, l );
   } else {
      // középső pixel
   }
   data[i] = v;
}
   


