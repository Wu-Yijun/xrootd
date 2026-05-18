rm -r xrootd-src/build/
git clone --depth 1 --branch v6.0.1 https://github.com/xrootd/xrootd.git xrootd-src
mkdir xrootd-src/build && cd xrootd-src/build/
        
# ================= 黑魔法注入开始 =================
# 在 project( XRootD ) 之后强行插入一个宏，将所有的 SHARED 覆盖为 STATIC
sed -i '/project( XRootD )/a \
macro(add_library name)\
    set(ARGS ${ARGN})\
    list(FIND ARGS "SHARED" idx)\
    if(idx GREATER -1)\
    list(REMOVE_AT ARGS ${idx})\
    list(INSERT ARGS ${idx} "STATIC")\
    endif()\
    _add_library(${name} ${ARGS})\
endmacro()' ../CMakeLists.txt
# ================= 黑魔法注入结束 =================


cmake .. \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/../../deps/xrootd \
    -DBUILD_SHARED_LIBS=OFF \
    -DXRDCL_ONLY=ON \
    -DENABLE_SERVER=OFF \
    -DENABLE_PYTHON=OFF \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    
cmake --build . -j 8
cmake --install .

export CC=$(which gcc)
export CXX=$(which g++)

# 编译完成后清理源码，节省缓存空间
cd ../..
# rm -rf xrootd-src

##################################################################

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

cmake --build . -j 8
cmake --install .
