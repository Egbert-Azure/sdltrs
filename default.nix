{ stdenv
, lib
, fetchFromGitLab
, cmake
, readline
, SDL2 }:

with lib;
stdenv.mkDerivation rec {
  pname   = "sdl2trs";
  version = "1.2.20";

  src = fetchFromGitLab {
    owner  = "jengun";
    repo   = "sdltrs";
    rev    = "f00f6d6bec5dd9709ba817df4082e401a5882578";
    sha256 = "08yib9psiycvqfv98nrmla3kc7vgba3zjqi2mn5zlwzcpz3h36x6";
  };

  nativeBuildInputs = [ cmake ];
  buildInputs       = [ readline SDL2 ];

  meta = {
    homepage        = "https://gitlab.con/jengun/sdltrs/";
    description     = "TRS-80 Model I/III/4/4P Emulator for SDL2 (hardware rendering)";
    longDescription = ''
      SDL2TRS is an SDL2-based emulator for the Tandy/Radio Shack line of Zilog
      Z80-based microcomputers popular in the late 1970s and early 1980s.  It
      features cassette, floppy, and hard drive emulation, timer interrupt
      emulation, file import and export from the host operating system, support
      for most of the undocumented Z80 instructions, and a built-in debugger.
      Real floppy drives can be used and application-based sound can be
      played and real cassettes read and written directly through the
      sound card or via WAVE files.  Several Hi-Res graphics cards are
      emulated and, in Model 4/4P mode, mice are supported.  There is also
      real-time clock, sound card, serial port, joystick, and CPU clock
      speedup emulation.
      SDL2TRS requires ROM images from the original machines.  The ROMs are
      copyrighted by Radio Shack and are not freely licensed.  (Exception: in
      Model 4P mode, a freely-licensed boot ROM included with this package can be
      used to boot a Model 4 operating system from a diskette image.)
    '';
    license     = licenses.bsd2;
    platforms   = lib.platforms.all;
  };
}
