import pathlib 
import os 
import subprocess
import threading
import time


makefile_file = pathlib.Path(__file__).parent / "makefile"
status_file = pathlib.Path(__file__).parent / "status.txt"

# --- 1) Empty the status file at start ---
status_file.write_text("")  # truncate/clear


ALGORITHMS = [
    ("Alg23Peer", "alg23.json"),
    ("Alg24Peer", "alg24.json"),
    ("BrachaPeer", "bracha.json"),
    ("ImbsRaynalPeer", "imbsraynal.json"),
]


def change_makefile(alg_peer, alg_json):
    with open(makefile_file, "r") as f:
        lines = f.readlines()
    
    text = ""
    do_nothing = False
    for line in lines:
        
        if "#----->" in line:
            do_nothing = True
            text += "#----->\n"
            text += f"INPUTFILE := {alg_json}\n"
            text += f"ALGFILE := {alg_peer}\n"
            text += "#<-----\n"
        elif "#<-----" in line:
            do_nothing = False
        elif do_nothing:
            continue 
        else:
            text += line
            
    with open(makefile_file, "w") as f:
        f.write(text)


def run_test(alg_peer, alg_json):
    change_makefile(alg_peer, alg_json)
    print(f"Running tests for {alg_peer}")
    cmd = ["make", "run"]
    proc = subprocess.run(cmd)
    if proc.returncode != 0:
        print(f"[runner] ERROR: {' '.join(cmd)} exited with {proc.returncode}", flush=True)
    else:
        print(f"[runner] done: {' '.join(cmd)}", flush=True)


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
    print(f"{'alg23':<12}: [{'#'*perc_alg23}{'.'*(100-perc_alg23)}] {perc_alg23}%  - {alg23:>6}/{tests}")
    print(f"{'alg24':<12}: [{'#'*perc_alg24}{'.'*(100-perc_alg24)}] {perc_alg24}%  - {alg24:>6}/{tests}")
    print(f"{'bracha':<12}: [{'#'*perc_bracha}{'.'*(100-perc_bracha)}] {perc_bracha}%  - {bracha:>6}/{tests}")
    print(f"{'imbsraynal':<12}: [{'#'*perc_imbsraynal}{'.'*(100-perc_imbsraynal)}] {perc_imbsraynal}%  - {imbsraynal:>6}/{tests}")
    print("")

def main():
    # Start one thread per run_test
    threads = []
    for alg_peer, alg_json in ALGORITHMS:
        t = threading.Thread(target=run_test, args=(alg_peer, alg_json), daemon=False)
        t.start()
        threads.append(t)
        time.sleep(25)

    # Keep executing reader_status until all threads finish
    try:
        while any(t.is_alive() for t in threads):
            reader_status()
            time.sleep(0.5)
    finally:
        for t in threads:
            t.join()
        # final snapshot
        #reader_status()

if __name__ == "__main__":
    main()