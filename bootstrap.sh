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
cd ../..

# rocksdb
cd ../rocksdb
make -j 10

# main compilation
touch config.rpath
autoreconf
automake --add-missing
autoreconf
./configure
