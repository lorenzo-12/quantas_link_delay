import pathlib 
import os 
import subprocess
import threading
import time
import signal
import sys
from queue import Queue
import colorama 

RED = colorama.Fore.RED
RESET = colorama.Fore.RESET

simulation_file = pathlib.Path(__file__).parent / "quantas" / "Common" / "Simulation.hpp"
makefile_file = pathlib.Path(__file__).parent / "makefile"
status_file = pathlib.Path(__file__).parent / "status.txt"

# --- 1) Empty the status file at start ---
status_file.write_text("")  # truncate/clear

start_time = time.time()

MAX_CONCURRENCY = 4

ALGORITHMS = [
    ("BrachaPeer", "bracha.json"),
    ("Alg23Peer", "alg23.json"),
    ("Alg24Peer", "alg24.json"),
    ("ImbsRaynalPeer", "imbsraynal.json"),
]

ALGORITHMS_TO_RUN = []
def get_tests():
    global ALGORITHMS_TO_RUN
    directory_alg23 = pathlib.Path(__file__).parent / "quantas" / "Alg23Peer"
    directory_alg24 = pathlib.Path(__file__).parent / "quantas" / "Alg24Peer"
    directory_bracha = pathlib.Path(__file__).parent / "quantas" / "BrachaPeer"
    directory_imbsraynal = pathlib.Path(__file__).parent / "quantas" / "ImbsRaynalPeer"
    dirs = [
        (directory_alg23, "Alg23Peer"),
        (directory_alg24, "Alg24Peer"),
        (directory_bracha, "BrachaPeer"),
        (directory_imbsraynal, "ImbsRaynalPeer"),
    ]
    for directory, alg_class in dirs:
        json_files = [f for f in os.listdir(directory) if f.endswith(".json") and "test" not in f]
        for json_file in json_files:
            ALGORITHMS_TO_RUN.append((alg_class, json_file))


# Global stop event and process list
stop_event = threading.Event()
processes = []
processes_lock = threading.Lock()

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
        

def change_simulation(alg_peer): 
    print(f"Changing simulation to {alg_peer}")
    with open(simulation_file, "r") as f:
        text = f.read()
    
    if alg_peer == "BrachaPeer":
        text = text.replace("/* // comment start bracha", "// comment start bracha")
        text = text.replace("// comment end bracha */", "// comment end bracha")
        
        text = text.replace("/* // comment start other", "// comment start other")
        text = text.replace("// comment start other", "/* // comment start other")
        
        text = text.replace("// comment end other */", "// comment end other")
        text = text.replace("// comment end other", "// comment end other */")
    
    else:
        text = text.replace("/* // comment start bracha", "// comment start bracha")
        text = text.replace("// comment start bracha", "/* // comment start bracha")
        
        text = text.replace("// comment end bracha */", "// comment end bracha")
        text = text.replace("// comment end bracha", "// comment end bracha */")
        
        text = text.replace("/* // comment start other", "// comment start other")
        text = text.replace("// comment end other */", "// comment end other")

    with open(simulation_file, "w") as f:
        f.write(text)


def run_test(alg_peer, alg_json):
    change_makefile(alg_peer, alg_json)
    change_simulation(alg_peer)
    print(RED,f"Running tests for {alg_json}",RESET)
    cmd = ["make", "run"]
    
    # Start the subprocess (non-blocking)
    proc = subprocess.Popen(cmd)
    with processes_lock:
        processes.append(proc)

    # Wait for it to complete or for stop_event
    while proc.poll() is None:
        if stop_event.is_set():
            print(f"[runner] Terminating {alg_peer}...", flush=True)
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
            break
        time.sleep(0.5)

    with processes_lock:
        if proc in processes:
            processes.remove(proc)

    if proc.returncode == 0:
        print(f"[runner] done: {' '.join(cmd)}", flush=True)
    elif not stop_event.is_set():
        print(f"[runner] ERROR: {' '.join(cmd)} exited with {proc.returncode}", flush=True)



def handle_sigint(signum, frame):
    print("\n[!] Caught Ctrl-C, stopping all threads and subprocesses...", flush=True)
    stop_event.set()

    # Terminate all running processes
    with processes_lock:
        for proc in processes:
            if proc.poll() is None:
                proc.terminate()
        for proc in processes:
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()

    sys.exit(1)
    
    
def worker(q: Queue):
    while not stop_event.is_set():
        try:
            alg_peer, alg_json = q.get_nowait()
        except Exception:
            break  # queue is empty
        try:
            run_test(alg_peer, alg_json)
        finally:
            q.task_done()
    


def main():
    signal.signal(signal.SIGINT, handle_sigint)

    # Fill the queue with all algorithms to run
    q = Queue()
    for alg_peer, alg_json in ALGORITHMS_TO_RUN:
        q.put((alg_peer, alg_json))

    # Start up to MAX_CONCURRENCY worker threads
    threads = [
        threading.Thread(target=worker, args=(q,), daemon=True)
        for _ in range(min(MAX_CONCURRENCY, q.qsize()))
    ]

    try:
        for t in threads:
            t.start()
            time.sleep(30)

        # Wait for all tasks to finish
        while not q.empty():
            time.sleep(2)  # keep the main thread alive and responsive to SIGINT

        q.join()  # all done!

    finally:
        stop_event.set()
        for t in threads:
            t.join()


if __name__ == "__main__":
    get_tests()
    main()
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"All tests completed in {elapsed_time:.2f} seconds.")

