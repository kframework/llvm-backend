{
  lib, cleanSourceWith, src,
  cmake, flex, pkgconfig,
  llvmPackages,
  boost, gmp, jemalloc, libffi, libiconv, libyaml, mpfr,
  # Runtime dependencies:
  host,
}:

let inherit (llvmPackages) stdenv llvm; in

let
  pname = "llvm-backend";
  version = "0";
in

stdenv.mkDerivation {
  inherit pname version;
  src =
    # Avoid spurious rebuilds by ignoring files that don't affect the build.
    cleanSourceWith {
      name = "llvm-backend-src";
      inherit src;
      ignore =
        [
          "/nix" "*.nix" "*.nix.sh"
          "/.github"
          "/matching"
        ];
    };

  nativeBuildInputs = [ cmake flex llvm pkgconfig ];
  buildInputs = [ boost libyaml ];
  propagatedBuildInputs =
    [ gmp jemalloc libffi mpfr ]
    ++ lib.optional stdenv.isDarwin libiconv;

  cmakeFlags = [
    ''-DCMAKE_C_COMPILER=${lib.getBin stdenv.cc}/bin/cc''
    ''-DCMAKE_CXX_COMPILER=${lib.getBin stdenv.cc}/bin/c++''
    ''-DUSE_NIX=TRUE''
    ''-DCMAKE_SKIP_BUILD_RPATH=FALSE''
  ];

  cmakeBuildType = "FastBuild";

  NIX_CFLAGS_COMPILE = [ "-Wno-error" ];

  doCheck = true;
  checkPhase = ''
    runHook preCheck

    (
      # Allow linking to paths outside the Nix store.
      # Primarily, this allows linking to paths in the build tree.
      # The setting is only applied to the unit tests, which are not installed.
      export NIX_ENFORCE_PURITY=0
      make run-unittests
    )

    runHook postCheck
  '';

  passthru = {
    inherit (host) clang;
  };
}
