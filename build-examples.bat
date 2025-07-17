mkdir build
cmake -S . -B build -DCMAKE_CXX_STANDARD=17 > log.txt 2>&1
cmake --build build >> log.txt 2>&1
pause