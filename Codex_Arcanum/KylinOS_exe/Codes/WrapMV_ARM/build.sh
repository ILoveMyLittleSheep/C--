#!/bin/bash
set -e  # 出错时退出

# ========== 配置区域 ==========
PROJECT_DIR=$(pwd)
BUILD_DIR="$PROJECT_DIR/build"
SRC_DIR="$PROJECT_DIR/src"

# 工具路径
BOXPATH="box64"  # box64已在PATH中
WINEGCC="/home/user/wine/wine-10.17-amd64/bin/winegcc"

# x86_64 工具链
X64_BASE="/home/user/x86-64--glibc--stable-2022.08-1/bin"
X64_GCC="$X64_BASE/x86_64-linux-gcc"
X64_GXX="$X64_BASE/x86_64-linux-g++"
# ==============================

# 设置正确的链接器
echo "=== 1. 替换系统 ld ==="
echo "当前系统 ld: $(ls -l /usr/bin/ld 2>/dev/null || echo '不存在')"

sudo ln -sf "/home/user/x86-64--glibc--stable-2022.08-1/bin/x86_64-buildroot-linux-gnu-ld" /usr/bin/ld

echo "=========================================="
echo "    WrapMV 跨平台编译脚本 (ARM64 → x86-64)"
echo "=========================================="

echo "=== 2. 清理构建目录 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "=== 3. 编译 WrapMV.dll (x86-64 Windows DLL) ==="
echo "使用: winegcc (通过 box64)"

WRAP_LOG="$BUILD_DIR/wrapmv_compile.log"
$BOXPATH $WINEGCC -shared -m64 \
    -o "$BUILD_DIR/WrapMV.dll.so" \
    "$SRC_DIR/WrapMV.c" \
    "$SRC_DIR/WrapMV.def" \
    -I"$SRC_DIR" \
	-ldl -lpthread
	2>&1 | tee "$WRAP_LOG"

# 重命名为 .dll
if [ -f "$BUILD_DIR/WrapMV.dll.so" ]; then
    echo "✅ 生成 DLL 成功"

    # 创建符号链接
    cd "$BUILD_DIR"
    ln -sf WrapMV.dll.so WrapMV.dll
else
    echo "❌ 生成 DLL 失败"
    exit 1
fi

echo "=== 4. 验证生成文件 ==="
echo "生成的文件:"
ls -lh "$BUILD_DIR/" | grep -E "\.(so|dll)$"

echo ""
echo "文件类型检查:"
echo "1. WrapMV.dll:   $(file "$BUILD_DIR/WrapMV.dll" | cut -d: -f2-)"

echo ""
echo "=== 5. 恢复系统链接器 ==="
sudo ln -sf /usr/bin/aarch64-linux-gnu-ld.bfd /usr/bin/ld

echo ""
echo "=========================================="
echo "✅ 构建完成！"
echo "=========================================="
echo "生成的库文件:"
echo "  - $BUILD_DIR/WrapMV.dll    (Windows x86-64 DLL)"
echo ""
echo "使用说明:"
echo "1. 在 x86-64 Linux 上: 使用 libUnixMV.so"
echo "2. 在 Windows 或 Wine 上: 使用 WrapMV.dll"
echo "3. 在 ARM64 Linux + Wine + Box64 上:"
echo "   可以通过 Wine 加载 WrapMV.dll"
echo "=========================================="
echo "拷贝到XTOM路径"
cp "$BUILD_DIR/WrapMV.dll" "/home/user/.wine-xtom/drive_d/XTOM/"
cp "$BUILD_DIR/WrapMV.dll.so" "/home/user/.wine-xtom/drive_d/XTOM/"
cp "$BUILD_DIR/WrapMV.dll" "/home/user/.wine-xtom/drive_d/Release/"
cp "$BUILD_DIR/WrapMV.dll.so" "/home/user/.wine-xtom/drive_d/Release/"
