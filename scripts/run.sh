#!/bin/bash

map_fname1="res_tst1_map.map"
map_fname2="res_tst2_map.map"
map_fname3="res_tst3_map.map"

rm -f $map_fname1;
rm -f $map_fname2;
rm -f $map_fname3;

for i in `seq 1 8` ; do
    sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_'$i'/g' ./tst1.job;
    sed -i 's/\(\.\/.*out\) \(.*\)/\1 y '$i'/g' ./tst1.job;
    sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_'$i'/g' ./tst2.job;
    sed -i 's/\(\.\/.*out\) \(.*\)/\1 y '$i'/g' ./tst2.job;
    sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_'$i'/g' ./tst3.job;
    sed -i 's/\(\.\/.*out\) \(.*\)/\1 y '$i'/g' ./tst3.job;
    str1=""
    str2=""
    str3=""
    for j in `seq 1 20` ; do
#        str1="$str1 tst1_$i.o`qsub tst1.job | sed -n 's/\([0-9]*\).*/\1/p'`";
        str2="$str2 tst2_$i.o`qsub tst2.job | sed -n 's/\([0-9]*\).*/\1/p'`";
        str3="$str3 tst3_$i.o`qsub tst3.job | sed -n 's/\([0-9]*\).*/\1/p'`";
    done
#    echo $str1 >> $map_fname1;
    echo $str2 >> $map_fname2;
    echo $str3 >> $map_fname3;
done;
