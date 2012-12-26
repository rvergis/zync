xcodebuild -target ztool -configuration Release
cp ./build/Release/libztool.dylib /usr/local/lib
gcc -I ./build/Release/usr/local/include/ -o zmake/zmake zmake/zmake.c /usr/local/lib/libztool.dylib
