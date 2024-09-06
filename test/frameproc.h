#ifndef FRAMEPROCH
#define FRAMEPROCH

#include <vulcmp.h>
#include <vulimg.h>

typedef struct FrameProc {
   VigImage frame;
} * FrameProc;

/// új képkockát gyártó függvény
typedef VigImage (* vfp_next_proc)( FrameProc );

/// argumentum olvasó függvény
typedef bool (* vfp_arg_reader)( FrameProc, int argc, char ** argv, int * at );

/// inicializálás
void vfp_init( VcpStr name, uint32_t flags, FrameProc, int argc, char ** argv, vfp_arg_reader );

/// bemenet feldolgozása, futtatás
void vfp_process( FrameProc, vfp_next_proc );

/// standard paraméter olvasó
bool vfp_next_arg( FrameProc, int argc, char ** argv, int * at );

/// lezárás
void vfp_done( FrameProc );

/// két kép cseréje
void vfp_flip( VigImage * a, VigImage * b );

#endif // FRAMEPROCH
