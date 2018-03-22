rm -f bundle.zip
cp ../benchmarking/outputs/s3_get .
zip -9 bundle.zip s3_get
zip -9 bundle.zip handler.py
rm -f s3_get
