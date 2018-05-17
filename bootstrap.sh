# get third party libs
git submodule init
git submodule update

# compile libcuckoo
cd third_party/libcuckoo
cmake .
make -j 10

# wget and compile keyutils
cd ..
wget http://people.redhat.com/~dhowells/keyutils/keyutils-1.5.10.tar.bz2
tar xvjf keyutils-1.5.10.tar.bz2
rm keyutils-1.5.10.tar.bz2
cd keyutils-1.5.10
make -j 10
make install

#wget and compile krb5
cd ..
wget https://kerberos.org/dist/krb5/1.15/krb5-1.15.2.tar.gz
tar xvzf krb5-1.15.2.tar.gz
rm krb5-1.15.2.tar.gz
cd krb5-1.15.2/src
./configure --disable-shared --enable-static
make -j 10
cd lib
mkdir -p build && cd build
ar -x ../libapputils.a
ar -x ../libcom_err.a
ar -x ../libgssapi_krb5.a
ar -x ../libgssrpc.a
ar -x ../libk5crypto.a
ar -x ../libkadm5clnt_mit.a
ar -x ../libkadm5srv_mit.a
ar -x ../libkdb5.a
ar -x ../libkrb5.a
ar -x ../libkrad.a
ar -x ../libkrb5_db2.a
ar -x ../libkrb5_k5audit_test.a
ar -x ../libkrb5_k5tls.a
ar -x ../libkrb5_otp.a
ar -x ../libkrb5_pkinit.a
ar -x ../libkrb5support.a
ar -x ../libkrb5_test.a
ar -x ../libss.a
cd ..
ar -qc liball.a build/*.o
rm -rf build
cd ..
cd ..

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

#compile gflags
cd ../gflags
cmake ../gflags -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DBUILD_gflags_LIB=ON
make -j 10

#compile glog
cd ../glog
cmake ../glog
make -j 10

#compile aws-sdk-cpp
cd ../aws-sdk-cpp
cmake ../aws-sdk-cpp -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;core"
make -j 10

#compile libevent
cd ../libevent
automake --add-missing
autoconf
cmake ../libevent
./configure --disable-shared
make -j 10

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
