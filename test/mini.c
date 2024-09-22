#include "vulimg.h"
#include <stdio.h>

int main() {
// initialize GPU
VcpVulcomp v = vcp_init( "vigtest", VCP_VALIDATION );
// initialize vulimg
vig_init( v );
// load bmp image
FILE * fh = fopen( "test.bmp", "rb" );
VigImage i1 = vig_bmp_read( fh, vtl_fread );
fclose( fh );
// create other image
VigImage i2 = vig_image_create( vig_image_width(i1),
   vig_image_height(i1), vig_image_pixel(i1) );
// rotate image with 30 degrees
struct VigTransform tr = {
   .sx=0.866, .rx=-0.5,  .dx=0,
   .ry=0.5,   .sy=0.866, .dy=0
};
vig_image_transform( i1, i2, &tr );
// write as bmp
FILE * gh = fopen("test2.bmp", "wb" );
vig_bmp_write( i2, gh, vtl_fwrite );
// finalize
vig_done();
vcp_done(v);
}
