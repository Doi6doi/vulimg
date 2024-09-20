#version 450

#include "vulimg_comp.h"

layout (local_size_x=UGR, local_size_y=UGR, local_size_z=1) in;

layout (push_constant) uniform Constants {
   VigTransParams p;
};

layout (binding = 0 ) readonly buffer Source {
   uint source[];
};

layout( binding = 1 ) writeonly buffer Dest {
   uint dest[];
};

float getComp( int y, int x, int iComp ) {
   if ( x < 0 || p.src.width <= x ) return 0.0;
   int xComp = x * p.compCount + iComp;
   int xBit = xComp * p.compBits;
   uint v = source[ y*p.src.stride + xBit / 32 ];
   int s = xBit % 32;
   bitfieldExtract( v,0, p.compBits );
   return float( bitfieldExtract( v, 32-s-p.compBits, p.compBits ));
}

float calcCompRow( int y, float x, int iComp ) {
   if ( y < 0 || p.src.height <= y ) return 0.0;
   float mx = p.trans.sx + p.trans.rx;
   int x1, n;
   if ( -1 > mx ) {
      x1 = int( x + mx );
      n = int(-mx);
   } else if ( 1 < mx ) {
      x1 = int( x );
      n = int(mx);
   } else {
      float f = fract( x );
      float ret = getComp( y, int(x), iComp );
      if ( 0 != f )
         ret = (1-f)*ret + f*getComp( y, int(x)+1, iComp );
      return ret;
   }
   float ret = 0;
   for (int i=0; i<n; ++i)
      ret += getComp( y, x1+i, iComp );
   if ( 1 == p.compBits )
      ret += n/2;
   return ret / n;
}

float calcComp( uint y, uint x, int iComp ) {
  float dx = p.trans.sx * x + p.trans.rx * y + p.trans.dx;
  float dy = p.trans.ry * x + p.trans.sy * y + p.trans.dy;
  float my = p.trans.ry + p.trans.sy;
  int y1, n;
  if ( -1 > my ) {
     y1 = int( dy+my );
     n = int(-my);
  } else if ( 1 < my ) {
     y1 = int( dy );
     n = int(my);
  } else {
     float f = fract( dy );
     float ret = calcCompRow( int(dy), dx, iComp );
     if ( 0 != f )
        ret = (1-f)*ret + f*calcCompRow( int(dy)+1, dx, iComp );
     return ret;
  }
  float ret = 0;
  for (int i=0; i<n; ++i)
     ret += calcCompRow( y1+i, dx, iComp );
  if ( 0 == p.compBits )
     ret += n/2;
  return ret / n;
}
         
void main() {
   uint y = gl_GlobalInvocationID.y;
   if ( p.dst.height <= y ) return;
   uint x = gl_GlobalInvocationID.x;
   int xComp = 32*int(x) / p.compBits;
   int xPix = xComp / p.compCount;
   if ( p.dst.width <= xPix ) return;
   int iComp = xComp % p.compCount;
   uint ret = 0;
   for (int i=0; i<32; i += p.compBits) {
      uint v = uint( calcComp( y, xPix, iComp ) );
      ret = bitfieldInsert( ret, v, 32-i-p.compBits, p.compBits );
      if ( ++iComp == p.compCount ) {
         ++xPix;
         iComp = 0;
      }
   }
   dest[ y * p.dst.stride + x ] = ret;
}
   
