import pathlib 
import os 
import subprocess
import threading
import time
import signal
import sys
from queue import Queue
import colorama 
import argparse 
import json

parser = argparse.ArgumentParser(description="Run selected algorithm tests.")
parser.add_argument(
    "--alg",
    type=str,
    help="Algorithm to run (alg23, alg24, bracha, imbsraynal, cool)",
)
args = parser.parse_args()


DIR_SCRIPT_SH = pathlib.Path(__file__).parent.parent / "script_sh"
MAX_CONCURRENCY = 4
ALGORITHMS = [
    ("BrachaPeer", "bracha.json"),
    ("Alg23Peer", "alg23.json"),
    ("Alg24Peer", "alg24.json"),
    ("ImbsRaynalPeer", "imbsraynal.json"),
    ("COOLPeer", "cool.json")
]

ALGORITHMS_TO_RUN = []
def get_tests():
    global ALGORITHMS_TO_RUN
    ALGORITHMS_TO_RUN.clear()
    alg_filter = args.alg
    directory_alg23 = pathlib.Path(__file__).parent.parent / "quantas" / "Alg23Peer"
    directory_alg24 = pathlib.Path(__file__).parent.parent / "quantas" / "Alg24Peer"
    directory_bracha = pathlib.Path(__file__).parent.parent / "quantas" / "BrachaPeer"
    directory_imbsraynal = pathlib.Path(__file__).parent.parent / "quantas" / "ImbsRaynalPeer"
    directory_cool = pathlib.Path(__file__).parent.parent / "quantas" / "COOLPeer"
    dirs = []
    if alg_filter == "alg23":
        dirs = [(directory_alg23, "Alg23Peer")]
    elif alg_filter == "alg24":
        dirs = [(directory_alg24, "Alg24Peer")]
    elif alg_filter == "bracha":
        dirs = [(directory_bracha, "BrachaPeer")]
    elif alg_filter == "imbsraynal":
        dirs = [(directory_imbsraynal, "ImbsRaynalPeer")]
    elif alg_filter == "cool":
        dirs = [(directory_cool, "COOLPeer")]

    for directory, alg_class in dirs:
        json_files = sorted([f for f in os.listdir(directory) if f.endswith(".json") and "test" not in f])
        for json_file in json_files:
            ALGORITHMS_TO_RUN.append((alg_class, json_file))
    
    print(f"Found {len(ALGORITHMS_TO_RUN)} tests to run for algorithm filter '{alg_filter}'.")

get_tests()
json_str = json.dumps(ALGORITHMS_TO_RUN)
print(f"Tests to run: {json_str}")
print("\n\n\n")

text = """
#!/bin/bash

#OAR -l { host in ('big16','big17','big18', 'big19', 'big20')}/core=48,walltime=1000:0:0

source /etc/profile.d/modules.sh		# Shell initialization to use module
module purge							# Environment cleanup
module load python/anaconda3			# Loading of anaconda 3 module

python run_tests_groups.py --alg "alg24" --list '<xxx>'
""".strip()

i = 0
while True:
    start = i*MAX_CONCURRENCY
    end = start + MAX_CONCURRENCY
    batch = ALGORITHMS_TO_RUN[start:end]
    if batch == []:
        break
    batch_str = json.dumps(batch)
    print(f"Running batch: {batch_str}\n\n")
    i+=1
    
    script_path = DIR_SCRIPT_SH / f"script_{args.alg}_{i}.sh"
    script_text = text.replace("<xxx>", batch_str)
    
    with open(script_path, "w") as f:
        f.write(script_text)
        
    script_path.chmod(0o755)

