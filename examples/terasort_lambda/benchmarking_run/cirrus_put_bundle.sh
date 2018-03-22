rm -f bundle.zip
cp ../benchmarking/outputs/cirrus_put .
zip -9 bundle.zip cirrus_put
zip -9 bundle.zip handler.py
rm -f cirrus_put
