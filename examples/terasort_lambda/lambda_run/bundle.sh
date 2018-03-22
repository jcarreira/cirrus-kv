rm -f bundle.zip
cp ../lambda/outputs/main .
zip -9 bundle.zip main
zip -9 bundle.zip handler.py
rm -f main
