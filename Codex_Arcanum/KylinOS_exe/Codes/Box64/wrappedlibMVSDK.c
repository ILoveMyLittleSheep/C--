#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"
#include "emu/x64emu_private.h"
#include "callback.h"
#include "librarian.h"
#include "box64context.h"
#include "emu/x64emu_private.h"
#include "myalign.h"

const char* libMVSDKName = "libMVSDK.so";
// 这里的定义会影响wrappercallback.h头文件中结构体的名字
#define LIBNAME libMVSDK

// 定义ARM库的实际路径
// 在wrappedlib_init.h头文件中如果第一次动态链接库未加载成功，
// 会根据真实路径再次尝试加载
#ifndef ALTNAME
#define ALTNAME "/opt/HuarayTech/MVviewer/lib/libMVSDK.so"
#endif

// 这个头文件里会定义
// #define SUPER() ADDED_FUNCTIONS() \
//     GO(IMV_GetVersion, pFv_t)
// 进而在wrappercallback.h头文件定义的libMVSDK_my_s结构体为
// struct libMVSDK_my_s
// {
//     pFv_t IMV_GetVersion;
//     iFpip_t IMV_CreateHandle;
//     ... 
// } libMVSDK_my_t;
// 包含相机库所有函数的指针
#include "generated/wrappedlibMVSDKtypes.h"

// 在wrappercallback.h头文件中定义结构体 libMVSDK_my_s，同时定义
// 库句柄指针，用于后续的 dlsym 调用
// static library_t* my_lib = NULL;
// 创建结构体实例并初始化为0
// static libMVSDK_my_t my_libMVSDK = {0};
// 创建常量指针指向该实例,代码中通过 my->函数名 访问函数指针
// static libMVSDK_my_t * const my = &my_libMVSDK;
#include "wrappercallback.h"

// #define DEBUG_ENABLED 1
#ifdef DEBUG_ENABLED
    #define DEBUG_LOG(fmt, ...) printf("[libMVSDK wrapper] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_LOG(fmt, ...)
#endif

// ================定义图像帧的结构体================
typedef	void*	IMV_FRAME_HANDLE;				///< \~chinese 帧句柄

#define IMV_GVSP_PIX_MONO                           0x01000000
#define IMV_GVSP_PIX_RGB                            0x02000000
#define IMV_GVSP_PIX_COLOR                          0x02000000
#define IMV_GVSP_PIX_CUSTOM                         0x80000000
#define IMV_GVSP_PIX_COLOR_MASK                     0xFF000000

// Indicate effective number of bits occupied by the pixel (including padding).
// This can be used to compute amount of memory required to store an image.
#define IMV_GVSP_PIX_OCCUPY1BIT                     0x00010000
#define IMV_GVSP_PIX_OCCUPY2BIT                     0x00020000
#define IMV_GVSP_PIX_OCCUPY4BIT                     0x00040000
#define IMV_GVSP_PIX_OCCUPY8BIT                     0x00080000
#define IMV_GVSP_PIX_OCCUPY12BIT                    0x000C0000
#define IMV_GVSP_PIX_OCCUPY16BIT                    0x00100000
#define IMV_GVSP_PIX_OCCUPY24BIT                    0x00180000
#define IMV_GVSP_PIX_OCCUPY32BIT                    0x00200000
#define IMV_GVSP_PIX_OCCUPY36BIT                    0x00240000
#define IMV_GVSP_PIX_OCCUPY48BIT                    0x00300000

typedef enum _IMV_EPixelType
{
	// Undefined pixel type
	gvspPixelTypeUndefined = -1,

	// Mono Format
	gvspPixelMono1p = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY1BIT | 0x0037),
	gvspPixelMono2p = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY2BIT | 0x0038),
	gvspPixelMono4p = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY4BIT | 0x0039),
	gvspPixelMono8 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x0001),
	gvspPixelMono8S = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x0002),
	gvspPixelMono10 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0003),
	gvspPixelMono10Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0004),
	gvspPixelMono12 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0005),
	gvspPixelMono12Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0006),
	gvspPixelMono14 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0025),
	gvspPixelMono16 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0007),

	// Bayer Format
	gvspPixelBayGR8 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x0008),
	gvspPixelBayRG8 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x0009),
	gvspPixelBayGB8 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x000A),
	gvspPixelBayBG8 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY8BIT | 0x000B),
	gvspPixelBayGR10 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x000C),
	gvspPixelBayRG10 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x000D),
	gvspPixelBayGB10 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x000E),
	gvspPixelBayBG10 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x000F),
	gvspPixelBayGR12 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0010),
	gvspPixelBayRG12 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0011),
	gvspPixelBayGB12 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0012),
	gvspPixelBayBG12 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0013),
	gvspPixelBayGR10Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0026),
	gvspPixelBayRG10Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0027),
	gvspPixelBayGB10Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0028),
	gvspPixelBayBG10Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x0029),
	gvspPixelBayGR12Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x002A),
	gvspPixelBayRG12Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x002B),
	gvspPixelBayGB12Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x002C),
	gvspPixelBayBG12Packed = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY12BIT | 0x002D),
	gvspPixelBayGR16 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x002E),
	gvspPixelBayRG16 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x002F),
	gvspPixelBayGB16 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0030),
	gvspPixelBayBG16 = (IMV_GVSP_PIX_MONO | IMV_GVSP_PIX_OCCUPY16BIT | 0x0031),

	// RGB Format
	gvspPixelRGB8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x0014),
	gvspPixelBGR8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x0015),
	gvspPixelRGBA8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY32BIT | 0x0016),
	gvspPixelBGRA8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY32BIT | 0x0017),
	gvspPixelRGB10 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0018),
	gvspPixelBGR10 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0019),
	gvspPixelRGB12 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x001A),
	gvspPixelBGR12 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x001B),
	gvspPixelRGB16 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0033),
	gvspPixelRGB10V1Packed = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY32BIT | 0x001C),
	gvspPixelRGB10P32 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY32BIT | 0x001D),
	gvspPixelRGB12V1Packed = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY36BIT | 0X0034),
	gvspPixelRGB565P = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0035),
	gvspPixelBGR565P = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0X0036),

	// YVR Format
	gvspPixelYUV411_8_UYYVYY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY12BIT | 0x001E),
	gvspPixelYUV422_8_UYVY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x001F),
	gvspPixelYUV422_8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0032),
	gvspPixelYUV8_UYV = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x0020),
	gvspPixelYCbCr8CbYCr = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x003A),
	gvspPixelYCbCr422_8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x003B),
	gvspPixelYCbCr422_8_CbYCrY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0043),
	gvspPixelYCbCr411_8_CbYYCrYY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY12BIT | 0x003C),
	gvspPixelYCbCr601_8_CbYCr = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x003D),
	gvspPixelYCbCr601_422_8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x003E),
	gvspPixelYCbCr601_422_8_CbYCrY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0044),
	gvspPixelYCbCr601_411_8_CbYYCrYY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY12BIT | 0x003F),
	gvspPixelYCbCr709_8_CbYCr = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x0040),
	gvspPixelYCbCr709_422_8 = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0041),
	gvspPixelYCbCr709_422_8_CbYCrY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY16BIT | 0x0045),
	gvspPixelYCbCr709_411_8_CbYYCrYY = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY12BIT | 0x0042),

	// RGB Planar
	gvspPixelRGB8Planar = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY24BIT | 0x0021),
	gvspPixelRGB10Planar = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0022),
	gvspPixelRGB12Planar = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0023),
	gvspPixelRGB16Planar = (IMV_GVSP_PIX_COLOR | IMV_GVSP_PIX_OCCUPY48BIT | 0x0024),

	//BayerRG10p和BayerRG12p格式，针对特定项目临时添加,请不要使用
	//BayerRG10p and BayerRG12p, currently used for specific project, please do not use them
	gvspPixelBayRG10p = 0x010A0058,
	gvspPixelBayRG12p = 0x010c0059,

	//mono1c格式，自定义格式
	//mono1c, customized image format, used for binary output
	gvspPixelMono1c = 0x012000FF,

	//mono1e格式，自定义格式，用来显示连通域
	//mono1e, customized image format, used for displaying connected domain
	gvspPixelMono1e = 0x01080FFF
}IMV_EPixelType;

typedef struct _IMV_FrameInfo
{
	uint64_t				blockId;				///< \~chinese 帧Id(仅对GigE/Usb/PCIe相机有效)					\~english The block ID(GigE/Usb/PCIe camera only)
	unsigned int			status;					///< \~chinese 数据帧状态(0是正常状态)							\~english The status of frame(0 is normal status)
	unsigned int			width;					///< \~chinese 图像宽度											\~english The width of image
	unsigned int			height;					///< \~chinese 图像高度											\~english The height of image
	unsigned int			size;					///< \~chinese 图像大小											\~english The size of image
	IMV_EPixelType			pixelFormat;			///< \~chinese 图像像素格式										\~english The pixel format of image
	uint64_t				timeStamp;				///< \~chinese 图像时间戳(仅对GigE/Usb/PCIe相机有效)			\~english The timestamp of image(GigE/Usb/PCIe camera only)
	unsigned int			chunkCount;				///< \~chinese 帧数据中包含的Chunk个数(仅对GigE/Usb相机有效)	\~english The number of chunk in frame data(GigE/Usb Camera Only)
	unsigned int			paddingX;				///< \~chinese 图像paddingX(仅对GigE/Usb/PCIe相机有效)			\~english The paddingX of image(GigE/Usb/PCIe camera only)
	unsigned int			paddingY;				///< \~chinese 图像paddingY(仅对GigE/Usb/PCIe相机有效)			\~english The paddingY of image(GigE/Usb/PCIe camera only)
	unsigned int			recvFrameTime;			///< \~chinese 图像在网络传输所用的时间(单位:微秒,非GigE相机该值为0)	\~english The time taken for the image to be transmitted over the network(unit:us, The value is 0 for non-GigE camera)
	unsigned int			nReserved[19];			///< \~chinese 预留字段											\~english Reserved field
}IMV_FrameInfo;

typedef struct _IMV_Frame
{
	IMV_FRAME_HANDLE		frameHandle;			///< \~chinese 帧图像句柄(SDK内部帧管理用)						\~english Frame image handle(used for managing frame within the SDK)
	unsigned char*			pData;					///< \~chinese 帧图像数据的内存首地址							\~english The starting address of memory of image data
	IMV_FrameInfo			frameInfo;				///< \~chinese 帧信息											\~english Frame information
	unsigned int			nReserved[10];			///< \~chinese 预留字段											\~english Reserved field
}IMV_Frame;


#include <stdio.h>

// 保存为BMP图片（测试用）
void SaveAsBMP(unsigned char* pData, int width, int height, const char* filename) {
    // 1. 创建文件
    FILE* file = fopen(filename, "wb");
    
    // 2. 写入BMP文件头（54字节）和数据
    unsigned char header[54] = {
        0x42, 0x4D,           // "BM"
        0,0,0,0,              // 文件大小（稍后计算）
        0,0,0,0,              // 保留
        54,0,0,0,             // 数据偏移
        40,0,0,0,             // 信息头大小
        width & 0xFF, (width >> 8) & 0xFF, (width >> 16) & 0xFF, (width >> 24) & 0xFF, // 宽度
        height & 0xFF, (height >> 8) & 0xFF, (height >> 16) & 0xFF, (height >> 24) & 0xFF, // 高度
        1,0,                  // 平面数
        24,0,                 // 每像素位数
        0,0,0,0,              // 压缩方式
        0,0,0,0,              // 图像大小
        0,0,0,0,              // 水平分辨率
        0,0,0,0,              // 垂直分辨率
        0,0,0,0,              // 颜色数
        0,0,0,0               // 重要颜色数
    };
    
    int rowSize = ((width * 3 + 3) / 4) * 4;  // BMP要求每行4字节对齐
    int fileSize = 54 + rowSize * height;
    
    // 更新文件大小
    header[2] = fileSize & 0xFF;
    header[3] = (fileSize >> 8) & 0xFF;
    header[4] = (fileSize >> 16) & 0xFF;
    header[5] = (fileSize >> 24) & 0xFF;
    
    // 写入文件头
    fwrite(header, 1, 54, file);
    
    // 3. 写入图像数据（需要将单通道转为RGB）
    unsigned char* rgbData = (unsigned char*)malloc(rowSize);
    
    for (int y = height - 1; y >= 0; y--) {  // BMP是倒着存的
        for (int x = 0; x < width; x++) {
            unsigned char gray = pData[y * width + x];
            rgbData[x * 3 + 0] = gray;  // B
            rgbData[x * 3 + 1] = gray;  // G  
            rgbData[x * 3 + 2] = gray;  // R
        }
        // 填充对齐字节
        for (int x = width * 3; x < rowSize; x++) {
            rgbData[x] = 0;
        }
        fwrite(rgbData, 1, rowSize, file);
    }
    
    free(rgbData);
    fclose(file);
}

// ===============主函数实现===============
#define MAX_CAMERAS 16  // 最多16个相机
typedef void(*IMV_FrameCallBack)(IMV_Frame* pFrame, void* pUser);

typedef struct {
    uintptr_t callback;     // 回调函数地址
    void* user;             // 用户自定义数据
    void* handle;           // 相机handle
    int used;               // 标记是否使用
} CameraCallback;

static CameraCallback g_callbacks[MAX_CAMERAS] = {0};

// 根据句柄查找索引
static int find_index_by_handle(void* handle)
{
    for (int i = 0; i < MAX_CAMERAS; i++) 
    {
        if (g_callbacks[i].used && g_callbacks[i].handle == handle) 
        {
            return i;
        }
    }
    return -1;
}

// 回调包装函数
static void frame_callback_wrapper(IMV_Frame* arm_frame, void* user_data)
{
    DEBUG_LOG("arm 收到图像");
    int index = (int)(intptr_t)user_data;  // user_data 是自定义的索引号
    
    // 查找
    if (index < 0 || index >= MAX_CAMERAS || !g_callbacks[index].used) {
        DEBUG_LOG("Invalid callback index: %d\n", index);
        return;
    }
    // SaveAsBMP(arm_frame->pData, arm_frame->frameInfo.width, arm_frame->frameInfo.height, "ARM.bmp");
    CameraCallback* cb = &g_callbacks[index];
    if (!cb->callback || !arm_frame) return;
    
    // SaveAsBMP(arm_frame->pData, arm_frame->frameInfo.width, arm_frame->frameInfo.height, "ARM.bmp");

    RunFunctionFmt(cb->callback, "pp", arm_frame, cb->user);
    
    // 注意：这里不释放 data，因为它会在 IMV_AttachGrabbing 中管理
}

EXPORT int my_IMV_AttachGrabbing(x64emu_t* emu, void* handle, void* proc, void* pUser)
{
    DEBUG_LOG("IMV_AttachGrabbing called, callback: %p\n", proc);
    
    // 查找空闲位置
    int index = -1;
    for (int i = 0; i < MAX_CAMERAS; i++) 
    {
        if (!g_callbacks[i].used) 
        {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        DEBUG_LOG("No free callback slot\n");
        return -1;
    }
    
    // 设置回调信息
    g_callbacks[index].callback = (uintptr_t)proc;
    g_callbacks[index].handle = handle;
    g_callbacks[index].user = pUser;
    g_callbacks[index].used = 1;
    
    // 传递索引号作为 user_data
    int result = my->IMV_AttachGrabbing(handle, frame_callback_wrapper, (void*)(intptr_t)index);
    
    if (result != 0) {
        // 失败时清理
        g_callbacks[index].used = 0;
    }
    
    DEBUG_LOG("AttachGrabbing: handle=%p, index=%d, result=%d\n", 
              handle, index, result);
    return result;
}

EXPORT int my_IMV_StopGrabbing(x64emu_t* emu, void* handle)
{
    DEBUG_LOG("IMV_StopGrabbing called\n");
    int result = my->IMV_StopGrabbing(handle);
    // 清理回调数据
    int index = find_index_by_handle(handle);
    if (index != -1) 
    {
        DEBUG_LOG("Cleaning callback at index %d\n", index);
        g_callbacks[index].used = 0;
        g_callbacks[index].callback = 0;
        g_callbacks[index].user = NULL;
        g_callbacks[index].handle = NULL;
    }
    DEBUG_LOG("IMV_StopGrabbing returned: %d\n", result);
    return result;
}

EXPORT void* my_IMV_GetVersion(x64emu_t* emu) 
{
    DEBUG_LOG("IMV_GetVersion called\n");
    void* result = my->IMV_GetVersion();
    DEBUG_LOG("Version string: %s\n", (const char*)result);
    return result;
}

EXPORT int my_IMV_EnumDevices(x64emu_t* emu, void* pDeviceList, unsigned int interfaceType)
{
    DEBUG_LOG("IMV_EnumDevices called\n");
    
    // 简单打印所有信息
    DEBUG_LOG("emu: %p\n", emu);
    DEBUG_LOG("pDeviceList: %p\n", pDeviceList);
    DEBUG_LOG("interfaceType: %u (0x%x)\n", interfaceType, interfaceType);
    
    // 调用原始函数
    int result = my->IMV_EnumDevices(pDeviceList, interfaceType);
    DEBUG_LOG("returned: %d\n", result);
    
    return result;
}

EXPORT int my_IMV_CreateHandle(x64emu_t* emu, void** handle, int mode, void* pIdentifier) 
{
    DEBUG_LOG("IMV_CreateHandle called\n");
    int result = my->IMV_CreateHandle(handle, mode, pIdentifier);
    DEBUG_LOG("IMV_CreateHandle returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_DestroyHandle(x64emu_t* emu, void* handle)
{
    DEBUG_LOG("IMV_DestroyHandle called\n");
    int result = my->IMV_DestroyHandle(handle);
    DEBUG_LOG("IMV_DestroyHandle returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_OpenEx(x64emu_t* emu, void* handle, int accessPermission)
{
    DEBUG_LOG("IMV_OpenEx called\n");
    int result = my->IMV_OpenEx(handle, accessPermission);
    DEBUG_LOG("IMV_OpenEx returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_Close(x64emu_t* emu, void* handle)
{
    DEBUG_LOG("IMV_Close called\n");

    if(handle)
    {
        for (int i = 0; i < MAX_CAMERAS; i++) 
        {
            if (g_callbacks[i].handle == handle) 
            {
                // 清理回调信息
                g_callbacks[i].callback = NULL;
                g_callbacks[i].handle = NULL;
                g_callbacks[i].user = NULL;
                g_callbacks[i].used = 0;
                DEBUG_LOG("Clear g_callbacks[%d]\n", i);
                break;
            }
        }
    }
    
    int result = my->IMV_Close(handle);
    DEBUG_LOG("IMV_Close returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_FeatureIsWriteable(x64emu_t* emu, void* handle, const char* pFeatureName)
{
    DEBUG_LOG("IMV_FeatureIsWriteable called for feature: %s\n", pFeatureName);
    // 在 box64 中，函数指针都是 void* 类型
    int result = my->IMV_FeatureIsWriteable(handle, pFeatureName);
    DEBUG_LOG("IMV_FeatureIsWriteable returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_SetIntFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, int64_t intValue)
{
    DEBUG_LOG("IMV_SetIntFeatureValue called for feature: %s, value: %ld\n", pFeatureName, intValue);
    int result = my->IMV_SetIntFeatureValue(handle, pFeatureName, intValue);
    DEBUG_LOG("IMV_SetIntFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetIntFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, int64_t* pIntValue)
{
    DEBUG_LOG("IMV_GetIntFeatureValue called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetIntFeatureValue(handle, pFeatureName, pIntValue);
    if (result == 0 && pIntValue) {
        DEBUG_LOG("IMV_GetIntFeatureValue value: %ld\n", *pIntValue);
    }
    DEBUG_LOG("IMV_GetIntFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetIntFeatureMax(x64emu_t* emu, void* handle, const char* pFeatureName, int64_t* pIntValue)
{
    DEBUG_LOG("IMV_GetIntFeatureMax called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetIntFeatureMax(handle, pFeatureName, pIntValue);
    DEBUG_LOG("IMV_GetIntFeatureMax returned: %d\n", result);
    return result;
}


EXPORT int my_IMV_GetIntFeatureMin(x64emu_t* emu, void* handle, const char* pFeatureName, int64_t* pIntValue)
{
    DEBUG_LOG("IMV_GetIntFeatureMin called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetIntFeatureMin(handle, pFeatureName, pIntValue);
    DEBUG_LOG("IMV_GetIntFeatureMin returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetIntFeatureInc(x64emu_t* emu, void* handle, const char* pFeatureName, int64_t* pIntValue)
{
    DEBUG_LOG("IMV_GetIntFeatureInc called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetIntFeatureInc(handle, pFeatureName, pIntValue);
    DEBUG_LOG("IMV_GetIntFeatureInc returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_SetDoubleFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, double doubleValue)
{
    DEBUG_LOG("IMV_SetDoubleFeatureValue called for feature: %s, value: %f\n", pFeatureName, doubleValue);
    int result = my->IMV_SetDoubleFeatureValue(handle, pFeatureName, doubleValue);
    DEBUG_LOG("IMV_SetDoubleFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetDoubleFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, double* pDoubleValue)
{
    DEBUG_LOG("IMV_GetDoubleFeatureValue called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetDoubleFeatureValue(handle, pFeatureName, pDoubleValue);
    DEBUG_LOG("IMV_GetDoubleFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetDoubleFeatureMax(x64emu_t* emu, void* handle, const char* pFeatureName, double* pDoubleValue)
{
    DEBUG_LOG("IMV_GetDoubleFeatureMax called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetDoubleFeatureMax(handle, pFeatureName, pDoubleValue);
    DEBUG_LOG("IMV_GetDoubleFeatureMax returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetDoubleFeatureMin(x64emu_t* emu, void* handle, const char* pFeatureName, double* pDoubleValue)
{
    DEBUG_LOG("IMV_GetDoubleFeatureMin called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetDoubleFeatureMin(handle, pFeatureName, pDoubleValue);
    DEBUG_LOG("IMV_GetDoubleFeatureMin returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_SetEnumFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, uint64_t enumValue)
{
    DEBUG_LOG("IMV_SetEnumFeatureValue called for feature: %s, value: %lu\n", pFeatureName, enumValue);
    int result = my->IMV_SetEnumFeatureValue(handle, pFeatureName, enumValue);
    DEBUG_LOG("IMV_SetEnumFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetEnumFeatureValue(x64emu_t* emu, void* handle, const char* pFeatureName, uint64_t* pEnumValue)
{
    DEBUG_LOG("IMV_GetEnumFeatureValue called for feature: %s\n", pFeatureName);
    int result = my->IMV_GetEnumFeatureValue(handle, pFeatureName, pEnumValue);
    DEBUG_LOG("IMV_GetEnumFeatureValue returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_ExecuteCommandFeature(x64emu_t* emu, void* handle, const char* pFeatureName)
{
    DEBUG_LOG("IMV_ExecuteCommandFeature called for feature: %s\n", pFeatureName);
    int result = my->IMV_ExecuteCommandFeature(handle, pFeatureName);
    DEBUG_LOG("IMV_ExecuteCommandFeature returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_StartGrabbing(x64emu_t* emu, void* handle)
{
    DEBUG_LOG("IMV_StartGrabbing called\n");
    int result = my->IMV_StartGrabbing(handle);
    DEBUG_LOG("IMV_StartGrabbing returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_GetFrame(x64emu_t* emu, void* handle, void* pFrame, unsigned int timeoutMS)
{
    DEBUG_LOG("IMV_GetFrame called, timeout: %d ms\n", timeoutMS);
    IMV_Frame arm_frame;
    memset(&arm_frame, 0, sizeof(IMV_Frame));
    int result = my->IMV_GetFrame(handle, &arm_frame, timeoutMS);
    if (result == 0) 
    {
        // 获取x86端的帧指针
        IMV_Frame* x86_frame = (IMV_Frame*)pFrame;
        
        DEBUG_LOG("ARM Frame: width=%u, height=%u, size=%u, pData=%p\n",
               arm_frame.frameInfo.width, 
               arm_frame.frameInfo.height, 
               arm_frame.frameInfo.size,
               arm_frame.pData);
        
        // 1. 拷贝frameInfo
        if (arm_frame.frameInfo.size > 0) 
        {
            // 确保x86端有足够空间
            if (x86_frame) 
            {
                // 拷贝frameInfo结构体
                memcpy(&x86_frame->frameInfo, &arm_frame.frameInfo, sizeof(IMV_FrameInfo));
                
                // 2. 处理图像数据
                if (arm_frame.pData && arm_frame.frameInfo.size > 0) 
                {
                    // 分配内存给x86端
                    x86_frame->pData = (unsigned char*)malloc(arm_frame.frameInfo.size);
                    
                    if (x86_frame->pData) 
                    {
                        // 拷贝图像数据
                        memcpy(x86_frame->pData, arm_frame.pData, arm_frame.frameInfo.size);
                        DEBUG_LOG("Copied %u bytes of image data\n", arm_frame.frameInfo.size);
                    } else 
                    {
                        DEBUG_LOG("ERROR: Failed to allocate %u bytes for image data\n", 
                               arm_frame.frameInfo.size);
                        result = -101; // 返回错误
                    }
                } 
                else 
                {
                    x86_frame->pData = NULL;
                }
                
                // 3. 拷贝frameHandle
                x86_frame->frameHandle = arm_frame.frameHandle;
                
                // 4. 拷贝预留字段
                memcpy(x86_frame->nReserved, arm_frame.nReserved, sizeof(arm_frame.nReserved));
                
            } 
            else 
            {
                DEBUG_LOG("[libMVSSDK wrapper] ERROR: x86_frame is NULL!\n");
                result = -101;
            }
        }
    } 
    else 
    {
        DEBUG_LOG("GetFrame failed with error: %d\n", result);
    }
    
    DEBUG_LOG("Releasing ARM frame...\n");
    my->IMV_ReleaseFrame(handle, &arm_frame);

    DEBUG_LOG("IMV_GetFrame returned: %d\n", result);
    return result;
}

EXPORT int my_IMV_ReleaseFrame(x64emu_t* emu, void* handle, void* pFrame)
{
    DEBUG_LOG("IMV_ReleaseFrame called\n");

    if(!pFrame) return 0;
    IMV_Frame* arm_frame = (IMV_Frame*)pFrame;
    // 打印传入的值
    DEBUG_LOG("[BOX64-RELEASE] Received from Windows:\n");
    DEBUG_LOG("[BOX64-RELEASE]   x86_frame->pData       = 0x%016llX\n",
           (unsigned long long)arm_frame->pData);
    DEBUG_LOG("[BOX64-RELEASE]   x86_frame->frameHandle = 0x%016llX\n",
           (unsigned long long)arm_frame->frameHandle);

    if(arm_frame->pData)
    {
        free(arm_frame->pData);
        arm_frame->pData = NULL;
    }
    if(arm_frame->frameHandle)
    {
        arm_frame->frameHandle = NULL;
    }

    DEBUG_LOG("IMV_ReleaseFrame returned.");
    return 0;
}

// 可以添加更多函数的包装

// ============== 初始化配置 ==============
#define CUSTOM_INIT \
    printf_log(LOG_INFO, "Starting libMVSDK wrapper initialization\n"); \
    getMy(lib); \
    if(!my->IMV_GetVersion || !my->IMV_CreateHandle) { \
        printf_log(LOG_NONE, "ERROR: Required functions not found in libMVSDK\n"); \
    } \
    printf_log(LOG_INFO, "libMVSDK wrapper initialized successfully\n");

#define CUSTOM_FINI \
    freeMy();

#define ALTMY my_
// 如果定义了ALTMY，在wrappedlib_init.h中会执行函数指针重定向
// 如：my->IMV_CreateHandle = my_IMV_CreateHandle;
#include "wrappedlib_init.h"