#include <vulcmp.h>
#include <vulimg.h>
#include <stdio.h>

#define SMILEYBMP "smiley.bmp"
#define SMILEY2BMP "smiley2.bmp"
#define SMILEYRAW "smiley.raw"

int main() {
   VcpVulcomp v = vcp_init( "basic", VCP_VALIDATION );
   vig_init( v );
   FILE * f = fopen( SMILEYBMP,"rb");
   VigImage i1 = vig_bmp_read( f, vtl_fread );
   fclose(f);
   f = fopen( SMILEYRAW,"wb");
   vig_raw_write( i1, f, vtl_fwrite, false );
   fclose(f);
   VigImage i2 = vig_image_create( vig_image_width(i1),
      vig_image_height(i1), vig_image_pixel(i1) );
   f = fopen( SMILEYRAW,"rb");
   vig_raw_read( i2, f, vtl_fread, false );
   fclose(f);
   f = fopen( SMILEY2BMP,"wb");
   vig_bmp_write( i2, f, vtl_fwrite );
   fclose(f);
   vig_done();
   vcp_done( v );
   return 0;
}
