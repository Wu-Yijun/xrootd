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
            # "-Wl,-rpath,'$$ORIGIN/../../deps/xrootd/lib'",
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
            "-L<(module_root_dir)/deps/xrootd/lib",
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