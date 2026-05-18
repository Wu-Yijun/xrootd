rm -r xrootd-src/build/
git clone --depth 1 --branch v6.0.1 https://github.com/xrootd/xrootd.git xrootd-src
mkdir xrootd-src/build && cd xrootd-src/build/

cmake .. \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/../../deps/xrootd \
    -DXRDCL_ONLY=ON \
    -DENABLE_SERVER=OFF \
    -DENABLE_PYTHON=OFF \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON

cmake .. \
    -DCMAKE_INSTALL_PREFIX=${GITHUB_WORKSPACE}/deps/xrootd \
    -DXRDCL_ONLY=ON \
    -DENABLE_SERVER=OFF \
    -DENABLE_PYTHON=OFF \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

cmake --build . -j 8
cmake --install .


# 编译完成后清理源码，节省缓存空间
cd ../..
rm -rf xrootd-src

# 进入你的库目录
cd deps/xrootd/lib

# 剥离所有 .so 文件的调试信息和非导出符号
# 注意：这不会影响动态链接的运行，只会让文件变小
strip --strip-unneeded *.so


mkdir -p libs/linux-x64
# 只复制一份实体文件，去纹身、去版本号
# 1. 强行把它们自身的 SONAME 改为不带版本号的名字
# 1. 客户端核心基础库
cp deps/xrootd/lib/libXrdCl.so.6.0.1     libs/linux-x64/libXrdCl.so.6
cp deps/xrootd/lib/libXrdUtils.so.6.0.1  libs/linux-x64/libXrdUtils.so.6
cp deps/xrootd/lib/libXrdXml.so.6.0.1    libs/linux-x64/libXrdXml.so.6
# 2. 密码学与安全框架核心
cp deps/xrootd/lib/libXrdSec-6.so        libs/linux-x64/
cp deps/xrootd/lib/libXrdSecProt-6.so    libs/linux-x64/
cp deps/xrootd/lib/libXrdCrypto.so.6.0.1 libs/linux-x64/libXrdCrypto.so.6
cp deps/xrootd/lib/libXrdCryptossl-6.so  libs/linux-x64/libXrdCryptossl-6.so
# 3. 核心认证插件 (Auth Plugins)
cp deps/xrootd/lib/libXrdSeckrb5-6.so    libs/linux-x64/
cp deps/xrootd/lib/libXrdSecunix-6.so  libs/linux-x64/
cp deps/xrootd/lib/libXrdSecsss-6.so    libs/linux-x64/
cp deps/xrootd/lib/libXrdSecpwd-6.so    libs/linux-x64/
cp deps/xrootd/lib/libXrdSecztn-6.so    libs/linux-x64/
# cp deps/xrootd/lib/*.so.6.0.1    libs/linux-x64/*.so.6
# cp deps/xrootd/lib/*.so.6.0.1    libs/linux-x64/*.so.6
# cp deps/xrootd/lib/*.so.6.0.1    libs/linux-x64/*.so.6
# cp deps/xrootd/lib/*.so.6.0.1    libs/linux-x64/*.so.6

# sudo apt-get install -y patchelf
# 让该目录下所有的 .so 运行时都能在同级目录下找到彼此
# 这条命令是 O(n) 的，无论增加多少个库，都只需要这一行！
for f in libs/linux-x64/*.so*; do
    patchelf --set-rpath '$ORIGIN' "$f"
done