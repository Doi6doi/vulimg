#include "vulimg_impl.h"

#include "pyr.inc"
#include "delta8.inc"
#include <math.h>

VIG_NBEGIN()

TASK( pyr, 2, struct Vig_PyrParams );
TASK( delta8, 3, struct Vig_DeltaParams );

typedef int VigDir;

typedef struct Vig_DeltaParams * VigDeltaParams;

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
   int d = MIN( vig_image_width(a), vig_image_height(a) );
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
   static char perm[9] = { 5,2,4,6,8,1,3,7,9 };
   VigDir best = 0;
   sum[best] = VIG_MUCH;
   for (int j=0; j<9; ++j) {
      int k=perm[j];
      uint32_t kv = sum[k];
      if ( limit >= kv && sum[best] > kv )
         best = k;
   }
// vtl_ewrite( "best:%d v:%d dx:%d dy:%d", best, sum[best], *dx, *dy );   
   if (0 == best) {
      *dx = *dy = VIG_MUCH;
      return false;
   } else {
      *dx += vig_dx( best );
      *dy += vig_dy( best );
      return true;
   }
}   

void vig_dumpdimg( uint8_t * p, VigDeltaParams pars ) {
   uint32_t t = pars->img.stride * 4;
vyt_ewrite("dumpdimg p:%p %d %d %d %d t:%d", p, pars->img.width, pars->img.height, pars->top, pars->comps,t );
   for (int r=0; r<pars->img.height; ++r) {
      for (int c=0; c<pars->img.width * pars->comps; ++c) {
         int x = p[(r+pars->top)*t + c];
         fprintf( stderr, "%x", x * 15  / 255 );
      }
      fprintf( stderr, "\n" );
   }
   fprintf( stderr, "\n" );
}
   
      


/// piramis összehasonlító lépés cpu-val
void vig_pyr_delta_cpu( int i, int n, VigImage a,
   VigImage pa, VigImage pb, VigDeltaParams pars, 
   uint32_t lim )
{
   int m = pars->comps;
   int t = pars->img.stride * 4;
// fprintf( stderr, "\nVPDC lim:%d w:%d h:%d t:%d m:%d\n", 
// lim, pars->img.width, pars->img.height, pars->top, m );         
   uint32_t sums[10];
   uint8_t * qa = (uint8_t *)vig_image_address(pa);
   uint8_t * qb = (uint8_t *)vig_image_address(pb);
   uint32_t cw = pars->img.width-abs(pars->dx)-1;
   uint32_t ch = pars->img.height-abs(pars->dy)-1;
   for (VigDir d=0; d<=9; ++d)
      sums[d] = VIG_MUCH;
// fprintf( stderr, "qa:%p qb:%p cw:%d ch:%d\n", qa, qb, cw, ch );         
   for (VigDir d=1; d<=9; ++d) {
      uint32_t sum = 0;
      int32_t ddx = pars->dx + vig_dx(d);
      int32_t ddy = pars->dy + vig_dy(d);
      int r = MAX( 0, -ddy );
/*if ( 100 > pars->img.height && 1 == d) {
vig_dumpdimg( qa, pars );
vig_dumpdimg( qb, pars );
}*/
      int ru = r + ch;
      for ( ; r < ru; ++r ) {
         uint32_t rsum = 0;
         int c = MAX( 0, -ddx );
         int cu = c + cw*m;
         uint8_t *ra = qa + t*(r+pars->top) + c*m;
         uint8_t *rb = qb + t*(r+pars->top+ddy) + (c+ddx)*m;
// vtl_ewrite("ra:%p rb:%p c:%d cu:%d", ra, rb, c, cu );
         for ( ; c < cu; ++c ) {
// vtl_ewrite("qa:%p qb:%p ra:%p=%d rb:%p=%d", qa, qb, ra, *ra, rb, *rb );
            rsum += abs( (int)*ra - (int)*rb );
            ++ra;
            ++rb;
         }
         sum += rsum;
// vtl_ewrite("r:%d ddx:%d ddy:%d rsum:%d", r, ddx, ddy, rsum);         
         if ( lim < sum ) break;
      }
// vtl_ewrite("d:%d lim:%d sum:%d", d, lim, sum );
      sums[d] = sum;
      if ( lim > sum )
         lim = sum;
   }
   vig_pyr_delta_best( sums, lim, &pars->dx, &pars->dy );
   return;
}



static bool vig_pyr_delta_bests( VigDeltaParams p, float wclimit ) {
   static uint32_t sum[10];
   for (int j=1; j<=9; ++j)
      sum[j] = 0;
   uint32_t * sums = (uint32_t *)vcp_storage_address( vulimg.temp );
   uint32_t h = p->img.height;
   for (int i=0; i<h; ++i) {
      for (int j=1; j<=9; ++j) {
// vtl_ewrite( "i:%d j:%d s:%d", i,j, sums[10*i+j] );
         sum[j] += sums[ 10*i+j ];
      }
   }
/*   for (int j=1; j<=9; ++j)
      vtl_ewrite( "d:%d sum:%d", j, sum[j] );
      */ 
   return vig_pyr_delta_best( sum, h*255*wclimit, &p->dx, &p->dy );
}



/// egy piramis-összehasonlítási lépés
static bool vig_pyr_delta_step( int i, int n, VigImage a, 
   VigImage pa, VigImage pb, float limit, VigDeltaParams pars )
{
// vtl_ewrite("deltastep %d dx:%d dy:%d", i, pars->dx, pars->dy );
   if ( VIG_MUCH == pars->dx ) return true;
   int w = vig_image_width(a);
   int h = vig_image_height(a);
   int y = -h;
   for (int j = i; j<n; ++j) {
      y += h;
      w /=2;
      h /=2;
   }
   if ( n == i ) 
      y = 0;
   if ( 2 >= w || 2 >= h )
      return true;
   pars->img.width = w;
   pars->img.height = h;
   pars->img.stride = pa->stride;
   pars->top = y;
   pars->dx *= 2;
   pars->dy *= 2;
   uint32_t lim = round( limit * 255 *w *h * pars->comps );
   if ( i < PSMALL ) {
//   if ( i < 100 ) {
      vig_pyr_delta_cpu( i, n, a, pa, pb, pars, lim );
      return true;
   }
   if ( ! vig_temp_grow( 10*h*4 )) return false;
   VcpTask t = vig_delta8();
   if ( ! t ) return false;
   VcpStorage ss[3] = { pa->stor, pb->stor, vulimg.temp };
   vcp_task_setup( t, ss, 1, DIVC( h, UGR ), 1, pars );
   if ( ! vig_run( t )) return false;
   vig_pyr_delta_bests( pars, lim );
   return true;
}


bool vig_pyr_delta( VigImage a, VigImage b, VigImage pyra, VigImage pyrb,
   float limit, int32_t * dx, int32_t * dy )
{
   if ( ! vig_inited() ) return false;
   if ( ! vig_pyr_delta_check( a, b, false )) return false;
   if ( ! vig_pyr_delta_check( a, pyra, true )) return false;
   if ( ! vig_pyr_delta_check( a, pyrb, true )) return false;
   struct Vig_DeltaParams pars;
   pars.comps = vig_pixel_size( a->pixel )/8;
   pars.img.stride = a->stride;
   pars.dx = 0;
   pars.dy = 0;
   uint32_t n = vig_pyr_count( a );
   for (int i=0; i<n; ++i) {
      if ( ! vig_pyr_delta_step( i, n, a, pyra, pyrb, limit, &pars )) 
         return false;
   }
   if ( ! vig_pyr_delta_step( n, n, a, a, b, limit, &pars ))
      return false;
   *dx = pars.dx;
   *dy = pars.dy;
   return true;
}

static bool vig_pyrs_grow( uint32_t n ) {
   if ( vulimg.npyrs >= n ) return true;
   vigResult = VIG_HOSTMEM;
   VigPyrParams ret = REALLOC( vulimg.pyrs, struct Vig_PyrParams, n );
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
   if ( ! prs ) return NULL;
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
   struct Vig_PyrParams pars;
   vig_imgpar( img, & pars.src );
   vig_imgpar( pyr, & pars.dst );
   uint32_t pxs = vig_pixel_size( pyr->pixel );
   pars.compBits = pxs / vig_pixel_comps( pyr->pixel );
   VcpTask t = vig_pyr_setup( img, pyr );
   if ( ! t ) return false;
	return vig_run( t );
}

VIG_NEND()
