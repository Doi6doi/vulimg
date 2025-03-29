#include <frei0r.h>
#include <frei0r.h>
#include <vulcmp.h>
#include <vulimg.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static VcpVulcomp vul = NULL;

typedef enum Param { COUNT, LIMIT } Param;

typedef enum Part { LARGE, VERT, HORZ } Part;

#define DIFFMASK 0xff

typedef struct Deshake {
   uint32_t count;
   uint32_t dcount;
   float limit;
   struct Vyt_UVec2 dim;
   VigImage curr;
   VigImage prev;
   VigImage out;
   VigHist chist;
   VigHist phist;
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
   ret->limit = 0.2;
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
   ret->ds = NULL;
   ret->dc.x = ret->dc.y = 0;
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

/// shift image handles and deltas
static void shift( Deshake d ) {
   uint32_t n = d->count;
   if ( 0 == n ) return;
   swapImg( &d->out, &d->prev );
   if ( d->dcount < d->count )
      ++ d->dcount;
   for (int i=d->dcount-1; 0<i; --i)
      d->ds[i] = d->ds[i-1];
}

/// regresszió részeredményekből
static int regr( int n, int siv, int si, int sv, int sii ) {
   float a = (float)(n*siv - si*sv) / (n*sii - si*si);
   float ret = ((float)sv - a*si)/n;
fprintf( stderr, "regr %d %d %d %d %d: %.3g\n", n, siv, si, sv, sii, ret );
   return round(ret);
}

/// regressziós egyenes
static void regress( Deshake d ) {
   struct Vyt_IVec2 v = {.x=0, .y=0}, siv=v, sv=v;
   int32_t n = d->dcount;
   int32_t si=0, sii=0;
// fprintf( stderr, "regress n:%d\n", n ); 
   for (int i=n-2; 0<=i; --i) {
      v = vyt_Iv2_add( &v, d->ds+i );
// fprintf( stderr, "regress n:%d i:%d d:%d,%d v:%d,%d\n", 
//   n, i, d->ds[i].x, d->ds[i].y, v.x, v.y );
      si += i;
      sii += i*i;
      siv.x += v.x * i;
      siv.y += v.y * i;
      sv = vyt_Iv2_add( &sv, &v );
   }
   d->dc = *d->ds;
   d->ds->x = regr( n, siv.x, si, sv.x, sii );
   d->ds->y = regr( n, siv.y, si, sv.y, sii );
}

void histCopy( Deshake d, Part part, VytURect r, VytUVec2 u ) {
// fprintf( stderr, "histCopy %d r:%d,%d/%d,%d u:%d,%d\n", part,
//   r->left, r->top, r->width, r->height, u->x, u->y );
// fflush( stderr );   
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

static void compose( Deshake d ) {
fprintf( stderr, "compose %p %p\n", d->chist, d->phist );
   int m = d->dcount;
   if ( 1 == m ) {
      vig_image_copy( d->curr, d->out );
      swapHist( &d->chist, &d->phist );
      return;
   } else if ( 2 == m ) {
      d->ds->x = d->dc.x / 2 ;
      d->ds->y = d->dc.y / 2;
   } else {
      regress( d );
   }
fprintf( stderr, "ds:%d,%d dc:%d,%d\n", d->ds->x, d->ds->y, d->dc.x, d->dc.y );
   VytU ax = abs( d->ds->x - d->dc.x );
   VytU ay = abs( d->ds->y - d->dc.y );
   VytU bx = abs( d->dc.x );
   VytU by = abs( d->dc.y );
fprintf( stderr, "a:%d,%d b:%d,%d\n", ax, ay, bx, by );
   int w = d->dim.x;
   int h = d->dim.y;
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
   histCopy( d, VERT, &s, &v );
   s.width = w-bx;
   s.height = ax;
   vig_part_copy( d->prev, &s, d->out, &v );
   histCopy( d, HORZ, &s, &v );
   histCopy( d, LARGE, &r, &u );
   histExtends( d, d->phist );
}

static VytF diff( VigHist a, VigHist b, VytU n, VytF limit ) {
// fprintf( stderr, "diff n:%d limit:%.3g\n", n, limit ) ;
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
      return limit > diff( a, b, l, limit );
   }
   if ( ! delta( a+l, b+l, l/2, limit, ret ))
      return false;
   *ret *= 2;
   if ( 0 < *ret )
      b += *ret;
      else a -= *ret;
   l = l - abs(*ret)-1;
   VytF d0 = diff( a, b, l, limit );
   VytF dp = diff( a, b+1, l, fmin(d0,limit) );
   VytF dm = diff( a+1, b, l, fmin(dp,limit) );
// fprintf( stderr, "delta ret:%d l:%d d0:%f dp:%f dm:%f\n", *ret, l, d0, dp, dm );
   if ( limit < d0 && limit < dp && limit < dm )
      return false;
   if ( dp < d0 && dp < dm ) 
      ++ *ret;
   else if ( dm < d0 )
      -- *ret;
   return true;
}


void f0r_update( void * instance, double time, 
   const unsigned int * input, unsigned int * output 
) {
static int frm = 0;   
bool much;
   Deshake d = (Deshake)instance;
fprintf( stderr, "UPDATE %d ch:%p dh:%p w:%d\n", frm, d->chist, d->phist, d->dim.x );
   shift(d);
   vig_raw_read( d->curr, &input, vyt_mread, false );
   VytU w = d->dim.x;
   VytU h = d->dim.y;
   VigHist p = d->chist;
   vig_hist_create( d->curr, p, p+2*w, true );
   histExtends( d, p );
   d->ds->x = d->ds->y = 0;
much = true;   
   if ( ! delta( d->phist, p, w, d->limit, &d->ds->x ) )
      d->dcount = 1;
   else if ( ! delta( d->phist+2*w, p+2*w, h, d->limit, &d->ds->y ))
      d->dcount = 1;
   else
      much = false;
if ( much )
fprintf( stderr, "FRM: %d MUCH\n", frm );
else fprintf( stderr, "FRM: %d DX:%d DY:%d\n", frm, d->ds->x, d->ds->y );
++frm;
   compose(d);
   vig_raw_write( d->out, &output, vyt_mwrite, false );
}
   
   
   



