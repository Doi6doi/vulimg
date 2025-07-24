#include "vulimg_impl.h"

VIG_NBEGIN()

#include "white8.inc"
#include "wcloud8.inc"

TASK( white8, 2, struct Vig_WhiteParams );
TASK( wcloud8, 2, struct Vig_WCloudParams );

/// egy téglalap kiírása
/*
static void vig_rect_dump( VigRect r ) {
   DEBUG( "RECT -> %d, %d [%d,%d]/[%d,%d]", r->link, r->weight, 
      r->left, r->top, r->width, r->height );
}
*/

/// VigRect -> VtlRect
static void vig_rect_set( VytURect r, VigRect s ) {
   r->left = s->left;
   r->top = s->top;
   r->width = s->width;
   r->height = s->height;
}

/// VigRect -> VytURect
static void vig_part_set( VytURect r, VigRect s ) {
   r->left = s->left;
   r->top = s->top;
   r->width = s->width;
   r->height = s->height;
}

/*
/// VigRect -> VtlRect
static void vig_cloud_set( VtlCloud c, VigCloud s ) {
   c->weight = s->weight;
   c->mx = s->mx;
   c->my = s->my;
   c->dx = s->dx;
   c->dy = s->dy;
}
*/
/// egy rect betöltése a képből
static void vig_rect_load( uint32_t * ptr, uint32_t stride, VigRect r ) {
   r->link = ptr[0];
   r->weight = ptr[stride];
   uint32_t lt = ptr[2*stride];
   uint32_t wh = ptr[3*stride];
   r->left = lt & 0xffff;
   r->top = lt >> 16;
   r->width = wh & 0xffff;
   r->height = wh >> 16;
}

/// egy felhő betöltése a képből
static void vig_wcloud_load( uint32_t * ptr, uint32_t stride, VigCloud c ) {
   c->link = ptr[0];
   c->weight = ptr[stride];
   float * pf = (float *)ptr;
   c->mx = pf[2*stride];
   c->my = pf[3*stride];
   c->dx = pf[4*stride];
   c->dy = pf[5*stride];
}

static bool vig_wrects_grow( uint32_t n ) {
   if ( vulimg.nwhites >= n ) return true;
   vigResult = VIG_HOSTMEM;
   VigWhiteParams ret = REALLOC( vulimg.whites, struct Vig_WhiteParams, n );
   if ( !ret ) return false;
   vulimg.whites = ret;
   vulimg.nwhites = n;
   vigResult = VIG_SUCCESS;
   return ret;
}



/// fehér felhő konfigok száma
static bool vig_wclouds_grow( uint32_t n ) {
   if ( n <= vulimg.nwclouds ) return true;
   vigResult = VIG_HOSTMEM;
   VigWCloudParams ret = REALLOC( vulimg.wclouds, struct Vig_WCloudParams, n );
   if ( ! ret ) return false;
   vulimg.wclouds = ret;
   vulimg.nwclouds = n;
   vigResult = VIG_SUCCESS;
   return ret;
}

/// keresés a rendezett listában
static uint32_t vig_wrect_find( VigRect rr, uint32_t found, uint32_t weight ) {
   for ( int i=0; i < found; ++i )
      if ( weight > rr[i].weight )
         return i;
   return found;
}

/// keresés a rendezett listában
static uint32_t vig_wcloud_find( VigCloud cc, uint32_t found, uint32_t weight ) {
   for ( int i=0; i < found; ++i )
      if ( weight > cc[i].weight )
         return i;
   return found;
}

/// egy rect hozzáadása az eredményhez
static void vig_wrect_push( VigRect rr, VigRect r, uint32_t count, 
   uint32_t * found, uint32_t * good )
{
   uint32_t dst = vig_wrect_find( rr, *found, r->weight );
   *found = MIN( count, *found+1 );
   memmove( rr + dst + 1, rr + dst, (*found-dst-1)*sizeof(struct Vig_Rect) ); 
   rr[dst] = *r;
   *good = rr[*found].weight;
}

/// egy felhő hozzáadása az eredményhez
static void vig_wcloud_push( VigCloud cc, VigCloud c, uint32_t count, 
   uint32_t * found, uint32_t * good )
{
   uint32_t dst = vig_wcloud_find( cc, *found, c->weight );
   *found = MIN( count, *found+1 );
   memmove( cc + dst + 1, cc + dst, (*found-dst-1)*sizeof(struct Vig_Cloud) ); 
   cc[dst] = *c;
   *good = cc[*found].weight;
}

/// white rects config beállítás
static VcpTask vig_wrects_setup( VigImage img, float limit, float density,
   uint32_t minSize, uint32_t maxDist )
{
   uint32_t n = 1;
   uint32_t w = img->width;
   uint32_t h = img->height;
   uint32_t n4 = 4;
   while ( ( n4 < w || n4 < h ) ) {
	  ++n;
	  n4 *= 4;
   }
   if ( ! vig_wrects_grow( n )) return NULL;
   if ( ! vig_temp_grow( img->stride*h )) return NULL;
   VcpTask ret = vig_white8();
   if ( ! ret ) return NULL;
   VcpStorage ss[2] = { img->stor, vulimg.temp };
   vcp_task_setup( ret, ss, 0, 0, 0, NULL );
   VcpPart ps = vcp_task_parts( ret, n );
   uint32_t z = 4;
   for ( int i=0; i<n; ++i ) {
      VigWhiteParams pr = vulimg.whites+i;
      vig_imgpar( img, & pr->img );
      pr->phase = i;
      pr->limit = limit;
      pr->density = density;
      pr->minSize = minSize;
      pr->maxDist = maxDist;
	   VcpPart p = ps+i;
	   p->countX = DIVC( img->width, z );
	   p->countY = DIVC( img->height, z );
      p->countZ = 1;
      p->constants = pr;
      z *= 4;
   }
   return ret;
}

/// white clouds config beállítás
static VcpTask vig_wclouds_setup( VigImage img, float maxDist )
{
   uint32_t n = 1;
   uint32_t w = img->width;
   uint32_t h = img->height;
   uint32_t wi = 4;
   uint32_t hi = 6;
   while ( ( wi < w || hi < h ) ) {
	  ++n;
	  wi *= 4;
     hi *= 4;
   }
   if ( ! vig_wclouds_grow( n )) return NULL;
   if ( ! vig_temp_grow( w*h )) return NULL;
   VcpTask ret = vig_wcloud8();
   if ( ! ret ) return NULL;
   VcpStorage ss[2] = { img->stor, vulimg.temp };
   vcp_task_setup( ret, ss, 0, 0, 0, NULL );
   VcpPart ps = vcp_task_parts( ret, n );
   wi = 4;
   hi = 6;
   for ( int i=0; i<n; ++i ) {
      VigWhiteParams pr = vulimg.whites+i;
      vig_imgpar( img, & pr->img );
      pr->phase = i;
      pr->maxDist = maxDist;
	   VcpPart p = ps+i;
	   p->countX = DIVC( img->width, wi );
	   p->countY = DIVC( img->height, hi );
      p->countZ = 1;
      p->constants = pr;
      wi *= 4;
      hi *= 4;
   }
   return ret;
}

/// minden rect kiolvasása az eredményből
static void vig_wrects_result( VcpStorage s, uint32_t stride, 
   VytURect rects, uint32_t * count ) 
{
   struct Vig_Rect r;
   struct Vig_Rect rr[ *count ];
   uint32_t * ptr = (uint32_t *)vcp_storage_address( s );
   vig_rect_load( ptr, stride, & r );
   uint32_t good = 0;
   uint32_t found = 0;
   while ( EMPTY != r.link ) {
// DEBUG("readrect %p %d", ptr, r.link );	   
	  if ( good < r.weight )
		 vig_wrect_push( rr, & r, *count, & found, & good );
      if ( TAIL == r.link )
         break;
      vig_rect_load( ptr+r.link, stride, & r );
   }
/*for (int k=0; k < *count; ++k )
vig_rect_dump( rr+k );
DEBUG("count: %d", *count );   
*/
   for ( int i=0; i<found; ++i)
      vig_rect_set( rects+i, rr+i );
   *count = found;
}

/*
/// minden felhő kiolvasása az eredményből
static void vig_wclouds_result( VcpStorage s, uint32_t stride, 
   VtlCloud clouds, uint32_t * count ) 
{
   struct Vig_Cloud c;
   struct Vig_Cloud cc[ *count ];
   uint32_t * ptr = vcp_storage_address( s );
   vig_wcloud_load( ptr, stride, & c );
   uint32_t good = 0;
   uint32_t found = 0;
   while ( EMPTY != c.link ) {
	  if ( good < c.weight )
		 vig_wcloud_push( cc, & c, *count, & found, & good );
      if ( TAIL == c.link )
         break;
      vig_wcloud_load( ptr+c.link, stride, & c );
   }
   for ( int i=0; i<found; ++i)
      vig_cloud_set( clouds+i, cc+i );
   *count = found;
}
*/

bool vig_white_rects( VigImage img, float limit, 
   float density, uint32_t minSize, uint32_t maxDist, 
   VytURect rects, uint32_t * count )
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( vix_g8, img->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( *count <= 0 ) return false;
   if ( 0 > limit || 1 < limit ) return false;
   VcpTask t = vig_wrects_setup( img, limit, density, minSize, maxDist );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
   uint32_t * p = (uint32_t *)vcp_storage_address( vulimg.temp );
DEBUG( "DST1: %d", p[1] );
   if ( ! vig_run( t )) return false;
DEBUG( "RUN %d", vigResult );
   vig_drawallrects( img, 0 );
DEBUG( "NOW %d", vigResult );
   vig_wrects_result( vulimg.temp, img->stride, rects, count );
DEBUG( "RSLT %d", vigResult );
   return true;
}

/*
bool vig_white_clouds( VigImage img, float maxDist,
   VtlCloud clouds, uint32_t * count )
{
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( vix_g8, img->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( *count <= 0 ) return false;
   if ( maxDist < 4 ) return false;
   VcpTask t = vig_wclouds_setup( img, maxDist );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
   if ( ! vig_run( t )) return false;
//    vig_drawallclouds( img, 0 );
   vig_wclouds_result( vulimg.temp, img->stride, clouds, count );
   return true;
}
*/

/// minden téglalap kirajzolása
void vig_drawallrects( VigImage img, uint32_t n ) {
   uint32_t z = 4 << (2*n);
   uint32_t * p = (uint32_t *)vig_image_address( img );
   uint32_t stride = img->stride;
DEBUG("debug_data %d", p[DIDX] );	
   struct Vig_Rect gr;    
   struct Vyt_URect prt;
   for ( int y=0; y < img->height; y += z ) {
      for ( int x=0; x < img->width; x += z ) {
		  uint32_t idx = y*stride + x/4;
		  vig_rect_load( p+idx, stride, & gr );
		  if ( EMPTY != gr.link ) {
fprintf( stderr, "x:%d y:%d ", x, y );			  
//		     vig_rect_dump( & gr );
  		     vig_part_set( & prt, & gr );
           vig_draw_rect( img, &prt, 0xff );
        }
	  }
   }
}

/// minden felhő kirajzolása temp-ből
/*
 * void vig_drawallclouds( VigImage img, uint32_t n ) {
   uint32_t ix = 4 << (2*n);
   uint32_t iy = ix * 3/2;
   uint32_t * p = vcp_storage_address( vulimg.temp );
   uint32_t s = img->stride;
   struct Vig_Cloud gc;
   struct VtlCloud c;
   for ( int y=0; y < img->height; y += ix ) {
      for ( int x=0; x < img->width; x += iy ) {
		  uint32_t idx = y*s + x/4;
		  vig_wcloud_load( p+idx, s, & gc );
		  if ( EMPTY != gc.link ) {
fprintf( stderr, "x:%d y:%d ", x, y );			  
//		     vig_cloud_dump( & gr );
           vig_cloud_set( & c, & gc );
//           vig_draw_cloud( img, & c, 0xff );
        }
	  }
   }
}
*/

VIG_NEND()








