# wzlib 模块设计

> 路径: `wzlib/`
> 目标: 解析 MapleStory WZ/MS 资源包文件格式
> 依赖: 仅 OpenSSL EVP(AES)、zlib、Qt Core(QByteArray/QString)
> 不依赖: Qt Widgets、Qt GUI

## 1. 概述

wzlib 是整个项目的**最底层**模块,负责把 MapleStory 的二进制资源包(WZ/MS 格式)解析为内存中的树形数据结构。它的设计与实现质量直接决定整个项目的可移植性与正确性。

### 1.1 主要职责
1. 读取 WZ 文件头(PKG1 / PKG2)
2. 解密字符串与图像(基于 AES-256-ECB)
3. 构建目录树(懒加载 WzImage)
4. 解压 zlib 压缩的图像数据
5. 解码多种位图格式(ARGB4444/8888/1555, RGB565, DXT1/3/5, BC7, ...)
6. 解析矢量、声音、符号链接
7. 跨 WZ 文件路径查询(由 WzFinder 在 ui 层封装)

### 1.2 不做的事
- 不做渲染(由 client/ 和 ui/ 负责)
- 不做游戏对象抽象(由 client/ 负责)
- 不依赖 Qt Widgets(只依赖 Qt Core 基础类型)

## 2. 文件格式

### 2.1 总体布局

```
┌──────────────────────────────────────────────┐
│  Header (32 字节 PKG1 / 60+ 字节 PKG2)      │
│  ├─ Signature: 4 字节 ASCII "PKG1"/"PKG2"  │
│  ├─ FileSize: 8 字节 int64                  │
│  ├─ DataSize: 8 字节 int64 (0=外部包)      │
│  ├─ (PKG1) EncryptedVersion: 2 字节 int16   │
│  ├─ (PKG2) Hash1, Hash2: 4+4 字节          │
│  └─ (PKG2) HashVersion: 4 字节             │
├──────────────────────────────────────────────┤
│  Directory Tree                              │
│  ├─ Recursive 结构                          │
│  ├─ 每个 entry 头: 1 字节 type              │
│  │   0x02 = Sub Directory (无数据)         │
│  │   0x04 = WzImage                         │
│  │   0x05 = (其他扩展类型)                  │
│  ├─ 名称长度: 1/2/4 字节 compressed int     │
│  └─ 名称: 长度字节 + UTF-16/ASCII          │
├──────────────────────────────────────────────┤
│  WzImage 数据块 (懒加载)                     │
│  ├─ Size: 4 字节 int32                      │
│  ├─ CS32 (checksum): 4 字节                 │
│  ├─ HashedOffset: 8 字节 int64              │
│  └─ Data: Size 字节 (zlib + 可选加密)      │
│     └─ 解压后: Property 树                  │
└──────────────────────────────────────────────┘
```

### 2.2 Property 类型标签

```
0x00 = Null
0x01 = (Shorthand int? 1 字节,部分版本)
0x02 = Short   (2 字节 int16)
0x03 = Int     (4 字节 int32)
0x04 = Float   (4 字节 IEEE 754)
0x05 = Double  (8 字节 IEEE 754)
0x06 = String  (int32 length + UTF-8/16 字节)
0x07 = (扩展)
0x08 = (String, 加密)
0x09 = Sub     (子 Property,递归结构)
0x0A = (扩展)
0x0B = Uol     (符号链接)
0x0C = (扩展,Reserved)
0x0D = (扩展)
0x10 = Vector2D  (8 字节 = int32 x + int32 y)
0x11 = Convex   (凸多边形)
0x12 = Sound    (MP3/PCM)
0x13 = RawData
0x14 = (long?)
...
```

### 2.3 字符串与图像加密

**字符串(PKG1)**: 用 `WzCryptoContext::decryptString` 解密,XOR 流来自预生成的 `pkg1_keys_`

**字符串(PKG2)**: 用 `Pkg2DirStringKey`(8 字节)或 `Pkg2DirStringKeyV2`(KMST1199+, Mix 变换)

**图像**: 分块(4096 字节)独立加密,密钥流来自 `chunked_stream_xor_key`

**IV 种子**(各区域):
```cpp
namespace wzlib::crypto {
inline constexpr std::array<uint8_t, 4> IV_GMS = {0x4d, 0x23, 0xc7, 0x2b};
inline constexpr std::array<uint8_t, 4> IV_KMS = {0xb9, 0x7d, 0x63, 0xe9};
inline constexpr std::array<uint8_t, 4> IV_JMS = {0x69, 0x7a, 0x43, 0x2e};
inline constexpr std::array<uint8_t, 4> IV_TMS = {0x67, 0x6e, 0x73, 0x5b};
}
```

## 3. 核心类

### 3.1 WzObject(基类)

```cpp
namespace wzlib {

enum class ObjectType {
    Null, Short, Int, Long, Float, Double,
    String, Sub, Uol,
    Vector2D, Convex,
    Png, Sound, RawData, Video,
    Property  // Property 是 WzNode 而非 WzObject,这里保留枚举以扩展
};

class WzObject {
public:
    virtual ~WzObject() = default;
    virtual ObjectType type() const = 0;
    
    // 类型安全访问(返回 std::optional)
    std::optional<int32_t> asInt() const;
    std::optional<int64_t> asLong() const;
    std::optional<double>  asDouble() const;
    std::optional<std::string> asString() const;
    std::optional<WzVector>  asVector() const;
    
protected:
    WzObject() = default;
};

} // namespace wzlib
```

### 3.2 标量类型

```cpp
class WzInt : public WzObject {
public:
    explicit WzInt(int32_t v) : value_(v) {}
    ObjectType type() const override { return ObjectType::Int; }
    int32_t value() const { return value_; }
private:
    int32_t value_;
};

class WzString : public WzObject {
public:
    explicit WzString(std::string v) : value_(std::move(v)) {}
    ObjectType type() const override { return ObjectType::String; }
    const std::string& value() const { return value_; }
private:
    std::string value_;
};

class WzVector : public WzObject {
public:
    WzVector(int32_t x, int32_t y) : x_(x), y_(y) {}
    ObjectType type() const override { return ObjectType::Vector2D; }
    int32_t x() const { return x_; }
    int32_t y() const { return y_; }
private:
    int32_t x_, y_;
};
```

### 3.3 复合类型

```cpp
class WzUol : public WzObject {
public:
    explicit WzUol(std::string path) : path_(std::move(path)) {}
    ObjectType type() const override { return ObjectType::Uol; }
    const std::string& path() const { return path_; }
    
    // 解析符号链接
    // 例: "../otherNode" 或 "Map/0/100.img"
    WzNode* resolve(WzNode* context) const;
private:
    std::string path_;
};

class WzPng : public WzObject {
public:
    enum class Format {
        // 索引调色板
        ARGB4444 = 0x02,
        ARGB8888 = 0x04,
        ARGB1555 = 0x06,
        RGB565   = 0x07,
        // 压缩格式
        DXT1     = 0x201,
        DXT3     = 0x205,
        DXT5     = 0x209,
        BC7      = 0x20D,
        // 高位
        RGBA1010102 = 0x1002,
        R16         = 0x1011,
        A8          = 0x1012,
        RGBA32Float = 0x1015,
        // 未知
        Unknown
    };
    
    WzPng(Format fmt, int32_t w, int32_t h, std::vector<uint8_t> data)
        : format_(fmt), width_(w), height_(h), data_(std::move(data)) {}
    ObjectType type() const override { return ObjectType::Png; }
    
    // 解码为 QImage
    QImage toQImage() const;
    
    Format format() const { return format_; }
    int32_t width()  const { return width_; }
    int32_t height() const { return height_; }
    
private:
    Format format_;
    int32_t width_, height_;
    std::vector<uint8_t> data_;  // 已 zlib 解压的原始数据
};

class WzSound : public WzObject {
public:
    enum class Type { MP3, PCM };
    WzSound(Type t, int32_t durMs, std::vector<uint8_t> data)
        : type_(t), durationMs_(durMs), data_(std::move(data)) {}
    ObjectType type() const override { return ObjectType::Sound; }
    
    QByteArray toBytes() const;  // 完整音频字节
    
    Type type() const { return type_; }
    int32_t durationMs() const { return durationMs_; }
    
private:
    Type type_;
    int32_t durationMs_;
    std::vector<uint8_t> data_;
};
```

### 3.4 WzNode(树形结构)

```cpp
class WzNode {
public:
    WzNode(std::string name, WzNode* parent = nullptr)
        : name_(std::move(name)), parent_(parent) {}
    
    // 基础
    const std::string& name() const { return name_; }
    WzNode* parent() const { return parent_; }
    void setParent(WzNode* p) { parent_ = p; }
    
    // 子节点
    void addChild(std::unique_ptr<WzNode> child);
    WzNode* child(const std::string& name) const;
    const std::map<std::string, std::unique_ptr<WzNode>>& children() const {
        return children_;
    }
    int32_t childCount() const { return static_cast<int32_t>(children_.size()); }
    
    // 值
    const WzObject* value() const { return value_.get(); }
    WzObject* value() { return value_.get(); }
    void setValue(std::unique_ptr<WzObject> v) { value_ = std::move(v); }
    
    // 路径解析
    // 支持:
    //   "a/b/c"  - 简单路径
    //   "../x"   - 回溯父节点
    //   "a/../b" - 混合
    //   自动处理 UOL
    WzNode* get(const std::string& path) const;
    
    // 类型安全取值
    std::optional<int32_t> getInt(std::optional<int32_t> def = std::nullopt) const;
    std::optional<std::string> getString(std::optional<std::string> def = std::nullopt) const;
    std::optional<WzVector> getVector() const;
    
    // 工具
    std::string fullPath() const;  // 反向遍历父节点拼接
    
private:
    std::string name_;
    WzNode* parent_;
    std::map<std::string, std::unique_ptr<WzNode>> children_;
    std::unique_ptr<WzObject> value_;
    
    // 缓存: image 节点到所属 WzImage
    WzImage* owningImage_ = nullptr;
    friend class WzImage;
};
```

**WzNode::get 关键实现:**
```cpp
WzNode* WzNode::get(const std::string& path) const {
    WzNode* cur = const_cast<WzNode*>(this);
    size_t i = 0;
    while (i < path.size() && cur) {
        size_t j = path.find('/', i);
        if (j == i) { i++; continue; }  // 跳过空段
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        
        if (seg == "..") {
            cur = cur->parent_;
        } else if (!seg.empty()) {
            auto it = cur->children_.find(seg);
            if (it == cur->children_.end()) {
                return nullptr;
            }
            cur = it->second.get();
            // 自动 resolve UOL
            if (cur->value_ && cur->value_->type() == ObjectType::Uol) {
                auto* uol = static_cast<WzUol*>(cur->value_.get());
                cur = uol->resolve(cur);
                if (!cur) return nullptr;
            }
        }
        
        i = (j == std::string::npos) ? path.size() : j + 1;
    }
    return cur;
}
```

### 3.5 WzImage(懒加载的 .img)

```cpp
class WzImage {
public:
    WzImage() = default;
    WzImage(int64_t offset, int32_t size, int32_t cs32)
        : offset_(offset), size_(size), cs32_(cs32) {}
    
    // 解压(可能失败)
    // 注意: 多次调用安全(幂等)
    bool tryExtract(WzCryptoContext* crypto = nullptr);
    
    bool extracted() const { return extracted_; }
    WzNode* root() { return &root_; }
    
    // 工具
    int64_t offset() const { return offset_; }
    int32_t size() const { return size_; }
    int32_t cs32() const { return cs32_; }
    
private:
    int64_t offset_ = 0;
    int32_t size_ = 0;
    int32_t cs32_ = 0;
    bool extracted_ = false;
    
    WzNode root_;  // 根节点(空名)
    
    std::vector<uint8_t> compressedData_;  // 压缩+加密的原始数据
    
    // 解析 property 树
    void parseProperty(WzNode* parent, const uint8_t* data, size_t len, size_t& pos);
};
```

### 3.6 WzFile(单个 wz 文件)

```cpp
enum class WzType {
    Unknown = 0,
    Base, Character, Item, Mob, Npc, Skill, Map,
    String, Etc, Sound, UI, Reactor, Tile, Effect,
    List,  // 旧版 List.wz
    // ...
};

class WzFile {
public:
    // 打开 WZ 文件
    // 自动嗅探 PKG1 / PKG2
    // 自动初始化加密上下文
    static std::unique_ptr<WzFile> open(const std::string& path,
                                        WzCryptoContext* sharedCrypto = nullptr);
    
    ~WzFile();
    
    // 类型与文件名
    WzType type() const { return type_; }
    const std::string& fileName() const { return fileName_; }
    int32_t encryptedVersion() const { return header_.encryptedVersion; }
    
    // 树入口
    WzNode* rootNode() { return &rootNode_; }
    
    // 解密工具
    WzCryptoContext& crypto() { return crypto_; }
    
    // 内部(供 WzStructure 调用)
    WzImage* getOrCreateImage(int64_t offset, int32_t size, int32_t cs32);
    WzImage* imageAt(int64_t offset);
    
    // 关闭
    void close();
    
private:
    WzFile() = default;
    
    std::string fileName_;
    WzType type_ = WzType::Unknown;
    WzHeader header_;
    WzCryptoContext crypto_;
    
    std::unique_ptr<std::ifstream> stream_;
    
    // 目录项(从 WzDirectoryEntry 解析)
    struct DirEntry {
        std::string name;
        int32_t type;       // 0x02=Sub, 0x04=Image
        int64_t offset;
        int32_t size;
        int32_t cs32;
    };
    std::vector<DirEntry> directories_;
    
    // 图像(懒加载)
    std::map<int64_t, std::unique_ptr<WzImage>> images_;
    
    WzNode rootNode_;
    
    // 读取目录树
    void readDirectory();
    void readDirectoryEntry(int64_t offset, DirEntry& entry);
    WzNode* buildTreeFromDirectory(int64_t offset, int32_t parentType);
    
    friend class WzStructure;
};
```

### 3.7 WzStructure(多文件管理)

```cpp
class WzStructure {
public:
    // 加载主 wz
    static std::unique_ptr<WzStructure> load(const std::string& mainPath);
    
    // KMST1125+ Data 格式
    static std::unique_ptr<WzStructure> loadKmst1125(const std::string& dataPath);
    
    // 新 MS 格式
    static std::unique_ptr<WzStructure> loadMs(const std::string& msPath);
    
    ~WzStructure();
    
    // 跨文件路径查询(由 WzFinder 委托)
    WzNode* findNode(const std::string& path);
    
    // 探测状态
    bool isDataWz() const { return isDataWz_; }
    bool hasStringWz() const { return hasStringWz_; }
    bool hasHardCodedStrings() const { return hasHardCodedStrings_; }
    bool hasMap9Dir() const { return hasMap9Dir_; }
    
    // 加密上下文
    WzCryptoContext& crypto() { return crypto_; }
    
    // 文件列表
    const std::vector<std::unique_ptr<WzFile>>& files() const { return files_; }
    
private:
    WzStructure() = default;
    
    std::vector<std::unique_ptr<WzFile>> files_;
    WzCryptoContext crypto_;
    
    bool isDataWz_ = false;
    bool hasStringWz_ = false;
    bool hasHardCodedStrings_ = false;
    bool hasMap9Dir_ = false;
    
    // 探测区域与文件类型
    WzType detectFileType(const std::string& fileName);
    void detectRegion();  // GMS / KMS / JMS / TMS
    
    // 自动发现扩展包
    void discoverExtensions(const std::string& mainPath);
    void loadPacksFolder(const std::string& packsDir);
    
    // 跨文件解析
    WzNode* findByWzType(WzType type);
    WzNode* findInBaseDataWz(WzType type);
    
    // 兼容路径重写
    // Map001/002/2 → Map
    // Mob001/002/2 → Mob
    // Skill001..003 → Skill
    // Sound001/002/2 → Sound
    std::string rewriteLegacyPath(const std::string& path) const;
};
```

### 3.8 WzCryptoContext(加密)

```cpp
class WzCryptoContext {
public:
    WzCryptoContext() = default;
    
    // 初始化:从 IV 生成 2048 字节 XOR 流
    void initFromIv(const std::array<uint8_t, 4>& iv,
                    const std::array<uint8_t, 32>& aesKey);
    
    // 字符串解密
    // input: 加密的字节序列
    // offset: 在 wz 文件中的偏移(用于 PKG2)
    std::string decryptString(const uint8_t* input, size_t len,
                              int64_t offset = 0) const;
    
    // 原地解密(图像块)
    void decryptInPlace(uint8_t* data, size_t len, int64_t offset) const;
    
    // PKG2 字符串
    void setPkg2Hash(int32_t hash1, int32_t hashVersion);
    
    // 配置查询
    bool isInitialized() const { return initialized_; }
    bool isPkg2() const { return isPkg2_; }
    
private:
    bool initialized_ = false;
    bool isPkg2_ = false;
    
    // AES-256-ECB 密钥
    std::array<uint8_t, 32> aesKey_;
    
    // 预生成 2048 字节 XOR 流(PKG1 字符串)
    std::array<uint8_t, 2048> pkg1Keys_;
    
    // PKG2
    int32_t pkg2Hash1_ = 0;
    int32_t pkg2HashVersion_ = 0;
    std::array<uint8_t, 8> pkg2BaseKey_{};
    bool pkg2UseV2_ = false;  // KMST1199+ 用 Mix 变换
    
    // AES 加密单块(用于生成密钥流)
    void aesEncryptBlock(const uint8_t in[16], uint8_t out[16]) const;
    
    // PKG2 V2 字符串 Mix 变换
    void mixV2(uint8_t* data, size_t len) const;
    
    // 图像分块加密
    void decryptChunk(uint8_t* chunk, size_t len, int64_t chunkOffset) const;
};
```

**关键算法 — 密钥流生成:**
```cpp
void WzCryptoContext::initFromIv(const std::array<uint8_t, 4>& iv,
                                  const std::array<uint8_t, 32>& aesKey) {
    aesKey_ = aesKey;
    
    // 准备 16 字节 IV 块
    std::array<uint8_t, 16> block{};
    std::copy(iv.begin(), iv.end(), block.begin());
    
    // 反复加密 16 字节块,直到填满 2048 字节
    size_t pos = 0;
    while (pos < pkg1Keys_.size()) {
        aesEncryptBlock(block.data(), &pkg1Keys_[pos]);
        // block = block + 1 (递增 16 字节大整数)
        incrementBigEndian(block.data(), 16);
        pos += 16;
    }
    
    initialized_ = true;
    isPkg2_ = false;
}
```

**AES-256-ECB 加密(用 OpenSSL EVP):**
```cpp
void WzCryptoContext::aesEncryptBlock(const uint8_t in[16], uint8_t out[16]) const {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr,
                       aesKey_.data(), nullptr);
    EVP_CIPHER_CTX_set_padding(ctx, 0);  // 关闭 padding
    
    int outLen = 0;
    EVP_EncryptUpdate(ctx, out, &outLen, in, 16);
    EVP_CIPHER_CTX_free(ctx);
}
```

**字符串解密(PKG1):**
```cpp
std::string WzCryptoContext::decryptString(const uint8_t* input, size_t len,
                                            int64_t offset) const {
    std::string out;
    out.resize(len);
    
    if (isPkg2_) {
        if (pkg2UseV2_) {
            // PKG2 V2: 用 hash1 ^ hashVersion ^ 0x6D4C3B2A 经 Mix
            // ...
        } else {
            // PKG2 V1: 用 pkg2BaseKey_ 异或 + 简单 Mix
            // ...
        }
    } else {
        // PKG1: 直接 XOR
        for (size_t i = 0; i < len; ++i) {
            out[i] = input[i] ^ pkg1Keys_[i % pkg1Keys_.size()];
        }
    }
    
    return out;
}
```

### 3.9 图像格式解码(WzPng::toQImage)

```cpp
QImage WzPng::toQImage() const {
    switch (format_) {
        case Format::ARGB8888: return decodeARGB8888();
        case Format::ARGB4444: return decodeARGB4444();
        case Format::ARGB1555: return decodeARGB1555();
        case Format::RGB565:   return decodeRGB565();
        case Format::DXT1:     return decodeDXT1();
        case Format::DXT3:     return decodeDXT3();
        case Format::DXT5:     return decodeDXT5();
        case Format::BC7:      return decodeBC7();  // 简化版
        case Format::A8:       return decodeA8();
        case Format::R16:      return decodeR16();
        default: return QImage();
    }
}
```

**ARGB4444 解码(参考,完整版在实现中):**
```cpp
QImage WzPng::decodeARGB4444() const {
    QImage img(width_, height_, QImage::Format_ARGB32);
    const uint8_t* p = data_.data();
    
    for (int y = 0; y < height_; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < width_; ++x) {
            uint16_t v = p[0] | (p[1] << 8);
            p += 2;
            
            // 4444: A=4bit, R=4bit, G=4bit, B=4bit
            // 位序取决于 wz 版本,通常是 A R G B
            int a = ((v >> 12) & 0xF) * 0x11;
            int r = ((v >> 8)  & 0xF) * 0x11;
            int g = ((v >> 4)  & 0xF) * 0x11;
            int b = ( v        & 0xF) * 0x11;
            
            row[x] = qRgba(r, g, b, a);
        }
    }
    return img;
}
```

**DXT1 解码:**
```cpp
QImage WzPng::decodeDXT1() const {
    QImage img(width_, height_, QImage::Format_ARGB32);
    const uint8_t* p = data_.data();
    
    for (int y = 0; y < height_; y += 4) {
        for (int x = 0; x < width_; x += 4) {
            uint16_t c0 = p[0] | (p[1] << 8);
            uint16_t c1 = p[2] | (p[3] << 8);
            p += 4;
            
            // 扩展为 RGB888
            auto expand = [](uint16_t c) -> std::array<uint8_t, 4> {
                uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
                uint8_t g = ((c >> 5)  & 0x3F) * 255 / 63;
                uint8_t b = ( c        & 0x1F) * 255 / 31;
                return {r, g, b, 255};
            };
            auto c00 = expand(c0);
            auto c01 = expand(c1);
            
            // 计算插值颜色
            std::array<std::array<uint8_t, 4>, 4> colors = {c00, c01};
            if (c0 > c1) {
                // 4 色
                for (int i = 0; i < 3; ++i) {
                    colors[2 + i / 3][0] = (2 * colors[0][0] + colors[1][0]) / 3;
                    // ... 完整实现
                }
            } else {
                // 3 色 + 透明
                colors[2] = {(c00[0] + c01[0]) / 2, ...};
                colors[3] = {0, 0, 0, 0};
            }
            
            // 读取 4x4 块的 16 个 2-bit 索引
            for (int by = 0; by < 4; ++by) {
                for (int bx = 0; bx < 4; ++bx) {
                    int idx;
                    if (bx % 4 == 0) idx = *p++;
                    else idx >>= 2;
                    // 写像素
                }
            }
        }
    }
    return img;
}
```

### 3.10 StringLinker(字符串索引)

```cpp
struct StringResult {
    std::string name;
    std::string desc;
    std::string pdesc;          // 短描述
    std::string mapName;
    std::string streetName;
    std::vector<std::string> h;  // 多行帮助
    // ...
};

class StringLinker {
public:
    // 加载:从 String.wz + Item.wz + Etc.wz
    bool load(WzFile* stringWz, WzFile* itemWz, WzFile* etcWz);
    bool load(WzNode* stringRoot, WzNode* itemRoot, WzNode* etcRoot);
    
    // 查询
    const StringResult* get(int32_t id, const std::string& category) const;
    
    // 7 个分类
    const std::unordered_map<int32_t, StringResult>& eqp()      const { return eqp_; }
    const std::unordered_map<int32_t, StringResult>& item()     const { return item_; }
    const std::unordered_map<int32_t, StringResult>& mob()      const { return mob_; }
    const std::unordered_map<int32_t, StringResult>& npc()      const { return npc_; }
    const std::unordered_map<int32_t, StringResult>& skill()    const { return skill_; }
    const std::unordered_map<int32_t, StringResult>& map()      const { return map_; }
    const std::unordered_map<int32_t, StringResult>& setItem()  const { return setItem_; }
    
    void clear();
    bool hasValues() const { return !eqp_.empty() || !item_.empty(); }
    
private:
    std::unordered_map<int32_t, StringResult> eqp_;
    std::unordered_map<int32_t, StringResult> item_;
    std::unordered_map<int32_t, StringResult> mob_;
    std::unordered_map<int32_t, StringResult> npc_;
    std::unordered_map<int32_t, StringResult> skill_;
    std::unordered_map<int32_t, StringResult> map_;
    std::unordered_map<int32_t, StringResult> setItem_;
    
    void loadMob(WzNode* root);
    void loadNpc(WzNode* root);
    void loadSkill(WzNode* root);
    void loadItem(WzNode* root);
    void loadEqp(WzNode* root);
    void loadMap(WzNode* root);
    void loadSetItem(WzNode* root);
    
    void addAllValue(StringResult& sr, WzNode* node);
};
```

### 3.11 Calculator(表达式求值)

```cpp
class Calculator {
public:
    // 求值带参数 x, y, z, w 的表达式
    // 例: "x*2+30", "u(x/3)", "log10(x+1)"
    static std::optional<double> parse(const std::string& expression,
                                      std::initializer_list<double> args);
    
    static std::optional<double> parse(const std::string& expression,
                                      const std::vector<double>& args);
    
private:
    enum class TokenType { Num, Var, Op, LParen, RParen, Comma, Func };
    struct Token {
        TokenType type;
        std::string text;
        double value = 0.0;
    };
    
    static std::vector<Token> tokenize(const std::string& expr);
    static std::vector<Token> toRPN(const std::vector<Token>& tokens);
    static std::optional<double> evaluateRPN(const std::vector<Token>& rpn,
                                             const std::vector<double>& args);
};
```

### 3.12 BitmapOrigin(位图+锚点)

```cpp
class BitmapOrigin {
public:
    BitmapOrigin() = default;
    BitmapOrigin(QImage img, QPoint origin)
        : image_(std::move(img)), origin_(origin) {}
    
    static BitmapOrigin createFromNode(WzNode* node);
    
    const QImage& image() const { return image_; }
    QPoint origin() const { return origin_; }
    QPoint opOrigin() const { return {-origin_.x(), -origin_.y()}; }
    QRect rectangle(QPoint pos) const {
        return QRect(pos - origin_, image_.size());
    }
    
private:
    QImage image_;
    QPoint origin_;
};
```

## 4. 关键算法总结

| 算法 | 位置 | 难度 | 关键点 |
|---|---|---|---|
| AES 密钥流生成 | WzCryptoContext::initFromIv | 中 | 16 字节块反复加密,2048 字节 |
| 字符串解密 | WzCryptoContext::decryptString | 中 | PKG1 XOR / PKG2 Mix |
| 图像分块解密 | WzCryptoContext::decryptInPlace | 中 | 4096 字节独立块 |
| WzNode 路径解析 | WzNode::get | 中 | 支持 `..`、自动 UOL 解析 |
| WzImage 懒加载 | WzImage::tryExtract | 中 | zlib 解压 + 解密 + 解析 Property |
| WzPng ARGB 解码 | WzPng::toQImage | 低 | 简单位运算 |
| WzPng DXT1/3/5 解码 | WzPng::decodeDXT* | 高 | 颜色插值 + 4x4 块索引 |
| WzPng BC7 解码 | WzPng::decodeBC7 | 极高 | 复杂模式,留后期 |
| 跨文件路径解析 | WzStructure::findNode | 中 | 排除 NL/ES/FR/DE,base.wz 回退 |
| 表达式求值 | Calculator::parse | 中 | Shunting-yard 算法 |

## 5. 错误处理

```cpp
// wzlib 统一错误码
enum class ErrorCode {
    Ok = 0,
    FileNotFound,
    InvalidSignature,         // 不是 PKG1/PKG2
    UnsupportedVersion,
    EncryptedRegionMismatch,  // IV 错误
    CorruptedImage,           // checksum 失败
    ZlibDecompressFailed,
    InvalidPropertyType,
    UolResolutionFailed,      // 解析符号链接失败
    OutOfMemory,
    IOError,
    Unknown
};

// 用 std::expected (C++23) 或自定义 Result
template<typename T>
using Result = std::variant<T, ErrorCode>;

// 函数签名
Result<std::unique_ptr<WzFile>> WzFile::open(const std::string& path);
Result<WzNode*> WzStructure::findNode(const std::string& path);

// 错误日志
namespace wzlib::log {
void error(const std::string& fmt, ...);
void warn(const std::string& fmt, ...);
void info(const std::string& fmt, ...);
void debug(const std::string& fmt, ...);
}
```

## 6. 性能目标

| 指标 | 目标 |
|---|---|
| 打开 100MB wz(仅构建目录树) | < 1 秒 |
| 单个 WzPng 解码(1024×1024 ARGB8888) | < 10ms |
| 跨文件路径查询 | < 1ms |
| 内存占用(打开 1GB wz 全树) | < 300MB |

## 7. 测试用例(关键)

```
tests/wzlib/test_crypto.cpp
- AES 密钥流生成(用 GMS/KMS/JMS/TMS 的 IV,对比 hex)
- 字符串解密(用真实 wz 文件中的字符串)
- 图像分块解密

tests/wzlib/test_wz_node.cpp
- 路径解析("a/b", "a/../b", "/a", "../b")
- UOL 解析
- 跨 WzImage 跳转

tests/wzlib/test_wz_png.cpp
- 各格式解码(用预存的黄金图)
- 像素 diff < 1%

tests/wzlib/test_wz_structure.cpp
- 打开多个真实 wz,验证树结构
- 跨文件路径覆盖
- 兼容路径重写(Map001 → Map)

tests/wzlib/test_string_linker.cpp
- 加载真实 String.wz
- 查询已知 ID

tests/wzlib/test_calculator.cpp
- 表达式求值边界 case
- "u(x/3)", "log10(x+1)", "x*2+30"
```

## 8. 与原 C# 版的差异

| 关注点 | 原 C# | 新 C++/Qt | 说明 |
|---|---|---|---|
| AES 库 | System.Security.Cryptography.Aes | OpenSSL EVP | 跨平台 |
| zlib | SharpCompress | zlib (vcpkg) | 同名 |
| 数据结构 | `Wz_Node` 类 | `WzNode` 类 | 1:1 映射 |
| UOL 解析 | Wz_Uol.HandleUol | WzUol::resolve | 行为一致 |
| 字符串 | `string` | `std::string`(UTF-8) | 注意编码转换 |
| 错误处理 | 异常 | std::variant / std::expected | C++ 习惯 |

## 9. 总结

wzlib 是整个项目的基础,设计重点是**正确性、跨平台性、可测试性**。一旦该模块完成并通过单测,后续模块(client/ui)就可以基于其 API 构建,不需要再担心 WZ 解析的细节。
