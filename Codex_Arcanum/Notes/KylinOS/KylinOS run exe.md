> 操作系统：银河麒麟桌面操作系统（国防版）V10
>
> 版本号：2207
>
> 内核：linux 5.4.18-87.76-yx02-generic
>
> CPU：Phytium，D2000/8 S8I
>
> 内存：32GB
>
> 桌面：UKUI
>
> 硬件架构：ARM64
>
> 注：
>
> [20251110 失败]目前在Debian/Ubuntu仓库未能找到ARM64版本的wine，因此采用源码编译方法安装；
>
> [20251111 成功]使用Box64 + Wine技术路线，能够正常打开Wine，至于对Windows软件的适配性，需要进一步测试；
>
> [20251120 成功]处理了动态链接库的冲突，OM软件可以打开，但是打开项目时会闪退，需要进一步处理内部的依赖关系；

# 1、Wine环境配置

> 尝试方法：
>
> 1. 编译Wine Arm64源码，编译成功但不能正常运行，原因：
>
>   - 麒麟系统可能有定制修改，导致兼容性问题；
>   - Wine对ARM的原生支持还很有限，很多Windows程序包含x86/x64原生代码，无法在ARM上直接运行，所以不采用该方案；
>
> 2. 使用编译好的Wine Arm64的deb文件安装失败，原因：
>
>   - 依赖链在ARM上不完整；
>   - ARM版本的Wine包很少且不完整；
>
> 3. 使用麒麟仓库的Wine安装包，不使用原因：
>
>   - 版本过低，稳定版仅支持到Wine5版本，开发版也支持到Wine8版本；
>   - Wine组件版本管理比较混乱，组件与主程序版本不一致，容易导致一些意料之外的问题；
>
> 4. Box64 + Wine10
>
>   - Windows程序 → x86 Wine → box64 → ARM CPU
>
>   - 规避架构问题：用box64处理x86指令，Wine处理Windows API
>
>   - 利用成熟生态：x86 Wine生态远比ARM Wine成熟
>
>   - 解决依赖地狱：预编译的x86 Wine包含所有依赖
>
>   - 跨发行版兼容：不依赖特定Linux发行版的包管理
>
>     ```
>     Windows程序 (x86/x64)
>         ↓
>     Wine (x86/x64)   ← 处理Windows API调用
>         ↓  
>      box64          ← 动态二进制翻译
>          ↓
>     ARM64指令        ← 原生执行
>     ```

## 1.1 手动编译Wine Arm版本【失败】

### 1.1.1 克隆并切换到 stable 分支

- 第一步：安装 Git

  ```linux
  # 更新软件源并安装 git
  sudo apt update
  sudo apt install git -y
  
  # 验证安装
  git --version
  ```

- 第二步：使用 Git 克隆稳定版

  ```linux
  # 克隆 Wine 仓库
  cd ~
  git clone https://github.com/wine-mirror/wine.git
  
  # 进入目录
  cd wine
  
  # 切换到稳定分支
  git checkout stable
  
  # 验证当前分支
  git branch
  ```

- 第三步：Git 基本操作说明

  - 查看所有可用分支

  ```linux
  git branch -a
  ```

  - 查看当前状态

  ```linux
  git status
  ```

  - 查看提交历史

  ```linux
  git log --oneline -10
  ```

  - 如果需要更新代码

  ```linux
  git pull origin stable
  ```

  - 如果 GitHub 访问慢，用 Gitee 镜像

  ```linux
  # 删除之前的克隆（如果有）
  cd ~
  rm -rf wine
  
  # 使用 Gitee 镜像（国内速度更快）
  git clone https://gitee.com/mirrors/wine.git
  cd wine
  git checkout stable
  ```

  - 完整的 Git 使用流程

  ```linux
  # 1. 安装 Git
  sudo apt install git -y
  
  # 2. 克隆仓库
  git clone https://gitee.com/mirrors/wine.git
  
  # 3. 切换分支
  cd wine
  git checkout stable
  
  # 4. 验证
  git branch
  ```

  > Git 常用命令速查
  > git clone [url] 克隆远程仓库
  > git checkout [分支名] 切换分支
  > git branch	查看分支
  > git status	查看状态
  > git pull	更新代码

### 1.1.2 安装编译依赖

  - 安装交叉编译依赖

    ```linux
    # 安装基础编译工具
    sudo apt update
    sudo apt install build-essential -y
    
    # 安装必需依赖
    sudo apt install libx11-dev libfreetype6-dev libglu1-mesa-dev libgnutls28-dev -y
    
    # 安装工具
    sudo apt install flex bison autoconf -y
    
    # 可选但推荐的依赖
    sudo apt install libldap2-dev libsdl2-dev libvulkan-dev libpcap-dev libcups2-dev -y
    ```

### 1.1.3 配置编译选项

  ```linux
  # 基础配置（ARM64 会自动检测）
  ./configure --enable-win64 --prefix=/usr/local/wine-stable

  # 或者更详细的配置
  ./configure \
      --enable-win64 \
      --with-mingw \
      --with-vulkan \
      --prefix=/usr/local/wine-stable \
      --libdir=/usr/local/wine-stable/lib
  ```

  - 在ARM64上编译Wine需要交叉编译工具，导致配置失败，未能生成Makefile文件，因此在这里安装完整的交叉编译工具链

    > ARM64 Linux 系统
    >      ↓
    > 编译 ARM64 版本的 Wine
    >      ↓
    > 但 Wine 需要生成 Windows PE 格式的可执行文件
    >      ↓  
    > PE 格式是 x86/x64 架构的，需要交叉编译工具

    ```linux
    # 1. 查看最新版本
    curl -s https://api.github.com/repos/mstorsjo/llvm-mingw/releases/latest | grep browser_download_url
    
    # 【2-下载方法1】 下载通用 ARM64 版本（推荐）
    curl -L -o llvm-mingw.zip https://github.com/mstorsjo/llvm-mingw/releases/download/20251104/llvm-mingw-20251104-ucrt-aarch64.zip
    
    # 【2-下载方法2】 下载方法1连接github失败，手动下载后拷贝到此电脑
    文件版本： llvm-mingw-20251104-ucrt-ubuntu-22.04-aarch64.tar.xz
    
    # 【2-下载方法2】 进入下载目录
    cd ~/下载
    
    # 3. 解压到系统目录
    sudo tar -xf llvm-mingw-20251104-ucrt-ubuntu-22.04-aarch64.tar.xz -C /opt/
    
    # 4. 检查解压后的目录名
    ls -la /opt/ | grep llvm-mingw
    
    # 5. 如果目录名不是 llvm-mingw，创建符号链接
    sudo ln -sf /opt/llvm-mingw-20251104-ucrt-ubuntu-22.04-aarch64 /opt/llvm-mingw
    
    # 6. 配置环境变量：添加到 PATH
    echo 'export PATH="/opt/llvm-mingw/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    
    # 6. 配置环境变量：验证环境变量
    echo $PATH | grep llvm-mingw
    
    # 7. 验证安装
    which aarch64-w64-mingw32-clang
    aarch64-w64-mingw32-clang --version
    ```

    - 编译时遇到新问题，llvm-mingw因为版本兼容性问题无法在麒麟系统运行

    > 🏗️ GLIBC 是什么？
    > GLIBC（GNU C Library） 是 Linux 系统的核心基础库，相当于 Linux 的"心脏"。
    >
    > ```
    > 系统的"基础骨架"
    > 
    > 应用程序 (App1, App2, App3...)
    >     ↓
    > 各种编程语言库 (Python, Java, C++...)
    >     ↓  
    > GLIBC ← 系统的核心基础库
    >     ↓
    > Linux 内核 (Kernel)
    > ```

    ```linux
    # 查询系统的GLIBC版本（2.31）
    ldd --version
    # ldd (Ubuntu GLIBC 2.31-0kylin9.1k20.5) 2.31
    
    # 下载的llvm-mingw需要的最低GLIBC版本（3.4）
    strings /usr/lib/aarch64-linux-gnu/libstdc++.so.6 | grep GLIBCXX
    GLIBCXX_3.4
    GLIBCXX_3.4.1
    GLIBCXX_3.4.2
    GLIBCXX_3.4.3
    GLIBCXX_3.4.4
    GLIBCXX_3.4.5
    GLIBCXX_3.4.6
    GLIBCXX_3.4.7
    GLIBCXX_3.4.8
    GLIBCXX_3.4.9
    GLIBCXX_3.4.10
    GLIBCXX_3.4.11
    GLIBCXX_3.4.12
    GLIBCXX_3.4.13
    GLIBCXX_3.4.14
    GLIBCXX_3.4.15
    GLIBCXX_3.4.16
    GLIBCXX_3.4.17
    GLIBCXX_3.4.18
    GLIBCXX_3.4.19
    GLIBCXX_3.4.20
    GLIBCXX_3.4.21
    GLIBCXX_3.4.22
    GLIBCXX_3.4.23
    GLIBCXX_3.4.24
    GLIBCXX_3.4.25
    GLIBCXX_3.4.26
    GLIBCXX_3.4.27
    GLIBCXX_3.4.28
    GLIBCXX_DEBUG_MESSAGE_LENGTH
    
    # 这里重新选择llvm-mingw-20220906-ucrt-ubuntu-18.04-aarch64.tar.xz版本
    # https://github.com/mstorsjo/llvm-mingw/releases/download/20220906/llvm-mingw-20220906-ucrt-ubuntu-18.04-aarch64.tar.xz
    # 清理上一个版本的安装残余文件不作说明
    
    # 解压
    tar -xf llvm-mingw-20220906-ucrt-ubuntu-18.04-aarch64.tar.xz
    
    # 安装
    sudo mv llvm-mingw-20220906-ucrt-ubuntu-18.04-aarch64 /opt/llvm-mingw
    
    # 配置环境
    echo 'export PATH="/opt/llvm-mingw/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    ```

    ### 1.1.4 开始编译

    - 执行编译指令

      > 预计编译时间：
      > 首次编译：30分钟到2小时（取决于电脑性能）
      > 后续编译：会快很多 

        ```linux
        # 开始编译 Wine
        make -j$(nproc)
      
        # 或者如果你想知道编译进度
        make -j$(nproc) 2>&1 | tee build.log
        ```

    - 如果编译中断：

      ```linux
      # 继续编译（不会从头开始）
      make -j$(nproc)
      
      # 或者清理后重新编译
      make distclean
      ./configure --enable-win64 --prefix=/usr/local/wine-stable
      make -j$(nproc)
      ```

    - 如果编译遇到问题

      ```linux
      # 内存不足：减少并行编译数
      make -j2
      
      # 依赖缺失：根据错误信息安装对应开发包
      sudo apt install libxxxx-dev
      ```

    - 如果版本不能正常运行，可以编译更早版本的wine

      ```linux
      # 查看可用的 9.x 版本
      git tag | grep "wine-9" | sort -V
      
      # 切换到最新的 9.x 版本（比如 9.0）
      git checkout wine-9.0
      
      # 清理环境
      make distclean 2>/dev/null || make clean
      
      # 配置
      ./configure --enable-win64 --prefix=/usr/local/wine-9.0
      
      # 4. 开始编译（单线程）
      make -j1
      ```

    - 编译成功，但是运行失败，尝试方法二。

## 1.2 Box64+Wine 

  ### 1.2.1 手动编译安装Box64

  - 克隆Box64源码

    ```linux
    # 安装必须的依赖
    sudo apt install git cmake build-essential
    
    git clone https://github.com/ptitSeb/box64
    # github连接失败可以使用内网镜像
    git clone https://gitcode.com/gh_mirrors/bo/box64
    cd box64
    ```

  - 创建构建目录并编译

    ```linux
    mkdir build
    cd build
    
    # 配置编译选项
    cmake .. -DARM_DYNAREC=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
    
    # 开始编译（根据CPU核心数调整-j参数）
    make -j4
    ```

  - 安装Box64

    ```linux
    sudo make install
    ```

  - 配置系统

    ```linux
    # 重新加载系统库
    sudo ldconfig
    
    # 设置box64为默认的x86_64解释器
    sudo systemctl daemon-reload
    ```

  - 检查Box64安装

    ```linux
    box64 --version
    ```

  ### 1.2.2 x86_64 Wine包下载

  - 下载合适的Wine包

    > 官方构建版本 (Official Builds)
    >
    >   - 由Wine开发者或社区维护者预先编译好
    >   - 针对x86_64架构编译 

    ```linux
    cd ~
    # 下载专为box64优化的Wine构建
    wget https://github.com/Kron4ek/Wine-Builds/releases/download/10.0/wine-10.0-amd64.tar.xz
    tar -xf wine-10.0-amd64.tar.xz
    
    # 启动wineboot
    cd wine-10.0-amd64/bin
    # 测试wine安装版本
    box64 ./wine64 --version
    
    # 以下是windows环境的完整初始化流程
    # 1. 设置Wine架构（64位）
    export WINEARCH=win64
    
    # 2. 设置Wine前缀（可选，不设置就使用默认的 ~/.wine）
    # 可以使用不同WINEPREFIX创建独立环境（多个windows环境）
    export WINEPREFIX=~/.wine10
    
    # 3. 初始化Wine环境（只需要执行一次）
    box64 ./wine64 wineboot -i
    
    # 4. 运行winecfg进行配置（可选）
    box64 ./wine64 winecfg
    ```

  - wine-mono组件安装

    > `box64 ./wine64 wineboot`指令会提示wine-mono 组件，选择安装即可
    >
    > - wine-mono 是 .NET Framework 的开源实现
    > - 很多Windows程序需要.NET环境才能运行
    > - 包括：C#程序、ASP.NET应用、Unity游戏等
    >   如果安装失败，可以自行手动安装
    >
    > ```
    > # 手动下载wine-mono
    > cd ~/wine-10.0-amd64
    > wget https://dl.winehq.org/wine/wine-mono/8.1.0/wine-mono-8.1.0-x86.msi
    > # 使用Wine安装
    > wine10 msiexec /i wine-mono-8.1.0-x86.msi
    > ```linux
    > ```

  - 创建wine10脚本

    >`cat > ~/wine10 << 'EOF'` 
    >`cat`: 连接文件并打印到标准输出;
    >`\> ~/wine10`: 将输出重定向（创建）到当前用户主目录下的 wine10 文件;
    >`<< 'EOF'`: 一种称为 "Here Document" 的语法。它告诉 `cat` 从接下来的行中读取内容，直到遇到单独一行的 `EOF` 为止。单引号的 'EOF' 表示禁止变量替换，确保 `$PATH` 和 `$@` 等内容原样写入文件，而不是在执行此命令时就被展开;
    >`#!/bin/bash`: `Shebang`。指定这个脚本要用哪个解释器来执行，这里指定使用 /bin/bash.
    >
    >___
    >
    >`export WINEARCH=win64`
    >`export`: 设置环境变量，并使其对后续启动的任何子进程都可见;
    >`WINEARCH=win64`: 告诉 Wine 我们要模拟一个 64位 的 Windows 环境，这是至关重要的，因为它决定了 Wine 如何设置其内部结构.
    >
    >___
    >
    >`export WINEPREFIX=~/.wine10`
    >`WINEPREFIX`: 这是 Wine 中一个极其重要的概念;
    >`~/.wine10`: `~`代表当前用户的主目录。所以这行指令的意思是：将 Wine 的配置、安装的 Windows 程序以及虚拟的 C: 盘（drive_c）都存放在 `~/.wine10` 这个目录下.
    >为什么要这么做？ 默认的 Wine 前缀是 ~/.wine。通过创建一个新的前缀（.
    >wine10），你可以：
    >①隔离环境：避免与你系统上可能存在的其他 Wine 版本（如通过包管理器安装的）发生冲突。
    >②方便管理：如果这个基于 Box64 的 Wine 环境出了问题，直接删除整个 ~/.wine10 文件夹即可，不会影响其他 Wine 配置.
    >
    >___
    >
    >`export PATH=~/wine-x64/wine-10.0-amd64/bin:$PATH`
    >`PATH`: 系统用来查找可执行文件的环境变量;
    >`~/wine-x64/wine-10.0-amd64/bin:$PATH`: 之前下载并解压的、专为 x86_64 架构编译的 Wine 程序所在目录;
    >`:$PATH`: 将原有的 PATH 变量内容附加在新路径之后;
    >作用：这样设置后，当你在脚本中执行 wine64 时，系统会首先在 ~/wine-10.0-amd64/bin 目录下寻找，确保我们使用的是这个特定的 Wine 版本，而不是系统自带的（如果有的话）.
    >
    >___
    >
    >`box64 wine64 "$@"`
    >`box64`: 这是核心！它作为一个“翻译层”，拦截 wine64 这个 x86_64 程序对系统的调用，并将其转换为 ARM64 指令.
    >`wine64`: 这是我们要运行的 x86_64 版本的 Wine 程序.
    >`"$@"`: 这是一个 Bash 特殊变量，它代表了传递给脚本的所有命令行参数.
    >例如，当你执行 `~/wine10 notepad` 时，`$@` 就等同于 `notepad`.
    >当你执行 `~/wine10 winecfg` 时，`$@` 就等同于 `winecfg`.
    >整行作用：用 box64 来运行 wine64 程序，并将用户传给 wine10 脚本的任何参数原封不动地传给 wine64.
    >`EOF`: 标志着 "Here Document" 的结束，`cat` 命令在这里停止读取，并完成文件的创建.

    ```linux
    # 创建wine运行脚本
    cat > ~/wine-run << 'EOF'
    #!/bin/bash
    # 只运行 Windows 程序，不涉及组件安装
    export WINEARCH=win64
    export WINEPREFIX=~/.wine10
    box64 ~/wine-x64/wine-10.0-amd64/bin/wine64 "$@"
    EOF
    chmod +x ~/wine-run
    
    # 验证文件
    ls -la ~/wine-run
    cat ~/wine-run
    ```

  - 测试基本功能

    ```linux
    ~/wine-run --version
    # 测试Wine配置界面
    ~/wine-run winecfg
    
    # 测试运行Windows记事本
    ~/wine-run notepad
    
    # 测试命令行
    ~/wine-run cmd
    ```

### 1.2.3 完善Wine组件

  - 安装wintricks

    > winetricks 是专门为 Wine 环境下载和安装 Windows 组件的工具，但是Winetricks不一定能够正确识别Box64 + Wine的特殊环境，直接使用可能会出现问题，因此可以不用安装，直接下载所需的文件，自行使用Wine安装。

# 2、软件移植

## 2.1 动态链接库跨平台实现

### 2.1.1 技术路线选择

| 方案             | 核心原理                         | 优点                     | 缺点                    | 适用场景                         |
| :--------------- | -------------------------------- | ------------------------ | ----------------------- | -------------------------------- |
| 1.DLL包装器 ✅    | 拦截Win API调用，转至Linux原生库 | 性能好，控制精细         | 开发复杂，需深入理解API | 性能要求高的专用硬件             |
| 2.本地服务桥接   | Windows软件与Linux服务进程通信   | 架构清晰，易于调试       | 需要进程间通信          | 复杂设备，已有Linux驱动          |
| 3.设备直通虚拟机 | 虚拟机直接控制物理硬件           | 近乎原生性能，兼容性完美 | 资源开销大              | 对性能和稳定性要求极高的工业环境 |
| 4.协议重实现 ❌   | 在Wine内重新实现设备协议         | 无需外部依赖，部署简单   | 逆向工程难度大          | 协议已知的简单设备               |
| 5.混合容器化     | 容器内同时运行Win和Linux组件     | 资源隔离，部署灵活       | 配置复杂                | 云环境或现代部署需求             |

【方案1】DLL包装器的工作原理

```
Windows软件 → 调用原厂DLL函数 → DLL包装器 → 转换为Linux系统调用 → 实际硬件
     ↑              ↑              ↑
   (x86_64)      (空壳DLL)     (架构转换层)
```

【方案2】本地服务桥接的工作原理

```
Windows软件 → Wine环境 → 进程间通信 → Linux本地服务 → 硬件驱动
    ↑            ↑          ↑            ↑           ↑
 (调用DLL) (轻量代理DLL) (socket/管道) (原生C/C++)(直接控制)
```

【方案3】设备直通虚拟机的工作原理

```
物理硬件 → Linux KVM → Windows虚拟机 → 扫描软件
   ↑           ↑            ↑              ↑
(USB设备)   (VFIO直通)   (原生驱动)    (直接调用)
```

【方案4】协议重实现的工作原理

```C++
// 基于逆向分析重新实现串口协议
// serial_reimpl.c - 在Wine内重新实现协议

#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// 重新实现厂商专有协议
typedef struct {
    int linux_fd;
    DWORD windows_handle;
    BOOL is_connected;
} SERIAL_DEVICE;

HRESULT WINAPI VendorSerialInit(DWORD baudrate, LPVOID* handle) {
    SERIAL_DEVICE* dev = malloc(sizeof(SERIAL_DEVICE));
    
    // 打开Linux串口设备
    dev->linux_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (dev->linux_fd < 0) {
        free(dev);
        return E_FAIL;
    }
    
    // 配置串口参数
    struct termios options;
    tcgetattr(dev->linux_fd, &options);
    cfsetispeed(&options, B9600);  // 根据baudrate转换
    cfsetospeed(&options, B9600);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(dev->linux_fd, TCSANOW, &options);
    
    dev->is_connected = TRUE;
    *handle = dev;
    return S_OK;
}

// 重新实现专有数据包格式
HRESULT WINAPI VendorSendCommand(LPVOID handle, BYTE command, BYTE* data, DWORD data_len) {
    SERIAL_DEVICE* dev = (SERIAL_DEVICE*)handle;
    
    // 构建厂商专有协议帧
    BYTE packet[256];
    int packet_len = 0;
    
    // 帧头
    packet[packet_len++] = 0xAA;
    packet[packet_len++] = 0x55;
    
    // 命令字节
    packet[packet_len++] = command;
    
    // 数据长度
    packet[packet_len++] = data_len;
    
    // 数据内容
    memcpy(packet + packet_len, data, data_len);
    packet_len += data_len;
    
    // 校验和
    BYTE checksum = 0;
    for (int i = 0; i < packet_len; i++) {
        checksum ^= packet[i];
    }
    packet[packet_len++] = checksum;
    
    // 发送数据
    write(dev->linux_fd, packet, packet_len);
    return S_OK;
}
```

【方案5】Docker部署架构

```linux
# Dockerfile - 混合Windows/Linux环境
FROM ubuntu:20.04 AS base

# 安装Wine和Box64
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y wine wine32 winetricks

# 安装硬件访问工具
RUN apt-get install -y usbutils v4l-utils

# 复制Windows软件
COPY windows_app/ /app/windows/
COPY linux_drivers/ /app/linux/

# 设置启动脚本
COPY entrypoint.sh /app/
RUN chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
```

### 2.1.2 动态链接库分类处理

| DLL类型      | 处理方案  | 理由                           | 实施难度 |
| ------------ | --------- | ------------------------------ | -------- |
| 图形界面类   | 直接使用  | Wine对Qt、MFC等有较好支持      | ⭐        |
| 基础运行时库 | 直接使用  | Wine内置或可通过winetricks安装 | ⭐        |
| 纯计算函数   | 直接使用  | 架构转换开销可接受             | ⭐⭐       |
| 硬件交互类   | DLL包装器 | 需要直接访问Linux驱动          | ⭐⭐⭐⭐     |
| 显卡计算类   | 最难处理  | 涉及GPU架构差异                | ⭐⭐⭐⭐⭐    |
| 专用加密狗   | 混合方案  | 需要特定驱动支持               | ⭐⭐⭐      |

- 可以直接使用的库

  ```
  # Qt库
  Qt5Core.dll
  Qt5Gui.dll  
  Qt5Widgets.dll
  
  # VC++ 运行时库
  msvcp100.dll
  msvcr100.dll
  vcruntime140.dll
  
  # 系统API库
  kernel32.dll  # Wine有良好实现
  user32.dll
  gdi32.dll
  advapi32.dll
  # 数学计算、数据处理等
  calculation_engine.dll
  math_library.dll
  data_processing.dll
  ```

  这些库不涉及硬件特性和系统调用，Box64的指令转换足够高效，因此无需处理。

- 需要DLL包装器的DLL

  ```
  # 串口通信
  serial_communication.dll
  com_port_manager.dll
  
  # USB/HID设备
  hid_access.dll
  usb_controller.dll
  
  # 相机控制
  camera_sdk.dll
  frame_grabber.dll
  
  # 专用硬件
  scanner_controller.dll
  motion_control.dll
  ```

  包装示例

  ```C++
  // serial_wrapper.c
  #include <windows.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <termios.h>
  
  // 替换原厂串口DLL的函数
  BOOL WINAPI SerialOpenWrapper(LPCSTR portName, DWORD baudRate) {
      // 映射COM端口到Linux设备
      const char* linux_device = map_com_to_linux(portName);
      int fd = open(linux_device, O_RDWR | O_NOCTTY);
      
      // 配置串口参数
      struct termios options;
      tcgetattr(fd, &options);
      cfsetispeed(&options, convert_baudrate(baudRate));
      // ... 更多配置
      
      return (fd != -1);
  }
  ```

- 需要特殊处理的DLL
  显卡计算类

  ```
  # CUDA计算
  cudart64.dll
  nppial.dll
  nvgraph.dll
  
  # OpenCL计算
  opencl.dll
  
  # 专用GPU计算
  gpu_acceleration.dll
  cuda_plugin.dll
  ```

  处理方案选择

  ```mermaid
  graph TD
    A[GPU计算DLL] --> B{功能类型}
    B --> C[通用计算]
    B --> D[图形渲染]
    B --> E[AI推理]
    
    C --> F[转换到OpenCL]
    D --> G[使用Vulkan/OpenGL]
    E --> H[寻找ARM替代]
    
    F --> I[创建包装器]
    G --> J[使用WineD3D]
    H --> K[TensorFlow Lite等]
  ```

  **加密狗处理**

  ```
  # 威步加密狗
  WIBUCM64.dll
  
  # 其他许可证管理
  sentinel.dll
  hasplms.dll
  ```

## 2.2 初步运行测试

- 将XTOM的release文件夹拷贝到麒麟系统，使用`~/wine-run XTOM.exe`指令运行，首先遇到以下问题：

  ```linux
  004c:err:dnsapi:DllMain No libresolv support, expect problems
  0024:err:module:import_dll Library mfc100.dll (which is needed by L"C:\\Program Files\\Release\\EasyPODx64.dll") not found
  0024:err:module:import_dll Library EasyPODx64.dll (which is needed by L"C:\\Program Files\\Release\\XTOM.exe") not found
  0024:err:module:import_dll Library MVSDKmd.dll (which is needed by L"C:\\Program Files\\Release\\CAMCTRL_64.dll") not found
  0024:err:module:import_dll Library GxIAPI.dll (which is needed by L"C:\\Program Files\\Release\\CAMCTRL_64.dll") not found
  0024:err:module:import_dll Library DxImageProc.dll (which is needed by L"C:\\Program Files\\Release\\CAMCTRL_64.dll") not found
  0024:err:module:import_dll Library CAMCTRL_64.dll (which is needed by L"C:\\Program Files\\Release\\XTOM.exe") not found
  0024:err:module:import_dll Library WIBUCM64.dll (which is needed by L"C:\\Program Files\\Release\\XTOM.exe") not found
  0024:err:module:loader_init Importing dlls for L"C:\\Program Files\\Release\\XTOM.exe" failed, status c0000135
  ```

- 分析问题
  以第一句报错为例：`0024:err:module:import_dll Library mfc100.dll (which is needed by L"C:\\Program Files\\Release\\EasyPODx64.dll") not found`，意味着 EasyPODx64.dll 需要 mfc100.dll 动态链接库的支持，下载安装VC++2010即可

  ```
  wget https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x64.exe
  ~/wine-run vcredist_x64.exe
  ```

  除此之外，其余问题皆是由于`CAMCTRL_64.dll`未找到所需相机的动态链接库导致的，在这里尝试了

  1. 使用DLL重定向

    ```linux
  # 完全禁用这些DLL的加载
  export WINEDLLOVERRIDES="MVSDKmd=b,GxIAPI=b,DxImageProc=b,CAMCTRL_64=b,WIBUCM64=b"
  box64 wine "C:\\Program Files\\Release\\XTOM.exe"
    ```

  2. 创建最小化DLL占位文件

    ```
  # 快速创建所有缺失DLL的空文件
  cd ~/.wine/drive_c/windows/system32/
  for dll in MVSDKmd.dll GxIAPI.dll DxImageProc.dll CAMCTRL_64.dll; do
      touch "$dll"
  done
    ```

  重新尝试运行，依旧提示依赖库未找到的错误。

- 备份当前windows环境
  上一步说明这里想要绕过dll依赖只运行UI环境难度较大，因此尝试在windows环境安装相关的相机驱动，但是为了确保后续使用dll包装方法时windows环境是纯净的，因此需要备份当前的windows环境，用作后续的dll包装处理。

  > 当在Wine中安装了Windows驱动后，DLL加载的优先级会是：
  >
  > ```text
  > Wine内置DLL → Wine的system32目录 → Windows驱动DLL → 包装DLL
  > ```
  >

  完整备份当前windows环境方法

  ```linux
  # 1. 首先确认当前环境
  #!/bin/bash
  
  # 1. 设置变量
  export WINEARCH=win64
  export OLD_WINEPREFIX="/home/user/.wine10-17"
  export NEW_WINEPREFIX="/home/user/.wine-xtom"
  export WINE_BIN_DIR="/home/user/wine-x64/wine-10.17-amd64/bin"
  
  echo "=== Wine环境迁移 ==="
  echo "源: $OLD_WINEPREFIX"
  echo "目标: $NEW_WINEPREFIX"
  
  # 2. 检查源环境是否存在
  if [ ! -d "$OLD_WINEPREFIX" ]; then
      echo "❌ 源Wine环境不存在: $OLD_WINEPREFIX"
      exit 1
  fi
  
  # 3. 检查目标是否已存在
  if [ -d "$NEW_WINEPREFIX" ]; then
      read -p "⚠️ 目标环境已存在，是否覆盖？(y/n): " confirm
      if [ "$confirm" != "y" ]; then
          echo "操作取消"
          exit 0
      fi
      echo "移除现有环境..."
      rm -rf "$NEW_WINEPREFIX"
  fi
  
  # 4. 停止所有Wine进程（包括旧的）
  echo "停止Wine进程..."
  cd "$WINE_BIN_DIR"
  box64 ./wineserver -k 2>/dev/null || true
  sleep 3  # 等待进程完全退出
  
  # 5. 使用rsync进行更可靠的拷贝（保留权限）
  echo "拷贝Wine环境..."
  rsync -av --progress "$OLD_WINEPREFIX/" "$NEW_WINEPREFIX/"
  
  # 6. 修复可能的权限问题
  echo "修复权限..."
  find "$NEW_WINEPREFIX" -type f -name "*.exe" -exec chmod +x {} \; 2>/dev/null || true
  find "$NEW_WINEPREFIX" -type f -name "*.dll" -exec chmod +x {} \; 2>/dev/null || true
  
  # 7. 更新注册表中的路径（如果需要）
  # 某些程序可能硬编码了路径，需要更新
  echo "更新环境配置..."
  cat > "$NEW_WINEPREFIX/update_registry.reg" << 'EOF'
  REGEDIT4
  
  [HKEY_CURRENT_USER\Environment]
  "WINEPREFIX"="%USERPROFILE%\\.wine-xtom"
  EOF
  
  box64 ./wine regedit "$NEW_WINEPREFIX/update_registry.reg" 2>/dev/null || true
  
  # 8. 验证新环境
  echo "验证新环境..."
  export WINEPREFIX="$NEW_WINEPREFIX"
  
  # 运行wineboot初始化
  box64 ./wine wineboot -u 2>&1 | tail -5
  box64 ./wine64 winecfg &
  echo "新环境已就绪: $WINEPREFIX"
  ```

  拷贝完新环境后，拷贝此前新建的`wine-run`shell脚本并修改windows环境路径，使用该脚本运行测试windows环境。

- 在拷贝的测试windows环境安装相机驱动

  ```linux
  cd /home/user/下载
  ~/wine-test ./MVviewer_2_3.exe
  ~/wine-test ./Galaxy_Windows_CN_32bits-64bits_2.5.2509.9041.exe
  ```

- 在Wine搭建的windows环境下安装相机驱动完毕，继续运行，弹窗提示`This application failed to start because it could not find or load the Qt platform plugin “windows“`。

  > 具体来说，这个错误可能由以下几个原因引起：
  >
  > 1. 环境配置不正确：
  >    Qt应用程序依赖于一系列的环境变量来找到其所需的资源，包括平台插件。如果QT_QPA_PLATFORM_PLUGIN_PATH环境变量没有正确设置，或者指向的路径不包含qwindows.dll（Windows平台插件），那么应用程序将无法加载它。
  > 2. 文件缺失或损坏：
  >    qwindows.dll文件可能因安装不完整、文件损坏或误删除而缺失。这通常发生在Qt安装过程中，或者当应用程序被部署到目标机器时，如果某些必要的文件没有被包括在内。
  > 3. 路径问题：
  >    如果Qt应用程序被部署到了一个不包含必要插件的目录结构中，或者如果插件被移动到了不同的位置而没有更新相应的环境变量或配置文件，那么应用程序将无法找到它们。
  > 4. 权限问题：
  >    在某些情况下，应用程序可能没有足够的权限来访问其所需的文件或目录。这可能是由于操作系统的安全策略、文件系统的权限设置或运行应用程序的用户帐户的权限级别。
  > 5. Qt版本不兼容：
  >    如果应用程序是使用一个版本的Qt开发的，但是尝试在一个不兼容的Qt版本上运行，那么可能会出现各种问题，包括无法加载平台插件。
  > 6. 依赖库问题：
  >    Qt应用程序可能依赖于其他库或组件，这些库或组件如果缺失、损坏或版本不兼容，也可能导致无法加载平台插件。

  解决方法：
  在Windows环境下，使用windeployqt工具，重新生成Qt依赖，然后把相关QT依赖文件拷贝到程序路径下，即可正常运行软件。

  ```plaintext
  ├── XTOM.exe
  ├── Qt5Core.dll
  ├── Qt5Gui.dll
  ├── Qt5Widgets.dll
  └── platforms/
      └── qwindows.dll
  ```

- 打开工程时，软件闪退，在闪退的代码附近打印日志，定位闪退代码

  ```C++
  // Index = 1时，切换到OpenGL窗口
  QStackedWidget::setCurrentIndex(1);
  ```

  保存wine在命令窗口的输出

  ```linux
  ~/wine-test XTOM.exe 2>&1 | tee full_output.log
  ```

  检索wine输出的错误信息，发现程序在尝试使用OpenGL渲染时失败了，可能原因：

  - OpenGL版本不兼容
  - 图形驱动不支持某些特性
  - 在虚拟化环境中运行
  - ARM64与x86_64图形库的兼容性问题

  ```linux
  platform 1
  0128:fixme:msg:ChangeWindowMessageFilterEx 00000000000100BC c08c 1 0000000000000000
  Using native(wrapped) libGL.so.1
  Using native(wrapped) libvulkan.so.1
  X Error of failed request:  GLXBadFBConfig
  Major opcode of failed request:  152 (GLX)
  Minor opcode of failed request:  0 ()
  Serial number of failed request:  8055
  Current serial number in output stream:  8055
  ```

  直接使用OpenGL库编写测试程序，测试box64 + wine环境下是否能够正常使用显卡渲染OpenGL

  ```C++
  #include <windows.h>
  #include <GL/gl.h>
  #include <math.h>
  
  // 全局变量
  HDC hDC;
  HGLRC hRC;
  HWND hWnd;
  int window_width = 800;
  int window_height = 600;
  float rotation_angle = 0.0f;
  
  // 函数声明
  void DrawColorCube(void);
  void DrawWireCube(void);
  void DrawPyramid(void);
  void DrawScene(void);
  
  // 简单的透视投影实现
  void SimplePerspective(double fovy, double aspect, double zNear, double zFar) {
      double f = 1.0 / tan(fovy * 3.14159265358979323846 / 360.0);
      glLoadIdentity();
      glMultMatrixd((double[]){
          f/aspect, 0, 0, 0,
          0, f, 0, 0,
          0, 0, (zFar+zNear)/(zNear-zFar), -1,
          0, 0, (2*zFar*zNear)/(zNear-zFar), 0
      });
  }
  
  // 简单的视图矩阵实现
  void SimpleLookAt(double eyex, double eyey, double eyez,
                    double centerx, double centery, double centerz,
                    double upx, double upy, double upz) {
      double forward[3], side[3], up[3];
      
      forward[0] = centerx - eyex;
      forward[1] = centery - eyey;
      forward[2] = centerz - eyez;
      
      double len = sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
      forward[0] /= len; forward[1] /= len; forward[2] /= len;
      
      up[0] = upx; up[1] = upy; up[2] = upz;
      
      side[0] = forward[1] * up[2] - forward[2] * up[1];
      side[1] = forward[2] * up[0] - forward[0] * up[2];
      side[2] = forward[0] * up[1] - forward[1] * up[0];
      
      len = sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
      side[0] /= len; side[1] /= len; side[2] /= len;
      
      up[0] = side[1] * forward[2] - side[2] * forward[1];
      up[1] = side[2] * forward[0] - side[0] * forward[2];
      up[2] = side[0] * forward[1] - side[1] * forward[0];
      
      double m[16] = {
          side[0], up[0], -forward[0], 0,
          side[1], up[1], -forward[1], 0,
          side[2], up[2], -forward[2], 0,
          0, 0, 0, 1
      };
      
      glMultMatrixd(m);
      glTranslated(-eyex, -eyey, -eyez);
  }
  
  // OpenGL初始化
  BOOL InitOpenGL(HWND hWnd) {
      PIXELFORMATDESCRIPTOR pfd;
      int pixelformat;
      
      hDC = GetDC(hWnd);
      
      ZeroMemory(&pfd, sizeof(pfd));
      pfd.nSize = sizeof(pfd);
      pfd.nVersion = 1;
      pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
      pfd.iPixelType = PFD_TYPE_RGBA;
      pfd.cColorBits = 32;
      pfd.cDepthBits = 24;
      pfd.iLayerType = PFD_MAIN_PLANE;
      
      pixelformat = ChoosePixelFormat(hDC, &pfd);
      if (pixelformat == 0) return FALSE;
      if (SetPixelFormat(hDC, pixelformat, &pfd) == FALSE) return FALSE;
      Sleep(1000);
      hRC = wglCreateContext(hDC);
      if (hRC == NULL) return FALSE;
      if (wglMakeCurrent(hDC, hRC) == FALSE) return FALSE;
      
      // 设置视口和投影
      glViewport(0, 0, window_width, window_height);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      SimplePerspective(45.0, (double)window_width / (double)window_height, 0.1, 100.0);
      glMatrixMode(GL_MODELVIEW);
      
      glEnable(GL_DEPTH_TEST);
      glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
      
      return TRUE;
  }
  
  // 绘制彩色立方体
  void DrawColorCube(void) {
      glBegin(GL_QUADS);
      
      // 前面 - 红色
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3f(-1.0f, -1.0f, 1.0f);
      glVertex3f(1.0f, -1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, 1.0f);
      glVertex3f(-1.0f, 1.0f, 1.0f);
      
      // 后面 - 绿色
      glColor3f(0.0f, 1.0f, 0.0f);
      glVertex3f(-1.0f, -1.0f, -1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f);
      glVertex3f(1.0f, -1.0f, -1.0f);
      
      // 上面 - 蓝色
      glColor3f(0.0f, 0.0f, 1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f);
      glVertex3f(-1.0f, 1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f);
      
      // 下面 - 黄色
      glColor3f(1.0f, 1.0f, 0.0f);
      glVertex3f(-1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, -1.0f, 1.0f);
      glVertex3f(-1.0f, -1.0f, 1.0f);
      
      // 右面 - 品红色
      glColor3f(1.0f, 0.0f, 1.0f);
      glVertex3f(1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f);
      glVertex3f(1.0f, 1.0f, 1.0f);
      glVertex3f(1.0f, -1.0f, 1.0f);
      
      // 左面 - 青色
      glColor3f(0.0f, 1.0f, 1.0f);
      glVertex3f(-1.0f, -1.0f, -1.0f);
      glVertex3f(-1.0f, -1.0f, 1.0f);
      glVertex3f(-1.0f, 1.0f, 1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f);
      
      glEnd();
  }
  
  // 绘制线框立方体
  void DrawWireCube(void) {
      glBegin(GL_LINES);
      glColor3f(1.0f, 1.0f, 1.0f);
      
      // 前面
      glVertex3f(-1.0f, -1.0f, 1.0f); glVertex3f(1.0f, -1.0f, 1.0f);
      glVertex3f(1.0f, -1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f);
      glVertex3f(-1.0f, 1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, 1.0f);
      
      // 后面
      glVertex3f(-1.0f, -1.0f, -1.0f); glVertex3f(1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, -1.0f, -1.0f); glVertex3f(1.0f, 1.0f, -1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
      
      // 连接线
      glVertex3f(-1.0f, -1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, -1.0f, 1.0f); glVertex3f(1.0f, -1.0f, -1.0f);
      glVertex3f(1.0f, 1.0f, 1.0f); glVertex3f(1.0f, 1.0f, -1.0f);
      glVertex3f(-1.0f, 1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);
      
      glEnd();
  }
  
  // 绘制场景
  void DrawScene(void) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      
      // 设置相机
      SimpleLookAt(5.0, 4.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
      
      // 绘制旋转的彩色立方体
      glPushMatrix();
      glRotatef(rotation_angle, 1.0f, 1.0f, 0.5f);
      DrawColorCube();
      glPopMatrix();
      
      // 绘制线框立方体
      glPushMatrix();
      glTranslatef(2.5f, 0.0f, 0.0f);
      glRotatef(rotation_angle * 1.5f, 0.0f, 1.0f, 1.0f);
      DrawWireCube();
      glPopMatrix();
      
      SwapBuffers(hDC);
  }
  
  // 窗口过程函数
  LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
      switch (message) {
          case WM_CREATE:
              if (!InitOpenGL(hWnd)) {
                  PostQuitMessage(0);
              }
              break;
              
          case WM_SIZE:
              window_width = LOWORD(lParam);
              window_height = HIWORD(lParam);
              if (hDC && hRC) {
                  glViewport(0, 0, window_width, window_height);
                  glMatrixMode(GL_PROJECTION);
                  glLoadIdentity();
                  SimplePerspective(45.0, (double)window_width / (double)window_height, 0.1, 100.0);
                  glMatrixMode(GL_MODELVIEW);
              }
              break;
              
          case WM_PAINT: {
              PAINTSTRUCT ps;
              BeginPaint(hWnd, &ps);
              DrawScene();
              EndPaint(hWnd, &ps);
              break;
          }
              
          case WM_KEYDOWN:
              if (wParam == VK_ESCAPE) {
                  PostQuitMessage(0);
              }
              break;
              
          case WM_DESTROY:
              if (hRC) {
                  wglMakeCurrent(NULL, NULL);
                  wglDeleteContext(hRC);
              }
              if (hDC) {
                  ReleaseDC(hWnd, hDC);
              }
              PostQuitMessage(0);
              break;
              
          default:
              return DefWindowProc(hWnd, message, wParam, lParam);
      }
      return 0;
  }
  
  // 定时器回调
  VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT_PTR idTimer, DWORD dwTime) {
      rotation_angle += 1.0f;
      if (rotation_angle > 360.0f) rotation_angle -= 360.0f;
      InvalidateRect(hWnd, NULL, FALSE);
  }
  
  int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
      WNDCLASS wc = {0};
      MSG msg;
      
      wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      wc.lpfnWndProc = WndProc;
      wc.hInstance = hInstance;
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.lpszClassName = "OpenGLTest";
      
      if (!RegisterClass(&wc)) return 0;
      
      hWnd = CreateWindow("OpenGLTest", "OpenGL Graphics Test", 
                        WS_OVERLAPPEDWINDOW, 100, 100, window_width, window_height, 
                        NULL, NULL, hInstance, NULL);
      
      if (!hWnd) return 0;
      
      ShowWindow(hWnd, nCmdShow);
      UpdateWindow(hWnd);
      
      SetTimer(hWnd, 1, 16, TimerProc);
      
      while (GetMessage(&msg, NULL, 0, 0)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
      
      KillTimer(hWnd, 1);
      return msg.wParam;
  }
  ```

  编译为可执行程序

  ```linux
  # 编译
  x86_64-w64-mingw32-gcc -o opengl_test.exe opengl_test.c -lopengl32 -lgdi32 -mwindows
  # 运行
  ~/wine-test opengl_test.exe 
  ```

  上述测试样例可以在wine环境正常显示，说明在box64 + wine环境下OpenGL组件是完善的，对比Qt和原生OpenGL库分别实现的程序运行结果

  ```linux
  # OpenGL原生库测试程序运行打印
  Using emulated /home/user/wine-x64/wine-10.0-amd64/lib/wine/x86_64-unix/opengl32.so
  Using emulated /home/user/wine-x64/wine-10.0-amd64/lib/wine/x86_64-unix/winex11.so
  Using native(wrapped) libXext.so.6
  Using native(wrapped) libX11.so.6
  Using native(wrapped) libxcb.so.1
  Using native(wrapped) libXau.so.6
  Using native(wrapped) libXdmcp.so.6
  Using native(wrapped) libXinerama.so.1
  Using native(wrapped) libXxf86vm.so.1
  Using native(wrapped) libXrender.so.1
  Using native(wrapped) libXrandr.so.2
  Using native(wrapped) libXcomposite.so.1
  Using native(wrapped) libXi.so.6
  Using native(wrapped) libXcursor.so.1
  Using native(wrapped) libXfixes.so.3
  
  # Qt程序运行打印
  Using native(wrapped) libXext.so.6
  Using native(wrapped) libX11.so.6
  Using native(wrapped) libxcb.so.1
  Using native(wrapped) libXau.so.6
  Using native(wrapped) libXau.so.6
  Using native(wrapped) libXdmcp.so.6
  Using native(wrapped) libXinerama.so.1
  Using native(wrapped) libXxf86vm.so.1
  Using native(wrapped) libXrender.so.1
  Using native(wrapped) libXrandr.so.2
  Using native(wrapped) libXcomposite.so.1
  Using native(wrapped) libXi.so.6
  Using native(wrapped) libXcursor.so.1
  Using native(wrapped) libXfixes.so.3
  Using emulated /home/user/wine-x64/wine-10.0-amd64/lib/wine/x86_64-unix/opengl32.so
  Using native(wrapped) libvulkan.so.1
  Using native(wrapped) libGL.so.1
  error: invalid value for MESA_GL_VERSION_OVERRIDE: 
  X Error of failed request:  GLXBadFBConfig
    Major opcode of failed request:  152 (GLX)
    Minor opcode of failed request:  0 ()
    Serial number of failed request:  379
    Current serial number in output stream:  379
  ```

  分析原因：
  ARM硬件OpenGL驱动与wine的GLX请求不兼容，导致硬件加速路径失败。
  ```mermaid
  graph TD
    A[应用程序] --> B[wine + wined3d];
    B --> C[请求硬件OpenGL上下文];
    C --> D[包含高级特性: core profile, 版本3.0+];
    D --> E[ARM硬件驱动];
    E --> F{驱动检查};
    F -->|不支持这些特性| G[GLXBadFBConfig];
    F -->|驱动bug或不完整| G;
    
    style G fill:#ff6666
  ```

  解决方案：
  初次没有分析到问题所在，选择禁用wine自带的wined3d：`export WINEDLLOVERRIDES="opengl32=n,b;wined3d=n"`，实际上并没有解决问题，系统回退到CPU软件渲染（llvmpip），而且性能极低。
  ```mermaid
  graph TD
    A[应用程序] --> B[wine + wined3d=n];
    B --> C[跳过wined3d硬件加速];
    C --> D[回退到软件渲染路径];
    D --> E[Mesa软件渲染器 llvmpipe];
    E --> F[创建简单的OpenGL 2.0上下文];
    F --> G[软件渲染器完全支持];
    G --> H[成功运行<br/>但性能很低];
    
    style H fill:#90ee90
  ```

  这时注意到Wine在10.17版本更新了一个主要功能：
  ```text
  EGL renderer used by default for OpenGL.
  ```
  升级Wine版本后，问题解决，但是由于飞腾X100显卡对于OpenGL版本的支持较低问题导致的图形无法旋转问题，需要后续进一步解决。

  **wine 10.17的EGL后端通过使用更现代、更标准的API，以及为ARM平台特别优化的代码路径，成功实现了硬件加速，这才是真正的解决方案。**
  ```text
  EGL vs GLX 在ARM上的区别：

  GLX问题：
  1. 绑定X11，历史包袱重
  2. 扩展检测复杂
  3. ARM驱动实现不完整
  4. wine的GLX代码老旧

  EGL优势：
  1. 专为嵌入式/移动设计（ARM原生）
  2. API更简单、更标准化
  3. 更好的错误处理
  4. wine的EGL后端是新编写的
  ```

- 最后，补充一下OpenGL的完整渲染流程以更好地理解上述问题

    - 应用程序：调用 glClear(), glDrawElements() 等标准OpenGL函数
    - OpenGL 客户端库 (如 libGL.so)：提供函数接口 │ 参数验证和记录
    - 窗口系统绑定层：GLX (Linux/X11) 或 WGL (Windows) 或 EGL (移动/嵌入式)
        
        主要职责：
        1. 管理OpenGL上下文 (状态容器)
        2. 连接OpenGL与操作系统窗口
        3. 处理缓冲区交换 (双缓冲)
        4. 扩展功能查询 
    - 用户态显卡驱动：NVIDIA驱动 / AMD驱动 / Intel驱动 / Mesa (开源)
        
        核心工作：
        1. 将OpenGL命令翻译为GPU专用指令
        2. 编译和优化着色器程序 
        3. 管理纹理、缓冲区等资源
        4. 错误检查和性能优化 
    - 系统调用接口：ioctl() (Linux) 或 DXGK调用 (Windows)，从用户空间切换到内核空间
    - 内核态显卡驱动
        关键职责：
        1. 直接与GPU硬件通信
        2. 内存管理 (显存/系统内存)
        3. 命令队列调度
        4. 中断处理
        5. 多进程GPU访问安全
    - GPU硬件：并行执行：顶点处理 → 光栅化 → 片段处理 → 输出合并，结果写入帧缓冲区
    - 显示/合成系统

        Linux: X Server 或 Wayland合成器

        Windows: DWM桌面窗口管理器

        职责：获取各个窗口的帧缓冲区，合成最终屏幕图像
    - 屏幕显示，像素点亮！
    ```mermaid
    graph TD
    subgraph "应用程序层"
        A[调用glDrawElements等]
    end
    
    subgraph "OpenGL运行时"
        B[OpenGL库]
        C{窗口系统绑定}
        C -->|Linux/X11| D[GLX]
        C -->|Windows| E[WGL]
    end
    
    subgraph "显卡驱动层"
        F[用户态驱动<br>翻译OpenGL命令]
        G[内核态驱动<br>调度GPU]
    end
    
    subgraph "硬件层"
        H[GPU执行]
        I[帧缓冲区]
    end
    
    subgraph "显示层"
        J[显示服务器合成]
        K[屏幕显示]
    end
    
    A --> B --> C
    D --> F
    E --> F
    F --> G --> H --> I --> J --> K
    ```

## 2.3 Arm64硬件驱动安装

### 2.3.1 CodeMeter安装

- 官网下载Arm64架构支持的软件

  > 官网：www.wibu.com.cn
  > 安装包：codemeter-lite_8.40.7120.501_arm64.deb

- 安装CodeMeter

  ``` linux
  # 基本安装
  sudo dpkg -i codemeter-lite_8.40.7120.501_arm64.deb
  
  # 如果依赖有问题，修复依赖
  sudo apt install -f
  ```

- 创建快捷方式

  ```
  sudo tee /usr/share/applications/codemeter-web.desktop << 'EOF'
  [Desktop Entry]
  Version=1.0
  Type=Application
  Name=CodeMeter Web管理
  Comment=CodeMeter Web管理界面
  Exec=xdg-open http://localhost:22350
  Icon=web-browser
  Terminal=false
  StartupNotify=true
  Categories=System;
  EOF
  ```

- 使用CodeMeter
  CodeMter无需进行库的包装即可正常使用，运行原理：

  - CodeMeter服务以 root权限 运行
  - 服务可以访问所有USB设备
  - Wine应用程序通过 本地网络通信 (localhost:22350) 与CodeMeter服务交互

  Wine应用程序的连接： 

  ```text
  Wine中的WIBUCM64.dll → 网络请求 → localhost:22350 → CodeMeter服务 → 物理加密狗
      ↑                      ↑              ↑                ↑
  (x86_64, 需Box64转换)   (本地网络)    (ARM64原生服务)   (直接硬件访问)
  ```

  实际数据流

  ```text
  Wine应用程序 (x86_64)
          ↓ 网络请求 (localhost:22350)
  CodeMeter服务 (ARM64) 
          ↓ 系统调用 (ioctl, usb_control_msg)
  Linux内核 → USB子系统 → 物理加密狗
  ```

### 2.3.2 大恒相机驱动安装

- Linux ARM64版本驱动下载

  > 下载地址：https://www.daheng-imaging.com/index.php?m=content&c=index&a=lists&catid=59&czxt=30&sylx=&syxj=44#mmd

- 解压安装

  ```linux
  chmod +x Galaxy_camera.run
  sudo ./Galaxy_camera.run
  ```

- 新增快捷方式

  ```linux
  # IP配置工具快捷方式
  sudo tee /usr/share/applications/galaxy-ipconfig.desktop << 'EOF'
  > [Desktop Entry]
  > Version=1.0
  > Type=Application
  > Name=Galaxy IP Config
  > Name[zh_CN]=大恒IP配置工具
  > Comment=Galaxy Camera IP Configuration Tool
  > Comment[zh_CN]=大恒相机IP配置工具
  > Exec=/home/user/Galaxy_Camera/Galaxy_camera/bin/GxGigeIPConfig
  > Icon=network-wired
  > Categories=Graphics;Network;
  > Terminal=false
  > StartupNotify=true
  > EOF
  
  # 大恒相机快捷方式
  sudo tee /usr/share/applications/galaxyview.desktop << 'EOF'
  > [Desktop Entry]
  > Version=1.0
  > Type=Application
  > Name=Galaxy Camera Viewer
  > Name[zh_CN]=大恒相机查看器
  > Comment=Galaxy Camera Viewing Application (Qt5 Version)
  > Comment[zh_CN]=大恒相机查看应用程序 (Qt5版本)
  > Exec=/home/user/Galaxy_Camera/Galaxy_camera/bin/GxViewer_qt5
  > Icon=camera-web
  > Categories=Graphics;Viewer;
  > Terminal=false
  > StartupNotify=true
  > EOF
  ```

### 2.3.3 大华相机驱动安装

- Linux ARM64版本驱动下载

  > 下载地址：https://www.irayple.com/cn/serviceSupport/downloadCenter/18?p=17

- 解压安装

  ```linux
  chmod +x Galaxy_camera.run
  sudo ./Galaxy_camera.run
  ```

- 创建快捷方式

  ```
  sudo tee /usr/share/applications/mvviewer.desktop << 'EOF'
  [Desktop Entry]
  Version=1.0
  Type=Application
  Name=MVviewer
  Comment=MindVision相机查看器
  Exec=/bin/bash -c "cd /opt/HuarayTech/MVviewer/bin && ./run.sh"
  Icon=camera-web
  Terminal=false
  StartupNotify=true
  Categories=Graphics;
  EOF
  sudo chmod 644 /usr/share/applications/mvviewer.desktop
  ```

## 2.4 DLL包装器实现

1. 创建开发目录

  ```
mkdir -p ~/dll_wrappers
cd ~/dll_wrappers
  ```

2. 分析现有DLL函数

  ```
# 从现有DLL提取函数名（如果有的话）
cd "/home/user/.wine-xtom/drive_c/Program Files/XTOM"
for dll in MVSDKmd.dll GxIAPI.dll CAMCTRL_64.dll; do
    if [ -f "$dll" ]; then
        echo "=== $dll 导出函数 ==="
        strings "$dll" | grep -E "^(MV_|GX_|CAM_)" | head -10
    fi
done
  ```