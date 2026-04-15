{ pkgs ? import <nixpkgs> { } }:

let
  pythonEnv = pkgs.python3.withPackages (ps: with ps; [
    appdirs
    argcomplete
    colorama
    dotty-dict
    hid
    hjson
    jsonschema
    milc
    pygments
    pyserial
    pyusb
    pillow
  ]);
in
pkgs.mkShell {
  name = "qmk-firmware";

  buildInputs = with pkgs; [
    qmk
    pythonEnv
    gcc-arm-embedded
    dfu-util
    dfu-programmer
    git
    gnumake
  ];

  shellHook = ''
    # Prevent the avr-gcc wrapper from picking up host GCC flags
    unset NIX_CFLAGS_COMPILE_FOR_TARGET
  '';
}
