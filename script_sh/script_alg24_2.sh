#!/bin/bash

#OAR -l { host in ('big16','big17','big18', 'big19', 'big20')}/core=48,walltime=1000:0:0

source /etc/profile.d/modules.sh		# Shell initialization to use module
module purge							# Environment cleanup
module load python/anaconda3			# Loading of anaconda 3 module

python run_tests_groups.py --alg "alg24" --list '[["Alg24Peer", "alg24_opposite_same_same.json"], ["Alg24Peer", "alg24_opposite_same_silent.json"], ["Alg24Peer", "alg24_opposite_silent_opposite.json"], ["Alg24Peer", "alg24_opposite_silent_same.json"]]'