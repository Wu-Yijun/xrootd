{
  "targets": [
    {
      "target_name": "xrootd",
      "sources": [
        "src/addon.cpp",
        "src/utils/type_conversions.cpp",
        "src/core/XrdNodeCopyProcess.cpp",
        "src/core/XrdNodeEnv.cpp",
        "src/core/XrdNodeFile.cpp",
        "src/core/XrdNodeFileSystem.cpp",
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/deps/xrootd/include/xrootd",
        "<(module_root_dir)/src",
      ],
      "cflags_cc!": [
        "-fno-exceptions",
      ],
      "cflags_cc": [
        "-std=c++17",
        "-frtti"
      ],
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            "-L<(module_root_dir)/deps/xrootd/lib",
            "-lXrdCl",
            # 运行期：告诉 .node 文件去发布包的 libs 目录下找物理文件
            "-Wl,--disable-new-dtags",
            "-Wl,-rpath,'$$ORIGIN/../../libs/linux-<(target_arch)'",
          ]
        }],
        ["OS=='mac'", {
          "libraries": [
            # Mac 编译期链接阶段核心库
            "-L<(module_root_dir)/deps/xrootd/lib",
            "-lXrdCl",
            "-lXrdUtils",
            
            # Mac 运行期寻址：使用 @loader_path 寻址到分发依赖包目录
            # 注意：此处文件夹名称设为 darwin- 与 CI 矩阵逻辑保持严格一致
            "-Wl,-rpath,'@loader_path/../../libs/darwin-<(target_arch)'"
          ],
          "xcode_settings": {
            # 确保 Xcode 编译器使用 C++17 标准
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
            "CLANG_CXX_LIBRARY": "libc++",
            
            # 显式开启 RTTI 和异常处理（对应 Linux 的 cflags 调整）
            "GCC_ENABLE_CPP_RTTI": "YES",
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            
            # 设置最低兼容目标。设为 13.0 确保在 macos-14 (ARM) 上编译时，
            # 产物在 macos-13 系统上也不会因底层系统库版本过高而崩溃
            "MACOSX_DEPLOYMENT_TARGET": "13.0"
          }
        }]
      ]
    }
  ]
}