#rm -Rf build
#mkdir build
cd build
cmake -DSIMULATED=ON ..
make
cd ..
mv ./build/client_pi_main .
