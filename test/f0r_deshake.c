#include <frei0r.h>
#include <frei0r.h>
#include <vulcmp.h>
#include <vulimg.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static VcpVulcomp vul = NULL;

typedef enum Param { INERT, LIMIT } Param;

typedef enum Part { LARGE, VERT, HORZ } Part;

#define DIFFMASK 0xff

typedef struct Deshake {
   float limit;
   float inert;
   struct Vyt_UVec2 dim;
   VigImage curr;
   VigImage prev;
   VigImage out;
   VigHist chist;
   VigHist phist;
   struct Vyt_IVec2 v;
   struct Vyt_IVec2 d;
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
      case INERT:
         p->name = "inert";
         p->type = F0R_PARAM_DOUBLE;
         p->explanation = "Inertia of camera";
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
   VigPixel px = vix_rgba32;
   ret->limit = 0.1;
   ret->inert = 0.9;
   ret->dim.x = width;
   ret->dim.y = height;
   ret->curr = vig_image_create( width, height, px );
   ret->prev = vig_image_create( width, height, px );
   ret->out = vig_image_create( width, height, px );
   vig_check_fail();
   VytU hsz = 2*(width+height)*sizeof(VytF);
   ret->chist = vyt_alloc( NULL, hsz );
   ret->phist = vyt_alloc( NULL, hsz );
   memset( ret->chist, hsz, 0 );
   memset( ret->phist, hsz, 0 );
   ret->v.x = ret->v.y = ret->d.x = ret->d.y = 0;
   return ret;
}

void f0r_destruct( void * instance ) {
   double zero = 0;
   Deshake d = (Deshake)instance;
   vig_image_free( d->prev );
   vig_image_free( d->out );
   vig_image_free( d->curr );
   vyt_frame( false );
}

void f0r_set_param_value( f0r_instance_t instance, 
   f0r_param_t p, int i )
{
   Deshake d = (Deshake)instance;
   switch ( i ) {
      case INERT: d->inert = *(double *)p; break;
      case LIMIT: d->limit = *(double *)p; break;
   }
}

void f0r_get_param_value( f0r_instance_t instance,
   f0r_param_t p, int i )
{
   Deshake d = (Deshake)instance;
   switch ( i ) {
      case INERT: *(double *)p = d->inert; break;
      case LIMIT: *(double *)p = d->limit; break;
   }
}   

/// swap two images
static void swapImg( VigImage * a, VigImage * b ) {
   VigImage save = *a;
   *a = *b;
   *b = save;
}

/// swap two hist 
static void swapHist( VigHist * a, VigHist * b ) {
   VigHist save = *a;
   *a = *b;
   *b = save;
}

void histCopy( Deshake d, Part part, VytURect r, VytUVec2 u ) {
   VytU sf = sizeof(VytF);
   VytU w = d->dim.x;
   switch ( part ) {
      case LARGE:
         memcpy( d->phist + u->x, d->chist + r->left, r->width*sf );
         memcpy( d->phist + 2*w + u->y, d->chist + 2*w + r->top, r->height*sf );
      break;
      case VERT:
         memmove( d->phist + u->x, d->phist + r->left, r->width * sf ); 
      break;
      case HORZ:
         memmove( d->phist + 2*w + u->y, d->phist + 2*w + r->top, r->height*sf );
      break;
   }
}

static void histExtend( VigHist p, VytU n ) {
   if ( 1 >= n ) return;
   for (int i=n/2-1; 0<=i; --i)
      p[n+i] = (p[2*i]+p[2*i+1])/2;
   histExtend( p+n, n/2 );
}

static void histExtends( Deshake d, VigHist p ) {
   VytU w = d->dim.x;
   histExtend( p, w );
   histExtend( p+2*w, d->dim.y );
}   

/// összeállítás az előző képből és az újból
static void compose( Deshake d ) {
   float t = d->inert;
   d->v.x = t*d->v.x + (1-t)*d->d.x;
   d->v.y = t*d->v.y + (1-t)*d->d.y; 
   VytU ax = abs( d->d.x - d->v.x );
   VytU ay = abs( d->d.y - d->v.y );
   VytU bx = abs( d->v.x );
   VytU by = abs( d->v.y );
   int w = d->dim.x;
   int h = d->dim.y;
   struct Vyt_URect r = { .left=0, .top=0, .width=w-ax, .height=h-ay };
   struct Vyt_UVec2 u = { .x=ax, .y=ay };
   struct Vyt_URect s = { .left = bx, .top = by, .width = ax, .height = h-by };
   struct Vyt_UVec2 v = { .x=0, .y=0 };
   if ( d->d.x > d->v.x ) {
      r.left = ax;
      u.x = 0;
      s.left = w-ax;
      v.x = w-ax-bx;
   }
   if ( d->d.y > d->v.y ) {
      r.top = ay;
      u.y = 0;
      s.top = 0;
      v.y = by;
   }
   vig_part_copy( d->curr, &r, d->out, &u );
   vig_part_copy( d->prev, &s, d->out, &v );
   histCopy( d, VERT, &s, &v );
   s.width = w-bx;
   s.height = ax;
   vig_part_copy( d->prev, &s, d->out, &v );
   histCopy( d, HORZ, &s, &v );
   histCopy( d, LARGE, &r, &u );
   histExtends( d, d->phist );
}

/// vágás, az új kép jön
static void cut( Deshake d ) {
   swapImg( &d->curr, &d->out );
   swapHist( &d->chist, &d->phist );
   d->v.x = d->v.y = 0;
}

static VytF diffHist( VigHist a, VigHist b, VytU n, VytF limit ) {
   VytF ret = 0;
   for (int i=0; i<n; ++i) {
      ret += fabs( a[i]-b[i] );
      if ( ! ( i & DIFFMASK )) {
         if ( limit < ret/n )
            return ret/n;
      }
   }
   return ret/n;
}

void dumpHist( VigHist p, VytU n ) {
   fprintf( stderr, "HIST %p: ", p );
   for (int i=0; i<n; ++i)
      fprintf( stderr, " %.3g", p[i] );
   fprintf( stderr, "\n" );
}


static bool delta( VigHist a, VigHist b, VytU l, VytF limit, VytI * ret ) {
   if ( 4 >= l ) {
      *ret = 0;
      return limit > diffHist( a, b, l, limit );
   }
   if ( ! delta( a+l, b+l, l/2, limit, ret ))
      return false;
   *ret *= 2;
   if ( 0 < *ret )
      b += *ret;
      else a -= *ret;
   l = l - abs(*ret)-1;
   VytF d0 = diffHist( a, b, l, limit );
   VytF dp = diffHist( a, b+1, l, fmin(d0,limit) );
   VytF dm = diffHist( a+1, b, l, fmin(dp,limit) );
   if ( limit < d0 && limit < dp && limit < dm )
      return false;
   if ( dp < d0 && dp < dm ) 
      ++ *ret;
   else if ( dm < d0 )
      -- *ret;
   return true;
}

/// elmozdulás kiszámolása 
bool diff( Deshake d ) {
   VigHist p = d->chist;
   VytU w = d->dim.x;
   VytU h = d->dim.y;
   d->d.x = d->d.y = 0;
   return delta( d->phist, p, w, d->limit, &d->d.x )
      && delta( d->phist+2*w, p+2*w, h, d->limit, &d->d.y );
}   

void f0r_update( void * instance, double time, 
   const unsigned int * input, unsigned int * output 
) {
   Deshake d = (Deshake)instance;
   swapImg( &d->out, &d->prev );
   vig_raw_read( d->curr, &input, vyt_mread, false );
   VigHist p = d->chist;
   vig_hist_create( d->curr, p, p+2*d->dim.x, true );
   histExtends( d, p );
   if ( diff(d) )
      compose(d);
      else cut(d);
   vig_raw_write( d->out, &output, vyt_mwrite, false );
}
   
   
   



