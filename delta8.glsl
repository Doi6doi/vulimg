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
   while (0 != n) {
      inca();
      --n;
   }
}

/// b mutató növelése n-szer
void incbs(uint n) {
   while (0 != n) {
      incb();
      --n;
   }
}

void main() {
   uint y = gl_GlobalInvocationID.x;
   if (p.img.height <= y) return;
   uint x = gl_GlobalInvocationID.x+1;
   dest[ y*10 + x ] = 0;
   switch (x) {
      case 1: case 2: case 3: case 4: case 5: case 6:
         if (p.img.height == y+1) return; 
      break;
      case 7: case 8: case 9: if (0 == y) return;
   }
   uint c = p.comps;
   uint s = p.img.stride;
   uint n = (p.img.width-1) * c;
   ai = bi = (y+p.top)*s;
   ab = bb = 0;
   av = a[ai];
   bv = b[bi];
   switch (x) {
      case 1: incas(c); bi += s; break;
      case 2: bi += s; break;
      case 3: bi += s; incbs(c); break;
      case 4: incas(c); break;
      case 5: break;
      case 6: incbs(c); break;
      case 7: incas(c); bi -= s; break;
      case 8: bi -= s; break;
      case 9: incbs(c); bi -= s; break; 
   }
   uint sum = 0;
   while (0!=n) {
      uint aw = bitfieldExtract( av, ab, 8 );
      uint bw = bitfieldExtract( bv, bb, 8 );
      sum += abs( int(bw)-int(aw) );
      inca();
      incb();
      --n;
   }
   dest[ y*10 + x ] = sum;
}
   
