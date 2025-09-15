mkdir build
cmake -S . -B build -DLOGIT_CPP_BUILD_EXAMPLES=ON -DCMAKE_CXX_STANDARD=11 > log.txt 2>&1
cmake --build build >> log.txt 2>&1
pause