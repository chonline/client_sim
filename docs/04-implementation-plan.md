# 实施计划与跨平台打包

> 路径: 根目录
> 文档版本: v1.0 (2026-06-08)

## 1. 5 个里程碑

| 里程碑 | 周期 | 目标 | 交付物 |
|---|---|---|---|
| **M1** | 2-3 周 | 基础设施 | wzlib 完整实现 + Qt 主窗口 + WZ 树形浏览 + 图片查看器 + 30+ 单测 + 跨平台编译通过 |
| **M2** | 2-3 周 | 渲染 | 地图加载 + 角色/Avatar + 60Hz 渲染循环 + Foothold + 屏幕模式 |
| **M3** | 2-3 周 | 游戏对象 | Mob/Npc/Pet/TamingMob/Familiar + 聊天泡泡 + 名称标签 + 伤害数字 + 残影 |
| **M4** | 2-3 周 | 浏览 UI | 40+ Form + Tooltip 渲染器 + 自定义控件 + 装备/物品/技能/怪物/NPC 完整 |
| **M5** | 1-2 周 | 高级 + 跨平台打包 | Spine 骨骼 + 粒子 + BGM/SFX + Linux AppImage + macOS dmg + Windows installer |

## 2. M1 详细计划(当前执行)

### 2.1 M1 任务清单

#### Phase A: 项目脚手架(Day 1-2)
- [ ] 顶层 CMakeLists.txt
- [ ] vcpkg.json + CMakePresets.json
- [ ] .gitignore, .clang-format, .clang-tidy
- [ ] wzlib/CMakeLists.txt
- [ ] tests/CMakeLists.txt
- [ ] app/main.cpp (基础 QApplication + 空主窗口)
- [ ] CI 配置(GitHub Actions)

#### Phase B: wzlib 基础类型(Day 3-4)
- [ ] ObjectType 枚举
- [ ] WzObject 基类
- [ ] WzInt / WzDouble / WzString / WzVector
- [ ] WzUol(基础)
- [ ] WzNode(基础,不含 get 路径解析)

#### Phase C: wzlib 加密模块(Day 5-6)
- [ ] WzCryptoContext 基础
- [ ] AES-256-ECB 封装(OpenSSL EVP)
- [ ] 密钥流生成(initFromIv)
- [ ] 字符串解密(decryptString PKG1)
- [ ] 图像分块解密(decryptInPlace)
- [ ] 单测:AES 密钥流字节级对比

#### Phase D: wzlib 树结构(Day 7-9)
- [ ] WzHeader(PKG1 / PKG2)
- [ ] WzFile::open(嗅探文件头)
- [ ] 目录项读取(readDirectory)
- [ ] WzImage::tryExtract(zlib + 解析 Property)
- [ ] WzNode::get 路径解析(支持 `..`、UOL)
- [ ] WzStructure::load(主入口)
- [ ] WzStructure::discoverExtensions(XXXX2.wz)
- [ ] 单测:打开真实 wz、跨文件路径

#### Phase E: wzlib 图像解码(Day 10-12)
- [ ] WzPng 基础
- [ ] ARGB4444 / ARGB8888 / ARGB1555 / RGB565 解码
- [ ] DXT1 / DXT3 / DXT5 解码
- [ ] 黄金图测试(对比原 C# 版输出)

#### Phase F: wzlib 字符串链接(Day 13)
- [ ] StringResult 结构
- [ ] StringLinker::load
- [ ] 单测:加载真实 String.wz

#### Phase G: ui 基础框架(Day 14-15)
- [ ] ui/CMakeLists.txt
- [ ] MainWindow 基础(菜单 + 工具栏 + 中心区域)
- [ ] FileDialog 打开 WZ
- [ ] WzTreeView + WzTreeModel
- [ ] 选中节点显示图片(SingleImageViewer)
- [ ] DPI 适配

#### Phase H: 跨平台编译(Day 16)
- [ ] Linux 编译运行测试
- [ ] macOS 编译运行测试
- [ ] Windows 编译运行测试
- [ ] 修复任何平台特定问题

#### Phase I: M1 验收(Day 17-18)
- [ ] 全部单测通过
- [ ] 打开 KMS/Beta/GMS 任一 wz 看到树
- [ ] 选中节点显示对应图片
- [ ] 写 M1 验收报告
- [ ] 提交 M1 git tag

### 2.2 M1 验收标准

**功能性**:
- [ ] 打开 5MB ~ 500MB 大小的 wz 文件,无崩溃
- [ ] 树形浏览展开 / 折叠流畅(1000 节点 < 100ms)
- [ ] 选中节点后右侧显示对应图片
- [ ] 支持 WZ / MS 两种格式
- [ ] 跨 WZ 路径查询正常

**代码质量**:
- [ ] GTest 覆盖率 ≥ 70%
- [ ] 关键算法有单测
- [ ] 公开 API 有 Doxygen 注释
- [ ] clang-tidy 警告 0

**跨平台**:
- [ ] Ubuntu 22.04 编译运行
- [ ] macOS 13+ 编译运行
- [ ] Windows 11 编译运行

## 3. 构建系统

### 3.1 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.21)
project(client_sim
    VERSION 0.1.0
    DESCRIPTION "MapleStory GM Client - C++/Qt Cross-Platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

# 第三方
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# 选项
option(CLIENT_SIM_BUILD_TESTS "Build unit tests" ON)

# 子模块
add_subdirectory(wzlib)

# 暂时只构建 wzlib,后续里程碑再开 client/ui/app
# add_subdirectory(client)
# add_subdirectory(ui)
# add_subdirectory(app)

if(CLIENT_SIM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### 3.2 vcpkg.json

```json
{
  "name": "client-sim",
  "version-string": "0.1.0",
  "description": "MapleStory GM Client - C++/Qt Cross-Platform",
  "dependencies": [
    "qt6-base",
    "qt6-widgets",
    "zlib",
    "openssl",
    "gtest"
  ]
}
```

### 3.3 CMakePresets.json

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "CLIENT_SIM_BUILD_TESTS": "ON"
      }
    },
    {
      "name": "linux-gcc-debug",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/linux-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_COMPILER": "g++"
      }
    },
    {
      "name": "linux-gcc-release",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/linux-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_COMPILER": "g++"
      }
    },
    {
      "name": "macos-clang-debug",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/macos-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_COMPILER": "clang++"
      }
    },
    {
      "name": "macos-clang-release",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/macos-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_COMPILER": "clang++"
      }
    },
    {
      "name": "windows-msvc-debug",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/windows-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_COMPILER": "cl"
      }
    },
    {
      "name": "windows-msvc-release",
      "inherits": "base",
      "generator": "Ninja",
      "binaryDir": "build/windows-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_COMPILER": "cl"
      }
    }
  ],
  "buildPresets": [
    { "name": "linux-debug",   "configurePreset": "linux-gcc-debug" },
    { "name": "linux-release", "configurePreset": "linux-gcc-release" },
    { "name": "macos-debug",   "configurePreset": "macos-clang-debug" },
    { "name": "macos-release", "configurePreset": "macos-clang-release" },
    { "name": "windows-debug", "configurePreset": "windows-msvc-debug" },
    { "name": "windows-release", "configurePreset": "windows-msvc-release" }
  ]
}
```

## 4. 跨平台编译与打包

### 4.1 Linux

**环境准备**:
```bash
# Ubuntu 22.04
sudo apt install build-essential ninja-build cmake git \
                 libssl-dev zlib1g-dev

# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$HOME/vcpkg
```

**编译**:
```bash
cmake --preset linux-gcc-release
cmake --build build/linux-release
ctest --test-dir build/linux-release --output-on-failure
./build/linux-release/wzlib/wzlib_tests  # 或后续 app
```

**打包 AppImage**:
```bash
# 下载 linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

# 打包
./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --executable build/linux-release/app/client_sim \
    --desktop-file resources/client_sim.desktop \
    --icon-file resources/icon.png \
    --plugin qt \
    --output appimage
```

### 4.2 macOS

**环境准备**:
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 vcpkg
brew install cmake ninja
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$HOME/vcpkg
```

**编译**:
```bash
cmake --preset macos-clang-release
cmake --build build/macos-release
ctest --test-dir build/macos-release --output-on-failure
```

**打包 .dmg**:
```bash
macdeployqt build/macos-release/client_sim.app -dmg

# 签名
codesign --deep --force --sign "Developer ID Application: Your Name" \
    build/macos-release/client_sim.app
# 公证
xcrun notarytool submit build/macos-release/client_sim.dmg \
    --keychain-profile "notarytool-profile" --wait
```

### 4.3 Windows

**环境准备**:
```powershell
# 安装 Visual Studio 2022 (C++ 桌面开发)
# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "C:\vcpkg"
```

**编译**:
```powershell
cmake --preset windows-msvc-release
cmake --build build/windows-release
ctest --test-dir build/windows-release --output-on-failure
```

**打包 NSIS installer**:
```cmake
# 顶层 CMakeLists.txt 末尾
set(CPACK_GENERATOR "NSIS;ZIP")
set(CPACK_PACKAGE_NAME "MapleNecrocer")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MapleNecrocer")
include(CPack)
```

```powershell
cmake --build build/windows-release --target package
# 产物: build/windows-release/MapleNecrocer-0.1.0-win64.exe
```

## 5. CI 矩阵

`.github/workflows/ci.yml`:

```yaml
name: CI
on:
  push:
    branches: [main]
  pull_request:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-22.04, macos-13, windows-2022]
        config: [debug, release]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Install vcpkg
        uses: lukka/run-vcpkg@v11
        with:
          vcpkgGitCommitId: 7a6f0faa0d3f025d9f6a18a18ce9d83f74d6e906

      - name: Configure
        shell: bash
        run: |
          if [ "${{ matrix.os }}" = "windows-2022" ]; then
            cmake --preset windows-msvc-${{ matrix.config }}
          elif [ "${{ matrix.os }}" = "macos-13" ]; then
            cmake --preset macos-clang-${{ matrix.config }}
          else
            cmake --preset linux-gcc-${{ matrix.config }}
          fi

      - name: Build
        shell: bash
        run: |
          BUILD_DIR=$(ls -d build/*-${{ matrix.config }} 2>/dev/null || ls -d build/*)
          cmake --build "$BUILD_DIR" -j

      - name: Test
        shell: bash
        run: |
          BUILD_DIR=$(ls -d build/*-${{ matrix.config }} 2>/dev/null || ls -d build/*)
          ctest --test-dir "$BUILD_DIR" --output-on-failure
```

## 6. 验收与提交

每个里程碑结束:
1. 跑全部单测,确保绿色
2. 写验收报告(在 `docs/M{N}_report.md`)
3. 创建 git tag: `git tag -a v0.{N}.0 -m "M{N} 完成"`
4. 推送到远程

## 7. 进度跟踪

每个里程碑内部用 TaskCreate 跟踪子任务,完成一个更新一个状态。
