#include <vulimg.h>
#include <stdio.h>

VigImage pyr_create( VigImage img ) {
   VigImage ret = vig_image_create(
      vig_image_width(img)/2, vig_image_height(img),
      vig_image_pixel(img ));
   bool b = vig_pyr_create( img, ret );
   vig_check_fail();
   return ret;
}

VigImage load( VcpStr fname ) {
   FILE * f = fopen( fname, "r" );
   VigImage ret = vig_bmp_read( f, vtl_fread );
   vig_check_fail();
   fclose(f);
   return ret;
}

void save( VigImage img, VcpStr fname ) {
   FILE * f = fopen( fname, "w" );
   vig_bmp_write( img,f, vtl_fwrite );
   vig_check_fail();
   fclose(f);
}

int main() {
   VcpVulcomp v = vcp_init( "delt", VCP_VALIDATION );
//   vcp_select_physical( v, vtl_physical_cpu );
   vig_init( v );
   VigImage i1 = load( "s1.bmp" );
   VigImage i2 = load( "s2.bmp" );
   VigImage p1 = pyr_create( i1 );
   VigImage p2 = pyr_create( i2 );
   save(p1, "p1.bmp");
   save(p2, "p2.bmp");
   int32_t dx, dy;
   if ( ! vig_pyr_delta( i1, i2, p1, p2, 0.2, &dx, &dy ))
      vtl_die("Could not delta");
   vtl_ewrite( "dx:%d dy:%d", dx, dy );
   vig_done(v);
   vcp_done( v );
   return 0;
}

