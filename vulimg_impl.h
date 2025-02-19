#ifndef VULIMG_IMPLH
#define VULIMG_IMPLH

#include "vulimg.h"
#include <vultools.h>
#include "vulimg_comp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define REALLOC( p, type, n ) (type *)realloc( p, (n)*sizeof(type) )
#define DIVC( a, b ) (((a)+(b-1))/(b))
#define TICK 1000
#define DEBUG( fmt, ... ) fprintf( stderr, fmt "\n", __VA_ARGS__ ); fflush( stderr )
#define FAIL( fmt, ... ) { DEBUG( fmt, __VA_ARGS__ ); exit(1); }
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define PSMALL 5
#define TASK( name, nstor, conf ) \
   static VcpTask vig_##name() { \
      vigResult = VIG_TASKERR; \
      if ( ! vulimg.name ) { \
		 vulimg.name = vcp_task_create( vulimg.vulcomp, \
		    name##_spv, name##_spv_len, "main", nstor, sizeof( conf )); \
	  } \
      return vulimg.name; \
   } 

typedef struct VigImgParam * VigImgParam;
typedef struct VigRect * VigRect;
typedef struct VigCloud * VigCloud;
typedef struct VigCopyParams * VigCopyParams;
typedef struct VigJoinParams * VigJoinParams;
typedef struct VigPyrParams * VigPyrParams;
typedef struct VigWhiteParams * VigWhiteParams;
typedef struct VigWCloudParams * VigWCloudParams;

typedef struct VigVulimg {
   VcpVulcomp vulcomp;
   uint32_t nimg;
   VigImage * imgs;
   VcpStorage temp;
   uint32_t npyrs;
   VigPyrParams pyrs;
   uint32_t nwhites;
   VigWhiteParams whites;
   uint32_t nwclouds;
   VigWCloudParams wclouds;
   bool started;
   VcpTask copy1;
   VcpTask copy32;
   VcpTask join3;
   VcpTask plane3;
   VcpTask trans;
   VcpTask diff;
   VcpTask pyr;
   VcpTask white8;
   VcpTask dsum;
   VcpTask add8;
   VcpTask rect;
   VcpTask wcloud8;
   VcpTask delta8;
   VcpTask fill;
} * VigVulimg;

struct VigImage {
   VigPixel pixel;
   uint32_t width;
   uint32_t height;
   uint32_t stride;
   VcpStorage stor;
};

extern struct VigVulimg vulimg;
extern int vigResult;

/// inicializálva van-e
bool vig_inited();
/// temp méret beállítás
bool vig_temp_grow( uint64_t size );
/// kép paraméterek másolása
void vig_imgpar( VigImage i, VigImgParam p );
/// egyező felépítésű pixelek
bool vig_pixel_same( VigPixel a, VigPixel b );
/// task futtatás
bool vig_run( VcpTask t );
/// előjeles pixel
bool vig_pixel_signed( VigPixel pix );
/// komponensek száma
uint32_t vig_pixel_comps( VigPixel pix );

#endif // VULIMG_IMPLH
