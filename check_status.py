import pathlib 
import os 
import subprocess
import threading
import time
import signal
import sys
import json

TOTAL_TESTS = 0

bracha_file = pathlib.Path(__file__).parent / "quantas" / "BrachaPeer" / "bracha.json"
alg23_file = pathlib.Path(__file__).parent / "quantas" / "Alg23Peer" / "alg23.json"
alg24_file = pathlib.Path(__file__).parent / "quantas" / "Alg24Peer" / "alg24.json"
imbsraynal_file = pathlib.Path(__file__).parent / "quantas" / "ImbsRaynalPeer" / "imbsraynal.json"
def set_total_tests():
    tests = 0
    x = 0
    with open(bracha_file, "r") as f:
        js = json.load(f)
    x = len(js["experiments"])
    tests = int(js["experiments"][0]["tests"])
    global TOTAL_TESTS
    TOTAL_TESTS = x*tests
    print(TOTAL_TESTS)
        

status_file = pathlib.Path(__file__).parent / "status.txt"
def reader_status():
    with open(status_file, "r") as f:
        lines = f.readlines()
    tests = TOTAL_TESTS
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
    

set_total_tests()
while True:
    reader_status()
    time.sleep(1)
    