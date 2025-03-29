make {

   import { C; Cpp; Glsl; }

   init {
      $name := "vulimg";
      $ver := "20250317";
      $author := "Várnagy Zoltán";
      $ident := "https://github.com/Doi6doi/vulimg";

      $gs := ["copy1","copy32","join3","plane3","trans","diff","dsum","add8",
         "hist8","pyr","delta8","rect","fill","white8","wcloud8"];
      $cs := ["vulimg.c","draw.c","white.c","pyr.c"];
      $vh := ["vulimg_comp.h"];
      $hs := [$vh,"vulimg.h","vulimg_impl.h"];
      $libs := ["vulcmp","vytools"];
      $dirs := regexp( $libs, "#.+#", "../\\0" );

      $cdep := "c.dep";
      $pdep := "p.dep";
      $os := changeExt( $cs, C.objExt() );
      $clib := C.libFile( $name );
      $plib := Cpp.libFile( $name+"p" );
      $purge := [ $cdep, $pdep, $clib, $plib, "*"+C.objExt()] 
         + changeExt($gs,".inc") + changeExt( $gs, ".spv" );
      C.set({ incDir:$dirs, libMode:true, libDir:$dirs, lib:$libs+["m"],
         debug:true } );
   }

   target {
      
      build {
         genShd();
         genDep();
         genObj();
         genLib();
      }
      
      clean {
         purge( $purge );
      }
   }

   function {
      /// generate shader files
      genShd() {
         foreach ( x | $gs) {
            g := changeExt( x, ".glsl" );
            i := changeExt( x, ".inc" );
            if ( older( i, [g,$vh] ) ) {
               s := changeExt( x, ".spv" );
               Glsl.compile( s, g );
               C.sourceRes( i, s, x );
            }
         }
      }
   
      /// generate dependency files
      genDep() {
         gis := changeExt( $gs, ".inc" );
         if ( older( $cdep, $cs+$hs+gis ) )
            C.depend( $cdep, $cs );
      }

      /// generate object files
      genObj() {
         ds := C.loadDep( $cdep );
         foreach ( c | $cs ) {
            o := changeExt( c, C.objExt() );
            if ( older( o, ds[o] ))
               C.compile( o, c );
         }
      }

      genLib() {
         if ( older( $clib, $os ))
            C.link( $clib, $os );
      }
   }

}
/*
CPP=g++ -g $(DIRS:%=-L%) $(DIRS:%=-I%)

vulimgp.o: vulimgp.cpp vulimg.hpp
	$(CPP) -c -fPIC -Wall -o $@ $<

$(PLIB): vulimgp.o 
	$(CPP) -shared -o $@ $^ -lvulcmp -lvulimg

*/
