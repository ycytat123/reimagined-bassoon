# VST3 插件安装包 + 加壳保护 — 实施计划

## Context

为已编译好的 LTC Reader VST3 插件制作 Windows 安装包，安装到 VST3 标准路径，并对插件 DLL 进行加壳保护防止反向工程。

**现有产物：** `d:\fir\build\LTCReader_artefacts\Release\VST3\LTC Reader.vst3\`

## 工具选择

| 功能 | 工具 | 说明 |
|------|------|------|
| 安装包 | **Inno Setup 7** | 免费，轻量，脚本驱动，通过 winget 安装 |
| 压缩加壳 | **UPX 5.2** | 免费开源，压缩可执行文件，增加逆向难度 |

## 实施步骤

### Step 1: 安装工具

```bash
winget install JRSoftware.InnoSetup.7 --accept-source-agreements
winget install UPX.UPX --accept-source-agreements
```

### Step 2: 对 VST3 DLL 加壳保护

```bash
# UPX 压缩 VST3 二进制文件，减小体积并增加逆向难度
upx --best --lzma "d:\fir\release\LTC Reader.vst3\Contents\x86_64-win\LTC Reader.vst3"
```

UPX 参数说明：
- `--best`: 最高压缩率
- `--lzma`: 使用 LZMA 算法（比默认 NRV 压缩率更高）
- 输出：原文件会被压缩版本替换，自动备份原文件

### Step 3: 制作 Inno Setup 安装包脚本

```iss
; LTC Reader VST3 安装脚本
#define MyAppName "LTC Reader"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Hahahah"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commoncf}\VST3
; 目标路径指向系统 VST3 插件目录
; {commoncf} = C:\Program Files\Common Files

[Files]
Source: "d:\fir\release\LTC Reader.vst3\*"; DestDir: "{app}\LTC Reader.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
; 安装前检查：如果已存在，询问是否覆盖
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
```

### Step 4: 编译安装包

```bash
# 用 Inno Setup 编译器生成 exe 安装包
iscc "d:\fir\installer\setup.iss"
```

产出：`d:\fir\installer\LTC Reader Setup 1.0.0.exe`

## 文件结构

```
d:/fir/
├── installer/
│   ├── setup.iss                    # Inno Setup 安装脚本
│   └── LTC Reader Setup 1.0.0.exe   # 生成的安装包（产物）
├── release/
│   └── LTC Reader.vst3/             # 加壳后的 VST3 插件（中间产物）
└── ...
```

## 验证方法

1. 安装包能正确安装到 `C:\Program Files\Common Files\VST3\LTC Reader.vst3\`
2. REAPER 能扫描并加载安装后的插件
3. 插件功能正常（显示 LTC 时间码）
4. `upx -l` 确认压缩有效
5. 安装包大小明显小于未压缩版本
