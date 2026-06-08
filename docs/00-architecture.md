# MapleNecrocer C++/Qt 跨平台版 — 总架构设计

> 项目根目录: `/home/huihuige/workspace/cpp/client_sim`
> 文档版本: v1.0 (2026-06-08)
> 状态: 已审批,准备实施

## 1. 项目目标

将 `MapleNecrocer-master` (C# WinForms + MonoGame, Windows 64-bit) **完整复刻**为 **C++/Qt 6 跨平台桌面应用**,支持 Linux / macOS / Windows。

### 1.1 核心功能
1. 解析 WZ/MS 格式(MapleStory 资源包)
2. 渲染 2D 游戏世界(地图+角色+怪物+NPC+宠物+坐骑+粒子)
3. 40+ 浏览窗体(装备/物品/技能/怪物/NPC/...)
4. 装备/物品/技能悬浮 tooltip
5. Spine 骨骼动画 + BGM/SFX
6. 自动检测 WZ 区域版本(KMS/GMS/JMS/TMS)

### 1.2 技术栈
| 关注点 | 原 C# 版 | 新 C++/Qt 版 |
|---|---|---|
| 语言 | C# 12 (.NET 8) | C++17 |
| UI 框架 | Windows Forms + DotNetBar2 | **Qt 6 Widgets** |
| 2D 渲染 | MonoGame.Forms + SharpDX(DirectX 11) | **QGraphicsView + QPainter** |
| 骨骼动画 | spine-monogame | **spine-cpp** |
| 音频 | ManagedBass | **SDL2_mixer** |
| 图像解码 | 内置 + AForge | **自研(基于 Qt QImage)** |
| 加密 | .NET Aes | **OpenSSL EVP** |
| 构建 | MSBuild / dotnet build | **CMake + vcpkg** |
| 平台 | Windows x64 | **Linux + macOS + Windows** |

## 2. 4 层架构

```
┌────────────────────────────────────────────────────────────┐
│  app/        (可执行入口)                                    │
│  ─── 启动 QApplication、加载 QSS 主题、创建主窗口           │
├────────────────────────────────────────────────────────────┤
│  ui/         (Qt Widgets 表现层)                            │
│  ─── MainWindow + 40+ 浏览 Form + TreeView + Tooltip      │
│  ─── 依赖: client/, wzlib/, Qt 6 Widgets                   │
├────────────────────────────────────────────────────────────┤
│  client/     (游戏对象与渲染)                                │
│  ─── Map / Player / Mob / Npc / Pet / TamingMob            │
│  ─── MapBack / MapObj / MapTile / MapPortal / Particle     │
│  ─── Sprite 引擎 / 60Hz 渲染循环 / Spine 骨骼              │
│  ─── 依赖: wzlib/, spine-cpp, SDL2_mixer, Qt Core          │
├────────────────────────────────────────────────────────────┤
│  wzlib/      (资源文件解析,不依赖 Qt)                        │
│  ─── WzFile / WzImage / WzNode / WzPng / WzSound           │
│  ─── WzVector / WzUol / WzCrypto / WzStructure / MsFile    │
│  ─── StringLinker / Calculator / SpineLoader               │
│  ─── 依赖: OpenSSL EVP, zlib, libpng, stdlib               │
└────────────────────────────────────────────────────────────┘
```

### 2.1 目录结构
```
client_sim/
├── CMakeLists.txt              # 顶层构建脚本
├── CMakePresets.json           # 跨平台预设(linux/macOS/windows)
├── vcpkg.json                  # 依赖清单
├── README.md
├── LICENSE
├── .gitignore
├── .clang-format
├── .clang-tidy
├── docs/
│   ├── 00-architecture.md             # 本文件
│   ├── 01-wzlib-design.md             # wzlib 详细设计
│   ├── 02-wzlib-impl.md               # wzlib 实现细节
│   ├── 03-client-design.md            # client 详细设计
│   ├── 04-client-impl.md              # client 实现细节
│   ├── 05-ui-design.md                # ui 详细设计
│   ├── 06-ui-impl.md                  # ui 实现细节
│   ├── 07-port-guide.md               # 跨平台编译与打包
│   ├── 08-testing.md                  # 测试策略
│   └── MapleNecrocer-master-analysis.md  # 原项目分析
├── wzlib/                      # 第 1 层
│   ├── CMakeLists.txt
│   ├── include/wzlib/
│   │   ├── WzFile.h
│   │   ├── WzImage.h
│   │   ├── WzNode.h
│   │   ├── WzPng.h
│   │   ├── WzSound.h
│   │   ├── WzVector.h
│   │   ├── WzUol.h
│   │   ├── WzCrypto.h
│   │   ├── WzStructure.h
│   │   ├── MsFile.h
│   │   ├── StringLinker.h
│   │   ├── Calculator.h
│   │   ├── BitmapOrigin.h
│   │   ├── Gif.h
│   │   └── SpineLoader.h
│   ├── src/                             # 实现
│   └── tests/                           # 单测
├── client/                    # 第 2 层
│   ├── CMakeLists.txt
│   ├── include/client/
│   │   ├── Engine.h
│   │   ├── Sprite.h
│   │   ├── AnimatedSprite.h
│   │   ├── BackgroundSprite.h
│   │   ├── JumperSprite.h
│   │   ├── ParticleSprite.h
│   │   ├── SpineSprite.h
│   │   ├── Map.h
│   │   ├── Player.h
│   │   ├── Mob.h
│   │   ├── Npc.h
│   │   ├── Pet.h
│   │   ├── Familiar.h
│   │   ├── TamingMob.h
│   │   ├── Android.h
│   │   ├── MapleChair.h
│   │   ├── MapBack.h
│   │   ├── MapObj.h
│   │   ├── MapTile.h
│   │   ├── MapPortal.h
│   │   ├── MapParticle.h
│   │   ├── FootholdTree.h
│   │   ├── ChatBalloon.h
│   │   ├── NameTag.h
│   │   ├── DamageNumber.h
│   │   ├── AfterImage.h
│   │   ├── Skill.h
│   │   ├── Sound.h
│   │   ├── ImageCache.h
│   │   └── FontManager.h
│   └── src/
├── ui/                        # 第 3 层
│   ├── CMakeLists.txt
│   ├── include/ui/
│   │   ├── MainWindow.h
│   │   ├── RenderWidget.h
│   │   ├── WzTreeView.h
│   │   ├── WzTreeModel.h
│   │   ├── MapListView.h
│   │   ├── ItemTooltip.h
│   │   ├── WzFinder.h
│   │   ├── ControlManager.h
│   │   ├── DpiHelper.h
│   │   ├── AControl.h
│   │   ├── ACtrlButton.h
│   │   ├── ACtrlVScroll.h
│   │   ├── forms/                  # 40+ Form 头文件
│   │   │   ├── BrowseForm.h
│   │   │   ├── ListPreviewForm.h
│   │   │   ├── AvatarForm.h
│   │   │   ├── MobForm.h
│   │   │   ├── NpcForm.h
│   │   │   ├── SkillForm.h
│   │   │   ├── ItemForm.h
│   │   │   ├── CashForm.h
│   │   │   ├── ConsumeForm.h
│   │   │   ├── EtcForm.h
│   │   │   ├── ChairForm.h
│   │   │   ├── MountForm.h
│   │   │   ├── MorphForm.h
│   │   │   ├── DamageSkinForm.h
│   │   │   ├── MedalForm.h
│   │   │   ├── TitleForm.h
│   │   │   ├── RingForm.h
│   │   │   ├── PetForm.h
│   │   │   ├── FamiliarForm.h
│   │   │   ├── AndroidForm.h
│   │   │   ├── TotemEffectForm.h
│   │   │   ├── SoulEffectForm.h
│   │   │   ├── ReactorForm.h
│   │   │   ├── EffectForm.h
│   │   │   ├── EffectRingForm.h
│   │   │   ├── ChatRingForm.h
│   │   │   ├── CashEffectForm.h
│   │   │   ├── ViewForm.h
│   │   │   ├── WorldMapForm.h
│   │   │   ├── SaveMapForm.h
│   │   │   ├── ScaleForm.h
│   │   │   ├── OptionForm.h
│   │   │   ├── SelectFolderForm.h
│   │   │   └── ObjInfoForm.h
│   │   └── render/                # Tooltip 渲染器
│   │       ├── GearTooltipRender.h
│   │       ├── ItemTooltipRender.h
│   │       ├── SkillTooltipRender.h
│   │       ├── MobTooltipRender.h
│   │       ├── NpcTooltipRender.h
│   │       ├── SetItemTooltipRender.h
│   │       ├── MapTooltipRender.h
│   │       └── RecipeTooltipRender.h
│   └── src/
├── app/                       # 第 4 层
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── resources.qrc
├── scripts/                   # 构建脚本
│   ├── build_appimage.sh
│   ├── build_dmg.sh
│   └── build_installer.nsi
├── tests/                     # 跨模块单测
│   ├── CMakeLists.txt
│   ├── wzlib/
│   ├── client/
│   └── fixtures/
└── resources/                 # 应用图标、QSS 主题
    ├── icon.png
    ├── icon.ico
    └── theme.qss
```

## 3. 关键设计决策

| 决策 | 方案 | 理由 |
|---|---|---|
| WZ 解析 | 完全自研 C++(基于 OpenSSL EVP) | 不依赖 C#/.NET,跨平台;可控;性能更好 |
| 2D 渲染 | Qt QGraphicsView + 自定义 QGraphicsItem | 内置 Z 序、相机;跨平台;够用 |
| 字体回退 | Qt 字体系统 | 跨平台一致行为 |
| 音频 | SDL2_mixer | 跨平台统一后端,API 简单 |
| 骨骼动画 | spine-cpp 官方运行时 | 与 spine-monogame 行为一致 |
| 构建 | CMake + vcpkg | 跨平台统一;依赖锁定 |
| 测试 | GTest + Qt Test | wzlib 可纯 C++ 测,UI 用 Qt Test |
| 模块边界 | 4 层严格分层,下层不依赖上层 | wzlib 不依赖 Qt,client 不依赖 ui |

## 4. 调用关系总图

```
用户操作 (键盘/鼠标)
   ↓
ui/MainWindow (Qt 事件)
   ↓
ui/RenderWidget (嵌入 QGraphicsView)
   ↓
client/Engine (60Hz 主循环)
   ↓
client/Sprite 子类.draw()
   ↓
wzlib/WzNode::get() → wzlib/WzPng::toQImage() → QPixmap
   ↓
QPainter → QGraphicsView → 屏幕
```

## 5. 数据流(打开 WZ)

```
FileDialog → ui/MainWindow::openWz(path)
                ↓
            wzlib::WzStructure::load(path)
                ↓
            wzlib::WzFile::open(path, cryptoCtx)
                ↓ (读取 Header,生成 AES 密钥)
            读取 Directory Tree (懒加载 WzImage)
                ↓
            detectRegion() → WzType, HasStringWz, HasMap9Dir, ...
                ↓
            client/Engine::setWzStructure(wz)
                ↓
            client/Map::loadMap(mapId)  [用户后续操作]
                ↓
            client/* 精灵创建并加入 Engine
                ↓
            60Hz tick → draw → 屏幕
```

## 6. 关键模块设计(摘要)

### 6.1 wzlib 核心类

```cpp
namespace wzlib {

class WzObject { /* 基类 */ };

class WzInt/WzDouble/WzString/WzVector/WzUol/WzPng/WzSound
    : public WzObject;

class WzNode {
    std::string name_;
    WzNode* parent_;
    std::map<std::string, std::unique_ptr<WzNode>> children_;
    std::unique_ptr<WzObject> value_;
public:
    WzNode* get(const std::string& path) const;  // 支持 ".."
};

class WzImage {
    // 懒加载 .img 数据块
    bool tryExtract();
    WzNode* root();
};

class WzFile {
    std::unique_ptr<std::ifstream> stream_;
    WzHeader header_;
    std::array<uint8_t, 2048> xor_keys_;
    std::map<int64_t, WzImage> images_;
public:
    static std::unique_ptr<WzFile> open(const std::string& path,
                                        WzCryptoContext& ctx);
};

class WzStructure {
    std::vector<std::unique_ptr<WzFile>> files_;
    WzCryptoContext crypto_;
    WzNode root_;
public:
    static std::unique_ptr<WzStructure> load(const std::string& path);
    WzNode* findNode(const std::string& path);
};

class WzCryptoContext {
    std::array<uint8_t, 32> aes_key_;
    std::array<uint8_t, 2048> pkg1_keys_;
public:
    void initFromIv(const std::array<uint8_t, 4>& iv,
                    const std::array<uint8_t, 32>& aes_key);
    void decryptInPlace(uint8_t* data, size_t len, int64_t offset) const;
    std::string decryptString(const uint8_t* data, size_t len) const;
};

class StringLinker {
    // 7 个 Dictionary 缓存 ID -> StringResult
public:
    bool load(WzFile* stringWz, WzFile* itemWz, WzFile* etcWz);
    StringResult get(int32_t id, const std::string& category) const;
};

} // namespace wzlib
```

### 6.2 client 核心类

```cpp
namespace client {

class Sprite { /* 抽象基类,提供 update/draw */ };
class SpriteEx : public Sprite { /* 加 imageNode/currentFrame */ };
class AnimatedSprite : public SpriteEx { /* 帧动画 */ };
class BackgroundSprite : public SpriteEx { /* 平铺/视差 */ };
class JumperSprite : public AnimatedSprite { /* 跳跃/重力 */ };
class SpineSprite : public SpriteEx { /* spine-cpp 集成 */ };

class Engine {
public:
    static Engine& instance();
    void start();              // 启动 60Hz 循环
    void tick(int64_t elapsedMs);
    void addSprite(Sprite* s);
    void removeSprite(Sprite* s);
    
    QGraphicsView* renderView_;
    QGraphicsScene* renderScene_;
    std::set<Sprite*, ZOrderCompare> sprites_;
    
    std::shared_ptr<wzlib::WzStructure> wzStructure_;
};

class Map {
public:
    void loadMap(const QString& id);   // 异步
    // 内部: parseInfo/parseBack/parseTile/parseObj/parseLife/...
    
    std::vector<std::unique_ptr<MapBack>> backs_;
    std::unique_ptr<Player> player_;
    std::vector<std::unique_ptr<Mob>> mobs_;
};

class Player : public JumperSprite {
public:
    void onKeyDown(Qt::Key);
    void onKeyUp(Qt::Key);
    void update(int64_t) override;
    // Avatar 离屏组合 → QImage 1024x1024
    void renderAvatarToTarget(QPainter&);
};

class FootholdTree {
public:
    Foothold* findBelow(QPointF pos, Foothold* exclude=nullptr);
};

} // namespace client
```

### 6.3 ui 核心类

```cpp
namespace ui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    static MainWindow* instance();
    void openWz(const QString& path);
    
    // 嵌入的子控件
    RenderWidget* renderWidget_;
    WzTreeView* wzTreeView_;
    MapListView* mapListView_;
    ItemTooltip* tooltipView_;
};

class RenderWidget : public QWidget {
    QGraphicsView* view_;
    QGraphicsScene* scene_;
    // 60Hz 由 Engine::instance() 驱动
};

class WzTreeView : public QTreeView {
    // 树形浏览 WZ 节点,双击触发 QuickView
};

class ItemTooltip : public QWidget {
    void setTarget(QObject* item);  // Gear/Item/Skill/Mob/Npc
    void rebuild();
};

class WzFinder {
public:
    static WzFinder& instance();
    wzlib::WzNode* find(const QString& path);
    // 跨 WzFile 解析,处理 UOL/_inlink/_outlink
};

} // namespace ui
```

## 7. 实施里程碑

| 里程碑 | 周期 | 目标 | 关键验收 |
|---|---|---|---|
| **M1** | 2-3 周 | 基础设施: wzlib + Qt 主窗口 + WZ 树 + 图片查看 | 打开 wz 看到树,选中节点显示图片;Linux/Mac/Windows 均编译运行 |
| **M2** | 2-3 周 | 渲染: 地图 + 角色 + 60Hz 循环 + Foothold | 双击地图进入,角色移动,多层背景视差,60FPS |
| **M3** | 2-3 周 | 游戏对象: Mob/Npc/Pet/TamingMob/Familiar | 地图上怪物/宠物/坐骑走动,聊天泡泡,伤害数字 |
| **M4** | 2-3 周 | 浏览 UI: 40+ Form + Tooltip | QuickView 装备/物品/技能,40+ Form 可用 |
| **M5** | 1-2 周 | 高级: Spine + 音频 + 跨平台打包 | Spine 背景/对象,BGM/SFX,AppImage/dmg/installer |

总计 9-13 周,代码量预计 25,000-40,000 行 C++。

## 8. 跨平台差异处理

### 8.1 路径与文件名
- 统一使用 `/` 作为分隔符(QDir::toNativeSeparators 在显示时转换)
- wzlib 内部用 `std::string`(UTF-8),UI 层用 `QString`

### 8.2 字体
- 配置:每个区域(GMS/TMS/JMS/KMS)使用不同字体
- Qt 字体回退:若指定字体不存在,使用最近似的系统字体
- `FontManager::loadFromConfig(region)` 初始化

### 8.3 渲染
- QPainter 抽象所有 2D 绘制(底层 Windows=GDI/D2D,Linux=Cairo/X11,Mac=CoreGraphics)
- 不使用 OpenGL/DirectX,跨平台自动适配

### 8.4 音频
- SDL2_mixer 统一 3 个平台的音频后端
- 主音频路径 16-bit 立体声 44.1kHz

### 8.5 平台抽象层
```cpp
// wzlib/Platform.h
namespace wzlib::platform {
QString normalizePath(const QString&);
QFont getDefaultFont(const QString& family, int size);
QString tempDir();
int64_t currentTimeMs();
}
```

## 9. 测试策略

### 9.1 单元测试(GTest)
- wzlib 单独可测,不依赖 Qt
- AES 密钥流字节级对比原 C# 版(用 hex dump)
- PNG 解码像素对比(黄金图)
- 路径解析覆盖:含 UOL、`_inlink`、`_outlink`、`source`

### 9.2 集成测试
- 打开真实 wz 验证目录树大小、关键节点存在
- 加载真实地图验证 Tile/Back/Obj/Mob 数量
- 与原版截图对比(像素 diff < 1%)

### 9.3 视觉回归
- 3 个平台分别截图同一地图
- 像素 diff < 5%

### 9.4 性能基准
- 大型 wz 打开:< 2 秒
- 单张 1024×1024 PNG 解码:< 30ms
- 60Hz 渲染在 1080P 地图上稳定

## 10. 风险与缓解

| 风险 | 等级 | 缓解 |
|---|---|---|
| AES 密钥流与原版不一致 | 高 | 单测覆盖所有 IV,字节级对比 |
| DXT/BC7 解码不准确 | 中 | M1 优先 DXT1/3/5,BC7 留后;准备黄金图 |
| WZ 路径解析边界 case | 高 | 用原版打开多个真实 wz,验证树结构 |
| Qt 与 WinForms 行为差异 | 中 | 关键控件用 QWidget 重写,行为对齐 |
| Spine 运行时差异 | 中 | spine-cpp 与 spine-monogame 行为应一致 |
| macOS 签名/公证 | 中 | M5 处理,先 ad-hoc 签名 |
| 字体跨平台差异 | 低 | Qt 字体回退机制 |

## 11. 文档结构

```
docs/
├── 00-architecture.md       ← 本文件(总架构)
├── 01-wzlib-design.md       ← wzlib 详细设计
├── 02-wzlib-impl.md         ← wzlib 关键算法与代码
├── 03-client-design.md      ← client 详细设计
├── 04-client-impl.md        ← client 关键算法
├── 05-ui-design.md          ← ui 详细设计
├── 06-ui-impl.md            ← ui 关键算法
├── 07-port-guide.md         ← 跨平台编译与打包
├── 08-testing.md            ← 测试策略与覆盖率
└── MapleNecrocer-master-analysis.md  ← 原项目分析(已写)
```

## 12. 代码规范

- C++17,4 空格缩进,120 字符行宽
- 类 `PascalCase`,函数 `camelCase`,成员 `camelCase_`,常量 `UPPER_SNAKE_CASE`
- `#pragma once`,命名空间小写
- `.clang-format` + `.clang-tidy` 强制
- 公开 API Doxygen 注释
- 不写"代码做什么"注释,只写"为什么这样写"

## 13. CI/CD

- GitHub Actions 矩阵构建(Linux/macOS/Windows)
- 每次 PR 跑 GTest + clang-tidy + cppcheck
- 视觉回归截图对比
- 覆盖率报告(目标: wzlib ≥ 80%, client ≥ 60%)

## 14. 总结

`★ Insight ─────────────────────────────────────`
- **严格分层**是跨平台可移植性的关键 — wzlib 不依赖 Qt,可纯 C++ 编译,任何平台都能跑
- **QGraphicsView 替代 MonoGame** 看似降级,但配合自定义 QGraphicsItem 和离屏 QImage,足以应对 60Hz 渲染;且代码量减少 50%
- **多里程碑可运行交付** 避免了一次性提交 3 万行代码的风险,每个阶段都有可见反馈
`─────────────────────────────────────────────────`
