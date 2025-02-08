#include "frameproc.h"
#include <stdio.h>

static VcpVulcomp vfp_vc = NULL;
static uint32_t vfp_width = 0;
static uint32_t vfp_height = 0;
static VigPixel vfp_pixel = vix_Unknown;

void vfp_init( VcpStr name, uint32_t flags, FrameData d, FrameProc p, int argc, 
   char ** argv ) 
{
   int at = 1;
   while ( p->arg( d, argc, argv, & at ) )
      ;
   if ( ! vfp_width )
      vtl_die( "Image width missing (-w)" );
   if ( ! vfp_height )
      vtl_die( "Image height missing (-h)" );
   if ( ! vfp_pixel )
      vtl_die( "Pixel type missing (-p)" );
   if ( ! ( vfp_vc = vcp_init( name, flags )))
      vcp_check_fail();
   if ( ! vig_init( vfp_vc ))
      vig_check_fail();
   VigImage * frame = p->frame( d );
   if ( ! (*frame = vig_image_create( vfp_width, vfp_height, vfp_pixel )))
      vig_check_fail();
}
   
void vfp_done( FrameData fd, FrameProc fp ) {
   vig_image_free( *fp->frame( fd ) );
   vig_done();
   vcp_done( vfp_vc );
}

void vfp_process( FrameData fd, FrameProc fp ) {
   VigImage out;
   VigImage * img = fp->frame(fd);
   while ( vig_raw_read( *img, stdin, vtl_fread, false )) {
      if ( out = fp->next( fd ) )
         vig_raw_write( out, stdout, vtl_fwrite, false );
   }
}

   
void vfp_flip( VigImage * a, VigImage * b ) {
   VigImage c = *a;
   *a = *b;
   *b = c;
}

/// pixel argumentum olvasás
bool vfp_pixel_arg( int argc, char ** argv, int * at, VigPixel * ret ) {
   if ( argc <= *at )
      vtl_die( "Missing pixel argument");
   VcpStr s = argv[(*at)++];
   if ( vtl_same( s, "1" ))
      *ret = vix_1;
   else if ( vtl_same( s, "8" ))
      *ret = vix_8;
   else if ( vtl_same( s, "g8" ))
      *ret = vix_g8;
   else if ( vtl_same( s, "rgb24" ))
      *ret = vix_rgb24;
   else if ( vtl_same( s, "rgba32" ))
      *ret = vix_rgba32;
   else if ( vtl_same( s, "ybr24" ))
      *ret = vix_ybr24;
   else
      vtl_die( vtl_cat( "Unknown pixel argument (1,8,g8,rgb25,rgba32,ybr24): ",s ));
   return true;
}
   

// egész argumentum olvasás
bool vfp_nat_arg( int argc, char ** argv, int * at, uint32_t * ret ) {
   if ( argc <= *at )
      vtl_die( "Missing number argument");
   VcpStr s = argv[(*at)++];
   if ( ! vtl_nat( s, ret ))
      vtl_die( vtl_cat("Not a number argument: ", s));
   return true;
}
   

bool vfp_arg( FrameData fd, int argc, char ** argv, int * at ) {
   if ( argc <= *at ) return false;
   VcpStr s = argv[(*at)++];
   if ( vtl_same( s, "-w" ))
      return vfp_nat_arg( argc, argv, at, & vfp_width );
   if ( vtl_same( s, "-h" ))
      return vfp_nat_arg( argc, argv, at, & vfp_height );
   if ( vtl_same( s, "-p" ))
      return vfp_pixel_arg( argc, argv, at, & vfp_pixel );
   vtl_die( vtl_cat( "Unknown argument: ", s ));
}
