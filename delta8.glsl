#version 450
#include "vulimg_comp.h"

layout (local_size_x=9, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigDeltaParams p;
};

layout (binding = 0 ) readonly buffer A {
   uint a[];
};

layout (binding = 1 ) readonly buffer B {
   uint b[];
};

layout (binding = 1 ) writeonly buffer Dest {
   uint dest[];
};

uint d, s;
uint ai, bi, av, bv;
int ab, bb;

/// a mutató növelése
void inca() {
   if (24 == ab) {
      ++ai;
      av = a[ai];
      ab = 0;
   } else {
      ab += 8;
   }
}

/// b mutató növelése
void incb() {
   if (24 == bb) {
      ++bi;
      bv = b[bi];
      bb = 0;
   } else {
      bb += 8;
   }
}

/// a mutató növelése n-szer
void incas(uint n) {
   if ( 4 <= n ) {
      ai += n/4;
      n = n & 3;
   }
   while (0 != n) {
      inca();
      --n;
   }
}

/// b mutató növelése n-szer
void incbs(uint n) {
   if ( 4 <= n ) {
      bi += n/4;
      n = n & 3;
   }
   while (0 != n) {
      incb();
      --n;
   }
}

void usedx() {
   int dx = p.dx;
   switch (d) {
      case 1: case 4: case 7: --dx;
      case 3: case 6: case 9: ++dx;
   }
   if ( 0 < dx )
      incbs( uint( dx*p.comps ) );
   else if ( 0 > dx )
      incas( uint( -dx*p.comps ) );
}
   
void usedy() {
   int dy = p.dy;
   switch (d) {
      case 1: case 2: case 3: ++dy;
      case 7: case 8: case 9: --dy;
   }
   if ( 0 < dy )
      bi += uint( dy*s );
   else if ( 0 > dy )
      ai += uint( -dy*s );
}
   

void main() {
   uint y = gl_GlobalInvocationID.x;
   uint h = p.img.height - abs(p.dy) - 1;
   if ( p.img.height <= y ) return;
   d = gl_GlobalInvocationID.x + 1;
   if ( h <= y ) {
      dest[ y*10 + d ] = 0;
      return;
   }
   s = p.img.stride;
   uint n = p.comps*(p.img.width - abs(p.dx) - 1);
   ai = bi = (y+p.top)*s;
   ab = bb = 0;
   usedx();
   usedy();
   av = a[ai];
   bv = b[bi];
   uint sum = 0;
   while (0!=n) {
      uint aw = bitfieldExtract( av, ab, 8 );
      uint bw = bitfieldExtract( bv, bb, 8 );
      sum += abs( int(bw)-int(aw) );
      inca();
      incb();
      --n;
   }
   dest[ y*10 + d ] = sum;
}
   
