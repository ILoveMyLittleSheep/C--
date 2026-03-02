#include "WrapMV.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#ifdef __WINE__
#include <windows.h>
#endif

// ==================== SDK函数指针定义 ====================
typedef const char* (*IMV_GetVersion_t)();
typedef int (*IMV_EnumDevices_t)(WRAP_IMV_DeviceList*, unsigned int);
typedef int (*IMV_CreateHandle_t)(void**, int, void*);
typedef int (*IMV_DestroyHandle_t)(void*);
typedef int (*IMV_OpenEx_t)(void*, int);
typedef int (*IMV_Close_t)(void*);
typedef int (*IMV_StartGrabbing_t)(void*);
typedef int (*IMV_StopGrabbing_t)(void*);
typedef int (*IMV_GetFrame_t)(void*, WRAP_IMV_Frame*, unsigned int);
typedef int (*IMV_ReleaseFrame_t)(void*, WRAP_IMV_Frame*);
typedef int (*IMV_AttachGrabbing_t)(void*, void*, void*);
typedef int (*IMV_SetIntFeatureValue_t)(void*, const char*, int64_t);
typedef int (*IMV_GetIntFeatureValue_t)(void*, const char*, int64_t*);
typedef int (*IMV_GetIntFeatureMax_t)(void*, const char*, int64_t*);
typedef int (*IMV_GetIntFeatureMin_t)(void*, const char*, int64_t*);
typedef int (*IMV_GetIntFeatureInc_t)(void*, const char*, int64_t*);
typedef int (*IMV_SetDoubleFeatureValue_t)(void*, const char*, double);
typedef int (*IMV_GetDoubleFeatureValue_t)(void*, const char*, double*);
typedef int (*IMV_GetDoubleFeatureMax_t)(void*, const char*, double*);
typedef int (*IMV_GetDoubleFeatureMin_t)(void*, const char*, double*);
typedef int (*IMV_SetEnumFeatureValue_t)(void*, const char*, uint64_t);
typedef int (*IMV_GetEnumFeatureValue_t)(void*, const char*, uint64_t*);
typedef int (*IMV_ExecuteCommandFeature_t)(void*, const char*);
typedef int (*IMV_FeatureIsWriteable_t)(void*, const char*);

// ==================== 全局变量 ====================
static void* sdk_handle = NULL;
static atomic_bool sdk_loaded = ATOMIC_VAR_INIT(0);
static WRAP_IMV_FrameCallBack user_callback = NULL;

// SDK函数指针
static IMV_GetVersion_t pIMV_GetVersion = NULL;
static IMV_EnumDevices_t pIMV_EnumDevices = NULL;
static IMV_CreateHandle_t pIMV_CreateHandle = NULL;
static IMV_DestroyHandle_t pIMV_DestroyHandle = NULL;
static IMV_OpenEx_t pIMV_OpenEx = NULL;
static IMV_Close_t pIMV_Close = NULL;
static IMV_StartGrabbing_t pIMV_StartGrabbing = NULL;
static IMV_StopGrabbing_t pIMV_StopGrabbing = NULL;
static IMV_GetFrame_t pIMV_GetFrame = NULL;
static IMV_ReleaseFrame_t pIMV_ReleaseFrame = NULL;
static IMV_AttachGrabbing_t pIMV_AttachGrabbing = NULL;
static IMV_SetIntFeatureValue_t pIMV_SetIntFeatureValue = NULL;
static IMV_GetIntFeatureValue_t pIMV_GetIntFeatureValue = NULL;
static IMV_GetIntFeatureMax_t pIMV_GetIntFeatureMax = NULL;
static IMV_GetIntFeatureMin_t pIMV_GetIntFeatureMin = NULL;
static IMV_GetIntFeatureInc_t pIMV_GetIntFeatureInc = NULL;
static IMV_SetDoubleFeatureValue_t pIMV_SetDoubleFeatureValue = NULL;
static IMV_GetDoubleFeatureValue_t pIMV_GetDoubleFeatureValue = NULL;
static IMV_GetDoubleFeatureMax_t pIMV_GetDoubleFeatureMax = NULL;
static IMV_GetDoubleFeatureMin_t pIMV_GetDoubleFeatureMin = NULL;
static IMV_SetEnumFeatureValue_t pIMV_SetEnumFeatureValue = NULL;
static IMV_GetEnumFeatureValue_t pIMV_GetEnumFeatureValue = NULL;
static IMV_ExecuteCommandFeature_t pIMV_ExecuteCommandFeature = NULL;
static IMV_FeatureIsWriteable_t pIMV_FeatureIsWriteable = NULL;

// ==================== 调试宏 ====================
// #define DEBUG_ENABLED 1
#ifdef DEBUG_ENABLED
    #define DEBUG_LOG(fmt, ...) printf("[WRAPMV] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_LOG(fmt, ...)
#endif

#ifdef __WINE__
// DLL入口点（仅Wine环境需要）
BOOL WRAP_IMV_CALL DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    (void)hinstDLL;
    (void)reserved;
    
    if (reason == DLL_PROCESS_ATTACH) {
        DEBUG_LOG("DLL loaded (Wine environment)");
    } else if (reason == DLL_PROCESS_DETACH) {
        DEBUG_LOG("DLL unloaded (Wine environment)");
    }
    
    return TRUE;
}
#endif

// ==================== 回调线程核心数据结构定义 ====================
#define MAX_CAMERAS 10                   // 相机最大数量
#define QUEUE_FULL_TIMEOUT_NS 10000000   // 10ms，队列满超时（纳秒）
#define STAT_PRINT_INTERVAL 5            // 统计打印间隔（秒）

typedef struct {
    WRAP_IMV_Frame* frames;      // 帧数组
    int capacity;                // 队列容量
    int head;                    // 队列头（读位置）
    int tail;                    // 队列尾（写位置）
    int count;                   // 当前帧数
    atomic_int dropped_frames;   // 丢帧计数（统计用）
} FrameQueue;

// 相机上下文：句柄、回调、注册数据、图像数据、线程id
typedef struct {
    WRAP_IMV_HANDLE handle;            // SDK句柄
    WRAP_IMV_FrameCallBack callback;   // 用户回调函数
    void* user_data;                   // 用户数据
    atomic_bool active;                // 是否激活
    
    // 帧队列
    FrameQueue queue;                  // 帧队列
    pthread_mutex_t queue_mutex;       // 队列锁
    pthread_cond_t queue_not_empty;    // 队列非空条件
    pthread_cond_t queue_not_full;     // 队列非满条件
    
    // 处理线程
    #ifdef __WINE__
    HANDLE thread_handle;
    #else
    pthread_t thread_id;
    #endif
    atomic_bool thread_exit;
    
    // 统计信息
    atomic_long frames_processed;      // 已处理帧数
    atomic_long frames_received;       // 接收帧数
} CameraContext;

// 相机上下文队列
static CameraContext* camera_contexts[10] = {0};

// 相机上下文队列所线程锁
static pthread_rwlock_t camera_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static atomic_long frames_received = ATOMIC_VAR_INIT(0);  // SDK回调中使用的统计变量

// ==================== 队列操作函数 ====================
static int FrameQueue_Init(FrameQueue* queue, int capacity) 
{
    queue->frames = (WRAP_IMV_Frame*)calloc(capacity, sizeof(WRAP_IMV_Frame));
    if (!queue->frames) 
    {
        DEBUG_LOG("错误: 无法分配帧队列内存");
        return 0;
    }
    
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    atomic_store(&queue->dropped_frames, 0);
    
    DEBUG_LOG("帧队列初始化: 容量=%d", capacity);
    return 1;
}

static void FrameQueue_Destroy(FrameQueue* queue) 
{
    if (queue->frames) {
        // 释放所有帧的数据
        for (int i = 0; i < queue->capacity; i++) 
        {
            if (queue->frames[i].pData) 
            {
                free(queue->frames[i].pData);
                queue->frames[i].pData = NULL;
            }
        }
        free(queue->frames);
        queue->frames = NULL;
    }
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

// 入队（非阻塞，队列满时返回0）
static int FrameQueue_Push(FrameQueue* queue, const WRAP_IMV_Frame* frame) 
{
    if (queue->count >= queue->capacity) 
    {
        atomic_fetch_add(&queue->dropped_frames, 1);
        return 0; // 队列满
    }
    
    WRAP_IMV_Frame* dst = &queue->frames[queue->tail];
    
    // 分配或重新分配数据缓冲区
    if (dst->pData == NULL || dst->frameInfo.size < frame->frameInfo.size) 
    {
        if (dst->pData) 
        {
            free(dst->pData);
        }
        dst->pData = (unsigned char*)malloc(frame->frameInfo.size);
        if (!dst->pData) 
        {
            DEBUG_LOG("错误: 无法分配帧数据内存");
            return 0;
        }
    }
    
    // 拷贝帧数据
    memcpy(dst->pData, frame->pData, frame->frameInfo.size);
    dst->frameInfo = frame->frameInfo;
    dst->frameHandle = frame->frameHandle;
    
    // 更新队列指针
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    return 1;
}

// 出队（非阻塞，队列空时返回0）
static int FrameQueue_Pop(FrameQueue* queue, WRAP_IMV_Frame* frame) 
{
    if (queue->count <= 0) 
    {
        return 0; // 队列空
    }
    
    WRAP_IMV_Frame* src = &queue->frames[queue->head];
    
    // 拷贝帧数据（仅拷贝元数据，数据指针保持有效）
    frame->pData = src->pData;
    frame->frameInfo = src->frameInfo;
    frame->frameHandle = src->frameHandle;
    
    // 清除源帧的指针（防止重复释放）
    src->pData = NULL;
    
    // 更新队列指针
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    return 1;
}

// 获取队列状态
static int FrameQueue_IsFull(const FrameQueue* queue) 
{
    return queue->count >= queue->capacity;
}

static int FrameQueue_IsEmpty(const FrameQueue* queue) 
{
    return queue->count <= 0;
}

void SDK_FrameCallback(void* pFrame, void* pUser) 
{
    #ifdef __WINE__
    DWORD start_ms = GetTickCount();
    #endif
    WRAP_IMV_Frame* frame = (WRAP_IMV_Frame*)pFrame;
    WRAP_IMV_HANDLE handle = (WRAP_IMV_HANDLE)pUser;
    
    if (!frame || !frame->pData || frame->frameInfo.size == 0) 
    {
        DEBUG_LOG("收到无效帧\n"); 
        return;
    }
    
    // 统计接收帧数
    atomic_fetch_add(&frames_received, 1);
    
    // 通过handle找到相机上下文
    pthread_rwlock_rdlock(&camera_rwlock);
    CameraContext* ctx = NULL;
    for (int i = 0; i < MAX_CAMERAS; i++) 
    {
        if (camera_contexts[i] && 
            camera_contexts[i]->handle == handle &&
            atomic_load(&camera_contexts[i]->active)) 
        {
            ctx = camera_contexts[i];
            break;
        }
    }
    pthread_rwlock_unlock(&camera_rwlock);
    
    if (!ctx || !ctx->callback) 
    {
        DEBUG_LOG("该回调未注册\n"); 
        return;
    }
    
    pthread_mutex_lock(&ctx->queue_mutex);
    
    // 如果队列满，等待处理线程消费
    while (FrameQueue_IsFull(&ctx->queue) && 
           !atomic_load(&ctx->thread_exit)) 
    {
        // 通知处理线程加速处理
        pthread_cond_signal(&ctx->queue_not_empty);
        
        // 等待队列有空位（带超时，避免死锁）
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 10 * 1000000; // 10ms超时
        if (ts.tv_nsec >= 1000000000) 
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        int wait_result = pthread_cond_timedwait(&ctx->queue_not_full, 
                                                &ctx->queue_mutex, &ts);
        if (wait_result == ETIMEDOUT) 
        {
            // 超时，丢弃此帧
            DEBUG_LOG("警告: 队列满，丢弃帧 (大小: %u)", frame->frameInfo.size);
            pthread_mutex_unlock(&ctx->queue_mutex);
            return;
        }
    }
    
    // 帧入队
    if (FrameQueue_Push(&ctx->queue, frame)) {
        DEBUG_LOG("帧入队: 队列大小=%d/%d, width=%d, height=%d", 
                 ctx->queue.count, ctx->queue.capacity,
                 frame->frameInfo.width, frame->frameInfo.height);
        
        // 通知处理线程
        pthread_cond_signal(&ctx->queue_not_empty);
    } else {
        DEBUG_LOG("错误: 帧入队失败");
    }
    
    pthread_mutex_unlock(&ctx->queue_mutex);
    #ifdef __WINE__
    DWORD end_ms = GetTickCount();
    DEBUG_LOG("包装回调耗时：%lu ms\n", end_ms - start_ms);
    #endif
}

// ==================== Wine处理线程 ====================
// 保存为BMP图片（最简单）
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

#ifdef __WINE__
DWORD WINAPI CameraProcessor(LPVOID param) {
#else
void* CameraProcessor(void* param) {
#endif
    CameraContext* ctx = (CameraContext*)param;
    
    #ifdef __WINE__
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    #endif
    
    // 线程局部帧缓冲区（避免在队列中持有锁）
    WRAP_IMV_Frame local_frame;
    memset(&local_frame, 0, sizeof(local_frame));
    
    while (!atomic_load(&ctx->thread_exit)) 
    {
        pthread_mutex_lock(&ctx->queue_mutex);
        
        // 等待队列中有帧
        while (FrameQueue_IsEmpty(&ctx->queue) && 
               !atomic_load(&ctx->thread_exit)) 
        {
            // 解锁queue_mutex，等待队列非空信号，再加锁queue_mutex
            pthread_cond_wait(&ctx->queue_not_empty, &ctx->queue_mutex);
        }
        
        if (atomic_load(&ctx->thread_exit)) 
        {
            pthread_mutex_unlock(&ctx->queue_mutex);
            break;
        }
        
        // 从队列中取出一帧
        if (!FrameQueue_Pop(&ctx->queue, &local_frame)) 
        {
            pthread_mutex_unlock(&ctx->queue_mutex);
            continue;
        }
        
        // 通知SDK回调可以继续（队列有空位）
        pthread_cond_signal(&ctx->queue_not_full);
        pthread_mutex_unlock(&ctx->queue_mutex);

        #ifdef __WINE__
        DWORD start_ms = GetTickCount();
        #endif

        // 调用用户回调（不在锁内执行）
        if (ctx->callback) 
        {
            ctx->callback(&local_frame, ctx->user_data);
        }
        
        #ifdef __WINE__
        DWORD end_ms = GetTickCount();
        DEBUG_LOG("Windows回调耗时：%lu ms\n", end_ms - start_ms);
        #endif

        // 释放帧数据内存
        if (local_frame.pData) 
        {
            free(local_frame.pData);
            local_frame.pData = NULL;
        }
        
        // 统计处理帧数
        atomic_fetch_add(&ctx->frames_processed, 1);
        
        // 定期打印统计信息（每秒一次）
        static time_t last_stat_time = 0;
        time_t now = time(NULL);
        if (now != last_stat_time) 
        {
            long received = atomic_load(&ctx->frames_received);
            long processed = atomic_load(&ctx->frames_processed);
            int queue_size = ctx->queue.count;
            int dropped = atomic_load(&ctx->queue.dropped_frames);
            
            if (now % 5 == 0) { // 每5秒打印一次
                DEBUG_LOG("统计[%p]: 接收=%ld, 处理=%ld, 队列=%d, 丢弃=%d",
                         ctx->handle, received, processed, queue_size, dropped);
            }
            last_stat_time = now;
        }
        
        #ifdef __WINE__
        Sleep(0); // 让出时间片
        #endif
    }
    
    // 清理本地缓冲区
    if (local_frame.pData) {
        free(local_frame.pData);
    }
    
    return 0;
}

// ==================== SDK动态加载 ====================
static int LoadSDK() {
    if (atomic_load(&sdk_loaded)) {
        return 1;
    }
    
    DEBUG_LOG("加载相机SDK...");
    
    // 尝试加载libMVSDK.so
    sdk_handle = dlopen("libMVSDK.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!sdk_handle) {
        DEBUG_LOG("加载失败: %s", dlerror());
        return 0;
    }
    
    DEBUG_LOG("成功加载SDK库");
    
    // 加载所有函数
    #define LOAD_FUNC(name) \
        do { \
            pIMV_##name = (IMV_##name##_t)dlsym(sdk_handle, "IMV_" #name); \
            if (!pIMV_##name) { \
                DEBUG_LOG("警告: 找不到函数 IMV_" #name " - %s", dlerror()); \
            } else { \
                DEBUG_LOG("✓ 加载函数: IMV_" #name); \
            } \
        } while(0)
    
    LOAD_FUNC(GetVersion);
    LOAD_FUNC(EnumDevices);
    LOAD_FUNC(CreateHandle);
    LOAD_FUNC(DestroyHandle);
    LOAD_FUNC(OpenEx);
    LOAD_FUNC(Close);
    LOAD_FUNC(StartGrabbing);
    LOAD_FUNC(StopGrabbing);
    LOAD_FUNC(GetFrame);
    LOAD_FUNC(ReleaseFrame);
    LOAD_FUNC(AttachGrabbing);
    LOAD_FUNC(SetIntFeatureValue);
    LOAD_FUNC(GetIntFeatureValue);
    LOAD_FUNC(GetIntFeatureMax);
    LOAD_FUNC(GetIntFeatureMin);
    LOAD_FUNC(GetIntFeatureInc);
    LOAD_FUNC(SetDoubleFeatureValue);
    LOAD_FUNC(GetDoubleFeatureValue);
    LOAD_FUNC(GetDoubleFeatureMax);
    LOAD_FUNC(GetDoubleFeatureMin);
    LOAD_FUNC(SetEnumFeatureValue);
    LOAD_FUNC(GetEnumFeatureValue);
    LOAD_FUNC(ExecuteCommandFeature);
    LOAD_FUNC(FeatureIsWriteable);
    
    #undef LOAD_FUNC
    
    // 测试版本获取
    if (pIMV_GetVersion) {
        const char* version = pIMV_GetVersion();
        DEBUG_LOG("SDK版本: %s", version ? version : "未知");
    }
    
    // 检查必要函数
    if (!pIMV_EnumDevices || !pIMV_CreateHandle) {
        DEBUG_LOG("错误: 缺少必要的SDK函数");
        dlclose(sdk_handle);
        sdk_handle = NULL;
        return 0;
    }
    
    atomic_store(&sdk_loaded, 1);
    DEBUG_LOG("SDK加载完成");
    return 1;
}

// 确保SDK加载的辅助函数
static inline int EnsureSDKLoaded() {
    if (!atomic_load(&sdk_loaded)) {
        return LoadSDK();
    }
    return 1;
}

// ==================== API函数实现 ====================

int WRAP_IMV_CALL  WRAP_IMV_EnumDevices(WRAP_IMV_DeviceList* pDeviceList, unsigned int interfaceType) {
    if (!EnsureSDKLoaded() || !pIMV_EnumDevices) {
        DEBUG_LOG("SDK未加载或EnumDevices函数不可用");
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_EnumDevices(pDeviceList, interfaceType);
    DEBUG_LOG("原始SDK EnumDevices返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_CreateHandle(WRAP_IMV_HANDLE* handle, WRAP_IMV_ECreateHandleMode mode, void* pIdentifier) {
    if (!EnsureSDKLoaded() || !pIMV_CreateHandle) {
        DEBUG_LOG("SDK未加载或CreateHandle函数不可用");
        return WRAP_IMV_ERROR;
    }
    
    DEBUG_LOG("创建句柄，模式: %d", (int)mode);
    
    int result = pIMV_CreateHandle(handle, (int)mode, pIdentifier);
    DEBUG_LOG("CreateHandle返回: %d, 句柄: %p", result, handle ? *handle : NULL);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_DestroyHandle(WRAP_IMV_HANDLE handle) {
    if (!pIMV_DestroyHandle) {
        if (!EnsureSDKLoaded()) return WRAP_IMV_ERROR;
        if (!pIMV_DestroyHandle) return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_DestroyHandle(handle);
    DEBUG_LOG("DestroyHandle返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_OpenEx(WRAP_IMV_HANDLE handle, WRAP_IMV_ECameraAccessPermission accessPermission) {
    if (!EnsureSDKLoaded() || !pIMV_OpenEx) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_OpenEx(handle, (int)accessPermission);
    DEBUG_LOG("OpenEx返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_Close(WRAP_IMV_HANDLE handle) {
    if (!pIMV_Close) {
        if (!EnsureSDKLoaded()) return WRAP_IMV_ERROR;
        if (!pIMV_Close) return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_Close(handle);
    DEBUG_LOG("Close返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

bool WRAP_IMV_CALL WRAP_IMV_FeatureIsWriteable(WRAP_IMV_HANDLE handle, const char* pFeatureName) {
    if (!EnsureSDKLoaded() || !pIMV_FeatureIsWriteable) {
        return false;
    }
    
    int result = pIMV_FeatureIsWriteable(handle, pFeatureName);
    DEBUG_LOG("FeatureIsWriteable: %s, 结果: %d", pFeatureName, result);
    
    return (result != 0);
}

int WRAP_IMV_CALL  WRAP_IMV_SetIntFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, int64_t intValue) {
    if (!EnsureSDKLoaded() || !pIMV_SetIntFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_SetIntFeatureValue(handle, pFeatureName, intValue);
    DEBUG_LOG("SetIntFeatureValue: %s = %lld, 结果: %d", pFeatureName, intValue, result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetIntFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, int64_t* pIntValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetIntFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetIntFeatureValue(handle, pFeatureName, pIntValue);
    if (result == WRAP_IMV_OK && pIntValue) {
        DEBUG_LOG("GetIntFeatureValue: %s = %lld, 结果: %d", pFeatureName, *pIntValue, result);
    } else {
        DEBUG_LOG("GetIntFeatureValue: %s, 结果: %d", pFeatureName, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetIntFeatureMax(WRAP_IMV_HANDLE handle, const char* pFeatureName, int64_t* pIntValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetIntFeatureMax) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetIntFeatureMax(handle, pFeatureName, pIntValue);
    if (result == WRAP_IMV_OK && pIntValue) {
        DEBUG_LOG("GetIntFeatureMax: %s = %lld, 结果: %d", pFeatureName, *pIntValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetIntFeatureMin(WRAP_IMV_HANDLE handle, const char* pFeatureName, int64_t* pIntValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetIntFeatureMin) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetIntFeatureMin(handle, pFeatureName, pIntValue);
    if (result == WRAP_IMV_OK && pIntValue) {
        DEBUG_LOG("GetIntFeatureMin: %s = %lld, 结果: %d", pFeatureName, *pIntValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetIntFeatureInc(WRAP_IMV_HANDLE handle, const char* pFeatureName, int64_t* pIntValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetIntFeatureInc) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetIntFeatureInc(handle, pFeatureName, pIntValue);
    if (result == WRAP_IMV_OK && pIntValue) {
        DEBUG_LOG("GetIntFeatureInc: %s = %lld, 结果: %d", pFeatureName, *pIntValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_SetDoubleFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, double doubleValue) {
    if (!EnsureSDKLoaded() || !pIMV_SetDoubleFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_SetDoubleFeatureValue(handle, pFeatureName, doubleValue);
    DEBUG_LOG("SetDoubleFeatureValue: %s = %f, 结果: %d", pFeatureName, doubleValue, result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetDoubleFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, double* pDoubleValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetDoubleFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetDoubleFeatureValue(handle, pFeatureName, pDoubleValue);
    if (result == WRAP_IMV_OK && pDoubleValue) {
        DEBUG_LOG("GetDoubleFeatureValue: %s = %f, 结果: %d", pFeatureName, *pDoubleValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetDoubleFeatureMax(WRAP_IMV_HANDLE handle, const char* pFeatureName, double* pDoubleValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetDoubleFeatureMax) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetDoubleFeatureMax(handle, pFeatureName, pDoubleValue);
    if (result == WRAP_IMV_OK && pDoubleValue) {
        DEBUG_LOG("GetDoubleFeatureMax: %s = %f, 结果: %d", pFeatureName, *pDoubleValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetDoubleFeatureMin(WRAP_IMV_HANDLE handle, const char* pFeatureName, double* pDoubleValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetDoubleFeatureMin) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetDoubleFeatureMin(handle, pFeatureName, pDoubleValue);
    if (result == WRAP_IMV_OK && pDoubleValue) {
        DEBUG_LOG("GetDoubleFeatureMin: %s = %f, 结果: %d", pFeatureName, *pDoubleValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_SetEnumFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, uint64_t enumValue) {
    if (!EnsureSDKLoaded() || !pIMV_SetEnumFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_SetEnumFeatureValue(handle, pFeatureName, enumValue);
    DEBUG_LOG("SetEnumFeatureValue: %s = %lu, 结果: %d", pFeatureName, enumValue, result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_GetEnumFeatureValue(WRAP_IMV_HANDLE handle, const char* pFeatureName, uint64_t* pEnumValue) {
    if (!EnsureSDKLoaded() || !pIMV_GetEnumFeatureValue) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetEnumFeatureValue(handle, pFeatureName, pEnumValue);
    if (result == WRAP_IMV_OK && pEnumValue) {
        DEBUG_LOG("GetEnumFeatureValue: %s = %lu, 结果: %d", pFeatureName, *pEnumValue, result);
    }
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_ExecuteCommandFeature(WRAP_IMV_HANDLE handle, const char* pFeatureName) {
    if (!EnsureSDKLoaded() || !pIMV_ExecuteCommandFeature) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_ExecuteCommandFeature(handle, pFeatureName);
    DEBUG_LOG("ExecuteCommandFeature: %s, 结果: %d", pFeatureName, result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_StartGrabbing(WRAP_IMV_HANDLE handle) {
    if (!EnsureSDKLoaded() || !pIMV_StartGrabbing) {
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_StartGrabbing(handle);
    DEBUG_LOG("StartGrabbing返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_StopGrabbing(WRAP_IMV_HANDLE handle) {
    if (!pIMV_StopGrabbing) {
        if (!EnsureSDKLoaded()) return WRAP_IMV_ERROR;
        if (!pIMV_StopGrabbing) return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_StopGrabbing(handle);
    DEBUG_LOG("StopGrabbing返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL WRAP_IMV_AttachGrabbing(WRAP_IMV_HANDLE handle, 
                                         WRAP_IMV_FrameCallBack proc, 
                                         void* pUser) {
    if (!EnsureSDKLoaded() || !pIMV_AttachGrabbing) {
        return WRAP_IMV_ERROR;
    }
    
    DEBUG_LOG("AttachGrabbing: handle=%p, callback=%p, user_data=%p", 
             handle, proc, pUser);
    
    pthread_rwlock_wrlock(&camera_rwlock);
    
    // 检查是否已注册
    for (int i = 0; i < MAX_CAMERAS; i++) 
    {
        if (camera_contexts[i] && camera_contexts[i]->handle == handle) 
        {
            DEBUG_LOG("相机已注册，更新信息");
            CameraContext* ctx = camera_contexts[i];
            ctx->callback = proc;
            ctx->user_data = pUser;
            atomic_store(&ctx->active, 1);
            pthread_rwlock_unlock(&camera_rwlock);
            
            int result = pIMV_AttachGrabbing(handle, SDK_FrameCallback, handle);
            DEBUG_LOG("AttachGrabbing返回: %d", result);
            return result;
        }
    }
    
    // 注册新相机
    CameraContext* ctx = NULL;
    int camera_id = -1;
    
    for (int i = 0; i < MAX_CAMERAS; i++) 
    {
        if (!camera_contexts[i]) 
        {
            ctx = (CameraContext*)calloc(1, sizeof(CameraContext));
            if (!ctx) 
            {
                DEBUG_LOG("错误: 分配相机上下文内存失败");
                pthread_rwlock_unlock(&camera_rwlock);
                return WRAP_IMV_ERROR;
            }
            camera_contexts[i] = ctx;
            camera_id = i;
            break;
        }
    }
    
    if (!ctx) 
    {
        DEBUG_LOG("错误: 达到最大相机数量限制");
        pthread_rwlock_unlock(&camera_rwlock);
        return WRAP_IMV_ERROR;
    }
    
    // 初始化队列（容量根据帧率设置，这里使用10帧）
    #define QUEUE_CAPACITY 10
    
    if (!FrameQueue_Init(&ctx->queue, QUEUE_CAPACITY)) 
    {
        free(ctx);
        camera_contexts[camera_id] = NULL;
        pthread_rwlock_unlock(&camera_rwlock);
        return WRAP_IMV_ERROR;
    }
    
    // 初始化上下文
    ctx->handle = handle;
    ctx->callback = proc;
    ctx->user_data = pUser;
    atomic_store(&ctx->active, 1);
    atomic_store(&ctx->thread_exit, 0);
    atomic_store(&ctx->frames_processed, 0);
    atomic_store(&ctx->frames_received, 0);
    
    pthread_mutex_init(&ctx->queue_mutex, NULL);
    pthread_cond_init(&ctx->queue_not_empty, NULL);
    pthread_cond_init(&ctx->queue_not_full, NULL);
    
    // 启动处理线程
    #ifdef __WINE__
    DWORD thread_id;
    ctx->thread_handle = CreateThread(
        NULL,
        0,
        CameraProcessor,
        ctx,
        0,
        &thread_id
    );
    
    if (!ctx->thread_handle) 
    {
        DEBUG_LOG("错误: 无法创建处理器线程");
        FrameQueue_Destroy(&ctx->queue);
        pthread_cond_destroy(&ctx->queue_not_empty);
        pthread_cond_destroy(&ctx->queue_not_full);
        pthread_mutex_destroy(&ctx->queue_mutex);
        free(ctx);
        camera_contexts[camera_id] = NULL;
        pthread_rwlock_unlock(&camera_rwlock);
        return WRAP_IMV_ERROR;
    }
    #else
    if (pthread_create(&ctx->thread_id, NULL, CameraProcessor, ctx) != 0) 
    {
        DEBUG_LOG("错误: 无法创建处理器线程");
        FrameQueue_Destroy(&ctx->queue);
        pthread_cond_destroy(&ctx->queue_not_empty);
        pthread_cond_destroy(&ctx->queue_not_full);
        pthread_mutex_destroy(&ctx->queue_mutex);
        free(ctx);
        camera_contexts[camera_id] = NULL;
        pthread_rwlock_unlock(&camera_rwlock);
        return WRAP_IMV_ERROR;
    }
    #endif
    
    pthread_rwlock_unlock(&camera_rwlock);
    
    // 调用SDK注册回调
    int result = pIMV_AttachGrabbing(handle, SDK_FrameCallback, handle);
    DEBUG_LOG("AttachGrabbing返回: %d", result);
    
    return result;
}

int WRAP_IMV_CALL  WRAP_IMV_GetFrame(WRAP_IMV_HANDLE handle, WRAP_IMV_Frame* pFrame, unsigned int timeoutMS) {
    if (!EnsureSDKLoaded() || !pIMV_GetFrame) {
        return WRAP_IMV_ERROR;
    }
    
    if (!pFrame) {
        DEBUG_LOG("错误: pFrame为空指针");
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_GetFrame(handle, pFrame, timeoutMS);
    DEBUG_LOG("GetFrame返回: %d, timeout=%u", result, timeoutMS);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

int WRAP_IMV_CALL  WRAP_IMV_ReleaseFrame(WRAP_IMV_HANDLE handle, WRAP_IMV_Frame* pFrame) {
    if (!pIMV_ReleaseFrame) {
        if (!EnsureSDKLoaded()) return WRAP_IMV_ERROR;
        if (!pIMV_ReleaseFrame) return WRAP_IMV_ERROR;
    }
    
    if (!pFrame) {
        DEBUG_LOG("错误: pFrame为空指针");
        return WRAP_IMV_ERROR;
    }
    
    int result = pIMV_ReleaseFrame(handle, pFrame);
    DEBUG_LOG("ReleaseFrame返回: %d", result);
    
    return (result == WRAP_IMV_OK) ? WRAP_IMV_OK : WRAP_IMV_ERROR;
}

// ==================== 清理函数 ====================

void WRAP_IMV_Cleanup() 
{
    DEBUG_LOG("清理SDK资源");

    pthread_rwlock_wrlock(&camera_rwlock);
    
    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (camera_contexts[i]) {
            CameraContext* ctx = camera_contexts[i];
            
            // 停止线程
            atomic_store(&ctx->thread_exit, 1);
            atomic_store(&ctx->active, 0);
            
            // 唤醒线程以便退出
            pthread_mutex_lock(&ctx->queue_mutex);
            pthread_cond_broadcast(&ctx->queue_not_empty);
            pthread_cond_broadcast(&ctx->queue_not_full);
            pthread_mutex_unlock(&ctx->queue_mutex);
            
            #ifdef __WINE__
            if (ctx->thread_handle) {
                WaitForSingleObject(ctx->thread_handle, 1000);
                CloseHandle(ctx->thread_handle);
                ctx->thread_handle = NULL;
            }
            #else
            if (ctx->thread_id) {
                pthread_join(ctx->thread_id, NULL);
                ctx->thread_id = 0;
            }
            #endif
            
            // 打印最终统计
            DEBUG_LOG("相机[%p]统计: 接收=%ld, 处理=%ld, 丢弃=%d",
                     ctx->handle,
                     atomic_load(&ctx->frames_received),
                     atomic_load(&ctx->frames_processed),
                     atomic_load(&ctx->queue.dropped_frames));
            
            // 销毁队列
            FrameQueue_Destroy(&ctx->queue);
            
            pthread_mutex_destroy(&ctx->queue_mutex);
            pthread_cond_destroy(&ctx->queue_not_empty);
            pthread_cond_destroy(&ctx->queue_not_full);
            
            free(ctx);
            camera_contexts[i] = NULL;
        }
    }
    
    pthread_rwlock_unlock(&camera_rwlock);
    pthread_rwlock_destroy(&camera_rwlock);
    
    if (sdk_handle) {
        dlclose(sdk_handle);
        sdk_handle = NULL;
    }
    
    atomic_store(&sdk_loaded, 0);
    user_callback = NULL;
    
    // 重置所有函数指针
    pIMV_GetVersion = NULL;
    pIMV_EnumDevices = NULL;
    pIMV_CreateHandle = NULL;
    pIMV_DestroyHandle = NULL;
    pIMV_OpenEx = NULL;
    pIMV_Close = NULL;
    pIMV_StartGrabbing = NULL;
    pIMV_StopGrabbing = NULL;
    pIMV_GetFrame = NULL;
    pIMV_ReleaseFrame = NULL;
    pIMV_AttachGrabbing = NULL;
    pIMV_SetIntFeatureValue = NULL;
    pIMV_GetIntFeatureValue = NULL;
    pIMV_GetIntFeatureMax = NULL;
    pIMV_GetIntFeatureMin = NULL;
    pIMV_GetIntFeatureInc = NULL;
    pIMV_SetDoubleFeatureValue = NULL;
    pIMV_GetDoubleFeatureValue = NULL;
    pIMV_GetDoubleFeatureMax = NULL;
    pIMV_GetDoubleFeatureMin = NULL;
    pIMV_SetEnumFeatureValue = NULL;
    pIMV_GetEnumFeatureValue = NULL;
    pIMV_ExecuteCommandFeature = NULL;
    pIMV_FeatureIsWriteable = NULL;
    
    DEBUG_LOG("SDK已卸载");
}
