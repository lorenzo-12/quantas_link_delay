import pathlib 
import os
import json 
import random


dir_file_json = pathlib.Path(__file__).parent / "quantas" 
base_file = pathlib.Path(__file__).parent / "base.json"
results_dir = pathlib.Path(__file__).parent / "results"

alg_list = [("bracha","BrachaPeer")]
n_list = [100]
f_list = [20, 30, 35, 40, 45, 50]
p_list = [100, 90, 80, 70, 60, 50]
tests = 1000 

def getByzantineVector(n,f):
    byzantine_nodes = random.sample(range(0, n), f)
    vec = [0 for _ in range(n)]
    for val in byzantine_nodes:
        vec[val] = 1
    return byzantine_nodes, vec

def getByzantineSender(vec):
    return random.choice(vec)

def getGlobalDelays(n):
    x = []
    for i in range(1,21):
        for j in range(n*n//20):
            val = round(i*0.05,2)
            x.append(val)  
    random.shuffle(x)
        
    delays = {}
    counter = 0
    for i in range(n):
        delays[str(i)] = {}
        for j in range(n):
            delays[str(i)][str(j)] = x[counter]
            counter += 1
    return delays

experiments = {} 
experiments["experiments"] = []

for alg, alg_class in alg_list:
    (results_dir / alg).mkdir(parents=True, exist_ok=True)
    for n in n_list:
        for f in f_list:
            for p in p_list:
                with open(base_file, 'r') as base_f:
                    data = json.load(base_f)
                    del data['experiments'][0]

                exp = {}
                
                exp["parameters"] = {}
                exp["distribution"] = {}
                exp["topology"] = {}
                
                parameters = exp["parameters"]
                parameters["n"] = n
                parameters["f"] = f
                parameters["percentage"] = p
                byz_vec, vec = getByzantineVector(n,f)
                parameters["byzantine_nodes"] = vec
                parameters["sender"] = getByzantineSender(byz_vec)
                
                distribution = exp["distribution"]
                distribution["type"] = "GEOMETRIC"
                #distribution["type"] = "UNIFORM"
                distribution["maxDelay"] = 10
                distribution["global"] = 5
                distribution["global_delays"] = getGlobalDelays(n) 
                
                topology = exp["topology"]
                topology["type"] = "fullyComplete"
                topology["initialPeers"] = n
                topology["totalPeers"] = n 
                
                exp["logFile"] = f"results/{alg}/n{n}_f{f}_p{p}.json"
                file_path_on_disk = results_dir / alg / f"n{n}_f{f}_p{p}.json"
                with open(file_path_on_disk, 'w') as log_file:
                    json.dump({}, log_file)
                exp["tests"] = 100
                exp["rounds"] = 1000
                    
                experiments["experiments"].append(exp)

file_name = f"bracha.json"
file_path = dir_file_json / alg_class / file_name       
with open(file_path, 'w') as f:
    json.dump(experiments, f, indent=4)
