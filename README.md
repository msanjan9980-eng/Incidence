# INCIDENCE

> **REFLECT OR DIE**

A top-down arena game where you have no gun - you have a normal vector.

You pilot **KITE-9**, a mirror deflector drone aboard Halcyon Station. Every shot fired at you is a shot you can throw back. Read the angles, hold the line, and clear ten sectors of the Fold before the Warden wakes.

---

## The pitch

Traditional top-down shooters give you a barrel and ask you to aim. INCIDENCE gives you a **rotating shield of five facets** and asks you to angle. Every projectile in the world - yours, theirs, the Warden's - obeys one rule: reflect off the surface it strikes. Kill an enemy by returning its own bullet.

The math is not decoration. It's the whole game.

---

## Features

- **Reflection combat** - five-facet mirror shield; each facet has its own normal vector, so glancing hits behave differently from flat ones
- **Dash** - short cooldown, big impulse. Dodge, reposition, line up a facet
- **Ten sectors + Warden** - each room has a distinct layout, enemy composition, and ambient visual field
- **Character progression** - stats (Arc, Focus, Charge, Drift), skills, equipment, inventory
- **Narrative layer** - maintenance logs from Yara Okonkwo, quest chain, in-world codex
- **Multi-profile saves** - create named profiles, resume any run, delete on command
- **Animated everything** - parallax starfields, chromatic titles, hovering drones, particle bursts on every hit, CRT post-processing, per-room ambient fields

---

## Controls

| Input | Action |
|---|---|
| `Arrow Keys` | Move |
| `Q` / `E` | Rotate shield |
| `Space` | Dash |
| `Esc` | Pause / back |
| `Enter` | Confirm |
| `Tab` | Character sheet |
| `K` | Skill tree |
| `I` | Equipment |
| `L` | Log archive |
| `J` | Quest log |
| `M` | World map |

Menu navigation: UP/DOWN to select, ENTER to confirm, ESC to back out.

---

## Building

### Requirements

- CMake 3.20+
- MinGW-w64 (GCC 11+) - tested on Windows
- Git (CMake fetches sokol via FetchContent)
- Ninja (optional, but faster than Make)

### Install prerequisites (Windows)

**Option 1 - winget (Windows 10/11, easiest):**

    winget install Kitware.CMake
    winget install Git.Git
    winget install BrechtSanders.WinLibs.POSIX.UCRT
    winget install Ninja-build.Ninja

Then restart your terminal. Verify with:

    cmake --version
    g++ --version
    git --version
    ninja --version

**Option 2 - MSYS2 (full Unix-like toolchain):**

Download and install MSYS2 from https://www.msys2.org/, then in the MSYS2 MINGW64 shell:

    pacman -Syu
    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja git

Add `C:\msys64\mingw64\bin` to your system PATH.

**Option 3 - manual (any Windows version):**

- CMake: https://cmake.org/download/ (Windows x64 Installer)
- MinGW-w64: https://winlibs.com/ (extract to `C:\mingw64`, add `C:\mingw64\bin` to PATH)
- Git: https://git-scm.com/download/win
- Ninja: https://github.com/ninja-build/ninja/releases (drop `ninja.exe` into `C:\mingw64\bin`)

### Install prerequisites (macOS)

    brew install cmake git
    xcode-select --install

### Install prerequisites (Linux)

Debian / Ubuntu:

    sudo apt update
    sudo apt install build-essential cmake git ninja-build

Arch:

    sudo pacman -S base-devel cmake git ninja

Fedora:

    sudo dnf install gcc-c++ cmake git ninja-build

### Build

    git clone https://github.com/msanjan9980-eng/Incidence.git
    cd Incidence
    cmake -S . -B build -G "MinGW Makefiles"
    cmake --build build --parallel
    ./build/incidence.exe

The game opens borderless fullscreen at your monitor's native resolution.

---

## Project structure

    src/
      main.cpp              sokol_app entry point, Win32 window setup
      core/                 math, vec2, color
      platform/             input translation (sokol_app events -> Key enum)
      render/
        renderer.hpp/cpp    batched 2D renderer
        font.hpp            5x7 bitmap font
        icons.hpp           heart, bullet, chevron, hex, enemy icons
        effects.hpp         screen shake, flash, hitstop
        fx.hpp              scanlines, vignette, grain, glitch, chromatic
        particles.hpp       pooled CPU particle system
        field.hpp           hex grid, perspective floor, constellation
        room_ambient.hpp    per-sector background composition
        drone_fx.hpp        reflect arc, facet ticks, aim dot, dash radial
        levelup_fx.hpp      level-up celebration banner
        hud_fx.hpp          bar, radial, frame, ability icon
        banner.hpp          room banner visual effects
      game/
        drone.hpp/cpp       player (KITE-9)
        turret.hpp/cpp      Lancer / Banker / Lead enemies
        projectile.hpp/cpp  bullet + wall bounce + reflect
        warden.hpp/cpp      three-phase boss
        room.hpp            room data, door layout, enemy spawns
        world.hpp/cpp       simulation, collisions, room transitions
        character.hpp/cpp   stats, level, skills
        skill.hpp, equipment.hpp, inventory.hpp
        quest.hpp, log.hpp  narrative systems
        save.hpp/cpp        text-format save/load
        profile.hpp/cpp     profiles.cfg manager, per-profile saves
        screen.hpp          top-level UI state enum
      app/
        app.hpp/cpp         main loop, screen dispatch
        app_menu.cpp        main menu
        app_profile.cpp     profile creation + selection
        app_intro.cpp       intro cards + chapter backgrounds
        app_hud.cpp         gameplay overlay
        app_panels.cpp      character sheet, skill tree, equipment, logs, map

---

## Architecture notes

**Virtual canvas.** Fixed 1920x1080 virtual coordinate space, scaled to the physical window.

**Batched renderer.** render::Renderer accumulates quads, lines, and circles into batched draw calls, flushing on layer change (Alpha / Additive). MAX_BATCHES = 512.

**Particles.** fx::ParticleSystem is a fixed-size pooled array. Two instances: menu_particles_ and game_particles_. No runtime allocations.

**CPU-only effects.** Every visual is drawn with the same primitives as gameplay. No shaders beyond the base textured-quad pipeline.

**Saves.** save_game/load_game write a text format. profile.hpp manages profiles.cfg and routes reads/writes to name.sav.

---

## Status

Playable end-to-end: main menu -> intro -> title -> ten sectors -> Warden -> victory.

---

## License

Personal project. All rights reserved.
