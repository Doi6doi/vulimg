#include <frei0r.h>
#include <vulcmp.h>
#include <vulimg.h>
#include <stdint.h>
#include <math.h>

static VcpVulcomp vul = NULL;

typedef enum Param { COUNT, LIMIT } Param;

typedef struct Deshake {
   uint32_t count;
   uint32_t dcount;
   float limit;
   VigImage curr;
   VigImage prev;
   VigImage out;
   VigImage pyr;
   VigImage prevpyr;
   VytIVec2 ds;
   struct Vyt_IVec2 dc;
} * Deshake;
 
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
   i->name = "Deshake";
   i->author = "Várnagy Zoltán";
   i->plugin_type = F0R_PLUGIN_TYPE_FILTER;
   i->color_model = F0R_COLOR_MODEL_RGBA8888;
   i->frei0r_version = FREI0R_MAJOR_VERSION;
   i->major_version = 1;
   i->minor_version = 0;
   i->num_params = 2;
   i->explanation = "Deshake filter";
}

void f0r_get_param_info( f0r_param_info_t * p, int i ) {
   switch (i) {
      case COUNT:
         p->name = "count";
         p->type = F0R_PARAM_DOUBLE;
         p->explanation = "Number of frames to store";
      break;
      case LIMIT:
         p->name = "limit";
         p->type = F0R_PARAM_DOUBLE;
         p->explanation = "Limit of difference between frames";
      break;
   }
}

void * f0r_construct( unsigned int width, unsigned int height ) {
   vyt_frame(true);
   Deshake ret = vyt_alloc( NULL, sizeof( struct Deshake ));
   ret->count = 0;
   ret->dcount = 0;
   VigPixel px = vix_rgba32;
   ret->curr = vig_image_create( width, height, px );
   ret->prev = vig_image_create( width, height, px );
   ret->out = vig_image_create( width, height, px );
   ret->pyr = vig_image_create( width/2, height, px );
   ret->prevpyr = vig_image_create( width/2, height, px );
   vig_check_fail();
   double count = 5;
   double limit = 0.2;
   f0r_set_param_value( ret, &count, COUNT );
   f0r_set_param_value( ret, &limit, LIMIT );
   return ret;
}

void f0r_destruct( void * instance ) {
   double zero = 0;
   f0r_set_param_value( instance, &zero, COUNT );
   Deshake d = (Deshake)instance;
   vig_image_free( d->prevpyr );
   vig_image_free( d->pyr );
   vig_image_free( d->prev );
   vig_image_free( d->out );
   vig_image_free( d->curr );
   vyt_frame( false );
}

static void set_count( Deshake d, double v ) {
   uint32_t n = round(v);
   if ( n == d->count ) return;
   d->ds = vyt_alloc( d->ds, n*sizeof( struct Vyt_IVec2 ) );
   d->count = n;
}   

void f0r_set_param_value( f0r_instance_t instance, 
   f0r_param_t p, int i )
{
   Deshake d = (Deshake)instance;
   switch ( i ) {
      case COUNT: set_count( d, *(double *)p ); break;
      case LIMIT: d->limit = *(double *)p; break;
   }
}

void f0r_get_param_value( f0r_instance_t instance,
   f0r_param_t p, int i )
{
   Deshake d = (Deshake)instance;
   switch ( i ) {
      case COUNT: *(double *)p = d->count; break;
      case LIMIT: *(double *)p = d->limit; break;
   }
}   

/// swap two images
static void swap( VigImage * a, VigImage * b ) {
   VigImage save = *a;
   *a = *b;
   *b = save;
}

/// shift image and pyramid handles
static void shift( Deshake d ) {
   uint32_t n = d->count;
   if ( 0 == n ) return;
   swap( &d->out, &d->prev );
   swap( &d->pyr, &d->prevpyr );
   if ( d->dcount < d->count )
      ++ d->dcount;
   for (int i=d->dcount-1; 0<i; --i)
      d->ds[i] = d->ds[i-1];
}

/// regressziós egyenes
static void regress( Deshake d ) {
}

static void compose( Deshake d ) {
   int m = d->dcount;
   if ( 1 == m ) {
      vig_image_copy( d->curr, d->out );
      return;
   } else if ( 2 == m ) {
      d->ds->x = d->dc.x / 2 ;
      d->ds->y = d->dc.y / 2;
   } else {
      regress( d );
   }
   VytU ax = abs( d->ds->x - d->dc.x );
   VytU ay = abs( d->ds->y - d->dc.y );
   VytU bx = abs( d->dc.x );
   VytU by = abs( d->dc.y );
   int w = vig_image_width( d->out );
   int h = vig_image_height( d->out );
   struct Vyt_URect r = { .left=0, .top=0, .width=w-ax, .height=h-ay };
   struct Vyt_UVec2 u = { .x=ax, .y=ay };
   struct Vyt_URect s = { .left = bx, .top = by, .width = ax, .height = h-by };
   struct Vyt_UVec2 v = { .x=0, .y=0 };
   if ( d->ds->x < d->dc.x ) {
      r.left = ax;
      u.x = 0;
      s.left = w-ax;
      v.x = w-ax-bx;
   }
   if ( d->ds->y < d->dc.y ) {
      r.top = ay;
      u.y = 0;
      s.top = 0;
      v.y = by;
   }
   vig_part_copy( d->curr, &r, d->out, &u );
   vig_part_copy( d->prev, &s, d->out, &v );
   s.width = w-bx;
   s.height = ax;
   vig_part_copy( d->prev, &s, d->out, &v );
}


void f0r_update( void * instance, double time, 
   const unsigned int * input, unsigned int * output 
) {
   Deshake d = (Deshake)instance;
   shift(d);
   vig_raw_read( d->curr, &input, vyt_mread, false );
   vig_pyr_create( d->curr, d->pyr );
   vig_pyr_delta( d->prev, d->curr, d->prevpyr, d->pyr,
      d->limit, &d->dc.x, &d->dc.y );
   if ( VIG_MUCH == d->dc.x )
      d->dcount = 1;
   compose(d);
   vig_raw_write( d->out, &output, vyt_mwrite, false );
}
   
   
   
   



