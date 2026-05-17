// src/addon.cpp
#include <napi.h>

// 引入你的核心包装类
#include "core/XrdNodeFile.h"
#include "core/XrdNodeFileSystem.h"
#include "core/XrdNodeCopyProcess.h"
#include "core/XrdNodeEnv.h"
// #include "core/XrdNodeUrl.h"

// 模块初始化函数
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    // 依次注册所有的 JS 类和静态方法
    XrdNodeFile::Init(env, exports);
    XrdNodeFileSystem::Init(env, exports);
    XrdNodeCopyProcess::Init(env, exports);
    // XrdNodeUrl::Init(env, exports);
    
    // 环境变量直接作为静态方法挂载到 exports 上
    XrdNodeEnv::Init(env, exports);

    return exports;
}

// 宏注册：将 InitAll 绑定到 node-gyp target 名字上 (这里名字必须是 xrootd_bindings)
NODE_API_MODULE(xrootd_bindings, InitAll)