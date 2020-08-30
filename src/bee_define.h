#ifndef __BEE_DEFINE_H__
#define __BEE_DEFINE_H__

#ifdef WIN32
#ifdef BEE_USE_AS_DLL
#ifdef BEE_EXPORTS
#define BEE_API __declspec(dllexport)
#else
#define BEE_API __declspec(dllimport)
#endif
#else
#define BEE_API
#endif
#define BEE_CALL __stdcall
#define BEE_CALLBACK __stdcall
#else
#define BEE_API
#define BEE_CALL
#define BEE_CALLBACK
#endif

//////////////////////////////////////////////////////////////////////////
// C Standards
//////////////////////////////////////////////////////////////////////////
#if defined(__STDC__)
#define PREDEF_STANDARD_C_1989
#if defined(__STDC_VERSION__)
#define PREDEF_STANDARD_C_1990
#if (__STDC_VERSION__ >= 199409L)
#define PREDEF_STANDARD_C_1994
#endif
#if (__STDC_VERSION__ >= 199901L)
#define PREDEF_STANDARD_C_1999
#endif
#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Pre-C89 compilers do not recognize certain keywords.
// Let the preprocessor remove those keywords for those compilers.
//////////////////////////////////////////////////////////////////////////
#if !defined(PREDEF_STANDARD_C_1989) && !defined(__cplusplus)
#define const
#define volatile
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

/// 错误码.
typedef enum BeeErrorCode {
  kBeeErrorCode_Success = 0,         //!< 成功.
  kBeeErrorCode_Invalid_Param,       //!< 非法参数.
  kBeeErrorCode_Invalid_State,       //!< 非法状态.
  kBeeErrorCode_Null_Pointer,        //!< 空指针.
  kBeeErrorCode_Not_Implemented,     //!< 未实现.
  kBeeErrorCode_Timeout,             //!< 超时.
  kBeeErrorCode_Not_Connected,       //!< 未连接.
  kBeeErrorCode_Error_Http_Status,   //!< 错误的Http状态码.
  kBeeErrorCode_Error_Data,          //!< 错误数据.
  kBeeErrorCode_Resolve_Fail,        //!< DNS解析失败.
  kBeeErrorCode_Connect_Fail,        //!< 连接失败.
  kBeeErrorCode_SSL_Handshake_Fail,  //!< SSL握手失败.
  kBeeErrorCode_Ws_Handshake_Fail,   //!< WebSocket握手失败.
  kBeeErrorCode_Write_Fail,          //!< 写数据失败.
  kBeeErrorCode_Read_Fail,           //!< 读数据失败.
  kBeeErrorCode_Crypto_Error,        //!< 加解密错误.
  kBeeErrorCode_Not_Enough_Memory,   //!< 内存不足.
  kBeeErrorCode_Count                //!< 错误码总数.
} BeeErrorCode;

/// 平台类型.
typedef enum BeePlatformType {
  kPlatformType_None = 0,        //!< 空类型.
  kPlatformType_PC,              //!< Windows电脑.
  kPlatformType_Mac,             //!< 苹果电脑.
  kPlatformType_IPhone,          //!< 苹果手机.
  kPlatformType_IPad,            //!< 苹果平板.
  kPlatformType_Android_Phone,   //!< Android手机.
  kPlatformType_Android_Pad,     //!< Android平板.
  kPlatformType_Android_TV,      //!< Android电视.
  kPlatformType_Android_Router,  //!< Android路由器.
  kPlatformType_Android_Box      //!< Android盒子.
} BeePlatformType;

/// 网络类型.
typedef enum BeeNetType {
  kNetType_None = 0,  //!< 无网络.
  kNetType_WireLine,  //!< 有线网络.
  kNetType_Wifi,      //!< 无线局域网.
  kNetType_2G,        //!< 2G网络.
  kNetType_3G,        //!< 3G网络.
  kNetType_4G,        //!< 4G网络.
  kNetType_5G         //!< 5G网络.
} BeeNetType;

#endif  // __BEE_DEFINE_H__
