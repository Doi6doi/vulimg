#ifndef FRAMEPROCH
#define FRAMEPROCH

#include <vulcmp.h>
#include <vulimg.h>

/// a tárolt adatok
typedef struct FrameData * FrameData; 

/// képkocka feldolgozó objektum
typedef struct FrameProc {
   /// aktuális képkocka
   VigImage * (* frame)( FrameData );
   /// egy argumentum olvasása
   bool (* arg)( FrameData, int argc, char ** argv, int * at );
   /// következő képkockát gyártó függvény
   VigImage (* next)( FrameData );
} * FrameProc;
 
/// inicializálás
void vfp_init( VcpStr name, uint32_t flags, FrameData, FrameProc, 
   int argc, char ** argv );

/// bemenet feldolgozása, futtatás
void vfp_process( FrameData, FrameProc );

/// standard paraméter olvasó
bool vfp_arg( FrameData, int argc, char ** argv, int * at );

/// egész paraméter olvasás
bool vfp_nat_arg( int argc, char ** argv, int * at, uint32_t * ret );

/// pixel paraméter olvasás
bool vfp_pixel_arg( int argc, char ** argv, int * at, VigPixel * ret );

/// lezárás
void vfp_done( FrameData, FrameProc );

/// két kép cseréje
void vfp_flip( VigImage * a, VigImage * b );


#endif // FRAMEPROCH
