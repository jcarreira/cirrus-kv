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
