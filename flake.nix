{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            meson
            ninja
            pkg-config
            cmake
          ];

          buildInputs = with pkgs; [
            boost
            ffmpeg
            openssl
            SDL2
            openxr-loader
            zlib
          ];
        };
      }
    );
}
