#rm -Rf build
#mkdir build
cd build
cmake -DSIMULATED=OFF ..
make
mv ./client_pi_main ..
cd ..