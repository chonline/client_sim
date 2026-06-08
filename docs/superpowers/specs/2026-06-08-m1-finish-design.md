# M1 Finish — Qt6 UI Shell for WZ Browser

> Spec date: 2026-06-08
> Scope: finish M1 (Phase G + parts of Phase I) from `docs/04-implementation-plan.md`
> Status: design, awaiting user review

## 1. Background and motivation

The M1 milestone in `docs/04-implementation-plan.md` is "基础设施"
(infrastructure): the `wzlib` parser library + a minimal Qt UI that lets a
user open a directory of `.wz` files, browse the resulting virtual tree,
and inspect individual image nodes. M1 was scoped to end there; rendering
maps, characters, animation, and so on are M2.

In practice M1 commit `30365e3` delivered the `wzlib` library, four
unit-test executables for it, and the `wzdump` CLI tool, but did not
deliver the UI shell (Phase G) or the M1 acceptance report and tag
(Phase I). This spec closes that gap.

## 2. Goals and non-goals

### 2.1 Goals (in scope)

- A Qt 6.5+ desktop application `client_sim` with:
  - `File → Open Directory` that opens a directory of `.wz` files and
    builds a `WzStructure` from them.
  - A left-hand `QTreeView` showing the union of all `.wz` files, grouped
    by file basename.
  - A right-hand `QGraphicsView`-based image viewer that displays the
    `WzPng` payload of the selected tree node, with mouse-wheel zoom,
    click-and-drag pan, and a reset-to-1.0x action.
  - A status bar showing the current node path, image dimensions, and
    pixel format.
- Lazy `WzImage` extraction: only the image under the expanded tree node
  is decompressed and parsed. The user can browse a 500 MB WZ bundle
  without extracting every image up front.
- The `app/` sub-project is built **only** when Qt6 with the `Widgets`
  module is available. With Qt6 missing, the project still builds
  `wzlib` + unit tests + `wzdump`, exactly as it does today.
- A written `docs/M1_report.md` that records what was verified locally
  and what was not, and that defers the M1 git tag to after the UI is
  exercised on a Qt6-equipped machine.

### 2.2 Non-goals (out of scope)

- Worker-thread extraction. M1 extracts synchronously on the UI thread
  with a `Qt::WaitCursor`. The 60 Hz render loop in M2 is the natural
  place to introduce a worker thread for extraction, and the design of
  that worker thread depends on M2's render-loop structure.
- Multi-frame PNG / sprite animation, Spine, particles, audio. Those are
  M3 and M5 per the implementation plan.
- Search, filtering, bookmarks, recent-files persistence, MDI, drag-drop
  of WZ files into the tree. These are post-M1 UX improvements.
- Cross-platform packaging (AppImage, .dmg, NSIS installer). These are
  M5 per the implementation plan.
- CI on GitHub Actions, vcpkg manifests. The plan lists them under M1
  Phase A but they are pushed past UI verification so we do not commit
  CI to a UI that has not been exercised.

## 3. Architecture overview

```
client_sim/
├── CMakeLists.txt          # add_subdirectory(app) gated on Qt6::Widgets
├── app/                    # new, Qt UI sub-project
│   ├── CMakeLists.txt      # qt_standard_project_setup + qt_add_executable
│   ├── main.cpp            # QApplication entry, show MainWindow
│   ├── MainWindow.{h,cpp}  # menu, toolbar, central splitter, status bar
│   ├── WzTreeModel.{h,cpp} # QAbstractItemModel adapter for WzStructure
│   ├── WzTreeView.{h,cpp}  # view + lazy extraction trigger
│   ├── ImageViewer.{h,cpp} # QGraphicsView-based image canvas
│   └── RgbaToPixmap.{h,cpp}# WzPng::decodeRgba() -> QImage
├── docs/
│   ├── M1_report.md        # new
│   └── superpowers/specs/2026-06-08-m1-finish-design.md  # this file
└── tests/
    └── fixtures/wz/README.md  # new, instructions for the user
```

`app/` is a leaf sub-project. It depends on `wzlib` (link-only) and
`Qt6::Core` + `Qt6::Widgets`. It does not modify `wzlib/`.

## 4. Component design

### 4.1 `MainWindow` (QMainWindow)

Responsibility: top-level chrome. Owns the `WzStructure` lifetime, holds
the `WzTreeView` + `ImageViewer` in a `QSplitter`, owns the menu and
toolbar.

Public API:
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setStructure(std::unique_ptr<WzStructure> s,
                      const QString& sourcePath);
    void closeStructure();
    bool hasStructure() const noexcept { return structure_ != nullptr; }
private slots:
    void onOpenDirectory();
    void onResetZoom();
    void onAbout();
private:
    std::unique_ptr<WzStructure> structure_;
    WzTreeView*   treeView_   = nullptr;
    ImageViewer*  imageView_  = nullptr;
    QAction*      actOpenDir_ = nullptr;
    QAction*      actReset_   = nullptr;
};
```

Menu and toolbar contents:

- `File` menu:
  - `Open Directory...` (Ctrl+O)
  - separator
  - `Exit` (Ctrl+Q)
- `View` menu:
  - `Reset Zoom` (Ctrl+0)
- `Help` menu:
  - `About client_sim`

The toolbar is a thin `QToolBar` with two actions mirrored from the
menus: `Open Directory` and `Reset Zoom`. This avoids a third navigation
surface; future M2+ features can add toolbar actions as needed.

`onOpenDirectory()` flow:
1. `QFileDialog::getExistingDirectory(this, tr("Open WZ Directory"))`.
2. Call `WzStructure::open(dir.absolutePath().toStdString())`. On
   failure, show `QMessageBox::warning` with the `ErrorCode` reason and
   abort. Do not clear the current structure on failure.
3. On success, take ownership of the new `WzStructure` and call
   `setStructure(...)`.
4. Update window title to
   `tr("client_sim — %1").arg(QFileInfo(dir).fileName())` and status bar
   to `tr("Loaded %1 files from %2").arg(s->fileCount()).arg(dir)`.

### 4.2 `WzTreeModel` (QAbstractItemModel)

Responsibility: map the `WzStructure` virtual tree onto Qt's Model/View
API.

The WZ tree is:
```
WzStructure (invisible root)
  └─ WzFile "Base.wz"
       └─ WzDirectory  (a directory tree node)
            └─ WzDirectory ... (deeper directories)
                 └─ WzImage  (a .img leaf, lazy)
                      └─ WzNode (sub-property, lazy)
                           └─ WzNode ... (sub-properties, lazy)
                                └─ WzPng / WzInt / WzString / ...
```

We map this 1:1 to `QModelIndex` by storing a `TreeNode*` in
`internalPointer()`.

```cpp
struct TreeNode {
    enum class Kind { WzFile, WzDirectory, WzImage, WzNode };
    Kind          kind = Kind::WzDirectory;
    WzFile*       file = nullptr;     // for all rows
    WzDirectory*  dir  = nullptr;     // for WzDirectory / WzImage rows
    WzImage*      img  = nullptr;     // for WzImage rows
    WzNode*       node = nullptr;     // for WzNode rows
    std::string   path;               // slash-joined path under the WzFile
    bool          expanded = false;  // whether children are built
    std::vector<std::unique_ptr<TreeNode>> children;
};
```

`WzTreeModel` owns the `TreeNode` tree. When the model is reset, it
frees the old tree.

`columnCount() = 2`:
- Column 0: name.
- Column 1: type string (`"Png"`, `"Int"`, `"String"`, `"Property"`, etc.),
  empty for `WzFile` and un-extracted `WzImage` rows.

Roles:
- `Qt::DisplayRole`: name (col 0) or type string (col 1).
- `Qt::DecorationRole` (col 0): a 16x16 thumbnail for rows whose
  `WzPng` value is decodable. `WzFile` rows use a folder icon. Empty for
  other rows.
- `Qt::ToolTipRole`: the full slash-joined path including the WZ file
  basename.
- Custom `TypeRole = Qt::UserRole + 1`: the `wzlib::ObjectType` value
  cast to int, or `-1` for non-value rows. Viewers use this to decide
  whether the row is a renderable image.

Lazy building:
- The top level is built when `setStructure()` is called: one child per
  `WzFile`, each pointing at that file's root `WzDirectory`.
- A `WzDirectory` row has children once `expanded = true`. Building
  children means:
  - For non-leaf directories, one child per child `WzDirectory`.
  - For leaf directories, one child per `WzImage`.
- A `WzImage` row has children once `tryExtract()` succeeded AND
  `expanded = true`. The children are the image's root `WzNode`'s
  children. Decoding the actual `WzPng` is not required to enumerate
  children.
- `WzNode` rows are expanded on demand: children are the node's
  children. If a child node has a `WzPng` value, that is a leaf row
  with no children.

Reentrancy: `WzTreeView` triggers expansion via `expanded(QModelIndex)`,
which calls back into the model via `fetchMore()` / `canFetchMore()`. We
override `canFetchMore()` to return `!row->expanded` and `fetchMore()` to
build the children and emit `beginInsertRows` / `endInsertRows`. This
is the documented Qt pattern for lazy models.

```cpp
class WzTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit WzTreeModel(QObject* parent = nullptr);
    ~WzTreeModel() override;

    void setStructure(WzStructure* s);
    WzStructure* structure() const noexcept { return structure_; }
    void reset();

    // QAbstractItemModel
    QModelIndex index(int row, int col, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& idx) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    bool hasChildren(const QModelIndex& parent) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Accessors used by WzTreeView
    TreeNode* nodeForIndex(const QModelIndex& idx) const;
    QString   fullPath(const QModelIndex& idx) const;

private:
    WzStructure* structure_ = nullptr;
    std::unique_ptr<TreeNode> invisibleRoot_;
    // Thumbnail cache: 16x16 QPixmap by WzPng* pointer. Bounded; see §6.
    mutable QHash<const WzPng*, QPixmap> thumbCache_;

    void buildWzFileChildren(TreeNode* fileNode);
    void buildDirectoryChildren(TreeNode* dirNode);
    void buildImageChildren(TreeNode* imgNode);
    void buildWzNodeChildren(TreeNode* nodeRow);
    QPixmap makeThumbnail(const WzPng* png) const;
};
```

### 4.3 `WzTreeView` (QTreeView)

Responsibility: tree presentation, lazy extraction trigger, selection
broadcast.

```cpp
class WzTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit WzTreeView(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* m) override;
signals:
    // Emitted when the user selects a node whose value is a WzPng.
    void imageActivated(const WzPng* png, int width, int height,
                        const QString& formatName, const QString& path);
    // Emitted when the user selects a node that is not a renderable image.
    void nonImageSelected(const QString& path);
private slots:
    void onExpanded(const QModelIndex& idx);
    void onActivated(const QModelIndex& idx);
private:
    void ensureExtracted(WzImage* img);
    QString pathFor(const QModelIndex& idx) const;
};
```

`ensureExtracted()` is the single chokepoint that calls
`WzImage::tryExtract()`. It uses
`QApplication::setOverrideCursor(Qt::WaitCursor)` + a `QScopeGuard` to
restore the cursor, and updates the status bar via the parent
`MainWindow` through the existing signal `imageActivated` (a 0x0
`QSize` in the signal means "extract failed"). On failure, the row's
`WzImage*` is left unextracted and the user can re-trigger extraction
by collapsing and re-expanding the row.

`onActivated()`:
1. `TreeNode* tn = model->nodeForIndex(idx)`.
2. If `tn->kind == WzNode` and `tn->node->value() && tn->node->value()->asPng()`:
   - `WzPng::decodeRgba()` is **not** part of `WzImage::tryExtract()`; it
     is called here on the UI thread. It caches its result internally,
     so subsequent re-selections of the same node are free.
   - Emit `imageActivated(png, w, h, formatName, path)`.
3. Otherwise, emit `nonImageSelected(path)`.

### 4.4 `ImageViewer` (QGraphicsView)

Responsibility: render one `QImage` at a time, with zoom and pan.

```cpp
class ImageViewer : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageViewer(QWidget* parent = nullptr);

    void setImage(QImage img);
    void clearImage();
    QSize  currentImageSize() const;
    bool   hasImage() const noexcept { return !image_.isNull(); }
public slots:
    void resetZoom();
private:
    QGraphicsScene* scene_ = nullptr;
    QGraphicsPixmapItem* item_ = nullptr;
    QImage image_;
    qreal zoom_ = 1.0;

    void wheelEvent(QWheelEvent* e) override;
    void applyZoom(qreal factor);
};
```

Behavior:
- `setImage()` calls `clearImage()`, sets `image_`, creates a
  `QPixmap::fromImage(image_)`, places it in a single
  `QGraphicsPixmapItem` centered at the scene origin, sets
  `setSceneRect(pixmap.rect())`, and calls `resetZoom()`.
- `clearImage()` removes the item and resets `image_` to a null `QImage`.
- `resetZoom()`: `resetTransform()`, `centerOn(item_)`.
- `wheelEvent()`: `angleDelta().y() > 0` → `scale(1.15, 1.15)`, else
  `scale(1 / 1.15, 1 / 1.15)`. `zoom_` is updated and `zoom_` is clamped
  to `[0.05, 20.0]`. A factor that would push it past the clamp is
  ignored. (Qt 6 API: `QWheelEvent::delta()` is removed in Qt 6; use
  `angleDelta().y()` which returns a `QPoint` in 1/8 degree units.)
- `setRenderHint(QPainter::SmoothPixmapTransform, true)` is set in the
  constructor for bilinear filtering at fractional zoom levels.
- `setDragMode(QGraphicsView::ScrollHandDrag)` is set in the constructor
  for click-and-drag pan.

### 4.5 `RgbaToPixmap` (free functions)

Responsibility: convert `WzPng::decodeRgba()` output to `QImage`.

```cpp
namespace client_sim::ui {
    // Returns a QImage in Format_RGBA8888 whose pixel buffer is an
    // independent copy of `data`. The returned QImage is safe to keep
    // after `data` is freed.
    QImage rgbaToQImage(const uint8_t* data, int width, int height);
}
```

`WzPng::decodeRgba()` returns `std::vector<uint8_t>` in
`R, G, B, A` order, 8 bits per channel, top-to-bottom, no padding —
this matches `QImage::Format_RGBA8888` exactly. No per-pixel swizzle is
required.

The implementation uses `QImage::copy()` to detach from the input buffer
so the returned `QImage` owns its pixels:

```cpp
QImage rgbaToQImage(const uint8_t* data, int width, int height) {
    QImage tmp(reinterpret_cast<const uchar*>(data),
               width, height, width * 4, QImage::Format_RGBA8888);
    return tmp.copy();  // forces a deep copy
}
```

`MainWindow` will use `RgbaToPixmap::rgbaToQImage` when
`WzTreeView::imageActivated` fires, then call `ImageViewer::setImage`.

## 5. Build system

### 5.1 Top-level `CMakeLists.txt` change

Replace the current `find_package(Qt6 6.5 QUIET)` block with:

```cmake
find_package(Qt6 6.5 QUIET)
if(Qt6_FOUND AND TARGET Qt6::Widgets)
    set(CLIENT_SIM_HAVE_QT6 TRUE)
    message(STATUS "Qt6 with Widgets found — building client_sim UI")
else()
    set(CLIENT_SIM_HAVE_QT6 FALSE)
    message(STATUS
        "Qt6 (with Widgets) not found — skipping client_sim UI "
        "(wzlib + tests + tools only)")
endif()

# ... existing subdirs ...

if(CLIENT_SIM_HAVE_QT6)
    add_subdirectory(app)
endif()
```

This keeps the existing build green on machines without Qt6 (the current
CI / dev box situation).

### 5.2 `app/CMakeLists.txt` (new)

```cmake
qt_standard_project_setup()

qt_add_executable(client_sim
    main.cpp
    MainWindow.h         MainWindow.cpp
    WzTreeModel.h        WzTreeModel.cpp
    WzTreeView.h         WzTreeView.cpp
    ImageViewer.h        ImageViewer.cpp
    RgbaToPixmap.h       RgbaToPixmap.cpp
)

target_link_libraries(client_sim
    PRIVATE
        wzlib
        Qt6::Core
        Qt6::Widgets
)

set_target_properties(client_sim PROPERTIES
    WIN32 TRUE
    MACOSX_BUNDLE TRUE
)
```

The `RgbaToPixmap` header has a paired `.cpp` because the
`QImage::copy()` call is non-trivial enough to deserve a translation
unit (and to keep `-Wweak-vtables` clean if the project ever turns on
`-Werror`).

### 5.3 `MOC` and `AUTOMOC`

`qt_standard_project_setup()` from Qt 6.3+ enables `AUTOMOC`. The
`Q_OBJECT` macros in `MainWindow`, `WzTreeModel`, `WzTreeView`, and
`ImageViewer` will be picked up automatically — no manual `moc_*.cpp`
listings required.

## 6. Resource limits and caches

- Thumbnail cache in `WzTreeModel`: capped at 256 entries. When a 257th
  thumbnail is added, the oldest is evicted. A `QHash<const WzPng*,
  QPixmap>` is acceptable here because the cache key is the address of
  a `WzPng` living inside a `WzImage` owned by a `WzFile` owned by the
  `WzStructure`; the `WzStructure` outlives the model, so the pointers
  remain stable for the model's lifetime.
- The thumbnail is generated lazily on first `DecorationRole` request
  for that row, so we never decode a PNG we do not need.
- The model holds a non-owning `WzStructure*`. Ownership stays in
  `MainWindow`. The model is reset (and the thumbnail cache cleared)
  in `setStructure()` and in `reset()`.

## 7. Error handling

| Failure | Behavior |
|---|---|
| No `.wz` files in the chosen directory | `QMessageBox::warning` with the `ErrorCode` text; no structural change. |
| `WzStructure::open` returns a failure | `QMessageBox::warning`; current structure preserved. |
| `WzImage::tryExtract` fails on expansion | Row stays un-extracted; status bar shows `"Failed to extract: <reason>"`. User can collapse + re-expand to retry. |
| Selected node is not a `WzPng` | `ImageViewer::clearImage()`; status bar shows `"Not an image: <path>"`. |
| `WzPng::decodeRgba` returns an empty buffer | Same as above, with `"Decode failed"`. |
| Window closed while a structure is loaded | `MainWindow` destructor destroys the `WzStructure` cleanly via `std::unique_ptr`. |

All errors are also logged via the existing `wzlib::Log` facilities so
post-mortem debugging on the user's Qt6 machine is straightforward.

## 8. Testing

### 8.1 What runs locally (no Qt6 required)

The four existing unit tests continue to pass. The M1 acceptance criteria
that can be checked from wzlib alone:

- `WzStructure::open()` round-trips a real WZ directory — gated on a
  fixture. The test file uses `GTEST_SKIP()` with an explanatory
  message until a fixture is provided. The fixture path is
  `tests/fixtures/wz/`; the `README.md` in that directory tells the user
  where to drop files.
- Cross-WZ path resolution: same gating, same fixture.
- `WzPng::decodeRgba` byte order: a synthetic test that builds a known
  4x4 ARGB8888 image and asserts on the exact byte sequence. Runs
  locally without any fixture.
- `WzString::get` and `WzInt::get` type-safety tests: small synthetic
  cases. Already covered by the existing test suites.

A new test target `test_wz_structure` is added to `tests/`. The CMake
target looks for the fixture and `GTEST_SKIP`s when absent:

```cmake
add_executable(test_wz_structure main.cpp)
target_link_libraries(test_wz_structure PRIVATE wzlib gtest gtest_main)
add_test(NAME test_wz_structure COMMAND test_wz_structure)
```

### 8.2 What must be run on a Qt6-equipped machine

The implementation plan's UI acceptance bullets are translated into a
manual checklist in `docs/M1_report.md`:

1. `client_sim` starts, window appears, menu bar visible.
2. `File → Open Directory` opens a directory containing `Base.wz`,
   `Map.wz`, `Character.wz`, and `String.wz` from a real KMS install.
3. The tree populates with one top-level entry per WZ file, expandable.
4. Expanding a `WzImage` row extracts the image (verifiable by
   sub-property rows appearing). Status bar briefly shows
   `"Extracting..."`.
5. Selecting a row that has a `WzPng` value (e.g.
   `Character/00002000.img/stand/0/canvas`) shows the image in the
   right pane at 1.0x zoom, centered.
6. Mouse wheel zooms in and out; `Ctrl+0` resets to 1.0x; click-and-drag
   pans the canvas.
7. Opening a 500 MB WZ directory does not exhaust RAM (the model never
   extracts images until expansion).

The report explicitly states that items 1-7 are **not verified on the
current dev environment** (no Qt6, no network) and lists the OS / Qt
version the user is expected to use.

### 8.3 `M1_report.md` and git tag

`docs/M1_report.md` is written before the report. It records:

- The commit hash on which M1 was based (`30365e3`).
- A summary of the new `app/` code and the CMake gating.
- A summary of the new `test_wz_structure` target.
- The verified-locally test list and pass status.
- The deferred-to-Qt6-machine checklist.
- A note: the `v0.1.0` tag is **not** created in this change set. The
  tag is created only after the user confirms the UI checklist on a
  Qt6-equipped machine.

## 9. Documentation

- `docs/M1_report.md` — written by this change.
- `tests/fixtures/wz/README.md` — short; tells the user to drop WZ files
  there. No copyrighted content is bundled in the repository.
- `docs/superpowers/specs/2026-06-08-m1-finish-design.md` — this file.
- Public API headers (`MainWindow.h`, `WzTreeModel.h`, `WzTreeView.h`,
  `ImageViewer.h`, `RgbaToPixmap.h`) get one-paragraph Doxygen blocks
  matching the style of the existing `wzlib/` headers.

## 10. Risks

- **Qt6 API drift.** The code targets Qt 6.5+ specifically. If the user's
  Qt is older, the `qt_add_executable` / `qt_standard_project_setup`
  helpers may not exist. The CMake error from the Qt config step is
  specific enough to point at this.
- **`QAbstractItemModel` correctness.** Tree models are easy to get
  subtly wrong (off-by-one in `parent()`, stale internal pointers after
  `reset()`). The plan is to keep the model small and write a unit test
  for the `index/parent` round-trip on a synthetic 3-level tree. The
  test uses only `QAbstractItemModel` and does not require a
  `WzStructure`, so it can run on the dev box if Qt6 is later installed.
- **WzStructure lifetime.** The model holds a non-owning pointer. If
  the structure is destroyed before the model, we crash. The contract is
  enforced by `MainWindow`: it owns the `WzStructure` and resets the
  model in its destructor. The model asserts this in debug builds via
  `Q_ASSERT(structure_ != nullptr || invisibleRoot_->children.empty())`
  in `data()`.
- **Png decodeRgba() is currently called from the UI thread.** A 4K DXT5
  image could take 100+ ms. The implementation plan defers worker
  threads to M2; M1 accepts the freeze and signals it via the wait
  cursor.

## 11. Open questions

None. The user has confirmed: directory-open UX, QGraphicsView image
viewer, scope = M1 finish only.

## 12. Decision log

- **Open by directory, not by single WZ file.** Cross-WZ path resolution
  is an M1 acceptance criterion; opening a single file would make that
  criterion untestable in the UI.
- **`QAbstractItemModel` over `QTreeWidget`.** Tree widgets force a copy
  of the data; an item model maps 1:1 to the `WzStructure` tree and
  leaves room for search / filter / multi-pane later.
- **Synchronous extraction with a wait cursor over a worker thread.**
  Worker threads are M2's problem. M1 stays simple.
- **Qt 6.5+ (not 5.15).** `qt_standard_project_setup()` and
  `qt_add_executable` are 6.3+; 6.5 is the lowest version that gets
  them in a stable form. Matches the plan.
- **Tag deferred.** Creating `v0.1.0` against a UI that has never been
  exercised would mislead anyone who pulls the tag.
