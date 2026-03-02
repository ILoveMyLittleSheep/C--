#ifndef WRAPMV_H
#define WRAPMV_H

#include <stdint.h>     // 标准整型
#include <stdbool.h>    // 标准布尔型

// ==================== DLL导出定义 ====================
#ifdef __WINE__
    // Wine + winegcc 环境
    #include <windows.h>  // winegcc提供简化的windows.h
    
    // 确保WINAPI有定义（winegcc应该已经定义了）
    #ifndef WINAPI
    #define WINAPI
    #endif

    #define WRAP_IMV_CALL WINAPI

#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    // 原生Windows环境 (MSVC/MinGW)
    #include <windows.h>  // 标准的windows.h
    
    #define WRAP_IMV_CALL WINAPI  // WINAPI已定义为__stdcall

#else
    // 纯Unix/Linux环境（非Wine）
    #define WRAP_IMV_CALL
    
    // 提供基本类型定义
    typedef void* HANDLE;
    typedef int BOOL;
    #define TRUE 1
    #define FALSE 0
#endif

// ==================== 参数方向标记 ====================
#define IN
#define OUT
#define IN_OUT

/// \~chinese
/// \brief 错误码 
/// \~english
/// \brief Error code
#define WRAP_IMV_OK						0			///< \~chinese 成功，无错误							\~english Successed, no error
#define WRAP_IMV_ERROR					-101		///< \~chinese 通用的错误							\~english Generic error
#define WRAP_IMV_INVALID_HANDLE			-102		///< \~chinese 错误或无效的句柄						\~english Error or invalid handle
#define WRAP_IMV_INVALID_PARAM			-103		///< \~chinese 错误的参数							\~english Incorrect parameter
#define WRAP_IMV_INVALID_FRAME_HANDLE	-104		///< \~chinese 错误或无效的帧句柄					\~english Error or invalid frame handle
#define WRAP_IMV_INVALID_FRAME			-105		///< \~chinese 无效的帧								\~english Invalid frame
#define WRAP_IMV_INVALID_RESOURCE		-106		///< \~chinese 相机/事件/流等资源无效				\~english Camera/Event/Stream and so on resource invalid
#define WRAP_IMV_INVALID_IP				-107		///< \~chinese 设备与主机的IP网段不匹配				\~english Device's and PC's subnet is mismatch
#define WRAP_IMV_NO_MEMORY				-108		///< \~chinese 内存不足								\~english Malloc memery failed
#define WRAP_IMV_INSUFFICIENT_MEMORY	-109		///< \~chinese 传入的内存空间不足					\~english Insufficient memory
#define WRAP_IMV_ERROR_PROPERTY_TYPE	-110		///< \~chinese 属性类型错误							\~english Property type error
#define WRAP_IMV_INVALID_ACCESS			-111		///< \~chinese 属性不可访问、或不能读/写、或读/写失败	\~english Property not accessible, or not be read/written, or read/written failed
#define WRAP_IMV_INVALID_RANGE			-112		///< \~chinese 属性值超出范围、或者不是步长整数倍	\~english The property's value is out of range, or is not integer multiple of the step
#define WRAP_IMV_NOT_SUPPORT			-113		///< \~chinese 设备不支持的功能						\~english Device not supported function

#define WRAP_IMV_MAX_DEVICE_ENUM_NUM	100			///< \~chinese 支持设备最大个数		\~english The maximum number of supported devices
#define WRAP_IMV_MAX_STRING_LENTH		256			///< \~chinese 字符串最大长度		\~english The maximum length of string
#define WRAP_IMV_MAX_ERROR_LIST_NUM		128			///< \~chinese 失败属性列表最大长度 \~english The maximum size of failed properties list

typedef	void*	WRAP_IMV_HANDLE;						///< \~chinese 设备句柄				\~english Device handle 
typedef	void*	WRAP_IMV_FRAME_HANDLE;				///< \~chinese 帧句柄				\~english Frame handle 

#define WRAP_IMV_GVSP_PIX_MONO                           0x01000000
#define WRAP_IMV_GVSP_PIX_RGB                            0x02000000
#define WRAP_IMV_GVSP_PIX_COLOR                          0x02000000
#define WRAP_IMV_GVSP_PIX_CUSTOM                         0x80000000
#define WRAP_IMV_GVSP_PIX_COLOR_MASK                     0xFF000000

// Indicate effective number of bits occupied by the pixel (including padding).
// This can be used to compute amount of memory required to store an image.
#define WRAP_IMV_GVSP_PIX_OCCUPY1BIT                     0x00010000
#define WRAP_IMV_GVSP_PIX_OCCUPY2BIT                     0x00020000
#define WRAP_IMV_GVSP_PIX_OCCUPY4BIT                     0x00040000
#define WRAP_IMV_GVSP_PIX_OCCUPY8BIT                     0x00080000
#define WRAP_IMV_GVSP_PIX_OCCUPY12BIT                    0x000C0000
#define WRAP_IMV_GVSP_PIX_OCCUPY16BIT                    0x00100000
#define WRAP_IMV_GVSP_PIX_OCCUPY24BIT                    0x00180000
#define WRAP_IMV_GVSP_PIX_OCCUPY32BIT                    0x00200000
#define WRAP_IMV_GVSP_PIX_OCCUPY36BIT                    0x00240000
#define WRAP_IMV_GVSP_PIX_OCCUPY48BIT                    0x00300000

/// \~chinese
///枚举：设备类型
/// \~english
///Enumeration: device type
typedef enum WRAP_IMV_ECameraType
{
	typeGigeCamera = 0,						///< \~chinese GIGE相机				\~english GigE Vision Camera
	typeU3vCamera = 1,						///< \~chinese USB3.0相机			\~english USB3.0 Vision Camera
	typeCLCamera = 2,						///< \~chinese CAMERALINK 相机		\~english Cameralink camera
	typePCIeCamera = 3,						///< \~chinese PCIe相机				\~english PCIe Camera
	typeUndefinedCamera = 255				///< \~chinese 未知类型				\~english Undefined Camera
}WRAP_IMV_ECameraType;

/// \~chinese
///枚举：接口类型
/// \~english
///Enumeration: interface type
typedef enum WRAP_IMV_EInterfaceType
{
	interfaceTypeGige = 0x00000001,			///< \~chinese 网卡接口类型  		\~english NIC type
	interfaceTypeUsb3 = 0x00000002,			///< \~chinese USB3.0接口类型		\~english USB3.0 interface type
	interfaceTypeCL = 0x00000004, 			///< \~chinese CAMERALINK接口类型	\~english Cameralink interface type
	interfaceTypePCIe = 0x00000008,			///< \~chinese PCIe接口类型         \~english PCIe interface type
	interfaceTypeAll = 0x00000000,			///< \~chinese 忽略接口类型			\~english All types interface type
	interfaceInvalidType = 0xFFFFFFFF		///< \~chinese 无效接口类型			\~english Invalid interface type
}WRAP_IMV_EInterfaceType;

/// \~chinese
///枚举：创建句柄方式
/// \~english
///Enumeration: Create handle mode
typedef enum WRAP_IMV_ECreateHandleMode
{
	modeByIndex = 0,						///< \~chinese 通过已枚举设备的索引(从0开始，比如 0, 1, 2...)	\~english By index of enumerated devices (Start from 0, such as 0, 1, 2...)
	modeByCameraKey,						///< \~chinese 通过设备键"厂商:序列号"							\~english By device's key "vendor:serial number"
	modeByDeviceUserID,						///< \~chinese 通过设备自定义名									\~english By device userID
	modeByIPAddress,						///< \~chinese 通过设备IP地址									\~english By device IP address.
}WRAP_IMV_ECreateHandleMode;

/// \~chinese
///枚举：访问权限
/// \~english
///Enumeration: access permission
typedef enum WRAP_IMV_ECameraAccessPermission
{
	accessPermissionOpen = 0,				///< \~chinese GigE相机没有被连接			\~english The GigE vision device isn't connected to any application. 
	accessPermissionExclusive,				///< \~chinese 独占访问权限					\~english Exclusive Access Permission   
	accessPermissionControl, 				///< \~chinese 非独占可读访问权限			\~english Non-Exclusive Readbale Access Permission  
	accessPermissionControlWithSwitchover,  ///< \~chinese 切换控制访问权限				\~english Control access with switchover enabled.	
	accessPermissionUnknown = 254,  		///< \~chinese 无法确定						\~english Value not known; indeterminate.   	
	accessPermissionUndefined				///< \~chinese 未定义访问权限				\~english Undefined Access Permission
}WRAP_IMV_ECameraAccessPermission;

/// \~chinese
///枚举：图像格式
/// \~english
/// Enumeration:image format
typedef enum WRAP_IMV_EPixelType
{
	// Undefined pixel type
	gvspPixelTypeUndefined = -1,

	// Mono Format
	gvspPixelMono1p = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY1BIT | 0x0037),
	gvspPixelMono2p = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY2BIT | 0x0038),
	gvspPixelMono4p = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY4BIT | 0x0039),
	gvspPixelMono8 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x0001),
	gvspPixelMono8S = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x0002),
	gvspPixelMono10 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0003),
	gvspPixelMono10Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0004),
	gvspPixelMono12 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0005),
	gvspPixelMono12Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0006),
	gvspPixelMono14 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0025),
	gvspPixelMono16 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0007),

	// Bayer Format
	gvspPixelBayGR8 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x0008),
	gvspPixelBayRG8 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x0009),
	gvspPixelBayGB8 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x000A),
	gvspPixelBayBG8 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY8BIT | 0x000B),
	gvspPixelBayGR10 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x000C),
	gvspPixelBayRG10 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x000D),
	gvspPixelBayGB10 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x000E),
	gvspPixelBayBG10 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x000F),
	gvspPixelBayGR12 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0010),
	gvspPixelBayRG12 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0011),
	gvspPixelBayGB12 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0012),
	gvspPixelBayBG12 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0013),
	gvspPixelBayGR10Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0026),
	gvspPixelBayRG10Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0027),
	gvspPixelBayGB10Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0028),
	gvspPixelBayBG10Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0029),
	gvspPixelBayGR12Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x002A),
	gvspPixelBayRG12Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x002B),
	gvspPixelBayGB12Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x002C),
	gvspPixelBayBG12Packed = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x002D),
	gvspPixelBayGR16 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x002E),
	gvspPixelBayRG16 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x002F),
	gvspPixelBayGB16 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0030),
	gvspPixelBayBG16 = (WRAP_IMV_GVSP_PIX_MONO | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0031),

	// RGB Format
	gvspPixelRGB8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x0014),
	gvspPixelBGR8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x0015),
	gvspPixelRGBA8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY32BIT | 0x0016),
	gvspPixelBGRA8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY32BIT | 0x0017),
	gvspPixelRGB10 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0018),
	gvspPixelBGR10 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0019),
	gvspPixelRGB12 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x001A),
	gvspPixelBGR12 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x001B),
	gvspPixelRGB16 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0033),
	gvspPixelRGB10V1Packed = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY32BIT | 0x001C),
	gvspPixelRGB10P32 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY32BIT | 0x001D),
	gvspPixelRGB12V1Packed = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY36BIT | 0X0034),
	gvspPixelRGB565P = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0035),
	gvspPixelBGR565P = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0X0036),

	// YVR Format
	gvspPixelYUV411_8_UYYVYY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x001E),
	gvspPixelYUV422_8_UYVY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x001F),
	gvspPixelYUV422_8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0032),
	gvspPixelYUV8_UYV = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x0020),
	gvspPixelYCbCr8CbYCr = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x003A),
	gvspPixelYCbCr422_8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x003B),
	gvspPixelYCbCr422_8_CbYCrY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0043),
	gvspPixelYCbCr411_8_CbYYCrYY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x003C),
	gvspPixelYCbCr601_8_CbYCr = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x003D),
	gvspPixelYCbCr601_422_8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x003E),
	gvspPixelYCbCr601_422_8_CbYCrY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0044),
	gvspPixelYCbCr601_411_8_CbYYCrYY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x003F),
	gvspPixelYCbCr709_8_CbYCr = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x0040),
	gvspPixelYCbCr709_422_8 = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0041),
	gvspPixelYCbCr709_422_8_CbYCrY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY16BIT | 0x0045),
	gvspPixelYCbCr709_411_8_CbYYCrYY = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY12BIT | 0x0042),

	// RGB Planar
	gvspPixelRGB8Planar = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY24BIT | 0x0021),
	gvspPixelRGB10Planar = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0022),
	gvspPixelRGB12Planar = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0023),
	gvspPixelRGB16Planar = (WRAP_IMV_GVSP_PIX_COLOR | WRAP_IMV_GVSP_PIX_OCCUPY48BIT | 0x0024),

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
}WRAP_IMV_EPixelType;

/// \~chinese
/// \brief GigE设备信息
/// \~english
/// \brief GigE device information
typedef struct WRAP_IMV_GigEDeviceInfo
{
	/// \~chinese
	/// 设备支持的IP配置选项\n
	/// value:4 相机只支持LLA\n
	/// value:5 相机支持LLA和Persistent IP\n
	/// value:6 相机支持LLA和DHCP\n
	/// value:7 相机支持LLA、DHCP和Persistent IP\n
	/// value:0 获取失败
	/// \~english
	/// Supported IP configuration options of device\n
	/// value:4 Device supports LLA \n
	/// value:5 Device supports LLA and Persistent IP\n
	/// value:6 Device supports LLA and DHCP\n
	/// value:7 Device supports LLA, DHCP and Persistent IP\n
	/// value:0 Get fail
	unsigned int nIpConfigOptions;
	/// \~chinese
	/// 设备当前的IP配置选项\n
	/// value:4 LLA处于活动状态\n
	/// value:5 LLA和Persistent IP处于活动状态\n
	/// value:6 LLA和DHCP处于活动状态\n
	/// value:7 LLA、DHCP和Persistent IP处于活动状态\n
	/// value:0 获取失败
	/// \~english
	/// Current IP Configuration options of device\n
	/// value:4 LLA is active\n
	/// value:5 LLA and Persistent IP are active\n
	/// value:6 LLA and DHCP are active\n
	/// value:7 LLA, DHCP and Persistent IP are active\n
	/// value:0 Get fail
	unsigned int nIpConfigCurrent;
	unsigned int nReserved[3];						///< \~chinese 保留					\~english Reserved field

	char macAddress[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备Mac地址			\~english Device MAC Address
	char ipAddress[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备Ip地址			\~english Device ip Address
	char subnetMask[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 子网掩码				\~english SubnetMask
	char defaultGateWay[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 默认网关				\~english Default GateWay
	char protocolVersion[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 网络协议版本			\~english Net protocol version
	/// \~chinese
	/// Ip配置有效性\n
	/// Ip配置有效时字符串值"Valid"\n
	/// Ip配置无效时字符串值"Invalid On This Interface"
	/// \~english
	/// IP configuration valid\n
	/// String value is "Valid" when ip configuration valid\n
	/// String value is "Invalid On This Interface" when ip configuration invalid
	char ipConfiguration[WRAP_IMV_MAX_STRING_LENTH];
	char chReserved[6][WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 保留					\~english Reserved field

}WRAP_IMV_GigEDeviceInfo;

/// \~chinese
/// \brief Usb设备信息
/// \~english
/// \brief Usb device information
typedef struct WRAP_IMV_UsbDeviceInfo
{
	bool bLowSpeedSupported;						///< \~chinese true支持，false不支持，其他值 非法。	\~english true support,false not supported,other invalid
	bool bFullSpeedSupported;						///< \~chinese true支持，false不支持，其他值 非法。	\~english true support,false not supported,other invalid
	bool bHighSpeedSupported;						///< \~chinese true支持，false不支持，其他值 非法。	\~english true support,false not supported,other invalid
	bool bSuperSpeedSupported;						///< \~chinese true支持，false不支持，其他值 非法。	\~english true support,false not supported,other invalid
	bool bDriverInstalled;							///< \~chinese true安装，false未安装，其他值 非法。	\~english true support,false not supported,other invalid
	bool boolReserved[3];							///< \~chinese 保留		
	unsigned int Reserved[4];						///< \~chinese 保留									\~english Reserved field

	char configurationValid[WRAP_IMV_MAX_STRING_LENTH];	///< \~chinese 配置有效性							\~english Configuration Valid
	char genCPVersion[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese GenCP 版本							\~english GenCP Version
	char u3vVersion[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese U3V 版本号							\~english U3v Version
	char deviceGUID[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备引导号							\~english Device guid number                  
	char familyName[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备系列号							\~english Device serial number 
	char u3vSerialNumber[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 设备序列号							\~english Device SerialNumber
	char speed[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese 设备传输速度							\~english Device transmission speed
	char maxPower[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备最大供电量						\~english Maximum power supply of device
	char chReserved[4][WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 保留									\~english Reserved field

}WRAP_IMV_UsbDeviceInfo;

/// \~chinese
/// \brief GigE网卡信息
/// \~english
/// \brief GigE interface information
typedef struct WRAP_IMV_GigEInterfaceInfo
{
	char description[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  网卡描述信息		\~english Network card description
	char macAddress[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  网卡Mac地址			\~english Network card MAC Address
	char ipAddress[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  设备Ip地址			\~english Device ip Address
	char subnetMask[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  子网掩码			\~english SubnetMask
	char defaultGateWay[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese  默认网关			\~english Default GateWay
	char chReserved[5][WRAP_IMV_MAX_STRING_LENTH];			///< 保留							\~english Reserved field
}WRAP_IMV_GigEInterfaceInfo;

/// \~chinese
/// \brief USB接口信息
/// \~english
/// \brief USB interface information
typedef struct WRAP_IMV_UsbInterfaceInfo
{
	char description[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  USB接口描述信息		\~english USB interface description
	char vendorID[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  USB接口Vendor ID	\~english USB interface Vendor ID
	char deviceID[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  USB接口设备ID		\~english USB interface Device ID
	char subsystemID[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  USB接口Subsystem ID	\~english USB interface Subsystem ID
	char revision[WRAP_IMV_MAX_STRING_LENTH];				///< \~chinese  USB接口Revision		\~english USB interface Revision
	char speed[WRAP_IMV_MAX_STRING_LENTH];					///< \~chinese  USB接口speed		\~english USB interface speed
	char chReserved[4][WRAP_IMV_MAX_STRING_LENTH];			///< 保留							\~english Reserved field
}WRAP_IMV_UsbInterfaceInfo;

/// \~chinese
/// \brief 设备通用信息
/// \~english
/// \brief Device general information
typedef struct WRAP_IMV_DeviceInfo
{
	WRAP_IMV_ECameraType			nCameraType;								///< \~chinese 设备类别			\~english Camera type
	int								nCameraReserved[5];							///< \~chinese 保留				\~english Reserved field

	char							cameraKey[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 厂商:序列号		\~english Camera key
	char							cameraName[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 用户自定义名		\~english UserDefinedName
	char							serialNumber[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备序列号		\~english Device SerialNumber
	char							vendorName[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 厂商				\~english Camera Vendor
	char							modelName[WRAP_IMV_MAX_STRING_LENTH];			///< \~chinese 设备型号			\~english Device model
	char							manufactureInfo[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 设备制造信息		\~english Device ManufactureInfo
	char							deviceVersion[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 设备版本			\~english Device Version
	char							cameraReserved[5][WRAP_IMV_MAX_STRING_LENTH];	///< \~chinese 保留				\~english Reserved field
	union
	{
		WRAP_IMV_GigEDeviceInfo		gigeDeviceInfo;								///< \~chinese  Gige设备信息	\~english Gige Device Information
		WRAP_IMV_UsbDeviceInfo		usbDeviceInfo;								///< \~chinese  Usb设备信息		\~english Usb  Device Information
	}DeviceSpecificInfo;

	WRAP_IMV_EInterfaceType			nInterfaceType;								///< \~chinese 接口类别			\~english Interface type
	int								nInterfaceReserved[5];						///< \~chinese 保留				\~english Reserved field
	char							interfaceName[WRAP_IMV_MAX_STRING_LENTH];		///< \~chinese 接口名			\~english Interface Name
	char							interfaceReserved[5][WRAP_IMV_MAX_STRING_LENTH];	///< \~chinese 保留				\~english Reserved field
	union
	{
		WRAP_IMV_GigEInterfaceInfo		gigeInterfaceInfo;							///< \~chinese  GigE网卡信息	\~english Gige interface Information
		WRAP_IMV_UsbInterfaceInfo		usbInterfaceInfo;							///< \~chinese  Usb接口信息		\~english Usb interface Information
	}InterfaceInfo;
}WRAP_IMV_DeviceInfo;

/// \~chinese
/// \brief 设备信息列表
/// \~english
/// \brief Device information list
typedef struct WRAP_IMV_DeviceList
{
	unsigned int		nDevNum;					///< \~chinese 设备数量									\~english Device Number
	WRAP_IMV_DeviceInfo*		pDevInfo;					///< \~chinese 设备息列表(SDK内部缓存)，最多100设备		\~english Device information list(cached within the SDK), up to 100
}WRAP_IMV_DeviceList;

/// \~chinese
/// \brief 帧图像信息
/// \~english
/// \brief The frame image information
typedef struct WRAP_IMV_FrameInfo
{
	uint64_t				blockId;				///< \~chinese 帧Id(仅对GigE/Usb/PCIe相机有效)					\~english The block ID(GigE/Usb/PCIe camera only)
	unsigned int			status;					///< \~chinese 数据帧状态(0是正常状态)							\~english The status of frame(0 is normal status)
	unsigned int			width;					///< \~chinese 图像宽度											\~english The width of image
	unsigned int			height;					///< \~chinese 图像高度											\~english The height of image
	unsigned int			size;					///< \~chinese 图像大小											\~english The size of image
	WRAP_IMV_EPixelType		pixelFormat;			///< \~chinese 图像像素格式										\~english The pixel format of image
	uint64_t				timeStamp;				///< \~chinese 图像时间戳(仅对GigE/Usb/PCIe相机有效)			\~english The timestamp of image(GigE/Usb/PCIe camera only)
	unsigned int			chunkCount;				///< \~chinese 帧数据中包含的Chunk个数(仅对GigE/Usb相机有效)	\~english The number of chunk in frame data(GigE/Usb Camera Only)
	unsigned int			paddingX;				///< \~chinese 图像paddingX(仅对GigE/Usb/PCIe相机有效)			\~english The paddingX of image(GigE/Usb/PCIe camera only)
	unsigned int			paddingY;				///< \~chinese 图像paddingY(仅对GigE/Usb/PCIe相机有效)			\~english The paddingY of image(GigE/Usb/PCIe camera only)
	unsigned int			recvFrameTime;			///< \~chinese 图像在网络传输所用的时间(单位:微秒,非GigE相机该值为0)	\~english The time taken for the image to be transmitted over the network(unit:us, The value is 0 for non-GigE camera)
	unsigned int			nReserved[19];			///< \~chinese 预留字段											\~english Reserved field
}WRAP_IMV_FrameInfo;

/// \~chinese
/// \brief 帧图像数据信息
/// \~english
/// \brief Frame image data information
typedef struct WRAP_IMV_Frame
{
	WRAP_IMV_FRAME_HANDLE		frameHandle;			///< \~chinese 帧图像句柄(SDK内部帧管理用)						\~english Frame image handle(used for managing frame within the SDK)
	unsigned char*			pData;					///< \~chinese 帧图像数据的内存首地址							\~english The starting address of memory of image data
	WRAP_IMV_FrameInfo			frameInfo;				///< \~chinese 帧信息											\~english Frame information
	unsigned int			nReserved[10];			///< \~chinese 预留字段											\~english Reserved field
}WRAP_IMV_Frame;


#ifndef WINAPI
#ifdef __GNUC__
#define WINAPI __attribute__((ms_abi))
#else
#define WINAPI __stdcall
#endif
#endif
// Windows回调类型（MSABI调用约定）
typedef void (WINAPI *WRAP_IMV_FrameCallBack)(
    WRAP_IMV_Frame* pFrame,
    void* pUser
);

#if defined(__cplusplus)
extern "C"
{
#endif

     int WRAP_IMV_CALL WRAP_IMV_EnumDevices(OUT WRAP_IMV_DeviceList *pDeviceList, IN unsigned int interfaceType);
     int WRAP_IMV_CALL WRAP_IMV_CreateHandle(OUT WRAP_IMV_HANDLE* handle, IN WRAP_IMV_ECreateHandleMode mode, IN void* pIdentifier);
     int WRAP_IMV_CALL WRAP_IMV_DestroyHandle(IN WRAP_IMV_HANDLE handle);
     int WRAP_IMV_CALL WRAP_IMV_OpenEx(IN WRAP_IMV_HANDLE handle, IN WRAP_IMV_ECameraAccessPermission accessPermission);
     int WRAP_IMV_CALL WRAP_IMV_Close(IN WRAP_IMV_HANDLE handle);
     bool WRAP_IMV_CALL WRAP_IMV_FeatureIsWriteable(IN WRAP_IMV_HANDLE handle, IN const char* pFeatureName);
     int WRAP_IMV_CALL WRAP_IMV_SetIntFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char* pFeatureName, IN int64_t intValue);
     int WRAP_IMV_CALL WRAP_IMV_GetIntFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char* pFeatureName, OUT int64_t* pIntValue);
     int WRAP_IMV_CALL WRAP_IMV_GetIntFeatureMax(IN WRAP_IMV_HANDLE handle, IN const char* pFeatureName, OUT int64_t* pIntValue);
     int WRAP_IMV_CALL WRAP_IMV_GetIntFeatureMin(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT int64_t *pIntValue);
     int WRAP_IMV_CALL WRAP_IMV_GetIntFeatureInc(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT int64_t *pIntValue);
     int WRAP_IMV_CALL WRAP_IMV_SetDoubleFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, IN double doubleValue);
     int WRAP_IMV_CALL WRAP_IMV_GetDoubleFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT double *pDoubleValue);
     int WRAP_IMV_CALL WRAP_IMV_GetDoubleFeatureMax(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT double *pDoubleValue);
     int WRAP_IMV_CALL WRAP_IMV_GetDoubleFeatureMin(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT double *pDoubleValue);
     int WRAP_IMV_CALL WRAP_IMV_SetEnumFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, IN uint64_t enumValue);
     int WRAP_IMV_CALL WRAP_IMV_GetEnumFeatureValue(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName, OUT uint64_t *pEnumValue);
     int WRAP_IMV_CALL WRAP_IMV_ExecuteCommandFeature(IN WRAP_IMV_HANDLE handle, IN const char *pFeatureName);
     int WRAP_IMV_CALL WRAP_IMV_StartGrabbing(IN WRAP_IMV_HANDLE handle);
     int WRAP_IMV_CALL WRAP_IMV_StopGrabbing(IN WRAP_IMV_HANDLE handle);
     int WRAP_IMV_CALL WRAP_IMV_AttachGrabbing(IN WRAP_IMV_HANDLE handle, IN WRAP_IMV_FrameCallBack proc, IN void *pUser);
     int WRAP_IMV_CALL WRAP_IMV_GetFrame(IN WRAP_IMV_HANDLE handle, OUT WRAP_IMV_Frame *pFrame, IN unsigned int timeoutMS);
     int WRAP_IMV_CALL WRAP_IMV_ReleaseFrame(IN WRAP_IMV_HANDLE handle, IN WRAP_IMV_Frame *pFrame);

#if defined(__cplusplus)
}
#endif
#endif // WRAPMV_H