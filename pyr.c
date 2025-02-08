#include "vulimg_impl.h"

#include "pyr.inc"
#include "delta8.inc"
#include <math.h>

TASK( pyr, 2, struct VigPyrParams );
TASK( delta8, 2, struct VigDeltaParams );

typedef int VigDir;

typedef struct VigDeltaParams * VigDeltaParams;

/// irány delták
static int vig_dx( VigDir i ) {
   switch (i) {
      case 1: case 4: case 7: return -1;
      case 3: case 6: case 9: return 1;
      default: return 0;
   }
}

static int vig_dy( VigDir i ) {
   switch (i) {
      case 1: case 2: case 3: return 1;
      case 7: case 8: case 9: return -1;
      default: return 0;
   }
}

/// piramis lépések száma
static int vig_pyr_count( VigImage a ) {
   int d = MAX( vig_image_width(a), vig_image_height(a) );
   int ret = 0;
   while ( 1 < d ) {
      ++ret;
      d /= 2;
   }
   return ret;
}

// méretek és pixel ellenőrzése
static bool vig_pyr_delta_check( VigImage a, VigImage b, bool pyr ) {
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( a->pixel, b->pixel ) ) return false;
   if ( vig_pixel_signed( b->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( vig_image_width(a) / (pyr?2:1) != vig_image_width(b)) return false;
   if ( vig_image_height(a) != vig_image_height(b)) return false;
   return true;
}

/// pirams lépés eredmény olvasás
static bool vig_pyr_delta_best( uint32_t * sum, 
   uint32_t limit, int32_t * dx, int32_t * dy ) 
{
   VigDir best = 0;
   sum[best] = VIG_MUCH;
   for (int j=1; j<=9; ++j) {
      uint32_t jv = sum[j];
      if ( sum[best] > jv && limit > jv )
         best = j;
   }
   if (0 == best) {
      *dx = *dy = VIG_MUCH;
      return false;
   } else {
      *dx = 2 * *dx + vig_dx( best );
      *dy = 2 * *dy + vig_dy( best );
      return true;
   }
}   


/// piramis összehasonlító lépés cpu-val
static bool vig_pyr_delta_cpu( int i, int n, VigImage a,
   VigImage pa, VigImage pb, VigDeltaParams pars, 
   float limit, int32_t * dx, int32_t * dy )
{
   int m = pars->comps;
   int t = pars->img.stride * 4;
   uint32_t lim = round( limit * 255
      * pars->img.width * pars->img.height * m );
   uint32_t sums[10];
   for (VigDir d=0; d<=9; ++d)
      sums[d] = VIG_MUCH;
   int best = 0;
   uint8_t * qa = vig_image_address(pa);
   uint8_t * qb = vig_image_address(pb);
   for (VigDir d=1; d<=9; ++d) {
      uint32_t sum = 0;
      int r = 7<=d ? 1 : 0; 
      int ru = r + pars->img.height-1;
      for ( ; r < ru; ++r ) {
         int c = (1==d||4==d||7==d) ? m : 0;
         int cu = c + (pars->img.width-1)*m;
         uint8_t *ra = qa + t*r + c;
         uint8_t *rb = qb + t*r + t*vig_dy(d) + c + vig_dx(d)*m;
         for ( ; c < cu; ++c ) {
            sum += abs( (int)*ra - (int)*rb );
            ++ra;
            ++rb;
         }
         if ( lim < sum ) break;
      }
      sums[d] = sum;
      if ( sums[best] > sum ) 
         best = d;
   }
   return vig_pyr_delta_best( sums, lim, dx, dy );
}



static bool vig_pyr_delta_bests( int h, float wclimit, 
   int32_t * dx, int32_t * dy ) 
{
   uint32_t * sum = vcp_storage_address( vulimg.temp );
   for (int i=1; i<h; ++i) {
      for (int j=1; j<=9; ++j)
         sum[j] += sum[ 10*i+j ];
   }
   for (int j=1; j<=9; ++j)
      sum[j] /= h;
   return vig_pyr_delta_best( sum, h*255*wclimit, dx, dy );
}



/// egy piramis-összehasonlítási lépés
static bool vig_pyr_delta_step( int i, int n, VigImage a, 
   VigImage pa, VigImage pb, float limit, int32_t * dx, int32_t * dy )
{
   struct VigDeltaParams pars;
   vig_imgpar( pa, & pars.img );
   int w = vig_image_width(a);
   int h = vig_image_height(a);
   int y = -h;
   for (int j = i; j<n; ++j) {
      y += h;
      w /=2;
      h /=2;
   }
   pars.comps = vig_pixel_size( a->pixel );
   pars.img.width = w;
   pars.img.height = h;
   if ( i < PSMALL )
      return vig_pyr_delta_cpu( i, n, a, pa, pb, &pars, limit, dx, dy );
   if ( ! vig_temp_grow( 10*h*4 )) return false;
   VcpTask t = vig_delta8();
   if ( ! t ) return false;
   VcpStorage ss[2] = { pa->stor, pb->stor };
   vcp_task_setup( t, ss, 1, DIVC( h, UGR ), 1, &pars );
   if ( ! vig_run( t )) return false;
   vig_pyr_delta_bests( h, limit*pars.comps*w, dx, dy );
   return false;
}


bool vig_pyr_delta( VigImage a, VigImage b, VigImage pyra, VigImage pyrb,
   float limit, int32_t * dx, int32_t * dy )
{
   if ( ! vig_inited() ) return false;
   if ( ! vig_pyr_delta_check( a, b, false )) return false;
   if ( ! vig_pyr_delta_check( a, pyra, true )) return false;
   if ( ! vig_pyr_delta_check( a, pyrb, true )) return false;
   uint32_t n = vig_pyr_count( a );
   *dx = *dy = 0;
   for (int i=0; i<n; ++i) {
      if ( ! vig_pyr_delta_step( i, n, a, pyra, pyrb, limit, dx, dy )) 
         return false;
      if ( VIG_MUCH == *dx ) return true;
   }
   return vig_pyr_delta_step( n, n, a, a, b, limit, dx, dy );
}

static bool vig_pyrs_grow( uint32_t n ) {
   if ( vulimg.npyrs >= n ) return true;
   vigResult = VIG_HOSTMEM;
   VigPyrParams ret = REALLOC( vulimg.pyrs, struct VigPyrParams, n );
   if ( !ret ) return false;
   vulimg.pyrs = ret;
   vulimg.npyrs = n;
   vigResult = VIG_SUCCESS;
   return ret;
}


static VcpTask vig_pyr_setup( VigImage src, VigImage dst ) {
   VcpStorage ss [2] = { src->stor, dst->stor };
   // lépésszám meghatározás
   uint32_t nrows = vig_image_height(src);
   uint32_t ncols = vig_image_width(src);
   uint32_t n = vig_pyr_count(src);
   int32_t ps = vig_pixel_size( dst->pixel );
   uint32_t compCount = vig_pixel_comps( dst->pixel );
   uint32_t compBits = ps / compCount;
   // konfiguráció kitöltése
   if ( ! vig_pyrs_grow( n ))
      return NULL;
   vigResult = VIG_TASKERR;
   VcpTask ret = vig_pyr();
   if ( ! ret ) return NULL;
   vcp_task_setup( ret, ss, 0, 0, 0, NULL );
   VcpPart prs = vcp_task_parts( ret, n );
   vcp_check_fail();   
   uint32_t row = 0;
   for ( int i=0; i<n; ++i ) {
      VigPyrParams py = vulimg.pyrs+i;
      vig_imgpar( src, & py->src );
      vig_imgpar( dst, & py->dst );
      py->compBits = compBits;
      py->compCount = compCount;
      py->height = (nrows /= 2);
      py->width = (ncols /= 2); 
      py->row = row;
      row += nrows;
      VcpPart pr = prs+i;
      pr->countX = DIVC( ncols * ps, 32*UGR );
      pr->countY = DIVC( nrows, UGR );
      pr->countZ = 1;
      pr->constants = py;
   }
   vigResult = VIG_SUCCESS;
   return ret;
}


bool vig_pyr_create( VigImage img, VigImage pyr ) {
   if ( ! vig_inited() ) return false;
   vigResult = VIG_PIXELERR;
   if ( ! vig_pixel_same( img->pixel, pyr->pixel )) return false;
   if ( vig_pixel_signed( img->pixel )) return false;
   vigResult = VIG_COORDERR;
   if ( img == pyr ) return false;
   uint32_t w = vig_image_width(img);
   uint32_t h = vig_image_height(img);
   if ( 2 > w || 2 > h ) return false;
   if ( w/2 > vig_image_width(pyr)) return false;
   if ( h > vig_image_height(pyr)) return false;
   struct VigPyrParams pars;
   vig_imgpar( img, & pars.src );
   vig_imgpar( pyr, & pars.dst );
   uint32_t pxs = vig_pixel_size( pyr->pixel );
   pars.compBits = pxs / vig_pixel_comps( pyr->pixel );
   VcpTask t = vig_pyr_setup( img, pyr );
   if ( ! t ) return false;
   vigResult = VIG_TASKERR;
	return vig_run( t );
}
