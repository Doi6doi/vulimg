make {

   init {
      $fn := "deshake";

      $dirs := ["..","../../vulcmp","../../vytools"];
      $libs := ["vulcmp","vulimg","vytools","m","vulkan"];

      $C := tool("C", {incDir:$dirs, libDir:$dirs, lib:$libs, show:true});

      $fdc := "f0r_"+$fn+".c";
      $fdo := $C.libFile( $fn );

      $purge := [$fdo];
   }
   
   target {
    
      build {
         genFre();
      }

      run {
         build();
         exec( runPath()+" ffplay -i shk.mkv -vf \"frei0r=libdeshake:filter_params=0.9|0.1\"" );
//         exec( runPath()+" ffmpeg -t 1 -ss 18 -i shk.mkv -t 1 -vf \"frei0r=libdeshake:filter_params=0.9|0.1\" shk/output_%04d.png" );
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
         $C.set( "libMode", true );
         $C.build( $fdo, $fdc );
      }

      runPath() {
         lp := implode(":",$dirs);
         return "FREI0R_PATH=. LD_LIBRARY_PATH="+lp;
      }

   }

}
