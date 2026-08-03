# LTC Reader 授权流程

## 第一步：获取机器码

在**客户**的电脑上运行：

```bash
# macOS
cat /var/lib/dbus/machine-id 2>/dev/null || echo "unknown" \
  | openssl dgst -sha256 | tr 'a-z' 'A-Z' | cut -c1-16

# Windows (Git Bash)
# 获取 MAC 地址列表的 SHA-256 前 16 位
```

然后把输出的 16 位机器码发给**授权签发方**。

## 第二步：签发许可证

**授权方**（持有 `license_private.pem`）用脚本生成：

```bash
./gen_license.sh sign "客户名称" <机器码> [到期日]
```

示例：
```bash
# 永久授权
./gen_license.sh sign "北京录音棚" D04F7A3BE18C9D2F

# 一年授权
./gen_license.sh sign "张三 Studios" D04F7A3BE18C9D2F 2027-08-01
```

生成 `.ltclic` 文件。

## 第三步：安装许可证

把 `.ltclic` 复制到客户机：

```bash
mkdir -p ~/Documents/LTC\ Reader/
cp 北京录音棚_D04F7A3BE18C9D2F.ltclic ~/Documents/LTC\ Reader/license.ltclic
```

Windows:
```
mkdir %USERPROFILE%\Documents\LTC Reader
copy 北京录音棚_xxx.ltclic "%USERPROFILE%\Documents\LTC Reader\license.ltclic"
```

## 第四步：重新加载插件

在 DAW 中重新扫描或重新加载 LTC Reader 插件。插件启动时自动读 `license.ltclic` 并验证 RSA 签名。验证通过 → 正常解码；验证失败 → 静音。
