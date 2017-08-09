# get third party libs
git submodule init
git submodule update

# compile libcuckoo
cd third_party/libcuckoo
autoreconf -fis
./configure
make -j 10

# Rename the shared cityhash library so that it is not recognized by the loader
# We need to do this because on Mac, there is no way to choose a static
# library over a dynamic one of the same name in the same directory without
# using the absolute path.

# TODO: This code block may cause errors on linux, check to be sure
# Code from goo.gl/ZSqX9o
if [ "$(uname)" == "Darwin" ]; then
    # Rename the library on Mac.
    # Mac uses .dylib extension
    mv cityhash-1.1.1/src/.libs/libcityhash.dylib cityhash-1.1.1/src/.libs/ignorelibcityhash.dylib
    mv cityhash-1.1.1/src/.libs/libcityhash.0.dylib cityhash-1.1.1/src/.libs/ignorelibcityhash.0.dylib
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    # Rename the library on Linux.
    # Linux uses .so extension for dynamic libraries
    mv cityhash-1.1.1/src/.libs/libcityhash.so cityhash-1.1.1/src/.libs/ignorelibcityhash.so
    mv cityhash-1.1.1/src/.libs/libcityhash.so.0 cityhash-1.1.1/src/.libs/ignorelibcityhash.so.0
    mv cityhash-1.1.1/src/.libs/libcityhash.so.0.0.0 cityhash-1.1.1/src/.libs/ignorelibcityhash.so.0.0.0
fi

# compile sparsehash
cd ../sparsehash
./configure
make -j 10

# compile flatbuffers
cd ../flatbuffers
cmake -G "Unix Makefiles"
make -j 10

# download eigen
cd ../
if [ ! -d "eigen_source" ]; then
  sh get_eigen.sh
fi
cd ..



# main compilation
touch config.rpath
autoreconf
automake --add-missing
autoreconf
./configure
