{
  "targets": [
    {
      "target_name": "xrootd_bindings",
      "sources": [
        "src/addon.cpp",
        "src/utils/type_conversions.cpp",
        "src/core/XrdNodeCopyProcess.cpp",
        "src/core/XrdNodeEnv.cpp",
        "src/core/XrdNodeFile.cpp",
        "src/core/XrdNodeFileSystem.cpp",
        "src/core/XrdNodeUrl.cpp",
        "src/workers/CopyWorker.cpp",
        "src/workers/FileSystemWorkers.cpp",
        "src/workers/FileWorkers.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/deps/xrootd/include/xrootd",
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc": [
        "-std=c++17",
        "-frtti"  # <--- 关键：强制开启 RTTI 选项
      ],
      "cflags_cc!": [ 
        "-fno-exceptions",
        "-fno-rtti"  # <--- 关键：从默认配置中强行移除“禁用RTTI”选项
      ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++"
      },
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            # 绝对关键：使用绝对路径或相对路径强制链接静态库 .a
            # 这样所有的 XRootD 逻辑都会被 "揉进" 最终的 .node 文件里
            "<(module_root_dir)/deps/xrootd/lib/libxrootd-client.a",
            "<(module_root_dir)/deps/xrootd/lib/libXrdUtils.a",
            "<(module_root_dir)/deps/xrootd/lib/libXrdCl.a",
            # "<(module_root_dir)/deps/xrootd/lib/libXrdPosix.a",
            # "<(module_root_dir)/deps/xrootd/lib/libXrdCrypto.a",
            # "<(module_root_dir)/deps/xrootd/lib/libXrdSec.a",
            # "<(module_root_dir)/deps/xrootd/lib/libXrdUtils.a",
            # "<(module_root_dir)/deps/xrootd/lib/libXrdSys.a",
            # 如果 XRootD 静态库内部依赖了其他系统库，你需要在这里显式链接
            "-lpthread",
            "-ldl",
            "-lz",
            "-lrt",
            "-lssl",
            "-lcrypto",
            "-luuid"
          ]
        }],
        ["OS=='mac'", {
          "libraries": [
            "<(module_root_dir)/deps/xrootd/lib/libxrootd-client.a",
            "<(module_root_dir)/deps/xrootd/lib/libXrdUtils.a",
            "<(module_root_dir)/deps/xrootd/lib/libXrdCl.a",
            # Mac 上的系统依赖
            "-framework CoreFoundation",
            "-framework Security"
          ]
        }]
      ]
    }
  ]
}