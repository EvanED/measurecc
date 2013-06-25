clang -emit-llvm -c blank.cc
clang -emit-llvm -c timer.cc

opt -load libnwa-output.so -hello -measure-spec funcs.txt   blank.o -o blank-rev.o  && /unsup/llvm-3.2/bin/llvm-dis blank-rev.o

llvm-link blank.o timer.o -o plain.o
llvm-link blank-rev.o timer.o -o fancy.o

llc -o plain.s plain.o
llc -o fancy.s fancy.o

g++ -lrt -o plain plain.s
g++ -lrt -o fancy fancy.s

echo "----- PLAIN -----"
./plain
echo "----- FANCY -----"
./fancy
