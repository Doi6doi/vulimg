#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>

#define SMILEYBMP "smiley.bmp"
#define SMILEY2BMP "smiley2.bmp"
#define SMILEYRAW "smiley.raw"

void ewrite( VcpStr msg ) {
   vtl_ewrite( msg );
}

int main() {
   VcpVulcomp v = vcp_init( "basic", VCP_VALIDATION );
   vig_init( v );
   vig_check_fail();
ewrite("read bmp");
   FILE * f = fopen( SMILEYBMP,"rb");
   VigImage i1 = vig_bmp_read( f, vtl_fread );
   fclose(f);
   vig_check_fail();
/*ewrite("write raw");
   f = fopen( SMILEYRAW,"wb");
   vig_raw_write( i1, f, vtl_fwrite, false );
   fclose(f);
   vig_check_fail();
ewrite("read raw");
*/
   VigImage i2 = vig_image_create( vig_image_width(i1),
      vig_image_height(i1), vig_image_pixel(i1) );
/*
   f = fopen( SMILEYRAW,"rb");
   vig_raw_read( i2, f, vtl_fread, false );
   fclose(f);
   vig_check_fail();
*/
ewrite("transform");
   struct VigTransform t = { 
//      .sx=1,      .rx=0, .dx=10,
//      .ry=0,      .sy=1, .dy=10 
      .sx=0.3,      .rx=0, .dx=0,
      .ry=0,      .sy=0.3, .dy=0 
//      .sx=0.7071,      .rx=0.7071, .dx=-60,
//      .ry=-0.7071,      .sy=0.7071, .dy=100 
   };
   vig_image_transform( i1, i2, & t );
   vig_check_fail();
ewrite("write bmp");
   f = fopen( SMILEY2BMP,"wb");
   vig_bmp_write( i2, f, vtl_fwrite );
   fclose(f);
   vig_check_fail();
   vig_done();
   vcp_done( v );
   return 0;
}
