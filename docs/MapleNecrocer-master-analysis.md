# MapleNecrocer-master 项目深度分析文档

> 分析目标: `/home/huihuige/workspace/cpp/MapleNecrocer-master`
> 分析日期: 2026-06-08
> 项目性质: 冒险岛(MapleStory)GM 客户端,基于 WZ 资源浏览/渲染的工具
> 主语言: C# (.NET 8.0-windows7.0, x64 64-bit)
> UI 框架: Windows Forms + DotNetBar2
> 渲染框架: MonoGame.Forms.DX (DirectX via SharpDX)
> 构建工具: MSBuild / Visual Studio 2022+

---

## 1. 项目概述

### 1.1 业务定位
MapleNecrocer 是 **冒险岛 GM 工具的 C# 重制版**,原版基于 Delphi 32-bit。其核心功能是:

1. **打开并解析** MapleStory 的 WZ/MS 资源包文件(图像、动画、声音、字符串)
2. **渲染 2D 游戏世界** —— 地图、角色、怪物、NPC、宠物、坐骑
3. **浏览/查看装备、物品、技能、配方等** 游戏内容数据
4. **支持地图预览、人物组合(Aquarium Avatar)、骨骼动画(Spine)**

### 1.2 主要特性(README 自述)
- 64-bit 客户端(原 Delphi 版 32-bit)
- 支持新版 `_Canvas` wz 文件
- 支持地图骨架(Skeleton)动画与粒子系统
- 自动检测 WZ 区域版本(无需手动选择 KMS/GMS/JMS)
- 同时兼容旧版/新版 WZ 格式的 WzLib

### 1.3 项目组成(4 个子项目)
| 子项目 | 作用 | 关键文件数 |
|---|---|---|
| `MapleNecrocer/` | 主程序(WinForms + MonoGame 渲染) | ~70 个 .cs |
| `WzComparerR2.WzLib/` | WZ/MS 资源文件格式解析库 | ~30 个 .cs |
| `WzComparerR2.Common/` | 公共工具库(字符串、骨骼、Gif等) | ~20 个 .cs |
| `WzComparerR2.PluginBase/` | 插件基础接口 | ~10 个 .cs |
| `Reference/` | 第三方 DLL(AForge、Bass、Spine、SharpDX、MonoGame) | 20+ DLL |

---

## 2. 顶层架构

### 2.1 解决方案结构

```
MapleNecrocer.sln
├── MapleNecrocer/                        # 主程序(WinExe)
│   ├── Program.cs                        # 入口 Main()
│   ├── MainForm.cs / MainForm.Designer.cs# 主窗口(44KB, 1000+ 行)
│   ├── RenderForm.cs / RenderFormDraw.cs # MonoGame 渲染窗口
│   ├── WzUtils.cs                        # 静态工具类 Wz / WzDict
│   ├── Extension.cs / GraphicsExtension.cs
│   ├── ImageFilter.cs                    # HSL 染色滤镜
│   ├── DataGridViewEx.cs                 # 带搜索的 DataGridView
│   ├── Win32.cs / DPIUtil.cs
│   │
│   ├── Client/                           # 客户端游戏对象
│   │   ├── MapleMap.cs                   # 地图管理
│   │   ├── MapleCharacter.cs             # 角色(主类 60KB)
│   │   ├── Mob.cs / Npc.cs / Pet.cs / TamingMob.cs
│   │   ├── MapBack.cs / MapObj.cs / MapTile.cs / MapPortal.cs
│   │   ├── MapParticle.cs / Footholds.cs / LadderRopes.cs
│   │   ├── ChatBalloon.cs / NameTag.cs / DamageNumber.cs
│   │   ├── AfterImage.cs / Skill.cs / Sound.cs
│   │   ├── Familiar.cs / Android.cs / Morph.cs / Reactor.cs
│   │   ├── MapleChair.cs / MapleEffect.cs / MapleTV.cs
│   │   ├── MouseEx.cs / ToolTip.cs
│   │   ├── Particle/                     # 粒子系统
│   │   ├── SpriteEngine/                 # 精灵引擎
│   │   └── UI/                           # UI 控件
│   │
│   ├── CharaSim/                         # 角色模拟(轻量,数据模型)
│   │   ├── Character.cs                  # 角色聚合
│   │   ├── CharacterStatus.cs
│   │   ├── CharaEquip.cs                 # 装备槽
│   │   ├── CharaProp.cs                  # 属性值
│   │   └── CharaSimLoader.cs             # 全局数据懒加载
│   │
│   ├── CharaSimControl/                  # 角色模拟 UI 控件(35 个文件)
│   │   ├── AControl.cs / ACtrlButton.cs / ACtrlVScroll.cs
│   │   ├── AfrmEquip.cs / AfrmItem.cs / AfrmStat.cs / AfrmTooltip.cs
│   │   ├── GearGraphics.cs / GearTooltipRender*.cs
│   │   ├── ItemTooltipRender*.cs / SetItemTooltipRender.cs
│   │   ├── MobTooltipRenderer.cs / NpcTooltipRenderer.cs
│   │   ├── MapTooltipRender.cs / SkillTooltipRender*.cs
│   │   ├── CashPackageRender.cs / RecipeTooltipRender.cs
│   │   └── ...
│   │
│   ├── *Form.cs                          # 各种浏览窗体(40+ Form)
│   │   ├── AvatarForm / MobForm / NpcForm / SkillForm
│   │   ├── CashForm / CashEffectForm / ConsumeForm / EtcForm
│   │   ├── ChairForm / MountForm / MorphForm / PetForm
│   │   ├── MedalForm / TitleForm / RingForm / FamiliarForm
│   │   ├── DamageSkinForm / EffectForm / EffectRingForm
│   │   ├── ChatRingForm / TotemEffectForm / SoulEffectForm
│   │   ├── ReactorForm / WorldMapForm / SaveMapForm
│   │   ├── ScaleForm / OptionForm / SelectFolderForm
│   │   └── ViewForm / RenderForm / ObjInfoForm
│   │
│   ├── SoundPlayer/                      # 音频
│   └── Resources/                        # 图标等资源
│
├── WzComparerR2.WzLib/                   # WZ/MS 解析库
├── WzComparerR2.Common/                  # 公共库
└── WzComparerR2.PluginBase/              # 插件接口
```

### 2.2 进程内模块层次

```
┌────────────────────────────────────────────────────────────┐
│   UI 层 (WinForms)                                         │
│   MainForm ── MenuStrip / TabControl / 40+ Form           │
│       │                                                    │
│       ├──> RenderForm (嵌入 MonoGame Control)             │
│       │      └──> RenderFormDraw (60Hz 渲染循环)            │
│       │                                                    │
│       ├──> AfrmTooltip (悬浮装备/物品 tooltip 窗口)        │
│       └──> 各 Form.Show() 单例模式                         │
├────────────────────────────────────────────────────────────┤
│   业务层 (Client/)                                         │
│   Map.LoadMap / Player / Mob / Npc / Pet / TamingMob     │
│   MapBack / MapTile / MapObj / MapPortal                  │
│   Particle / Skill / ChatBalloon / NameTag                │
├────────────────────────────────────────────────────────────┤
│   数据层 (CharaSim / CharaSimControl)                     │
│   Character / Gear / Item / Recipe / Skill / Mob / Npc    │
│   CharaSimLoader (懒加载缓存)                              │
│   StringLinker (ID -> 名称映射)                             │
├────────────────────────────────────────────────────────────┤
│   资源层 (WzComparerR2.WzLib / Common / PluginBase)       │
│   Wz_File / Wz_Image / Wz_Node (树形结构)                 │
│   Wz_Png / Wz_Sound / Wz_Vector / Wz_Uol                  │
│   Wz_Crypto (AES-256 ECB IV 扩展)                          │
│   Wz_Structure (多 wz 文件管理)                            │
│   PluginManager.FindWz (跨文件路径查询)                    │
├────────────────────────────────────────────────────────────┤
│   系统层 (MonoGame + SharpDX + Win32)                     │
│   MonoSpriteEngine (精灵绘制管线)                          │
│   RenderTarget2D (离屏渲染缓存)                            │
│   Spine (骨骼动画 V2/V4)                                  │
│   DirectX 11 (底层渲染 API)                                │
└────────────────────────────────────────────────────────────┘
```

---

## 3. 关键类详解

### 3.1 入口与生命周期

#### `Program.cs` (主入口)
```csharp
[STAThread]
static void Main() {
    Application.SetHighDpiMode(HighDpiMode.PerMonitor);
    ApplicationConfiguration.Initialize();
    Application.Run(new MainForm());
}
```
- 设置高 DPI 模式
- 创建 `MainForm` 并启动 WinForms 消息循环

#### `MainForm` 字段
```csharp
public partial class MainForm : Form {
    public static MainForm Instance;                          // 全局单例
    public static RenderForm RenderForm = new RenderForm();   // 嵌入的渲染窗口
    List<Wz_Structure> openedWz;                             // 已打开的 WZ 集合
    public static Wz_Node TreeNode;                           // 树根节点(用于 AdvTree)
    public DataGridViewEx MapListBox;                         // 地图列表(可搜索)
    public Dictionary<string, string> MapNames;               // 地图ID -> 名称
    public StringLinker stringLinker;                         // 字符串链接器
    public AfrmTooltip ToolTipView;                           // 装备/物品 tooltip
    DefaultLevel skillDefaultLevel = DefaultLevel.Level0;
    int skillInterval = 32;
}
```

#### 构造函数核心逻辑
```csharp
public MainForm() {
    InitializeComponent();
    Instance = this;
    openedWz = new List<Wz_Structure>();
    PluginManager.WzFileFinding += WzFileFinding;             // 注册 WZ 查找回调
    // 设置双缓冲
    if (!SystemInformation.TerminalServerSession) {...}
    RenderForm.TopLevel = false;
    RenderForm.Parent = this;
    RenderForm.Show();
    Sound.Init();                                             // 初始化音频
    ToolTipView = new AfrmTooltip();
    ToolTipView.StringLinker = this.stringLinker;
    ToolTipView.ShowID = true;
    ToolTipView.ShowMenu = true;
}
```

#### MainForm 关键方法
- `openWz(string)` — 打开 WZ 文件,根据后缀/版本路由到 `LoadMsFile`/`LoadKMST1125DataWz`/`Load`,**自动检测 WZ 区域版本**
- `DumpMapIDs()` — 列出 `String/Map.img` 与 `Map/Map/*` 下的所有地图 ID
- `WzFileFinding(...)` — `PluginManager.FindWz` 事件回调,处理跨 WZ 文件路径解析
- `QuickView(Wz_Node)` — 根据节点所在 WZ 文件类型(Character/Item/Skill/Mob/Npc/Etc/Map)创建对应的 `Gear`/`Item`/`Skill`/`Mob`/`Npc`/`Map` 对象并显示在 tooltip
- `LoadMap()` — 调用 `Map.LoadMap(Map.ID)`,把玩家定位到出生传送点,设置相机
- `SetScreenNormal()` — 切换屏幕模式(Normal/Scale/FullScreen),通过 Win32 `MoveWindow` 调整窗口
- `LoadMapButton_Click` / `FullScreenButton_Click` — 切换全屏
- `MobButton_Click` 等按钮回调 — 工厂方法创建对应的 Form 单例

### 3.2 资源层:WZ 解析

#### WZ 层次结构
```
Wz_Structure            (顶层容器,管理多个 wz 文件)
├── Wz_File[1..N]       (单个 .wz 文件,如 Character.wz, Map.wz)
│   ├── Wz_Header       (PKG1 / PKG2 头部)
│   ├── Wz_Directory[]  (目录项,懒加载)
│   └── Wz_Node 树
│       └── Wz_Image    (.img 数据块,独立 zlib 压缩)
│           ├── Wz_Png  (位图,ARGB4444/8888/1555,DXT1/3/5,BC7,RGBA1010102,R16,A8)
│           ├── Wz_Sound(MP3/PCM)
│           ├── Wz_Vector(2D 向量)
│           ├── Wz_Uol  (符号链接)
│           ├── Wz_Video
│           ├── Wz_RawData
│           └── Wz_Convex (凸多边形)
```

#### `Wz_File` (单个 WZ 文件)
- 持有 `FileStream` + `Wz_Header` + 加密 key
- `GetDirTree()` — 解析目录树(类型 `0x03=Directory`,`0x04=Image`)
- `TryExtract()` — 解压 `Wz_Image` 数据块
- 支持 `PKG1`(传统)与 `PKG2`(新)两种文件头

#### `Wz_Header`
- `WzPkg1Header`(含 `EncryptedVersion` 字段)
- `WzPkg2Header`(含 `Hash1/Hash2`,`CalcHashVersion()`)
- `WzVersionProfiles` — 嗅探 encver/格式; KMST1132+ 无 encver 时 WzVersion=777

#### `Wz_Image` (核心 30+KB)
- 单个 `.img` 节点的数据块
- 校验、加密检测、对象提取
- 解析文本式 wz(以 `#Property` 或 `Root` 开头的 `TextImageReaderV1/V2`)
- 解出的对象类型:`Property/Vector2D/Canvas/Sound_DX8/UOL/RawData/Video`

#### `Wz_Node` (树形结构)
- `Value` 可为任意类型(标量/Png/Sound/Uol 等)
- `WzNodeCollection` 为 `KeyedCollection` 索引
- 扩展方法(见 `Wz_NodeExtension2.cs`、`WzUtils.cs` 中的 `Wz_NodeExtension3`):
  - `GetNode(string path)` — 路径解析,支持 `..` 回溯
  - `GetLinkedSourceNode(GlobalFindNodeFunction)` — 查 `source/_inlink/_outlink`
  - `FullPathToFile2()` — 归一化路径(`Map001/002/2 → Map`)
  - `ExtractPng()` — 解码 `Wz_Png` 为 `Bitmap`
  - `GetValueEx<T>(default)` — 类型安全取值

#### `Wz_Png` (位图解码)
支持格式:
- 调色板索引(`Palette`)
- `ARGB4444` / `ARGB8888` / `ARGB1555` / `RGB565`
- `DXT1` / `DXT3` / `DXT5`
- `BC7` / `RGBA1010102` / `R16` / `A8` / `RGBA32Float`
- 数据可 zlib 压缩或 `ChunkedEncryptedInputStream` 加密
- `ExtractPng()` 还原 GDI+ `Bitmap`

#### `Wz_Sound`
- MP3 / PCM
- `ExtractSound()` 输出完整音频

#### `Wz_Crypto` (加密机制)
- **IV 种子**:
  - GMS: `iv_gms = {0x4d, 0x23, 0xc7, 0x2b}`
  - KMS: `iv_kms = {0xb9, 0x7d, 0x63, 0xe9}`
- **密钥生成**: `WzCryptoKey.EnsureKeySize()` 用 **AES-256-ECB** 对 IV 反复加密迭代,生成 XOR 流
- PKG1 字符串加密: `WzBinaryReader.ReadString()` 异或 `Pkg1Keys`
- PKG2 字符串加密: `Pkg2DirStringKey`(8 字节基础 key 异或) / `Pkg2DirStringKeyV2`(KMST1199,使用 `hash1 ^ hashVersion ^ 0x6D4C3B2A` 经 Mix 变换)
- 图像分块加密: `ChunkedEncryptedInputStream`
- `ChaCha20CryptoTransform.cs` / `Snow2CryptoTransform.cs` 额外支持

#### `Wz_Structure` (多文件管理)
- `Load(string path, bool useDirTree=true)` — 加载主 wz 并自动扩展
- `LoadMsFile(string path)` / `LoadKMST1125DataWz(string path)` — 加载新 MS 格式
- 加载 `Base.wz` 时自动发现 `XXXX2.wz` / `XXXX000.wz` 扩展包
- 加载 `List.wz` 目录
- Profile 缓存

#### `Ms_File` / `Ms_FileV2` / `Ms_Image` / `Ms_ImageV2`
- 新 MS 格式(V1 / V2)的独立实现
- 独立 Header + Entry

### 3.3 公共层

#### `StringLinker` (字符串链接器)
**作用**: 把游戏内 ID 映射到多语言名称/描述/技能说明

**字段**: 7 个 `Dictionary` 按类型分(Eqp / Item / Map / Mob / Npc / Skill / SetItem)+ 1 个 `Dictionary<string, StringResult> stringSkill2`(技能文本键索引)

**核心方法**:
```csharp
public bool Load(Wz_File stringWz, Wz_File itemWz, Wz_File etcWz);
public bool Load(Wz_Node stringNode, Wz_Node itemNode, Wz_Node etcNode);
public string GetDefaultString(Wz_Node node, string propName);
public void AddAllValue(StringResult sr, Wz_Node node);
public void Clear();
public bool HasValues { get; }
```

**加载策略**: 遍历 img 子节点(`Pet.img` / `Cash.img` / `Mob.img` / `Skill.img` 等),按 img 类型走不同嵌套深度(技能 3 层,装备 3 层,物品 1-2 层),`Int32.TryParse(node.Text, out id)` 后提取 `name/desc/pdesc/mapName/streetName/h/h1...` 字段写入 `StringResult`。

#### `Calculator` (表达式求值器)
- 静态类,求值带参数 `x, y, z, w` 的四则运算数学表达式
- 用于冒险岛装备属性公式(如 `x*2+30`)
- 流水线: `Lexer` 词法分析 → `Suffix` 转逆波兰式 → `Execute` 栈式求值
- 内置函数: `u()` 向上取整、`d()` 向下取整、`logN(x)` 以 N 为底的对数
- **签名**: `decimal Parse(string mathExpression, params decimal[] args)`

#### `SpineLoader` (骨骼动画加载)
**作用**: 从 WZ 节点对(`.atlas` + `.json`/`.skel`)加载 Spine 骨骼动画

**流程**:
- `SpineDetectionResult Detect(Wz_Node)` — 根据入参节点是 atlas 还是 skel,在兄弟节点中互查同名前缀,处理 KMST 1172 的 `SharedAtlasNodeName = "atlas"`,调用 `ResolveUol()` 解引用
- `ReadSpineVersionFromJson(string)` — 解析 `{"skeleton":{"spine":"x.x.x"}}`
- `ReadSpineVersionFromBinary(Stream, uint, int)` — 调官方库
- 推断 `SpineVersion.V2 / V4`
- `LoadSkeletonV2(Wz_Node, Spine.V2.TextureLoader)` / `LoadSkeletonV4(...)`

#### `BitmapOrigin` (位图+锚点)
- 把 `Bitmap` 与其 `Point origin`(贴图锚点)绑定
- `OpOrigin` / `Rectangle` / `CreateFromNode(Wz_Node, GlobalFindNodeFunction)`
- 跟随 `Wz_Uol`,调用 `GetLinkedSourceNode` 找 PNG,读取同节点下 `origin` 子节点

#### `Gif` / `GifCanvas` / `GifFrame` / `GifLayer`
- `Gif`:帧集合 `List<IGifFrame> Frames`
- `EncodeGif(...)` → `BuildInGifEncoder` / `EncodeGif2(...)` → `IndexGifEncoder`(调色板版)
- `GetRect()` 合并所有帧的 `Region`
- `CreateFromNode(Wz_Node, GlobalFindNodeFunction)` 按子节点 `0,1,2...` 顺序读取帧
- `GifCanvas`:多图层合成器
  - `Layers: List<GifLayer>`, `AlphaGradientDelay: int`(默认 30)
  - `Combine(): Gif` 流程: 收集所有图层所有帧的累计延时 → 二分搜索排序去重 → 按延时构建关键帧链表 → 对每个图层按关键帧时长拆帧(线性插值 alpha 渐变) → 合并为单个 `Gif` 对象

### 3.4 业务层:Client/

#### `EngineFunc`(全局单例,见 `Global.cs`)
- 静态持有:
  - `SpriteEngine` / `BackgroundEngine` — 两个 `MonoSpriteEngine` 实例
  - `GameCanvas` — 共享画布
  - 字体字典: `XnaFont` / `D2DFont`
- `FixedUpdate` 60Hz 步进
- `Map.LoadMap` 中 `EngineFunc.SpriteEngine.Camera` 充当世界相机

#### 精灵类层次
```
Sprite                                    (基类)
└── SpriteEx                              (ImageNode/Visible/Tag/DoMove/DoDraw)
    ├── AnimatedSprite                    (Animate/AnimRepeat/Frame)
    │   ├── BackgroundSprite              (TileMode 铺贴)
    │   │   └── Back                      (8 种 BackType 0-7)
    │   │       ├── 0-3: 基础
    │   │       └── 4-7: 自滚动(自动 RX/RY 像素/帧)
    │   │   └── FlowObj                   (横向/纵向流动铺贴)
    │   │   └── SpineBack                 (Spine 骨骼平铺)
    │   ├── JumperSprite                  (JumpSpeed/Height/Fall)
    │   │   ├── Player                    (键盘、跳跃、梯子、墙壁、传送)
    │   │   ├── Mob
    │   │   ├── Pet
    │   │   ├── Familiar
    │   │   └── TamingMob
    │   ├── ParticleSprite                (特效)
    │   └── PathSprite                    (NURBS 曲线)
    └── ...
```

#### Z 序(整数大数编码)
- `Player = 20000`
- `Mob = FH.Z * 100000 + 6000`
- `Front 背景 + 1000000`
- `MapTile / MapObj: Z = Layer * 100000 + z`

#### `Map` (静态类,地图主入口)
**`LoadMap(ID)` 流程**:
1. 清理旧精灵 / PlayerEx
2. 读 `Map/Map/X/ID.img`
3. 解析 `info`(link / miniMap / BGM / 视野 / 范围)
4. 建立 portal / foothold / tile / obj / back / particle 边界与相机
5. `Npc.Create()`、`Mob.Create()` 遍历 `life` 节点生成角色
6. 播放 BGM
**`FadeScreen`** — 内嵌实现传送淡入淡出

#### `Player`(`MapleCharacter.cs`,主类 60KB)
- 继承 `JumperSprite`
- 处理键盘、跳跃、梯子、墙壁、传送
- `CreateEquip` 加载每个装备部件
- `AvatarEngine`(独立 `MonoSpriteEngine`)先离屏渲染到 `AvatarTargetTexture` 1024×1024,再整体画到主屏
- `AvatarParts : SpriteEx` 是装备部件单元
- `UpdateFrame` 读取 `body/state/frame/delay` 切换帧
- 骨骼绑定关键节点:
  - `map/neck` — 头
  - `map/navel` — 身体
  - `map/hand` / `map/handMove` — 手臂
  - `map/brow` — 眉毛
- `Cap vslot` 决定是否遮盖头发
- `ZMap` + `path + /z` 控制部件 Z 序
- F1-F7 切换表情,Ctrl 攻击随机招式

#### `Mob`(怪物)
- 继承 `JumperSprite`
- 读 `Mob/ID.img` link → 生成多状态(stand / move / jump / fly / die1 / hit1)
- `DoMove` 随机切换方向、撞墙反转、被击退(`Zigzag/AnimDelay/a1` 透明度淡出)
- `MobCollision` 接收 `SkillCollision/AfterImage` 触发,扣 HP → `GetHit1/Die` → `DamageNumber.Create`
- `RenderTarget2D` 缓存 Lv+名称血条

#### `Npc`
- 继承 `SpriteEx`
- 简单动作循环(支持多 action 随机切换)
- `NpcText` 绘制黄色名称+功能名+ID
- `ChatBalloon` 在 NPC 头顶,`Msgs` 随机取一句,500 周期内显示
- `MapleTV` 标志位启动直播

#### `Pet`(宠物)
- 继承 `JumperSprite`
- 用 `Item/Pet/ID.img`,`stand0/move/hang` 状态
- 自动跟随玩家
- 跟随距离 > 70 触发 `move` 与跳跃
- 梯子上 `hang`,自动按 FH 寻路
- `PetNameTag` 继承 `MedalTag` 绘图,`PetEquip` 叠加在宠物身上的装备图层

#### `TamingMob`(坐骑)
- `LoadSaddleList` 缓存 `0191****` 鞍具映射
- `Create(ID)` 同时加载鞍+坐骑
- `DoMove` 跟随玩家状态(走/跳/趴/梯/绳)
- 通过 `characterAction` 字典返回 `stabT2/hideBody/fly` 等指令给 `Player`
- `Z` 经 `AvatarParts.ZMap` 计算实现身体被坐骑遮蔽
- `Navel` 向量驱动角色位置补偿

#### `MapBack`(多层滚动背景)
- 支持 8 种 `BackType` (0-7)
- 计算 `X = -PosX - (100+RX)/100 * (Camera + 屏幕/2) + Camera` 实现视差
- `flowX/Y` 持续流动
- `moveType` 1-3 给出 Cos 摆动;`moveR` 旋转
- `ani=2` 走 `SpineBack`,用 `SpineAnimatorV2/V4` + `SkeletonRenderer` 拼接平铺
- `Front=true` 画在玩家之上

#### `MapObj` / `MapTile` / `MapPortal`
- `MapTile`: 8 层(0-7)读 `Map/Tile/tS.img/u/no`, `Z = Layer*100000 + z + 1000`, 静态
- `MapObj`: `Obj`(静态/动画/`a0-a1` 淡入淡出/`moveType` 摆动/`moveR` 旋转)、`FlowObj`(横向/纵向流动铺贴)、`SpineObj`(V2/V4 Spine 骨骼)。`Z = Layer*100000 + z`
- `MapPortal`: 读取 `Map.Img/portal` 的 `pt/tm/tn/pn`, `Type=2/10` 显示动画, `Version 0-2` 自适应 `MapHelper.img`, `Find(P, ref onPortal)` 给玩家传送判定, `Type=10` 渐显

#### `AfterImage` / `DamageNumber` / `ChatBalloon` / `NameTag`
- `AfterImage`: `CanCollision=true`, 攻击时 `Create(Path)`, 逐帧推进, 通过 `lt/rb` 维护碰撞盒, `a1` 透明度淡出后 `Dead`, `OnCollision` 命中 Mob 扣 HP+触发 hit1 动作
- `DamageNumber`: 支持新旧两套皮肤(`Effect/BasicEff.img` vs `Effect/DamageSkin.img`),首字大其余小,逐位画到 `Y-0.5f/帧` 上升,50 帧后 `Alpha-=6` 淡出
- `ChatBalloon`: `SetStyle` 读 `UI/ChatBalloon.img/Style` 8 块贴图(arrow/c/n/ne/e/s/se/sw/w),按 `Col` 拼接, `MaxChars=90`, `RenderTargetFunc` 离屏绘制 3 行模板+文字, `ParseText` 按词换行
- `NameTag`: `RenderTarget2D` 缓存"黑色圆角底+白字", `MedalTag` 九宫格(w/c/e)拼接, `FixAlpha` 修半透明 PNG 边缘

#### `FootholdTree`(单例,见 `Footholds.cs`)
- 维护 FH 树结构
- `FindBelow(Vector2, ref Foothold)` — 给定 X 找脚下 FH
- `DrawFootholds()` — 调试用绘制

#### `Particle`(粒子系统,见 `Particle/`)
- `ParticleSprite` + `IRandom` 接口
- `MapParticle` 管理地图粒子

#### `Sound`(音频,见 `Sound.cs` + `SoundPlayer/`)
- `Init()` — 初始化 Bass
- `PlayendList` — 已结束播放列表(>100 时清理)
- 使用 `ManagedBass` 跨平台音频

### 3.5 数据层:CharaSim

#### `Character`(角色聚合)
**字段**:
- `Name`, `Guild`
- `Status` (CharacterStatus 角色属性集)
- `ItemSlots` (5×96 的 `ItemBase` 二维数组,物品栏)
- `Equip` (CharaEquip 装备槽)
- 静态 `Version` (FormulaVersion: Bigbang / Chaos 公式版本)

**关键方法**:
- `UpdateProps()` — 遍历已装备的 Gear 各项属性(基础/潜能/附加),通过 `addProp()` 累加到 `status` 各 `CharaProp`,并根据主属性计算二级战斗属性
- `ChangeGear(Gear)` / `ChangeGear(Gear, int)` — 穿装备,含 `checkGearReq` 职业限制、`checkGearPropReq` 属性需求判断,背包满则抛异常
- `CheckGearEnabled()` — 迭代法使装备需求自洽
- `CalcAttack()` — 按武器类型与公式版本计算最大/最小攻击力
- `ExpToNextLevel(int)` — 静态 1-300 级升级经验表

#### `CharaEquip`(装备槽)
- 66 个装备槽
- 两个 `Gear[66]`: `gearSlots` / `cashGearSlots`
- `RingCount=6`、`PendantCount=2`
- `GetGearSlot(GearType, int)` — 装备类型映射到具体槽位索引
- `AddGear(gear, out removed)` / `AddGear(gear, index, out removed)` — 放入装备,自动处理双手/副手/双弩/套服等冲突
- `GearsEquiped` — 迭代器遍历所有已装备

#### `CharaProp`(属性值)
**字段**: `BaseVal`, `GearAdd`, `BuffAdd`, `EBuffAdd`, `Rate`, `ABuffRate`, `PBuffRate`, `TotalMax`, `Smart`
**方法**:
- `GetSum()` — `(base + gear + buff + eBuff) * (100 + rate + aBuffRate + pBuffRate) / 100`,再按 `TotalMax` 截断
- `GetGearReqSum()` — 装备需求判定用(不含 eBuff)
- `ResetAdd()` / `ResetAll()`

#### `CharaSimLoader`(全局数据懒加载)
**字段**:
- `LoadedSetItems` (Dictionary<int,SetItem>)
- `LoadedExclusiveEquips` (Dictionary<int,ExclusiveEquip>)
- `LoadedCommoditiesBySN` / `LoadedCommoditiesByItemId` (商城商品)

**方法**:
- `LoadSetItems()` — 从 `Etc/SetItemInfo.img` + `Item/ItemOption.img` 构建 SetItem
- `LoadExclusiveEquips()` — 从 `Etc/ExclusiveEquip.img` 加载
- `LoadCommodities()` — 从 `Etc/Commodity.img` 加载
- `GetActionDelay(string)` — 从 `Character/00002000.img/<action>` 累加帧 delay

### 3.6 角色模拟 UI 控件:CharaSimControl/

`AControl` / `ACtrlButton` / `ACtrlVScroll` — 自定义控件基类

`AfrmEquip` / `AfrmItem` / `AfrmStat` / `AfrmTooltip` — 装备/物品/属性/tooltip 窗体

`GearGraphics` / `GearTooltipRender` / `GearTooltipRender2` — 装备图形渲染(35KB+82KB)
- 装备穿着图
- 装备 tooltip 详细属性
- 支持 V2(经典版)/ V2(新版)两套渲染

`ItemTooltipRender` / `ItemTooltipRender2` — 物品 tooltip(57KB)

`SetItemTooltipRender` — 套装 tooltip(22KB)
`MobTooltipRenderer` — 怪物 tooltip
`NpcTooltipRenderer` — NPC tooltip
`MapTooltipRender` — 地图 tooltip
`SkillTooltipRender` / `SkillTooltipRender2` — 技能 tooltip
`CashPackageRender` — 现金商品渲染(23KB)
`RecipeTooltipRender` — 配方 tooltip
`HelpTooltipRender` — 帮助 tooltip

`CharaSimControlGroup` — 组合控件容器
`RenderHelper` — 渲染助手
`TextBlock` — 文本块
`ButtonState` — 按钮状态枚举

### 3.7 业务层: 40+ 浏览 Form

| Form | 作用 |
|---|---|
| `AvatarForm` (60KB) | 角色 Avatar 组合(全部装备试穿预览) |
| `MobForm` | 怪物列表与预览 |
| `NpcForm` | NPC 列表与预览 |
| `SkillForm` | 技能列表与预览 |
| `ItemForm`(在 CharaSimControl 中) | 物品网格 |
| `CashForm` | 现金物品 |
| `CashEffectForm` | 现金特效 |
| `ConsumeForm` | 消耗品 |
| `EtcForm` | 杂物 |
| `ChairForm` | 椅子 |
| `MountForm` (33KB) | 坐骑(独立大模块) |
| `MorphForm` | 变形 |
| `DamageSkinForm` | 伤害皮肤 |
| `MedalForm` | 勋章 |
| `TitleForm` | 称号 |
| `RingForm` | 戒指 |
| `PetForm` | 宠物 |
| `FamiliarForm` | 家族宠物 |
| `AndroidForm` | 机器人/机器人 |
| `TotemEffectForm` | 图腾效果 |
| `SoulEffectForm` | 灵魂效果 |
| `ReactorForm` | 反应堆 |
| `EffectForm` | 效果 |
| `EffectRingForm` | 效果环 |
| `ChatRingForm` | 聊天环 |
| `ViewForm` | 视图 |
| `WorldMapForm` | 世界地图 |
| `SaveMapForm` | 保存地图 |
| `ScaleForm` | 缩放 |
| `OptionForm` | 选项 |
| `SelectFolderForm` | 文件夹选择 |
| `ObjInfoForm` | 对象信息 |
| `RenderForm` | 渲染窗口(嵌入 MainForm) |

---

## 4. 关键全局数据结构 (`WzUtils.cs`)

```csharp
internal class Wz {
    // 静态全局缓存
    public static Dictionary<string, Wz_Node> Data = new();         // 节点缓存
    public static Dictionary<string, Wz_Node> EquipData = new();    // 装备数据缓存
    public static Dictionary<string, Texture2D> ImageKeys = new();  // 图像键
    public static Dictionary<Wz_Node, Texture2D> ImageLib = new();  // 通用图像库
    public static Dictionary<Wz_Node, Texture2D> EquipImageLib = new(); // 装备图像库
    public static Dictionary<Wz_Node, Texture2D> UIImageLib = new();// UI 图像库
    public static Dictionary<string, Wz_Node> UIData = new();       // UI 数据
    
    // 兼容性标志
    public static string Region;                  // 自动检测的 WZ 区域
    public static bool HasStringWz;               // 是否有 String.wz
    public static bool HasHardCodedStrings;       // 是否硬编码字符串
    public static bool HasMap9Dir;                // 是否有 Map1-9 目录
    public static bool IsDataWz;                  // 是否 Data.wz 格式
    
    // 路径解析工具
    public static Wz_Node GetNode(string path);   // 跨文件路径解析(走 PluginManager.FindWz)
    public static Wz_Vector GetVector(string path);
    public static Bitmap GetBmp(string path);
    public static int GetInt(string path, int def=0);
    public static string GetStr(string path, string def="");
    public static bool GetBool(string path);
    public static bool HasNode(string path);
    public static Wz_Node GetNodeByID(string ID, WzType wzType);  // 通过 ID 反查节点
    
    // 数据扫描
    public static void DumpData(Wz_Node, dict, imageLib, useDye, hue, sat, light);
    public static void DumpDataA(Wz_Node, dict, imageLib);  // 单遍扫描
}
```

### 4.1 跨 WZ 文件路径解析(`Wz_NodeExtension3.GetNode`)

这是项目最复杂的逻辑之一,处理 4 种情况:
1. **节点存在且非 UOL**:直接返回(可能经过 `Map001 → Map` 归一化)
2. **节点存在但是 UOL**:
   - 解析 UOL 字符串
   - 递归跳转到目标节点
   - 处理 `_inlink` / `_outlink` 跨文件引用
3. **节点不存在**:扫描路径,找到第一个 UOL 节点,跳转后再取子路径
4. **图像外链**:处理 `source` / `_inlink` / `_outlink` 字段

---

## 5. 渲染管线

### 5.1 MonoGame 集成
- `MonoGame.Forms.DX` 提供 `MonoGameControl` 基类,嵌入 WinForms
- `RenderFormDraw` 继承 `MonoGameControl`
- 底层用 `SharpDX.Direct3D11` 调用 DirectX 11
- 不使用 OpenGL 渲染器(因为 Windows 优先)

### 5.2 主循环(60Hz FixedUpdate)
```csharp
protected override void Update(GameTime gameTime) {
    if (PreviousTime == 0) PreviousTime = (float)gameTime.TotalGameTime.TotalMilliseconds;
    float Now = (float)gameTime.TotalGameTime.TotalMilliseconds;
    float FrameTime = Now - PreviousTime;
    if (FrameTime > TimeDelta) FrameTime = TimeDelta;  // TimeDelta 由显示器刷新率计算
    PreviousTime = Now;
    Accumulator += FrameTime;
    while (Accumulator >= FixedUpdateDelta) {  // FixedUpdateDelta = 0.016666668f (60Hz)
        UpdateGame();
        Accumulator -= FixedUpdateDelta;
    }
}
```

### 5.3 帧逻辑(`UpdateGame()`)
1. **Viewer 模式**: 方向键移动相机
2. **重载地图**: `if (Map.ReLoad) Map.LoadMap(Map.ID)`
3. **相机平滑**: `Map.CameraSpeed = NewPos - CurrentPos`
4. **精灵推进**: `EngineFunc.SpriteEngine.Move(1)`
5. **椅子状态**: 按左/右键取消椅子
6. **屏幕模式分支**:
   - `Normal`: 直接画到屏幕
   - `Scale / FullScreen`: 先画到 `ScreenRenderTarget` (4000×4000),再用 `DrawStretch` 拉伸到目标尺寸
7. **FadeScreen 淡入淡出**
8. **BGM 名称显示**
9. **Foothold 调试绘制**
10. **声音列表清理**(`Sound.PlayendList.Count == 100` 时清理)

### 5.4 帧渲染(`Draw()`)
1. 清屏 `Color.Black`
2. `SpriteEngine.Dead()` 清理已死精灵
3. 按 `ScreenMode` 分支绘制:
   - `Normal`: 直接 `SpriteEngine.Draw()`
   - `Scale`: 拉伸 `ScreenRenderTarget`,可选 `ScanlineTexture4096`
   - `FullScreen`: 拉伸到全屏
4. **Map.ResetPos**: 重置背景/粒子位置
5. **UI 控件更新/绘制** (`UI.ControlManager.Update()` / `Draw()`)
6. **GameCursor 绘制** (自定义光标)

### 5.5 字体系统
```csharp
// GMS 字体
EngineFunc.AddFont(gd, "Arial13", "Arial", 13f);
EngineFunc.AddD2DFont("Arial14", "Arial", 14f);
EngineFunc.AddD2DFont("Arial13", "Arial", 13f);
EngineFunc.AddD2DFont("Arial12", "Arial", 12f);
EngineFunc.AddD2DFont("Arial10", "Arial", 10f);
// TMS 字体
EngineFunc.AddFont(gd, "Verdana11", "Verdana", 11f);
EngineFunc.AddFont(gd, "SimSun13", "SimSun", 13f);
EngineFunc.AddFont(gd, "SimSun14", "SimSun", 14f);
EngineFunc.AddFont(gd, "Verdana9", "Verdana", 9f);
// JMS 字体
EngineFunc.AddFont(gd, "MSGothic11", "MS Gothic", 11f);
EngineFunc.AddFont(gd, "MSGothic12", "MS Gothic", 12f);
EngineFunc.AddFont(gd, "MSGothic14", "MS Gothic", 14f);
```
- `XnaFont` — SpriteFont(XNA 字体光栅化)
- `D2DFont` — Direct2D 字体(更清晰)
- TMS 区域用 SimSun(宋体)
- JMS 区域用 MS Gothic

### 5.6 离屏渲染(`RenderTarget2D`)
用于缓存: 名称标签、伤害数字、装备 Avatar(1024×1024)、屏幕(4000×4000)

### 5.7 屏幕模式
- **Normal**: 1024×768 默认,在 RenderForm 内绘制
- **Scale**: 用 `ScaleForm.ScaleX/ScaleY` 缩放,可加扫描线
- **FullScreen**: `Screen.PrimaryScreen.Bounds` 全屏

---

## 6. 程序运行逻辑

### 6.1 启动流程
```
Main() 入口
  → ApplicationConfiguration.Initialize()
  → new MainForm() 构造
       → InitializeComponent() — 创建 UI 控件树
       → Instance = this
       → PluginManager.WzFileFinding += WzFileFinding  注册跨文件路径查找回调
       → RenderForm.Parent = this; RenderForm.Show()    嵌入渲染窗口
       → Sound.Init()                                   初始化音频(Bass)
       → ToolTipView = new AfrmTooltip()                创建 tooltip
       → stringLinker = new StringLinker()
  → MainForm_Load 事件
       → MapListBox = new DataGridViewEx(...)
       → MapListBox.SetToolTipEvent(WzType.Map, this)
       → comboBox2.SelectedIndex = 1                    默认屏幕模式
       → 检测 DPI
  → Application.Run 启动消息循环
```

### 6.2 打开 WZ 文件流程
```
btnItemOpenWz_Click / OpenFolderButton_Click
  → OpenFileDialog 选择文件 / 选择文件夹
  → openWz(string path)
       1. 检查是否已打开 → 是则提示
       2. new Wz_Structure()
       3. 根据扩展名分发:
          - .ms  → wz.LoadMsFile(path)
          - KMST1125 格式 → wz.LoadKMST1125DataWz(path), 加载 Packs/*.ms
          - 其他 → wz.Load(path, true)
       4. sortWzNode(wz.WzNode) 按 imgID 排序
       5. createNode(wz.WzNode) 转为 AdvTree 节点
       6. openedWz.Add(wz)
  → 探测 WZ 格式
       Wz.IsDataWz = (Wz.GetNode("Mob").FullPathToFile.LeftStr(4) == "Data")
       Wz.HasStringWz = (Wz.HasNode("Mob/0100100.img/info/name") == false)
       Wz.HasHardCodedStrings = (无 String.wz 时)
       Wz.HasMap9Dir = (Wz.HasNode("Map/Map/Map1"))
```

### 6.3 浏览地图
```
DataGridViewEx 列出 Map.img 节点 → CellClick
  → CellClick(BaseDataGridView, e)
       → Map.ID = ID (经过 info/link 重定向)
       → pictureBox1.Image = Wz.GetBmp("Map/.../miniMap/canvas")  小地图预览
  → LoadMapButton_Click → LoadMap()
       → Map.LoadMap(Map.ID) 加载地图
            → 清理旧精灵
            → 读 Map/Map/X/ID.img
            → 创建 Portal / Foothold / Tile / Obj / Back / Particle
            → 创建 Mob / Npc
            → 播放 BGM
       → 定位玩家到出生 portal
            Game.Player.X, Game.Player.Y
            BelowFH = FootholdTree.FindBelow(...)
       → 设置相机
            EngineFunc.SpriteEngine.Camera.X/Y
       → LoadedEff 首次加载
            加载 SetEffect / ItemEffect / TamingMob
            根据 WZ 节点存在与否启用/禁用按钮
            (Mount/Cash/CashEffect/Morph/DamageSkin/Medal/Title/Ring/Familiar/Android/Totem/Soul/Reactor)
       → ObjInfoForm.DumpObjs()
```

### 6.4 主循环
```
每帧 (60Hz FixedUpdate):
  UpdateGame()
    1. 处理输入 (Viewer 模式移动相机,Player 模式移动角色)
    2. Map.ReLoad 时重新 LoadMap
    3. SpriteEngine.Move(1) — 所有精灵 DoMove
    4. 椅子状态处理
    5. 绘制到 ScreenRenderTarget 或直接屏幕
    6. 清理播放完毕的 Sound
  Draw()
    1. 清屏
    2. SpriteEngine.Dead() — 清理已死精灵
    3. 按 ScreenMode 绘制
    4. 绘制 UI 控件
    5. 绘制自定义光标
```

### 6.5 QuickView(快速查看装备/物品)
```
用户在 AdvTree 中选择节点
  → QuickView(Wz_Node node)
       1. 根据所在 Wz_File.Type 分发
       2. Character → Gear.CreateFromNode(image.Node)
       3. Item → Item.CreateFromNode(itemNode) (按路径正则判断子类型)
       4. Skill → Recipe / Skill.CreateFromNode
       5. Mob → Mob.CreateFromNode
       6. Npc → Npc.CreateFromNode
       7. Etc → SetItem (从 LoadedSetItems 缓存查)
       8. Map → new Map(); Map.ImgNode = node
       9. ToolTipView.TargetItem = obj
       10. ToolTipView.Refresh() / Show()
```

### 6.6 跨 WZ 路径解析
```
Wz.GetNode("Map/Map/Map0/100000000.img/info/link")
  → PluginManager.FindWz(path)  // 触发 WzFileFinding 事件
       1. 解析 path 第一段,得到 Wz_Type
       2. 在所有 openedWz.wz_files 中找匹配 Type 的
       3. 排除 NL/ES/FR/DE 区域
       4. 找不到则尝试 baseWz (Data.wz) 内的子目录
       5. 对 preSearch 节点,逐级沿路径走
       6. 遇到 Wz_Image 则 TryExtract 后继续
       7. 返回最终节点
  → 如果节点是 Wz_Uol,调用 ResolveUol()
  → 如果节点有 _inlink / _outlink / source,继续跳转
```

---

## 7. 关键调用关系图

### 7.1 WZ 打开与解析
```
MainForm.openWz(path)
  └─> Wz_Structure.Load / LoadMsFile / LoadKMST1125DataWz
       └─> Wz_File 构造
            └─> Wz_Header 解析 (PKG1 / PKG2)
            └─> Wz_Crypto 初始化 (IV → AES 扩展密钥)
            └─> Wz_Directory[] 读取
            └─> Wz_Node 树构建 (懒加载)
  └─> sortWzNode
  └─> createNode (转为 AdvTree.Node)
  └─> Wz.IsDataWz / HasStringWz / HasHardCodedStrings / HasMap9Dir 检测
```

### 7.2 渲染管线
```
RenderFormDraw.Update (60Hz)
  └─> UpdateGame
       ├─> EngineFunc.SpriteEngine.Move(1)
       │    └─> 所有 Sprite.DoMove (Player / Mob / Npc / Pet / TamingMob / Back / MapObj / MapTile / MapPortal / Particle / ChatBalloon / NameTag / DamageNumber / AfterImage)
       └─> ScreenRenderTarget 绘制 (Scale/FullScreen)
  └─> RenderFormDraw.Draw
       ├─> SpriteEngine.Dead()
       ├─> SpriteEngine.Draw()
       │    └─> 所有 Sprite.DoDraw
       │         └─> ImageLib[ImageNode] → Texture2D
       │         └─> SpriteBatch.Draw
       ├─> Map.FadeScreen 淡入淡出
       ├─> Map.ShowBgmName
       ├─> Map.ShowFootholds → FootholdTree.DrawFootholds
       ├─> UI.ControlManager.Update / Draw
       └─> GameCursor.Draw
```

### 7.3 装备/物品查看
```
DataGridViewEx CellClick
  └─> MainForm.CellClick
       └─> 解析 map id,设 Map.ID
       └─> pictureBox1.Image = Wz.GetBmp(miniMap)
  或
AdvTree 节点双击
  └─> MainForm.QuickView(node)
       └─> PluginManager.FindWz (WzFileFinding 事件)
       └─> 根据 Wz_Type 分发:
            Character → Gear.CreateFromNode (WzComparerR2.Common.CharaSim)
            Item → Item.CreateFromNode
            Skill → Skill / Recipe.CreateFromNode
            Mob → Mob.CreateFromNode
            Npc → Npc.CreateFromNode
            Etc → SetItem (从缓存)
            Map → new Map()
       └─> ToolTipView.TargetItem = obj
       └─> ToolTipView.Refresh / Show
```

### 7.4 跨文件路径查找
```
Wz.GetNode("Foo/Bar/Baz")
  └─> PluginManager.FindWz(path)
       └─> WzFileFinding 事件
            ├─> 解析 path[0] 为 Wz_Type
            ├─> 遍历 openedWz.wz_files 找匹配 Type
            ├─> 排除 NL/ES/FR/DE
            ├─> 没找到则在 baseWz (Data.wz) 内找子目录
            └─> 逐级沿 path[1..] 走
                 ├─> 遇到 Wz_Image → TryExtract → 继续
                 └─> 返回节点
  └─> 处理 UOL / _inlink / _outlink / source
```

### 7.5 字体与渲染资源
```
RenderFormDraw.Initialize
  └─> EngineFunc.AddFont / AddD2DFont
  └─> ScreenRenderTarget = new RenderTarget2D(4000, 4000)
  └─> MouseEx.PlatformSetWindowHandle
  └─> TimeDelta = 1 / GetDeviceCaps(hdc, 116)  // 显示器刷新率

Wz.DumpData (扫描全树)
  └─> Scan1 (遇到 Wz_Uol 记入 NodeList1;遇到 Wz_Png 解码为 Texture2D)
  └─> Scan2 (沿 UOL 链跳转到目标,合并)
  └─> Scan3 (剩余 UOL 替换)
```

---

## 8. 第三方依赖

### 8.1 核心依赖
| 库 | 用途 |
|---|---|
| `DevComponents.DotNetBar2` | Office 风格 UI 控件库 |
| `MonoGame.Framework` + `MonoGame.Forms.DX` | 游戏循环、渲染管线 |
| `SharpDX` (Direct2D1, Direct3D11, DXGI, Mathematics, MediaFoundation, XAudio2, XInput) | DirectX 绑定 |
| `Spine` (spine-monogame) | 骨骼动画运行时 V2/V4 |
| `AForge` / `AForge.Imaging` | 图像处理(HSL 滤镜) |
| `ImageListView` | 缩略图列表 |
| `ImageManipulation` | 高级图像处理 |
| `ManagedBass` + `bass.dll` | 跨平台音频库 |
| `CharaSimResource` | 角色模拟静态资源 |

### 8.2 隐式依赖
- .NET 8.0 Windows Forms(UI 框架)
- Windows API(`User32.dll`, `gdi32.dll` — `MoveWindow`, `GetDeviceCaps`)
- DirectX 11(通过 SharpDX)

---

## 9. 关键设计模式与决策

### 9.1 设计模式
1. **单例模式** — `MainForm.Instance`, `RenderForm.Instance`, `EngineFunc`, `FootholdTree.Instance`
2. **静态类 / 静态字典** — `Wz.Data / ImageLib`, `EngineFunc.SpriteEngine`, `WzDict` 全局工具
3. **工厂方法** — `Gear.CreateFromNode`, `Item.CreateFromNode`, `Mob.CreateFromNode` 等
4. **状态机** — `JumperSprite.JumpState` (jsNone / jsJumping / jsFalling), `Mob.GetHit1/Die`
5. **享元模式** — `ImageLib[Wz_Node] → Texture2D` 缓存避免重复解码
6. **懒加载** — `Wz_Image.TryExtract`, `CharaSimLoader.LoadSetItemsIfEmpty`
7. **观察者/事件** — `PluginManager.WzFileFinding += ...`
8. **Composite** — `Wz_Node` 树形结构
9. **Template Method** — `Sprite` → `SpriteEx` → `JumperSprite` / `AnimatedSprite` / `BackgroundSprite` 的 `DoMove/DoDraw`
10. **Strategy** — `ScreenMode` (Normal/Scale/FullScreen) 切换渲染分支

### 9.2 性能优化
- **图像缓存**: `Dictionary<Wz_Node, Texture2D>` 避免重复 GPU 上传
- **离屏渲染**: `RenderTarget2D` 缓存名称/伤害数字/Aquarium
- **懒加载**: WZ 节点、WZ 图像、SetItem 等
- **60Hz 固定步长**: `FixedUpdate` 锁定物理/动画频率,与显示器刷新率解耦
- **MapID 索引**: MapNameList 用 `Dictionary<int, MapNameRec>`

### 9.3 跨平台问题
- 原项目是 **Windows 专用**:
  - `[STAThread]` COM 公寓模式
  - `Windows Forms` 窗体(`Form`, `PictureBox`, `DataGridView`)
  - `Win32.dll` P/Invoke (`MoveWindow`, `GetDC`, `GetDeviceCaps`)
  - `SharpDX` DirectX 11(Windows 独有)
  - `DotNetBar2` 第三方 Windows 控件
- 不能直接在 Linux/macOS 运行,需替换 UI 框架与渲染后端

---

## 10. 项目规模与代码量

| 模块 | .cs 文件 | 代码行(约) |
|---|---|---|
| MapleNecrocer | ~70 | ~10,000 |
| MapleNecrocer/CharaSimControl | 35 | ~6,000 |
| MapleNecrocer/Client | 30 | ~5,000 |
| WzComparerR2.WzLib | ~30 | ~5,000 |
| WzComparerR2.Common | ~20 | ~3,000 |
| WzComparerR2.PluginBase | ~10 | ~500 |
| **总计** | **~200** | **~30,000** |

最大单文件:
- `MapleCharacter.cs` — 60KB / 1700+ 行
- `GearTooltipRender2.cs` — 82KB
- `ItemTooltipRender2.cs` — 57KB
- `AvatarForm.cs` — 58KB
- `MainForm.cs` — 45KB
- `MountForm.cs` — 33KB
- `Wz_Image.cs` — 34KB

---

## 11. 总结

MapleNecrocer 是一个相对完整、面向冒险岛资源的**可视化浏览器与渲染器**,其核心价值在于:

1. **WZ/MS 格式解析**: 同时支持 KMS / GMS / JMS / TMS、新旧格式
2. **完整渲染管线**: MonoGame + DirectX + 60Hz 游戏循环
3. **复杂游戏对象系统**: 角色/怪物/NPC/宠物/坐骑/背景/对象/传送门/粒子,完整模拟
4. **40+ 浏览窗体**: 装备、物品、技能、怪物、NPC、地图、世界地图、椅子、坐骑、伤害皮肤…
5. **数据驱动**: 所有游戏内容来自 WZ 文件,无内置硬编码数据

**移植到 C++/Qt 的核心挑战**:
- WZ/MS 格式解析(约 5,000 行,涉及 AES 加密、zlib、DXT/BC7 图像解码)
- 2D 渲染管线(Qt 替代 MonoGame)
- 角色/地图的复杂动画系统
- 40+ 浏览 UI 窗体
- 跨 WZ 路径解析逻辑
- 字体系统(GMS/TMS/JMS 多区域)

`★ Insight ─────────────────────────────────────`
- 项目最巧妙的设计是 `Wz_GetNode(path)` 的跨文件路径解析,它用 `PluginManager.WzFileFinding` 事件解耦了"路径字符串"与"具体 WZ 文件",让上层代码可以透明访问任意 WZ 资源
- 角色渲染采用"离屏 Avatar + 整体合成"管线(`AvatarEngine` → `AvatarTargetTexture` 1024×1024 → 主屏),既支持装备自由搭配又避免了部件 Z 序问题
- Z 序用"整数大数编码"(Layer * 100000 + z)而非小数,避免了浮点比较的精度问题
`─────────────────────────────────────────────────`
