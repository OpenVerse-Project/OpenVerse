# OpenVerse UI Framework (OVUI) — Architecture Master Document v1.0

> **Status**: IN PROGRESS — Vulkan Render Backend complete, OVUI-36 Design Surface, OVUI-37 next
> **Root**: `/home/dataline/openverse/Frameworks/OVUI/`
> **Target**: Next-generation UI framework independent of engine, 10-year horizon

---

## 1.0 WHY OVUI: LIMITATIONS OF EXISTING FRAMEWORKS

### Qt 6
- **Problem**: QPainter/QML dual architecture adds ~40MB baseline per process
- **Problem**: QObject meta-object system forces MOC preprocessor — reflection at compile-time only
- **Problem**: QML JavaScript engine (V4) is single-threaded, interpreter-bound
- **Problem**: Scene graph is CPU-assembled, GPU only executes the draw list
- **Problem**: Style sheets are string-parsed every frame for dynamic changes
- **Problem**: No virtual scrolling — QListView creates widgets for invisible items

### Flutter
- **Problem**: Dart's garbage collector introduces unpredictable frame spikes (10-50ms)
- **Problem**: Impeller renderer is still maturing — Skia fallback is CPU-bound
- **Problem**: Widget rebuild is O(tree) even for single-property changes
- **Problem**: Layout is single-threaded, cannot exploit multi-core
- **Problem**: No native OS integration for menu bars, file dialogs, accessibility on desktop

### WPF / Avalonia
- **Problem**: XAML parsing at startup loads entire tree — cold start 500ms+
- **Problem**: DependencyProperty system uses hash tables for every get/set
- **Problem**: Visual tree is double-linked — every layout pass walks the entire tree
- **Problem**: GPU rendering via DirectX only, no Vulkan/Metal path
- **Problem**: Styling cascade recalculates on every InvalidateVisual()

### SwiftUI / Jetpack Compose
- **Problem**: Diffing algorithm is O(n²) in pathological cases (large lists)
- **Problem**: Compose compiler plugin ties to Kotlin version — no C++ equivalent
- **Problem**: Both are single-platform (Apple/Android) with limited desktop support
- **Problem**: State hoisting forces entire hierarchy re-composition on small changes

### Unreal Slate
- **Problem**: Declarative syntax is macro-heavy, unreadable, compile-time only
- **Problem**: No data binding — manual polling in Tick()
- **Problem**: Font rendering is CPU glyph atlas, no SDF path
- **Problem**: Widget pooling is manual (SWidgetPool), not automatic
- **Problem**: Style is compile-time FSlateStyleSet, no runtime cascade

### Common Flaws Across All
| Flaw | Qt | Flutter | WPF | SwiftUI | Slate |
|------|----|--------|-----|---------|-------|
| Widget tree → full rebuild on change | Yes | Yes | Yes | Yes | No |
| Layout single-threaded | Yes | Yes | Yes | Yes | Yes |
| GPU only executes draw, not compute layout | Yes | Yes | Yes | Yes | Yes |
| Style parse at runtime | Yes | No | Yes | No | No |
| Reflection at runtime only | No | No | No | No | No |
| Virtual scrolling built-in | No | Yes | Yes | No | Yes |
| Multi-backend GPU | Yes | Yes | No | Yes | No |

---

## 2.0 OVUI DIRECTORY STRUCTURE

```
Frameworks/OVUI/
├── README.md                       — Architecture overview
├── CMakeLists.txt                   — Root build: aggregates all ovui_* libraries
│
├── Core/                            — ovui_core (no dependencies)
│   ├── CMakeLists.txt
│   ├── Types.h                      — Rect, Size, Point, Color, EdgeInsets
│   ├── Geometry.h                   — Transform2D, Matrix3x3, HitTest
│   ├── ID.h                         — WidgetID (64-bit), LayerID, AnimationID
│   ├── Allocator.h                  — UI-specific allocator (arena-based)
│   └── Platform.h                   — OS detection, native handle wrappers
│
├── Runtime/                         — ovui_runtime (→ Core)
│   ├── CMakeLists.txt
│   ├── Scheduler.h                  — Frame scheduler, phase ordering
│   ├── Profiler.h                   — GPU/CPU timing per phase
│   ├── MemoryPool.h                 — Widget memory pool (chunk-based)
│   └── CommandBuffer.h              — UI command recording (retained + immediate)
│
├── Reactive/                        — ovui_reactive (→ Core, Runtime)
│   ├── CMakeLists.txt
│   ├── Observable.h                 — Observable<T>, computed<T>, subscription
│   ├── Binding.h                    — Two-way binding, transformers, validators
│   ├── DiffEngine.h                 — Tree diffing, property diffing, O(n) algorithm
│   └── StateStore.h                 — Centralized state container (Redux-like)
│
├── Graphics/                        — ovui_graphics (→ Core, RHI abstraction)
│   ├── CMakeLists.txt
│   ├── PaintEngine.h               — Vector paint (paths, fills, strokes, gradients)
│   ├── TextEngine.h                — SDF text atlas, glyph cache, shaping
│   ├── RenderPass.h                — UI render pass (FrameGraph integration)
│   ├── LayerCompositor.h           — Layer tree → composited output
│   ├── ShaderLibrary.h             — UI shaders (SDF, blur, shadow, clip)
│   └── Backend/
│       ├── Vulkan.h                 — Vulkan UI backend
│       ├── DX12.h                   — DirectX 12 UI backend
│       └── Metal.h                  — Metal UI backend
│
├── Layout/                          — ovui_layout (→ Core)
│   ├── CMakeLists.txt
│   ├── FlexEngine.h                — CSS flexbox implementation (GPU-compatible)
│   ├── GridEngine.h                — CSS grid implementation
│   ├── DockingEngine.h             — Multi-window, multi-monitor docking
│   ├── ResponsiveEngine.h          — Breakpoint-based responsive layouts
│   └── Virtualization.h            — Viewport-based widget virtualization
│
├── Styling/                         — ovui_styling (→ Core, Reactive, Graphics)
│   ├── CMakeLists.txt
│   ├── StyleEngine.h               — Quantum styling engine core
│   ├── SelectorEngine.h            — Selector matching (class, id, type, state)
│   ├── PropertyRegistry.h          — Animatable style properties
│   ├── CascadeSolver.h             — Specificity-based cascade resolution
│   ├── ThemeLoader.h               — OVSS (OVUI Style Sheet) loader
│   └── CompiledStyles.h            — Pre-compiled style tables (zero runtime parse)
│
├── Animation/                       — ovui_animation (→ Core, Reactive)
│   ├── CMakeLists.txt
│   ├── Timeline.h                   — Keyframe timeline engine
│   ├── SpringPhysics.h              — Spring-based animation solver
│   ├── Curve.h                      — Easing curves (cubic bezier, elastic, bounce)
│   ├── GPUAnimation.h              — GPU-computed animation (compute shader)
│   └── TransitionEngine.h          — Widget enter/exit transitions
│
├── Widgets/                         — ovui_widgets (→ all above)
│   ├── CMakeLists.txt
│   ├── Widget.h                     — Base widget class
│   ├── Container.h                  — Parent widget with children
│   ├── Primitives/
│   │   ├── Box.h                    — Rectangle with background/border/shadow
│   │   ├── Text.h                   — Rich text with SDF rendering
│   │   ├── Image.h                  — GPU texture-backed image
│   │   └── Canvas.h                 — Immediate-mode drawing surface
│   ├── Controls/
│   │   ├── Button.h                 — Push button with states
│   │   ├── Slider.h                 — Range slider (H/V)
│   │   ├── Checkbox.h               — Toggle with label
│   │   ├── RadioButton.h            — Radio group
│   │   ├── TextInput.h              — Text field with cursor, selection, IME
│   │   ├── ComboBox.h               — Dropdown selector
│   │   ├── SpinBox.h                — Numeric input with arrows
│   │   └── ColorPicker.h            — HSV/RGB/HEX color picker
│   ├── Containers/
│   │   ├── Window.h                 — OS window or embedded window
│   │   ├── Panel.h                  — Collapsible panel with title bar
│   │   ├── ScrollArea.h             — Virtualized scrollable area
│   │   ├── Splitter.h               — Resizable split pane
│   │   ├── TabView.h                — Tabbed container
│   │   └── StackView.h              — Stack/navigation container
│   ├── Lists/
│   │   ├── ListView.h               — Virtual scrolling list (1M items)
│   │   ├── TreeView.h               — Hierarchical tree with expand/collapse
│   │   ├── TableView.h              — Virtual grid/table (1M cells)
│   │   └── CollectionView.h         — Heterogeneous collection layout
│   ├── Menus/
│   │   ├── MenuBar.h                — Application menu bar
│   │   ├── ContextMenu.h            — Right-click context menu
│   │   └── PopupMenu.h              — Floating popup
│   └── Dialogs/
│       ├── Dialog.h                 — Modal/non-modal dialog
│       ├── FileDialog.h             — Native file open/save
│       ├── MessageDialog.h          — Alert/confirm/prompt
│       └── ProgressDialog.h         — Progress bar dialog
│
├── Native/                          — ovui_native (→ Core, platform SDKs)
│   ├── CMakeLists.txt
│   ├── NativeWindow.h               — Platform window abstraction
│   ├── NativeMenu.h                 — OS menu bar integration
│   ├── NativeDialog.h               — OS file/open/save dialogs
│   ├── NativeClipboard.h            — System clipboard
│   ├── NativeDragDrop.h             — OS drag and drop
│   └── Platform/
│       ├── Linux.h                   — X11/Wayland backend
│       ├── Windows.h                 — Win32 backend
│       └── macOS.h                   — Cocoa backend
│
├── Accessibility/                   — ovui_accessibility (→ Core, Widgets)
│   ├── CMakeLists.txt
│   ├── AccessibilityTree.h          — Accessible object hierarchy
│   ├── ScreenReader.h               — Screen reader bridge
│   ├── HighContrast.h               — High contrast theme
│   ├── KeyboardNavigation.h         — Full keyboard operability
│   └── Platform/
│       ├── ATK.h                     — Linux ATK/AT-SPI bridge
│       ├── UIA.h                     — Windows UI Automation bridge
│       └── NSAccessibility.h         — macOS accessibility bridge
│
├── Assets/                          — ovui_assets (→ Core)
│   ├── CMakeLists.txt
│   ├── FontManager.h                — Font loading, caching, fallback chains
│   ├── ImageLoader.h                — PNG, SVG, JPEG, WebP loading
│   ├── IconRegistry.h               — Named icon registry
│   └── AssetPipeline.h              — Asset compilation for OVUI
│
├── Compiler/                        — ovui_compiler (standalone tool)
│   ├── CMakeLists.txt
│   ├── Parser.h                     — OVML/OVSS lexer + parser (SIMD-accelerated)
│   ├── AST.h                        — Abstract Syntax Tree nodes
│   ├── Validator.h                  — Semantic validation (types, references)
│   ├── Optimizer.h                  — Tree shaking, constant folding, dead code elimination
│   ├── CodeGen.h                    — Bytecode generator
│   ├── Bytecode.h                   — OVUI Bytecode format specification
│   ├── HotReload.h                  — Differential compilation for live reload
│   └── ovuic.cpp                    — CLI tool entry point
│
├── Designer/                        — ovui_designer (standalone app)
│   ├── CMakeLists.txt
│   ├── DesignSurface.h              — Visual editor canvas
│   ├── PropertyInspector.h          — Reflection-based property editor
│   ├── WidgetPalette.h              — Drag-and-drop widget library
│   ├── LivePreview.h                — Real-time preview with hot reload
│   ├── AnimationTimeline.h          — Keyframe animation editor
│   ├── ThemeEditor.h                — Visual theme/style editor
│   └── Profiler.h                   — GPU/CPU frame profiler overlay
│
├── Testing/                         — ovui_testing (→ all)
│   ├── CMakeLists.txt
│   ├── TestHarness.h                — Automated UI testing framework
│   ├── ScreenshotComparator.h       — Pixel-perfect regression testing
│   ├── PerformanceBench.h           — Layout/style/render benchmarks
│   └── Fuzzer.h                     — Input fuzzing for robustness
│
├── Documentation/                   — Architecture docs, API reference, guides
│   ├── Architecture.md
│   ├── API_Reference.md
│   ├── OVML_Specification.md
│   ├── OVSS_Specification.md
│   ├── Backend_Guide.md
│   └── Migration_Guide.md
│
└── Samples/                         — Example applications
    ├── Counter/                      — Minimal reactive counter
    ├── TodoList/                     — CRUD with data binding
    ├── Dashboard/                    — Complex multi-panel layout
    ├── Editor/                       — Text editor with menus
    └── Gallery/                      — Widget showcase (all controls)
```

---

## 3.0 REVOLUTIONARY TECHNOLOGY SYSTEMS

### 3.1 Reactive Scene Graph (RSG)

**Concept**: Replace the classic widget tree with a directed acyclic graph (DAG) where nodes are "reactive cells" — each cell computes its output from inputs. When an input changes, only downstream cells are recomputed. No tree walk. No full rebuild.

**Architecture**:
```
┌─────────────────────────────────────────────────────┐
│                  Reactive Scene Graph                │
│                                                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐       │
│  │ State<T> │───▶│ Computed │───▶│ Widget   │       │
│  │ (source) │    │ <T>      │    │ (sink)   │       │
│  └──────────┘    └──────────┘    └──────────┘       │
│       │                │               │             │
│       ▼                ▼               ▼             │
│  ┌──────────────────────────────────────────┐       │
│  │         Dependency Tracker (DAG)          │       │
│  │  ┌───────┐  ┌───────┐  ┌───────────┐    │       │
│  │  │ Dirty  │  │ Topo   │  │ Thread    │    │       │
│  │  │ Flag   │  │ Sort   │  │ Affinity  │    │       │
│  │  └───────┘  └───────┘  └───────────┘    │       │
│  └──────────────────────────────────────────┘       │
│                                                      │
│  Update algorithm: O(dirty_nodes) per frame          │
│  Not O(tree_size)                                    │
└─────────────────────────────────────────────────────┘
```

**Key innovations**:
- Graph not tree: widgets can share computed values (no prop drilling)
- Dependency tracking via atomic version counters
- Invalidated nodes batched, topologically sorted, executed on thread pool
- State<T> is the single source of truth — widgets are pure sinks
- Diff before commit: only changed pixels are sent to GPU

**State<T> API**:
```cpp
template<typename T>
class State {
    T value;
    uint64_t version;  // atomic, incremented on set
    std::vector<Subscriber*> subscribers;  // intrusive list
public:
    const T& get() const;
    void set(const T& v);         // triggers invalidation
    State<T> map(auto fn);        // creates Computed<T> edge
    State<bool> equals(T target); // creates Computed<bool>
};
```

### 3.2 GPU Layout Engine (GLE)

**Concept**: Layout computation (flexbox, grid) is a parallel reduction problem. Execute it on GPU via compute shaders. CPU only assembles the layout tree structure; GPU resolves positions and sizes.

**Architecture**:
```
┌──────────────────────────────────────────────────────┐
│                  GPU Layout Pipeline                   │
│                                                       │
│  CPU: Build layout tree (directed graph of BoxNodes)  │
│  │                                                    │
│  ▼                                                    │
│  GPU: Upload tree as SSBO (struct-of-arrays layout)   │
│  │                                                    │
│  ▼                                                    │
│  GPU Compute Pass 1: Measure pass                     │
│    - Each widget computes intrinsic size from content  │
│    - Parallel per-leaf, reduction for containers       │
│    - Output: desired_size per node                     │
│  │                                                    │
│  ▼                                                    │
│  GPU Compute Pass 2: Flex allocation                  │
│    - Main axis: distribute space by flex-grow          │
│    - Cross axis: align items by align-items            │
│    - Each container is 1 workgroup, children parallel  │
│    - Output: final_rect per node                       │
│  │                                                    │
│  ▼                                                    │
│  GPU Compute Pass 3: Virtualization                   │
│    - Sample viewport → determine visible widgets       │
│    - Output: visibility_bitmask, culled list           │
│  │                                                    │
│  ▼                                                    │
│  CPU: Read back layout results + visibility           │
│    (only for changed nodes, incremental)               │
└──────────────────────────────────────────────────────┘
```

**Performance target**:
- 1 million widgets: layout in <1ms (GPU compute)
- CPU cost: O(changed_widgets), not O(total_widgets)
- Memory: 64 bytes per widget in SSBO (64MB for 1M widgets)

### 3.3 Quantum Styling System (QSS)

**Concept**: CSS is inherently incompatible with GPU execution due to string parsing, specificity computation as string comparison, and cascade walking. QSS replaces CSS with a compiled, integer-indexed, GPU-accessible style system.

**OVSS (OVUI Style Sheet) Language**:
```
// OVSS: compiled at build time, zero runtime parsing cost
// Selectors are integer IDs, properties are typed

theme "OpenVerse Dark" {
    palette {
        primary: #4FC3F7;
        surface: #1E1E2E;
        text: #CDD6F4;
        error: #F38BA8;
    }

    widget "Button" {
        background: palette.surface;
        color: palette.text;
        border-radius: 6px;
        padding: {8, 16, 8, 16};

        state hover {
            background: palette.surface.lighten(10%);
        }

        state pressed {
            background: palette.primary;
            transform: scale(0.97);
        }

        state disabled {
            opacity: 0.4;
        }

        animation {
            duration: 150ms;
            curve: ease-out;
        }
    }

    widget "Slider" {
        track-color: palette.surface.lighten(20%);
        thumb-color: palette.primary;
        track-height: 4px;
        thumb-size: 16px;
    }
}
```

**Compilation pipeline**:
```
OVSS source
    │
    ▼
ovssc (OVSS Compiler)
    │
    ├── Parser: OVSS → AST
    ├── Resolver: resolve palette references, compute derived colors
    ├── Flattener: resolve state combinations → flat property tables
    ├── Optimizer: deduplicate, constant-propagate, dead-code-eliminate
    ├── CodeGen: emit binary style tables
    │
    ▼
Compiled Style Table (CST) — binary blob:
    [Header: magic, version, widget_count, property_count]
    [WidgetTable: widget_id → property_offset]
    [StateTable: state_mask → property_variant_offset]
    [PropertyData: key → GPU-friendly float4/int4 values]
    [AnimationData: curve_type, duration, delay per property]
```

**Runtime resolution** (GPU-friendly):
```cpp
// Resolved in 3 integer lookups, 0 strings:
float4 bg_color = style_table[widget_id][state_mask].background;
// GPU: can read directly from SSBO — no CPU involvement
```

### 3.4 OVML Compiler (OVUI Markup Language)

**Concept**: Declarative UI description compiled to bytecode. No runtime reflection. No string parsing during layout. Hot-reloadable via differential compilation.

**OVML example**:
```xml
<Window title="My App" width="800" height="600">
    <VBox spacing="8" padding="16">
        <HBox spacing="4">
            <TextInput bind:text="searchQuery" placeholder="Search..."/>
            <Button text="Go" on:click="performSearch"/>
        </HBox>
        <ScrollArea flex="1">
            <ListView bind:items="results" delegate="resultCard"/>
        </ScrollArea>
        <HBox halign="right">
            <Text text="Status: {statusText}" color="secondary"/>
        </HBox>
    </VBox>
</Window>
```

**Compilation pipeline**:
```
OVML source          OVSS source
    │                    │
    ▼                    ▼
ovmlc (OVML Compiler)
    │
    ├── Lexer (SIMD-accelerated): OVML text → token stream
    ├── Parser (recursive descent): tokens → AST
    │       WindowNode
    │       ├── VBoxNode
    │       │   ├── HBoxNode
    │       │   │   ├── TextInputNode { bind:text = slot(0) }
    │       │   │   └── ButtonNode { on:click = fn(1) }
    │       │   ├── ScrollAreaNode
    │       │   │   └── ListViewNode { bind:items = slot(1), delegate = slot(2) }
    │       │   └── HBoxNode
    │       │       └── TextNode { text = expr(0) }
    ├── Resolver: identify bindings, expressions, component references
    ├── Optimizer: constant fold, tree shake, inline constants
    │       - Inline static layouts (no runtime overhead)
    │       - Identify virtualizable lists (pre-configure pooling)
    │       - Pre-compute style selectors (map to CST indices)
    ├── CodeGen: AST → OVUI Bytecode
    │
    ▼
OVUI Bytecode (ovbc) — binary:
    [Header: magic "OVBC", version, entry_point]
    [String Table: all string constants]
    [Widget Graph: serialized tree with typed nodes]
    [Binding Table: state → widget property map]
    [Event Table: event → handler slot map]
    [Style Refs: widget → CST index mappings]
    [Layout Hints: flex constraints, min/max sizes]

Runtime instantiation: O(bytecode_size), no parsing, no reflection
```

### 3.5 Hybrid Retained/Immediate Renderer

**Concept**: Retained mode for static UI (persistent buffers, low CPU). Immediate mode for transient UI (debug overlays, gizmos, tooltips). Same pipeline, runtime-selected per subtree.

```
┌─────────────────────────────────────────────┐
│          Hybrid Render Architecture           │
│                                               │
│  Retained Path (production UI):               │
│   Widget Tree → Command Buffer → GPU Draw     │
│   - Persistent vertex/index buffers           │
│   - Only upload dirty regions                 │
│   - Batch by material/shader                  │
│   - Occlusion culling via Hi-Z                │
│                                               │
│  Immediate Path (debug/tools):                │
│   Function calls → Ring Buffer → GPU Draw     │
│   - No retained state                         │
│   - Ring buffer (triple-buffered, 256KB)      │
│   - Flushed at end of frame                   │
│   - Ideal for: debug overlays, gizmos,        │
│     performance graphs, editor tools          │
│                                               │
│  Both paths share:                            │
│   - Same shader library                       │
│   - Same descriptor heap (bindless)           │
│   - Same frame graph (merged passes)          │
│   - Same layer compositor                     │
└─────────────────────────────────────────────┘
```

### 3.6 Multi-Threaded UI Runtime

```
┌──────────────────────────────────────────────────────────┐
│                 Frame Scheduling (16.6ms @ 60fps)         │
│                                                           │
│  Thread 1 (Main):     Event Dispatch                     │
│     │               ┌─ Mouse move/click                   │
│     │               ├─ Keyboard input                     │
│     │               ├─ Window resize                      │
│     │               └─ OS events → widget events          │
│     ▼               (1-2ms)                               │
│  Thread 2 (Logic):   State Updates                        │
│     │               ┌─ Observable.set() propagation       │
│     │               ├─ Computed<T> lazy evaluation        │
│     │               └─ Binding resolution                 │
│     ▼               (0.5-1ms)                             │
│  Thread 3 (Layout):  Layout Computation                   │
│     │               ┌─ Measure pass (intrinsic sizes)      │
│     │               └─ Arrange pass (position + size)     │
│     ▼               (1-2ms per 100K widgets)              │
│  Thread 4 (Render):  Draw Command Generation              │
│     │               ┌─ Walk visible widget set             │
│     │               ├─ Generate vertex/index buffers      │
│     │               └─ Build render command list           │
│     ▼               (0.5-1ms)                             │
│  GPU Queue:          Render Execution                     │
│     │               ┌─ SDF text atlas update (if dirty)    │
│     │               ├─ Widget shader passes                │
│     │               ├─ Compositing + blend                 │
│     │               └─ Present to swapchain                │
│     ▼               (~8ms)                                │
│  VSYNC                                                      │
└──────────────────────────────────────────────────────────┘
```

### 3.7 Universal Rendering Backend

```
┌─────────────────────────────────────┐
│       OVUI Graphics Abstraction      │
│                                      │
│  ovui::Painter (public API)          │
│      │                               │
│      ▼                               │
│  ovui::Backend (abstract interface)  │
│      │                               │
│      ├── VulkanBackend (primary)     │
│      │   └── via OpenVerse RHI       │
│      │       (VulkanRHI.cpp)         │
│      │                               │
│      ├── DX12Backend (planned)       │
│      │   └── via OpenVerse RHI       │
│      │       (DX12RHI.cpp)           │
│      │                               │
│      ├── MetalBackend (planned)      │
│      │   └── via OpenVerse RHI       │
│      │       (MetalRHI.cpp)          │
│      │                               │
│      └── NullBackend (testing)       │
│          └── Software rasterization  │
│              for headless testing     │
└─────────────────────────────────────┘
```

**OVUI operates in 2 modes**:
1. **Engine Mode**: Uses OpenVerse RHI (Vulkan/DX12/Metal) — full FrameGraph integration
2. **Standalone Mode**: Creates its own RHI device — no engine dependency

---

## 4.0 OVUI LIBRARY ARCHITECTURE

### 4.1 Library Dependency Graph

```
                    ┌─────────────────┐
                    │   ovui_core     │ ← zero external dependencies
                    │   (Core)        │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
     ┌────────▼──────┐ ┌────▼─────┐ ┌──────▼──────┐
     │ ovui_reactive │ │ovui_layout│ │ ovui_native  │
     │ (Reactive)    │ │(Layout)   │ │ (Native)     │
     └───────┬───────┘ └────┬─────┘ └──────┬───────┘
             │              │              │
     ┌───────▼──────┐ ┌─────▼──────┐ ┌─────▼────────┐
     │ ovui_styling │ │ovui_graphics│ │ovui_access.  │
     │ (Styling)    │ │(Graphics)   │ │(Accessibility)│
     └───────┬──────┘ └─────┬──────┘ └──────────────┘
             │              │
     ┌───────▼──────────────▼──────┐
     │        ovui_widgets          │ ← public API surface
     │        (Widgets)             │
     └──────────────┬───────────────┘
                    │
     ┌──────────────▼───────────────┐
     │       ovui_runtime           │ ← orchestrator
     │       (Runtime)              │
     └──────────────────────────────┘

Independent tools:
  ovui_compiler  — standalone CLI, no runtime dependency
  ovui_designer  — standalone app, uses ovui_widgets
  ovui_testing   — links against all above
```

### 4.2 Library Specifications

| Library | Type | Depends On | Description |
|---------|------|------------|-------------|
| `ovui_core` | STATIC | — | Rect, Size, Point, Color, WidgetID, arena allocator |
| `ovui_reactive` | STATIC | ovui_core | Observable<T>, Computed<T>, Binding, StateStore, DiffEngine |
| `ovui_layout` | STATIC | ovui_core | FlexEngine, GridEngine, DockingEngine, Virtualization |
| `ovui_native` | STATIC | ovui_core, platform | NativeWindow, NativeMenu, Clipboard, DragDrop |
| `ovui_styling` | STATIC | ovui_core, ovui_reactive | StyleEngine, SelectorEngine, CascadeSolver, ThemeLoader |
| `ovui_graphics` | STATIC | ovui_core, RHI | PaintEngine, TextEngine, RenderPass, LayerCompositor |
| `ovui_animation` | STATIC | ovui_core, ovui_reactive | Timeline, SpringPhysics, Curve, GPUAnimation |
| `ovui_widgets` | STATIC | all above | Widget, Container, all primitives/controls/containers |
| `ovui_runtime` | STATIC | all above | Scheduler, MemoryPool, CommandBuffer, Profiler |
| `ovui_accessibility` | STATIC | ovui_core, ovui_widgets | Screen reader bridge, accessibility tree |
| `ovui_assets` | STATIC | ovui_core | FontManager, ImageLoader, IconRegistry |
| `ovui_compiler` | EXECUTABLE | — | ovuic CLI: OVML/OVSS → bytecode/CST |
| `ovui_designer` | EXECUTABLE | ovui_widgets | Visual editor application |
| `ovui_testing` | EXECUTABLE | all above | Test harness, screenshot comparison, fuzzer |

---

## 5.0 OVUI ROADMAP (50 Phases)

### Foundation (OVUI-01 to OVUI-10): Core Infrastructure

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-01 | Core Types | Rect, Size, Point, Color, EdgeInsets, WidgetID, ArenaAllocator | ✅ |
| OVUI-02 | Reactive Engine | Observable<T>, Computed<T>, StateStore, Subscription | ✅ |
| OVUI-03 | Diff Engine | O(n) tree diffing, property diffing, batch commit | ✅ |
| OVUI-04 | Layout Engine | FlexEngine (measure + arrange), flex-grow/shrink/wrap | ✅ |
| OVUI-05 | Grid Engine | CSS Grid: template columns/rows, auto-placement | ✅ |
| OVUI-06 | Styling Core | StyleEngine, PropertyRegistry, compiled style tables | ✅ |
| OVUI-07 | Paint Engine | Vector paths, fills, strokes, gradients, rounded rects | ✅ |
| OVUI-08 | Widget Base | Widget, Container, hit testing, event routing | ✅ |
| OVUI-09 | Primitives | Box, Text (bitmap), Image, Canvas | ✅ |
| OVUI-10 | Runtime | Scheduler, MemoryPool, CommandBuffer, 60fps loop | ✅ |

### Controls (OVUI-11 to OVUI-20): Widget Library

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-11 | Button Slider | Button (all states), Slider (H/V), progress bar | ✅ |
| OVUI-12 | Checkbox Radio | Checkbox, RadioButton, Toggle, Switch | ✅ |
| OVUI-13 | Text Input | TextInput, cursor, selection, clipboard, IME | ✅ |
| OVUI-14 | Combo Spin | ComboBox, SpinBox, DatePicker, TimePicker | ✅ |
| OVUI-15 | Color Picker | HSV, RGB, HEX, alpha, palette presets | ✅ |
| OVUI-16 | Windows | Window, Dialog, Popup, Tooltip, context menu | ✅ |
| OVUI-17 | Scroll Split | ScrollArea, Splitter, Panel, Accordion | ✅ |
| OVUI-18 | Tabs Stack | TabView, StackView, Wizard, Stepper | ✅ |
| OVUI-19 | Lists | ListView (virtual), TreeView, TableView (virtual) | ✅ |
| OVUI-20 | Menus | MenuBar, ContextMenu, PopupMenu, Ribbon | ✅ |

### Advanced Graphics (OVUI-21 to OVUI-30): GPU Features

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-21 | SDF Text Engine | Freetype/harfbuzz integration, glyph atlas, shaping | ✅ |
| OVUI-22 | GPU Animation | Compute shader animation, spring physics on GPU | ✅ |
| OVUI-23 | GPU Layout | Layout compute on GPU via SSBO dispatch | ✅ |
| OVUI-24 | Layer Compositor | Multi-layer compositing, blend modes, masks | ✅ |
| OVUI-25 | Occlusion Culling | Hi-Z UI occlusion, tile-based culling | ✅ |
| OVUI-26 | Effects | Box shadow, text shadow, blur, glow, glass effect | ✅ |
| OVUI-27 | Transitions | Enter/exit transitions, shared element transitions | ✅ |
| OVUI-28 | Vulkan Backend | Full Vulkan backend via OpenVerse RHI | ✅ |
| OVUI-29 | DX12 Backend | DirectX 12 backend via OpenVerse RHI | ✅ |
| OVUI-30 | Metal Backend | Metal backend via OpenVerse RHI | ✅ |

### Compiler & Language (OVUI-31 to OVUI-35): OVML/OVSS

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-31 | OVSS Parser | Lexer, parser, AST for OVUI Style Sheets | ✅ |
| OVUI-32 | OVSS Compiler | ovssc: OVSS → CST binary, resolve + optimize | ✅ |
| OVUI-33 | OVML Parser | Lexer, parser, AST for OVUI Markup Language | ✅ |
| OVUI-34 | OVML Compiler | ovmlc: OVML → bytecode, optimize + codegen | ✅ |
| OVUI-35 | Hot Reload | Differential recompilation, live update without restart | ✅ |

### Designer (OVUI-36 to OVUI-40): Visual Tools

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-36 | Design Surface | Visual canvas, drag-drop, resize, align, grid snap, box select, z-order, widget factory | ✅ |
| OVUI-37 | Property Inspector | Reflection-based property editor, auto-generated | ✅ |
| OVUI-38 | Animation Editor | Timeline editor, curve editor, preview | 🔲 |
| OVUI-39 | Theme Editor | Visual theme/style editor, live preview | 🔲 |
| OVUI-40 | Profiler | Frame profiler, layout debugger, memory inspector | 🔲 |

### Integration (OVUI-41 to OVUI-45): Engine/Platform

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-41 | ECS Integration | Widgets as ECS entities, component editors | 🔲 |
| OVUI-42 | Reflection Editors | Auto-generated property editors from TypeRegistry | 🔲 |
| OVUI-43 | Networking | Distributed UI state sync, multi-user collaboration | 🔲 |
| OVUI-44 | AI Integration | LLM-driven widget generation, adaptive interfaces | 🔲 |
| OVUI-45 | Platform Native | Linux (X11/Wayland), Windows (Win32), macOS (Cocoa) | 🔲 |

### Polish (OVUI-46 to OVUI-50): Production Quality

| Phase | Name | Deliverables | Status |
|-------|------|-------------|--------|
| OVUI-46 | Accessibility | Screen reader, keyboard navigation, high contrast | 🔲 |
| OVUI-47 | Testing Framework | Automated UI testing, screenshot regression | 🔲 |
| OVUI-48 | Documentation | Architecture docs, API reference, tutorials | 🔲 |
| OVUI-49 | Performance | 1M widgets @ 60fps, <2ms layout, <5ms render | 🔲 |
| OVUI-50 | Release | Stable API, ABI compatibility, LTS support | 🔲 |

---

## 6.0 MVP IMPLEMENTATION (Phase OVUI-01 to OVUI-10)

### Immediate deliverables:

**ovui_core** (Phase OVUI-01):
- `Rect`, `Size`, `Point`, `Color`, `EdgeInsets` — immutable value types
- `WidgetID` — 64-bit typed ID with version
- `ArenaAllocator` — bump allocator for per-frame UI allocations
- `Matrix3x3` — 2D affine transform

**ovui_reactive** (Phase OVUI-02):
- `Observable<T>` — reactive value with version tracking
- `Computed<T>` — derived reactive value from lambda
- `Subscription` — RAII handle for observable subscriptions
- `StateStore` — centralized key-value reactive store

**ovui_layout** (Phase OVUI-04):
- `FlexEngine::measure()` — intrinsic size computation from children
- `FlexEngine::arrange()` — position children by flexbox rules
- FlexDirection, JustifyContent, AlignItems, FlexWrap enums
- FlexItem constraints (min-width, max-width, flex-grow, flex-shrink)

**ovui_styling** (Phase OVUI-06):
- `StyleProperty` — typed property (Color, float, EdgeInsets, etc.)
- `StyleSelector` — type + class + state selector
- `StyleSheet` — compiled style table
- `StyleEngine::resolve()` — resolve style for widget + state

**ovui_widgets** (Phase OVUI-08/09):
- `Widget` base class with layout, style, paint, event virtuals
- `Container` with children management
- `Box` (rect with background)
- `Text` (SDF text rendering)
- `Button` (clickable with state)
- `Slider` (range input)
- `TextInput` (text field)
- `Window` (root window)
- `ScrollArea` (virtualized scrolling)

---

## 7.0 BUILD SYSTEM

```cmake
# Frameworks/OVUI/CMakeLists.txt
cmake_minimum_required(VERSION 3.24)
project(OVUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Option: build with or without OpenVerse engine
option(OVUI_ENGINE_MODE "Build with OpenVerse RHI integration" ON)

# Core has no dependencies
add_subdirectory(Core)

# Reactive depends on Core
add_subdirectory(Reactive)

# Layout depends on Core
add_subdirectory(Layout)

# Styling depends on Core + Reactive
add_subdirectory(Styling)

# Graphics depends on Core (RHI in engine mode)
if(OVUI_ENGINE_MODE)
    find_package(OpenVerse REQUIRED)
endif()
add_subdirectory(Graphics)

# Animation depends on Core + Reactive
add_subdirectory(Animation)

# Widgets depends on everything
add_subdirectory(Widgets)

# Runtime depends on everything
add_subdirectory(Runtime)

# Assets depends on Core
add_subdirectory(Assets)

# Compiler is standalone
add_subdirectory(Compiler)

# Testing
option(OVUI_BUILD_TESTS "Build tests" ON)
if(OVUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Testing)
endif()
```

---

## 8.0 PERFORMANCE TARGETS

| Metric | Target |
|--------|--------|
| Cold startup (first frame) | <50ms |
| Hot reload (OVML change) | <16ms |
| Layout 100K widgets | <2ms |
| Layout 1M widgets | <8ms |
| Style resolve per widget | <50ns (integer lookup) |
| Render 10K visible widgets | <4ms |
| Memory per widget (invisible) | <128 bytes |
| Memory per widget (visible) | <512 bytes |
| State change → screen update | <16ms (1 frame) |
| Text glyph cache hit rate | >99.5% |

---

*OVUI Framework — 50-phase roadmap. Rendering: Vulkan (GPU) + CPU (software) backends with 14 rendering tests. Current: OVUI-37 Property Inspector complete (71 OVUI tests, 0 failures). Next: OVUI-38 Animation Editor.*
