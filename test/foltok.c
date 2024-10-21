#include <vulcmp.h>
#include <vulimg.h>
#include <vultools.h>
#include <stdio.h>

#define FOLTOKBMP "foltok.bmp"
#define FOLTOK1BMP "foltok1.bmp"
#define FOLTOK2BMP "foltok2.bmp"
#define NRECTS 20

int main() {
   VcpVulcomp v = vcp_init( "foltok", VCP_VALIDATION );
   vig_init( v );
   vig_check_fail();
vtl_ewrite("read bmp");
   FILE * f = fopen( FOLTOKBMP,"rb");
   VigImage img = vig_bmp_read( f, vtl_fread );
   fclose(f);

   f = fopen( FOLTOK1BMP,"wb");
   vig_bmp_write( img, f, vtl_fwrite );
   fclose(f);


   vig_check_fail();
vtl_ewrite("apply foltok");
   struct VtlRect rr[NRECTS];
   uint32_t n = NRECTS;
   vig_white_rects( img, 0.5, 0.2, 0, 10, rr, &n );
   vig_check_fail();
/*   for (int i=0; i<n; ++i)
      vig_draw_rect( img, rr+i, 0xff );
*/
vtl_ewrite("write bmp");
   f = fopen( FOLTOK2BMP,"wb");
   vig_bmp_write( img, f, vtl_fwrite );
   fclose(f);
   vig_check_fail();
   vig_done();
   vcp_done( v );
   return 0;
}
