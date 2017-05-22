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
cd ../..

# main compilation
autoreconf
automake --add-missing
autoreconf
./configure
