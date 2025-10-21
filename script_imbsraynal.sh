#!/bin/bash

#OAR -l {host='big17'}/core=48,walltime=1000:0:0


source /etc/profile.d/modules.sh		# Shell initialization to use module
module purge							# Environment cleanup
module load python/anaconda3			# Loading of anaconda 3 module

python run_tests.py --alg "imbsraynal"
