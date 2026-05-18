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
        "src/workers/CopyWorker.cpp",
        "src/workers/FileSystemWorkers.cpp",
        "src/workers/FileWorkers.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/deps/xrootd/include/xrootd",
      ],
      "cflags_cc": [
        "-fno-exceptions",
        "-std=c++17",
        "-frtti"
      ],
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            "-L<(module_root_dir)/libs/linux-<(target_arch)",
            "-lXrdCl",
            "-lXrdUtils",
            "-Wl,-rpath,'$$ORIGIN/../../libs/linux-<(target_arch)'"
            # "<(module_root_dir)/deps/xrootd/lib/libXrdCrypto.a",
            # 如果 XRootD 静态库内部依赖了其他系统库，你需要在这里显式链接
            # "-lpthread",
            # "-ldl",
            # "-lz",
            # "-lrt",
            # "-lssl",
            # "-lcrypto",
            # "-luuid"
          ]
        }],
        ["OS=='mac'", {
          "libraries": [
            # Mac 的编译时链接
            "-L<(module_root_dir)/libs/mac-<(target_arch)",
            "-lXrdCl",
            "-lXrdUtils",
            
            # Mac 的运行时寻找 (使用 @loader_path 代替 $$ORIGIN)
            "-Wl,-rpath,'@loader_path/../../libs/mac-<(target_arch)'"

            # Mac 上的系统依赖 ?? 
            # "-framework CoreFoundation",
            # "-framework Security"
          ]
        }]
      ]
    }
  ]
}