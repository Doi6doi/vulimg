#include <frei0r.h>
#include <vulcmp.h>
#include <vulimg.h>
#include <vytools.h>
#include <stdint.h>
#include <stdlib.h>

static VcpVulcomp vul = NULL;

typedef struct Fre {
   VigImage curr;
   VigImage prev;
   VigImage out;
} * Fre;


int f0r_init() {
   vul = vcp_init("f0r_deshake", VCP_VALIDATION | VCP_8BIT );
   vcp_check_fail();
   vig_init( vul );
   vig_check_fail();
   return 0; 
}

void f0r_deinit() {
   vig_done();
   vcp_done( vul );
}

void f0r_get_plugin_info( f0r_plugin_info_t * i ) {
   i->name = "Fre";
   i->author = "Várnagy Zoltán";
   i->plugin_type = F0R_PLUGIN_TYPE_FILTER;
   i->color_model = F0R_COLOR_MODEL_RGBA8888;
   i->frei0r_version = FREI0R_MAJOR_VERSION;
   i->major_version = 1;
   i->minor_version = 0;
   i->num_params = 0;
   i->explanation = "Fre filter";
}

void f0r_get_param_info( f0r_param_info_t * p, int i ) {
}

void f0r_get_param_value( f0r_instance_t instance,
   f0r_param_t p, int i )
{
}

void f0r_set_param_value( f0r_instance_t instance, 
   f0r_param_t p, int i )
{
}

void * f0r_construct( unsigned int width, unsigned int height ) {
   Fre ret = realloc( NULL, sizeof( struct Fre ));
   ret->curr = vig_image_create( width, height, vix_rgba32 );
   ret->prev = vig_image_create( width, height, vix_rgba32 );
   ret->out = vig_image_create( width, height, vix_rgba32 );
   return ret;
}

void f0r_destruct( void * instance ) {
   Fre fre = (Fre)instance;
   vig_image_free( fre->curr );
   vig_image_free( fre->prev );
   vig_image_free( fre->out );
   fre = realloc( fre, 0 );
}

void swap( VigImage * a, VigImage * b ) {
   VigImage save = *a;
   *a = *b;
   *b = save;
}

void f0r_update( void * instance, double time, 
   const unsigned int * input, unsigned int * output 
) {
   Fre fre = (Fre)instance;
   swap( &fre->curr, &fre->prev );
   vig_raw_read( fre->curr, &input, vyt_mread, false );
   vig_image_diff( fre->prev, fre->curr, fre->out );
   vig_raw_write( fre->out, &output, vyt_mwrite, false );
}
   
   
int main() {
   return 0;
}   



