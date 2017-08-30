# get third party libs
git submodule init
git submodule update

# compile libcuckoo
cd third_party/libcuckoo
autoreconf -fis
./configure
make -j 10

# compile sparsehash
cd ../sparsehash
./configure
make -j 10

# compile flatbuffers
cd ../flatbuffers
cmake -G "Unix Makefiles"
make -j 10

# rocksdb
cd ../rocksdb
make -j 10 static_lib
cd ../..

# main compilation
touch config.rpath
autoreconf
automake --add-missing
autoreconf
./configure
