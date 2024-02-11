let
    pkgs = import <nixpkgs> {};
in
    pkgs.stdenv.mkDerivation {
    name = "raxml-ng-repr";
    version = "0.0.1";
    builder = "${pkgs.bash}/bin/bash";
    args = [ ./test-build.sh ];
    buildInputs = with pkgs; [ coreutils gnumake gcc cmake mpi bison flex gmp git gtest gbenchmark openssl libdwarf pkg-config];
    system = builtins.currentSystem;
    src = ./.;

}
