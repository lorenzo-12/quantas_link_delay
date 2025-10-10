import pathlib 
import os 


makefile_file = pathlib.Path(__file__).parent / "makefile"

algorithms_peer = ["Alg23Peer", "Alg24Peer", "BrachaPeer", "ImbsRaynalPeer"]
algorithms_json = ["alg23.json", "alg24.json", "bracha.json", "imbsraynal.json"]


for alg_peer, alg_json in zip(algorithms_peer, algorithms_json):
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
    
    os.system("make run")
    
    