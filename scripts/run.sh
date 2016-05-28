#!/bin/bash

sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_n/' ./tst1.job;
sed -i 's/\(\.\/.*out\) \(.*\)/\1 n/' ./tst1.job;
sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_n/' ./tst2.job;
sed -i 's/\(\.\/.*out\) \(.*\)/\1 n/' ./tst2.job;

qsub tst1.job
qsub tst2.job

for i in 1 2 4 8 ; do
    sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_'$i'/g' ./tst1.job;
    sed -i 's/\(\.\/.*out\) \(.*\)/\1 y '$i'/g' ./tst1.job;
    sed -i 's/\(\#PBS -N\) \(tst.\)\(_.*\)/\1 \2_'$i'/g' ./tst2.job;
    sed -i 's/\(\.\/.*out\) \(.*\)/\1 y '$i'/g' ./tst2.job;
    qsub tst1.job
    qsub tst2.job
done
