echo "------ BUILDING CORE LIBRARY      ------"
cd movidacore && make
echo "------ BUILDING EXTENSION LIBRARY ------"
cd ../movidawidgets && make
echo "------ BUILDING MOVIDA            ------"
cd ../movida && make
echo "------ BUILDING BASIC.MPI PLUGIN  ------"
cd ../basicmpi && make
cd ..
echo "------ DONE!                      ------"
