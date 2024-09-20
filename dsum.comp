#version 450
#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=1, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigDSumParams p;
};

layout (binding = 0 ) readonly buffer A {
   uint a[];
};

layout (binding = 1 ) readonly buffer B {
   uint b[];
};

layout( binding = 2 ) writeonly buffer Dest {
   uint dest[];
};

/// egy kép pixeleinek összegzése
void uasum(uint x) {
   uint ret = 0;
   for ( uint y=0; y < p.height; ++y ) {
      int ia = int(x+p.aleft);
      uint va = a[ (y+p.atop)*p.astride + ia*4 ];
      ret += bitfieldExtract( va, 8*(ia%4), 8 );
   }
   dest[x] = ret;
}

/// egy kép előjeles értékek összegzése
void sasum(uint x) {
   int ret = 0;
   for ( uint y=0; y < p.height; ++y ) {
      int ia = int(x+p.aleft);
      int va = int( a[ (y+p.atop)*p.astride + ia*4 ] );
      ret += bitfieldExtract( va, 8*(ia%4), 8 );
   }
   dest[x] = uint( ret );
}

void uabsum(uint x) {
   uint ret = 0;
   for ( uint y=0; y < p.height; ++y ) {
      int ia = int(x+p.aleft);
      uint va = a[ (y+p.atop)*p.astride + ia*4 ];
      va = bitfieldExtract( va, 8*(ia%4), 8 );
      int ib = int(x+p.bleft);
      uint vb = b[ (y+p.btop)*p.bstride + ib*4 ];
      vb = bitfieldExtract( vb, 8*(ib%4), 8 );
      if ( va <= vb )
         ret += (vb-va);
         else ret += (va-vb);
   }
   dest[x] = ret;
}

void sabsum(uint x) {
   uint ret = 0;
   for ( uint y=0; y < p.height; ++y ) {
      int ia = int(x+p.aleft);
      int va = int( a[ (y+p.atop)*p.astride + ia*4 ] );
      va = bitfieldExtract( va, 8*(ia%4), 8 );
      int ib = int(x+p.bleft);
      int vb = int( b[ (y+p.btop)*p.bstride + ib*4 ] );
      vb = bitfieldExtract( vb, 8*(ib%4), 8 );
      if ( va <= vb )
         ret += uint(vb-va);
         else ret += uint(va-vb);
   }
   dest[x] = ret;
}

void main() {
   uint x = gl_GlobalInvocationID.x;
   if ( p.width <= x ) return;
   switch ( p.mode ) {
      case 1: uasum(x);
      case 2: sasum(x);
      case 3: uabsum(x);
      case 4: sabsum(x);
   }
}
   
