# **C++**

## **1. C++内存分配**

在 C++ 中，内存分配机制涉及 **栈（Stack）**、**堆（Heap）** 和 **静态存储区（Static Storage）**，不同的分配方式适用于不同的场景。C++ 提供了多种内存管理方式，包括 **手动管理（**`new`**/**`delete`**）** 和 **智能指针（Smart Pointers）**，以及底层的内存操作（如 `malloc`/`free`）。  

------

### **1.1 C++ 内存分配的基本方式**

**(1) 栈（Stack）**

- **分配方式**：由编译器自动管理，函数调用时分配，函数返回时释放。  
- **特点**：
  - 速度快，但大小有限（通常几 MB，取决于系统）。 
  - 存储局部变量、函数参数、返回地址等。 
  - 超出栈大小会导致 **栈溢出（Stack Overflow）**。  
- **示例**：

```C++
void foo() {
    int x = 10; // x 在栈上分配
} // 函数结束，x 自动释放
```

**(2) 堆（Heap / Dynamic Memory）**

- **分配方式**：由程序员手动管理（或智能指针自动管理），使用 `new`/`delete` 或 `malloc`/`free`。  
- **特点**：
  - 分配速度较慢，但空间大（受系统 RAM 限制）。 
  - 需要手动释放，否则会导致 **内存泄漏（Memory Leak）**。  
  - 适用于动态数据结构（如链表、树、动态数组）。 
- **示例**：

```C++
int* p = new int(42); // 在堆上分配一个 int
delete p;             // 必须手动释放
```

**(3) 静态存储区（Static Storage）**

- **分配方式**：全局变量、静态变量（`static`）、常量（`const`）存储在静态区。  
- **特点**：
  - 程序启动时分配，程序结束时释放。 
  - 生命周期最长，但占用固定内存。 
- **示例**：

```C++
int globalVar = 100;       // 全局变量（静态存储区）
void foo() {
    static int count = 0;  // 静态局部变量（只初始化一次）
    count++;
}
```

**(4) 常量存储区（Read-Only Memory, ROM）**

- **存储内容**：字符串常量、`constexpr` 变量等。  
- **特点**：
  - 程序运行期间不可修改，否则会导致 **段错误（Segmentation Fault）**。  
- **示例**：

```C++
const char* str = "Hello"; // "Hello" 存储在常量区
```

------

### **1.2 C++ 动态内存管理（**`new`**/**`delete` **vs** `malloc`**/**`free`**）**

| **特性**              | `new` **/** `delete` **(C++)** | `malloc` **/** `free` **(C)** |
| --------------------- | ------------------------------ | ----------------------------- |
| **是否调用构造/析构** | ✅ 调用构造函数/析构函数        | ❌ 仅分配内存                  |
| **返回类型**          | 返回具体类型指针（如 `int*`）  | 返回 `void*`（需强制转换）    |
| **内存不足行为**      | 抛出 `std::bad_alloc` 异常     | 返回 `NULL`                   |
| **内存对齐**          | 支持（C++11 `alignas`）        | 需手动调整                    |
| **适用场景**          | C++ 对象（类、结构体）         | 纯内存分配（无构造）          |

**示例对比**：

```C++
// C++ new/delete
int* p1 = new int(42);  // 分配并初始化
delete p1;              // 释放
// C malloc/free
int* p2 = (int*)malloc(sizeof(int));
*p2 = 42;
free(p2);
```

------

### **1.3 智能指针（自动内存管理）**

C++11 引入智能指针，避免手动 `delete`，防止内存泄漏：

- `std::unique_ptr`（独占所有权，不可复制）：

```C++
std::unique_ptr<int> p1(new int(42));
```

- `std::shared_ptr`（共享所有权，引用计数）：

```C++
std::shared_ptr<int> p2 = std::make_shared<int>(42);
```

- `std::weak_ptr`（避免循环引用）：

```C++
std::weak_ptr<int> p3 = p2;
```

------

### **1.4 常见内存问题**

| **问题**     | **原因**               | **解决方案**          |
| ------------ | ---------------------- | --------------------- |
| **内存泄漏** | `new` 后未 `delete`    | 使用智能指针          |
| **野指针**   | 访问已释放的内存       | 释放后置 `nullptr`    |
| **双重释放** | 多次 `delete` 同一指针 | 使用 RAII（智能指针） |
| **内存碎片** | 频繁分配/释放小块内存  | 使用内存池            |

------

### **1.5 总结**

| **内存区域** | **分配方式**   | **生命周期** | **适用场景**       |
| ------------ | -------------- | ------------ | ------------------ |
| **栈**       | 自动分配       | 函数作用域   | 局部变量、临时对象 |
| **堆**       | `new`/`malloc` | 手动控制     | 动态数据结构       |
| **静态区**   | 全局/静态变量  | 程序运行期   | 全局配置           |
| **常量区**   | 字符串常量     | 程序运行期   | 只读数据           |

**最佳实践**：

- **优先使用栈和智能指针**，减少手动 `new`/`delete`。  
- **避免裸指针**，防止内存泄漏和悬垂指针。  
- **大内存分配考虑内存池**，减少碎片。  

C++ 内存管理灵活但复杂，正确使用 RAII 和智能指针可以大幅降低风险。

## **2. memcpy 函数解析**

### **2.1 拷贝原理**

`memcpy(char* dst, const char* src, size_t length)` 的功能是将从源地址 `src` 开始的连续 `length` 字节数据，**逐字节复制**到目标地址 `dst`。它的底层实现通常是：

- **按字节（或机器字长）复制**：为了提高效率，可能按 CPU 字长（如 4/8 字节）分段拷贝，剩余部分按字节处理。
- **无重叠检查**：假设源（`src`）和目标（`dst`）内存区域**不重叠**。如果重叠，行为未定义（需用 `memmove` 代替）。

### **2.2 内存独立性**

- **不会共享内存**：`memcpy` 执行后，`dst` 和 `src` 指向**不同的内存块**，内容相同但物理地址独立。

```C++
char src[] = "Hello";
char dst[6];
memcpy(dst, src, 6); // 复制后，dst 和 src 内容相同，但地址不同
```

- 修改 `dst` 不会影响 `src`，反之亦然。

### **2.3 特殊情况**

- **浅拷贝问题**：若 `src` 包含指针（如结构体内有动态分配的内存），`memcpy` 仅复制指针值（地址），而**不复制指向的数据**。此时 `dst` 和 `src` 会共享同一块深层内存（需深拷贝解决）。

```C++
struct Example {
    char* buffer;
} src, dst;
src.buffer = malloc(10);
memcpy(&dst, &src, sizeof(src)); // dst.buffer 和 src.buffer 指向同一内存
```

### **2.4 与** `memmove` **的区别**

- `memmove` 会检查内存重叠，确保复制正确性：

```C++
char str[] = "ABCDEF";
memmove(str + 2, str, 4); // 安全处理重叠
memcpy(str + 2, str, 4); // 可能未定义行为
```

### **2.5 总结**

- `memcpy` 是**独立内存的逐字节复制**，`dst` 和 `src` 不共享内存（除非手动指向同一地址或浅拷贝指针）。
- 若需处理重叠内存，应使用 `memmove`。

## **3. C++ 串口通信详解**

`sio_flush` 确实是某些串口通信库(如 Tera Term 的 TTKernel32 库)中的函数，用于清空串口缓冲区。在 C++ 中，串口通信可以通过多种方式实现，下面我将详细介绍串口通信的各种操作和流程。

### **3.1 串口通信常用方法**

#### **3.1.1 操作系统原生 API**

- **Windows**: 使用 Win32 API (`CreateFile`, `ReadFile`, `WriteFile` 等)
- **Linux/Unix**: 使用 termios 库 (`open`, `read`, `write`, `tcsetattr` 等)

#### **3.1.2 跨平台库**

- **Boost.Asio**: 提供跨平台的串口通信支持
- **Qt QSerialPort**: Qt 框架提供的串口类
- **libserial**: 专门用于串口通信的 C++ 库
- **Poco Serial**: Poco 框架的串口组件

#### **3.1.3 专用串口库**

- **Tera Term 的 TTKernel32** (包含 `sio_*` 系列函数)
- **其他硬件厂商提供的专用库**

### **3.2 串口通信完整流程**

#### **3.2.1 打开串口**

```C++
// Windows 示例
HANDLE hSerial = CreateFile(
    "COM3",                  	  // 串口名称
    GENERIC_READ | GENERIC_WRITE, // 读写权限
    0,                      // 共享模式(独占)
    NULL,                   // 安全属性
    OPEN_EXISTING,          // 打开已存在设备
    FILE_ATTRIBUTE_NORMAL,  // 文件属性
    NULL                    // 模板文件句柄
);
if (hSerial == INVALID_HANDLE_VALUE) {    
    // 错误处理
}
```

#### **3.2.2 配置串口参数**

```C++
// Windows DCB 结构体配置
DCB dcbSerialParams = {0};
dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
if (!GetCommState(hSerial, &dcbSerialParams)) {    
    // 错误处理
}
dcbSerialParams.BaudRate = CBR_115200;  // 波特率
dcbSerialParams.ByteSize = 8;           // 数据位
dcbSerialParams.StopBits = ONESTOPBIT;  // 停止位
dcbSerialParams.Parity = NOPARITY;      // 校验位
if (!SetCommState(hSerial, &dcbSerialParams)) {    
    // 错误处理
}
// 设置超时
COMMTIMEOUTS timeouts = {0};
timeouts.ReadIntervalTimeout = 50;
timeouts.ReadTotalTimeoutConstant = 50;
timeouts.ReadTotalTimeoutMultiplier = 10;
timeouts.WriteTotalTimeoutConstant = 50;
timeouts.WriteTotalTimeoutMultiplier = 10;
SetCommTimeouts(hSerial, &timeouts);
```

#### **3.2.3 读写数据**

**写入数据**

```C++
char data[] = "Hello Serial Port";
DWORD bytesWritten;
if (!WriteFile(hSerial, data, sizeof(data), &bytesWritten, NULL)) {    
    // 错误处理
}
```

**读取数据**

```C++
char buffer[256];
DWORD bytesRead;
if (!ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL)) {
    // 错误处理
}
```

#### **3.2.4 控制信号**

```C++
// 设置DTR信号
EscapeCommFunction(hSerial, SETDTR);
// 清除RTS信号
EscapeCommFunction(hSerial, CLRRTS);
```

#### **3.2.5清空缓冲区**

```C++
// 清空输入缓冲区
PurgeComm(hSerial, PURGE_RXCLEAR);
// 清空输出缓冲区
PurgeComm(hSerial, PURGE_TXCLEAR);
```

#### **3.2.6 关闭串口**

```C++
CloseHandle(hSerial);
```

### **3.3 Boost.Asio 示例**

```C++
#include <boost/asio.hpp>
#include <iostream>
using namespace boost::asio;
int main() {    
    io_service io;    
    serial_port port(io, "COM3");        // 设置参数
    port.set_option(serial_port_base::baud_rate(115200));
    port.set_option(serial_port_base::character_size(8));
    port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    port.set_option(serial_port_base::parity(serial_port_base::parity::none));
    port.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));        
    // 写入数据    
    write(port, buffer("Hello Serial Port\n"));        
    // 读取数据    char buf[256];    
    size_t len = read(port, buffer(buf));    
    std::cout.write(buf, len);        
    return 0;
}
```

### **3.4 Qt QSerialPort 示例**

```C++
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
int main() {    
    QSerialPort serial;    
    serial.setPortName("COM3");    
    serial.setBaudRate(QSerialPort::Baud115200);    
    serial.setDataBits(QSerialPort::Data8);    
    serial.setParity(QSerialPort::NoParity);    
    serial.setStopBits(QSerialPort::OneStop);    
    serial.setFlowControl(QSerialPort::NoFlowControl);        
    if (!serial.open(QIODevice::ReadWrite)) {        
        qDebug() << "Failed to open port";        
        return -1;    
    }        
    // 写入数据    
    serial.write("Hello Serial Port\n");    
    serial.waitForBytesWritten(1000);        
    // 读取数据    
    if (serial.waitForReadyRead(1000)) {        
        QByteArray data = serial.readAll();        
        while (serial.waitForReadyRead(10))            
            data += serial.readAll();        
        qDebug() << "Received:" << data;    
    }        
    serial.close();    
    return 0;
}
```

### **3.5 常见操作详解**

#### **3.5.1 波特率设置**

波特率是串口通信中最基本的参数，表示每秒传输的符号数。常见值有：

- 9600
- 19200
- 38400
- 57600
- 115200

#### **3.5.2 数据位设置**

表示每个字符的数据位数，通常是5-8位，最常用的是8位。

#### **3.5.3 停止位设置**

表示字符结束的标志位，可以是1位、1.5位或2位。

#### **3.5.4 校验位设置**

用于简单的错误检测，可以是：

- 无校验(None)
- 奇校验(Odd)
- 偶校验(Even)
- 标记(Mark)
- 空格(Space)

#### **3.5.5 流控制**

控制数据流的方式：

- 无流控(None)
- 硬件流控(RTS/CTS)
- 软件流控(XON/XOFF)

### **3.6 串口通信最佳实践**

1. **错误处理**：所有串口操作都应检查返回值并进行适当的错误处理
2. **超时设置**：合理设置读写超时，避免程序挂起
3. **线程安全**：在多线程环境中使用串口时要注意同步
4. **资源释放**：确保在程序退出或异常时正确关闭串口
5. **日志记录**：记录重要的串口操作和通信数据，便于调试
6. **缓冲区管理**：合理设置缓冲区大小，避免数据丢失或溢出

### **3.7 常见问题解决**

1. **串口无法打开**
   - 检查串口名称是否正确
   - 确认串口未被其他程序占用
   - 检查用户权限(特别是在Linux下)
2. **数据接收不完整**
   - 检查波特率等参数是否与设备匹配
   - 增加适当的延时或使用更长的超时
   - 检查硬件连接是否可靠
3. **数据乱码**
   - 确认双方的通信参数(波特率、数据位、停止位、校验位)完全一致
   - 检查是否有电磁干扰
4. **通信速度慢**
   - 尝试提高波特率
   - 优化数据处理流程
   - 减少不必要的延时
## 4. C++容器
### 4.1 std::multimap 类型容器

std::multimap 是 C++ STL 中的一个关联容器，它允许存储键值对，并且可以包含多个具有相同键的元素。

#### 4.1.1 基本特性

- 按键自动排序（默认升序）
- 允许重复键
- 基于红黑树实现，查找效率为 O(log n)
- 元素类型为 std::pair<const Key, T>

#### 4.1.2 创建和初始化

```cpp
#include <map>
#include <string>

// 创建一个空的 multimap
std::multimap<int, std::string> mmap1;

// 使用初始化列表创建
std::multimap<int, std::string> mmap2 = {
    {1, "apple"},
    {2, "banana"},
    {1, "apricot"},  // 允许重复键
    {3, "cherry"}
};
```

#### 4.1.3 插入元素

```cpp
// 使用 insert 方法
mmap1.insert(std::make_pair(1, "apple"));
mmap1.insert({2, "banana"});
mmap1.emplace(1, "apricot");  // 使用 emplace 直接构造

// 插入多个元素
mmap1.insert({{3, "cherry"}, {2, "blueberry"}, {1, "avocado"}});
```

#### 4.1.4 访问元素

```cpp
// 遍历所有元素
for (const auto& pair : mmap1) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}

// 查找特定键的所有元素
auto range = mmap1.equal_range(1);
for (auto it = range.first; it != range.second; ++it) {
    std::cout << "Found: " << it->second << std::endl;
}

// 检查键是否存在
if (mmap1.find(2) != mmap1.end()) {
    std::cout << "Key 2 exists" << std::endl;
}
```

#### 4.1.5 删除元素

```cpp
// 删除特定键的所有元素
size_t num_erased = mmap1.erase(1);

// 删除单个元素
auto it = mmap1.find(2);
if (it != mmap1.end()) {
    mmap1.erase(it);
}

// 删除一定范围内的元素
auto first = mmap1.lower_bound(2);
auto last = mmap1.upper_bound(3);
mmap1.erase(first, last);
```

#### 4.1.6 其他常用操作

```cpp
// 获取容器大小
std::cout << "Size: " << mmap1.size() << std::endl;

// 检查是否为空
if (mmap1.empty()) {
    std::cout << "Multimap is empty" << std::endl;
}

// 获取键比较函数
auto cmp = mmap1.key_comp();

// 清空容器
mmap1.clear();
```

#### 4.1.7 自定义比较函数

```cpp
// 自定义比较函数
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char c1, char c2) { return tolower(c1) < tolower(c2); }
        );
    }
};

// 使用自定义比较函数的 multimap
std::multimap<std::string, int, CaseInsensitiveCompare> case_insensitive_map;
```

#### 4.1.8 性能考虑

- 插入和删除操作：O(log n)
- 查找操作：O(log n)
- 遍历操作：O(n)
- 如果需要更快的查找但可以接受稍慢的插入，考虑使用 std::unordered_multimap

#### 4.1.9 实际应用示例

```cpp
// 电话簿示例 - 一个人可能有多个电话号码
std::multimap<std::string, std::string> phonebook;

phonebook.insert({"John", "123-4567"});
phonebook.insert({"John", "765-4321"});
phonebook.insert({"Alice", "555-1234"});

// 查找 John 的所有电话号码
auto john_numbers = phonebook.equal_range("John");
for (auto it = john_numbers.first; it != john_numbers.second; ++it) {
    std::cout << it->second << std::endl;
}
```

std::multimap 在需要维护键值对并且允许键重复的场景下非常有用，如索引、反向映射等。