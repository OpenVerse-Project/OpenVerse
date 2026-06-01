# OpenVerse Engine — Agent Instructions

OpenVerse Engine is a next-generation open-source game engine, transformed from O3DE (Open 3D Engine).
Located at `/home/dataline/openverse/`.

## Quick Start

```bash
cd /home/dataline/openverse

# Show engine info
./scripts/openverse.sh info

# Build all new OpenVerse modules (standalone)
cmake -B build-ov -S Source -G Ninja -DCMAKE_BUILD_TYPE=Profile -DCMAKE_CXX_STANDARD=20
cmake --build build-ov -j$(nproc)

# Build full engine (includes O3DE Code + new Source)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Profile
cmake --build build -j$(nproc) --target ov_core ov_renderer ov_ai ov_editor ov_physics ov_networking ov_audio

# Python tools
python3 scripts/openverse_core.py info
python3 scripts/openverse_core.py build
```

## Architecture

### New Engine (Source/)
```
Source/
├── CMakeLists.txt
├── Core/           # ov_core: ECS, Memory, Jobs, Math, Reflect, Assets
│   ├── Core.h/cpp  # Platform detection, asserts, time, threading
│   ├── Compat.h    # AZ ⇄ OV bridge for gradual migration
│   ├── ECS/        # Archetype-based SoA entity component system
│   ├── Memory/     # Pool/Buddy/Frame allocators, GPU memory
│   ├── Jobs/       # Fiber work-stealing job system
│   ├── Math/       # Vec2/3/4, Quat, Mat4, header-only
│   ├── Reflect/    # TypeRegistry, property editing, OV_REFLECT macros
│   └── Assets/     # AssetID (128-bit), database, streaming
│
├── RHI/            # ov_rhi: Render Hardware Interface (Vulkan/DX12/Metal)
├── Renderer/       # ov_renderer: GPU Scene, Frame Graph, PBR
│   └── Shaders/    # GLSL compute shaders → SPIR-V
├── AI/             # ov_ai: Orchestrator, NPC Brain, World/Asset Gen
├── Editor/         # ov_editor: Dock, Command Palette, Search, Themes
├── Physics/        # ov_physics: RigidBody, Fluids, Crowds, Vehicles
├── Networking/     # ov_networking: Replication, Lobby, Dedicated Server
├── Audio/          # ov_audio: 3D Audio + Animation (Skeleton, Graph, IK)
└── Animation/      # (merged into Audio/)
```

### Legacy Engine (Code/)
The existing O3DE codebase at `Code/` remains fully functional with the rebranding:
- `engine.json` → `engine_name: "openverse"`
- 83 Gems under `Gems/`
- Editor, Launcher, Asset Pipeline — all rebranded to "OpenVerse"

### Build Targets

| Target | Type | Description |
|--------|------|-------------|
| `ov_core` | STATIC | Core framework (1.7 MB compiled) |
| `ov_rhi` | INTERFACE | RHI abstraction header |
| `ov_renderer` | STATIC | Renderer + shaders (309 KB) |
| `ov_ai` | STATIC | AI systems (1.5 MB) |
| `ov_editor` | STATIC | Editor framework (1.1 MB) |
| `ov_physics` | STATIC | Physics simulation (333 KB) |
| `ov_networking` | STATIC | Network stack (382 KB) |
| `ov_audio` | STATIC | Audio + Animation (403 KB) |

## Build Verification

```bash
# Quick: build all new modules
cmake --build build-ov -j$(nproc)

# Full: build within engine context
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Profile
cmake --build build -j$(nproc) --target ov_core ov_renderer ov_ai ov_editor ov_physics ov_networking ov_audio

# Run Python verification
python3 scripts/openverse_core.py info
```

## Key Files

| File | Purpose |
|------|---------|
| `engine.json` | Engine identity (engine_name: "openverse", version: "2.7.0") |
| `CMakeLists.txt` | Main engine build (includes Code/ + Source/) |
| `scripts/openverse.sh` | Launcher script (create, build, launch projects) |
| `scripts/openverse_core.py` | Python bindings and CLI tools |
| `cmake/FindOpenVerse.cmake` | CMake find module for projects |
| `Code/` | Legacy O3DE code (rebranded) |
| `Source/` | New OpenVerse modules |
| `Gems/` | 83 engine Gems |
| `Templates/` | 16 project/gem templates |

## Development Flow

1. **New engine code** goes in `Source/` with `ov_` prefixed namespaces
2. **Legacy code** remains in `Code/` with `AZ::` namespaces
3. **Bridge** via `Source/Core/Compat.h` for gradual migration
4. **Build** with CMake + Ninja (Profile for dev, Release for dist)
5. **Test** with `scripts/openverse.sh test`

## Rebranding Summary

- `engine.json`: `engine_name: "openverse"`
- Applications: "OpenVerse Editor", "OpenVerse Launcher", "OpenVerse Asset Pipeline"
- Settings: `/OpenVerse/` (was `/O3DE/`)
- User dir: `~/.openverse/` (was `~/.o3de/`)
- Macros: `OPENVERSE_GEM_NAME` (was `O3DE_GEM_NAME`)
- Classes: `OpenVerseStylesheet` (was `O3DEStylesheet`)
- Archive: `OVAR` (was `O3AR`)
- Copyright: "OpenVerse Engine Project" (was "Open 3D Engine")
- 19,556 files modified across all phases
