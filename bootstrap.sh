# get third party libs
git submodule init
git submodule update

# compile libcuckoo
cd third_party/libcuckoo
cmake .
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

# download eigen
cd ../
if [ ! -d "eigen_source" ]; then
  sh get_eigen.sh
fi
cd ..



# download test file for benchmark
cd ./benchmarks
wget http://norvig.com/big.txt
for i in {1..16};do cat big.txt >> very_big.txt; done
rm ./big.txt
cd ..

# main compilation
touch config.rpath
autoreconf
automake --add-missing
autoreconf
./configure
