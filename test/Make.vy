make {

   import { C; }

   init {
      $fn := "deshake";
      $fdc := "f0r_"+$fn+".c";
      $fdo := C.libFile( $fn );
      $dirs := ["..","../../vulcmp","../../vytools"];
      $libs := ["vulcmp","vulimg","vytools","m","vulkan"];
      C.set({incDir:$dirs, libDir:$dirs, lib:$libs, show:true});
      $purge := [$fdo];
   }
   
   target {
    
      build {
         genFre();
      }

      run {
         build();
         exec( runPath()+" ffplay -i shk.mkv -vf frei0r=libdeshake" );
//         exec( runPath()+" ffmpeg -i shk.mkv -t 1 -vf frei0r=libdeshake output_%04d.png" );
      }

      debug {
         build();
         exec( runPath()+" gdb --args ffplay -vf frei0r=libdeshake shk.mkv" );
      }
      
      clean {
         purge( $purge );
      }
   }

   function {
   
      genFre() {
         C.set( "libMode", true );
         C.build( $fdo, $fdc );
      }

      runPath() {
         lp := implode(":",$dirs);
         return "FREI0R_PATH=. LD_LIBRARY_PATH="+lp;
      }

   }

}
