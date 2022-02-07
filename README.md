Updated version of Mark Grebe's [SDLTRS]:

  * Included all patches by [EtchedPixels]: banked memory support, Lowe LE18
  * Fixed various SegFaults: ROM Selection Menu, Scaling in Fullscreen
  * Reworked the TextGUI: new shortcuts and key bindings, help screen
  * Ctrl-A, Ctrl-C & Ctrl-V can now be used in the Emulator (CP/M & WordStar)
  * Display scanlines to simulate an old CRT monitor resolution
  * Access to real floppy disks works now on Linux
  * Tried to fix reported bugs to the original version
  * Port to SDL2 (see [BUILDING.md])
  * Support Exatron Stringy Floppy for TRS-80 Model I
  * Select and execute CMD files directly in the Emulator
  * Save screenshot of the Emulator window as BMP file
  * Show Z80 registers in the window title bar
  * Adjust speed of Z80 CPU on the fly
  * Emulate Z80 memory refresh register
  * Support Holmes Sprinter II/III speed-up kits for TRS-80 Model I/III
  * Change Z80 CPU default MHz of each TRS-80 Model
  * More accurate emulation of Z80 block instructions
  * Joystick emulation with Mouse
  * Support Prologica CP-300/500 16kB ROM with extra 2kB Z80 monitor
  * Support Seatronics Super Speed-Up Board for all TRS-80 Models
  * Support UART I/O ports for System 80
  * Load and Save TRS-80 memory in debugger
  * Patch Model I ROM to boot from hard disk drive
  * Support EACA EG 3200 (Genie III) system
  * 480x192 HRG resolution for LNW80 and TCS SpeedMaster/Genie IIs
  * CP/M banking support for TRS-80 Model I clones
  * Support EG-64 Memory-Banking-Adaptor from TCS
  * Support Lubomir Soft Banker for TRS-80 Model I
  * Emulation of the TCS Genie IIIs system
  * Support EG 64.1 memory expansion for TRS-80 Model I
  * Support Schmidtke 80-Z Video Card for TRS-80 Model I
  * Emulate EG 3210 Programmable Graphics Adaptor for Genie III
  * Support some David Keil's TRS-80 Emulator extensions

SDL(2)TRS is based on Tim Mann's excellent TRS-80 emulator [xtrs] and also
has very low system requirements: it works on all platforms supported by the
[SDL] library, even on machines with only a few hundred MHz of CPU speed.

## License

  [BSD 2-Clause](LICENSE)

## SDL2

This branch contains the SDL2 version with hardware rendering support.

This version is available in [RetroPie] since version 4.6.6 and Valerio
Lupi's fork of [RetroPie-Setup] ...

Thanks to Tércio Martins a package for Arch Linux is available in the [AUR].

## Screenshots

![screenshot](screenshots/sdltrs01.png)
![screenshot](screenshots/sdltrs02.png)
![screenshot](screenshots/sdltrs03.png)
![screenshot](screenshots/sdltrs04.png)

[AUR]: https://aur.archlinux.org/packages/sdl2trs/
[BUILDING.md]: BUILDING.md
[EtchedPixels]: https://www.github.com/EtchedPixels/xtrs
[RetroPie]: https://github.com/RetroPie
[RetroPie-Setup]: https://github.com/valerino/RetroPie-Setup
[SDL]: https://www.libsdl.org
[SDLTRS]: http://sdltrs.sourceforge.net
[xtrs]: https://www.tim-mann.org/xtrs.html
