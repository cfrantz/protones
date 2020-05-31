# ProtoNES in Rust

A NES emulator based on Fogelman's NES.

Includes an ImGui-based user interface.

## TODO

### Basic Functionality

- Improve framerate stability.
- Save SRAM to disk.
- Savestates.
- State history (e.g. rewind).
- Play back FM2 movies.
- Add MMC3 mapper.
- Add MMC5 mapper.
- Add other mappers as needed.

### User Interface

- Add keyboard input.
- Load preferences as rusty object notation from a file.

### Debug Functionality

- Improve PPU debug to colorize (like the C++ version).
- Memory Hexdump.
- Disassembly.
- CPU single stepping.
- Load user-supplied address/symbol definitions.

### Scripting

- Investigate adding Python support.

### Idiomatic Rust

- Refactor APU: Trait interfaces.
- Clean up in gui/app.
- Get a code review from someone knowledgable about Rust.

### Build

- Figure out cross-platform builds.
  - Linux
  - Windows
  - MacOS
