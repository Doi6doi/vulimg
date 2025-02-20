#include <vulimg.hpp>

using namespace vig;

#define SMILEYBMP "smiley3.bmp"
#define SMILEY2BMP "smiley2.bmp"
#define SMILEYRAW "smiley.raw"
 
int main() {
   Vulimg v;
   Image i1 = Bmp::read( InFile(SMILEYBMP) );
   i1.write( OutFile(SMILEYRAW), false );
   Image i2( i1, false );
   Image i3( i1, false );
   FTrans2 t( 0.7071, 0.7071, 0.7071, 0.7071, -60, 100 );
   Draw::transform( i1, t, i2 );
   Draw::transform( i2, t, i3 );
   URect r(0,0,80,120);
   Draw::copy( i3, r, i3, UVec2(91,91) );
   Bmp::write( i3, OutFile(SMILEY2BMP) );
   return 0;
}
