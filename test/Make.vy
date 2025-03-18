make {

   import { C; }

   init {
      $fn := "none";
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
      
      clean {
         purge( $purge );
      }
   }

   function {
   
      genFre() {
         C.set( "libMode", true );
         C.build( $fdo, $fdc );
      }
   }

}
