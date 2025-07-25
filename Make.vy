make {

   init {
      $name := "vulimg";
      $ver := "20250317";
      $author := "Várnagy Zoltán";
      $ident := "https://github.com/Doi6doi/vulimg";

      $libs := ["vulcmp","vytools"];
      $plibs := regexp($libs,"#.+#","\\0p");
      $dirs := regexp( $libs, "#.+#", "../\\0" );

      $C := tool("C",{ incDir:$dirs, libMode:true, libDir:$dirs, lib:$libs+["m"],
         show:true } );
      $Cpp := tool("Cpp", {incDir:$dirs, libMode:true, libDir:$dirs, lib:$plibs,
         show:true } );
      $Glsl := tool("Glsl");

      $gs := ["copy1","copy32","join3","plane3","trans","diff","dsum","add8",
         "hist8","pyr","delta8","rect","fill","white8","wcloud8"];
      $cs := ["vulimg.c","draw.c","white.c","pyr.c"];
      $cps := ["vulimgp.cpp"];
      $vh := ["vulimg_comp.h"];
      $hs := [$vh,"vulimg.h","vulimg_impl.h"];

      $cdep := "c.dep";
      $pdep := "p.dep";
      $os := changeExt( $cs, $C.objExt() );
      $ccs := regexp( $cs, "#(.*)\\.c#", "p_\\1.cpp" );
       $clib := $C.libFile( $name );
      $plib := $Cpp.libFile( $name+"p" );
      $purge := [ $cdep, $pdep, "*.so","*.dll","*.lib","*.exp"] 
         + changeExt($gs,".inc") + changeExt($gs,".spv");
   }

   target {
      
      menu {
         Dlg := tool("Dialog");
         m := Dlg.menu("VulImg")
            .item("VulImg is a C and C++ library of"
             +" low-level image manipulation with GPU through vulkan")
            .item("Build libraries",build)
            .item("Clean generated files",clean)
            .item("Test libraries",test)
            .item("Build documentation",docs);
         case (system()) {
            "Linux": m.item("Create Debian package",deb);
            "Windows": m.item("Create Windows zip", wzip);
         }
         m.exec();
      }

      build {
         genShd();
         genCcs();
         genDeps();
         genObjs();
         genLibs();
      }
      
      clean { purge( $purge ); }

      docs { make("docs"); }

      test { make("test"); }

      deb {
         build();
         makeDeb();
      }

      wzip {
         build();
         makeWZip();
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
               $Glsl.compile( s, g );
               $Cpp.sourceRes( i, s, x );
            }
         }
      }

      /// generate copy of .c files for c++ compiling
      genCcs() {
         foreach ( c | $cs ) {
            cc := regexp( c, "#(.*)\\.c#", "p_\\1.cpp" );
            if ( older( cc, c ))
               copy( c, cc );
         }
      }
   
      /// generate dependency files
      genDeps() {
         gis := changeExt( $gs, ".inc" );
         if ( older( $cdep, $cs+$hs+gis ) )
            $C.depend( $cdep, $cs );
         if ( older( $pdep, $ccs+$cps+gis ) )
            $Cpp.depend( $pdep, $ccs+$cps );
      }

      /// generate object files
      genObjs() {
         ds := $C.loadDep( $cdep );
         foreach ( c | $cs ) {
            o := changeExt( c, $C.objExt() );
            if ( older( o, ds[o] ))
               $C.compile( o, c );
         }
         ds := $Cpp.loadDep( $pdep );
         foreach ( c | $ccs + $cps ) {
            o := changeExt( c, $Cpp.objExt() );
            if ( older( o, ds[o] ))
               $Cpp.compile( o, c );
         }
      }

      /// Generate libraries
      genLibs() {
         os := changeExt( $cs, $C.objExt() );
         if ( older( $clib, os ))
            $C.link( $clib, os );
         pos := changeExt( $cps+$ccs, $Cpp.objExt() );
         if ( older( $plib, pos ))
            $Cpp.link( $plib, pos );
      }
   }

}
