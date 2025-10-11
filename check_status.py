import pathlib 
import os 
import subprocess
import threading
import time
import signal
import sys

status_file = pathlib.Path(__file__).parent / "status.txt"
def reader_status():
    with open(status_file, "r") as f:
        lines = f.readlines()
    tests = 36*100
    alg23 = 0
    alg24 = 0
    bracha = 0
    imbsraynal = 0
    for line in lines:
        line = line.strip().replace('"',"")
        if line == "alg23":
            alg23 += 1
        elif line == "alg24":
            alg24 += 1
        elif line == "bracha":
            bracha += 1
        elif line == "imbsraynal":
            imbsraynal += 1
    perc_alg23 = int((alg23/tests)*100)
    perc_alg24 = int((alg24/tests)*100)
    perc_bracha = int((bracha/tests)*100)
    perc_imbsraynal = int((imbsraynal/tests)*100)

    os.system('clear')
    print("Status so far:")
    print(f"{'alg23':<12}: [{'#'*perc_alg23}{'.'*(100-perc_alg23)}] {perc_alg23:<3}%  - {alg23:>4}/{tests}")
    print(f"{'alg24':<12}: [{'#'*perc_alg24}{'.'*(100-perc_alg24)}] {perc_alg24:<3}%  - {alg24:>4}/{tests}")
    print(f"{'bracha':<12}: [{'#'*perc_bracha}{'.'*(100-perc_bracha)}] {perc_bracha:<3}%  - {bracha:>4}/{tests}")
    print(f"{'imbsraynal':<12}: [{'#'*perc_imbsraynal}{'.'*(100-perc_imbsraynal)}] {perc_imbsraynal:<3}%  - {imbsraynal:>4}/{tests}")
    print("")
    

while True:
    reader_status()
    time.sleep(1)