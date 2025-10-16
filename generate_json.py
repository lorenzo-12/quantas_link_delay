import pathlib 
import os
import json 
import random


dir_file_json = pathlib.Path(__file__).parent / "quantas" 
base_file = pathlib.Path(__file__).parent / "base.json"
results_dir = pathlib.Path(__file__).parent / "results_all"

alg_list = [("bracha","BrachaPeer"), ("imbsraynal", "ImbsRaynalPeer"), ("alg24", "Alg24Peer"), ("alg23", "Alg23Peer")]
n_list = [101]
f_list = [20, 25, 33, 40]
p_list = [100, 90, 80, 70, 60, 50]
tests = 1000 

def get_honest_groups(n, p, byz_nodes):
    num_honest = n - len(byz_nodes)
    group_size = num_honest * p // 100
    all_honest = [i for i in range(n) if i not in byz_nodes]
    random.shuffle(all_honest)
    group_1 = all_honest[:group_size]
    group_2 = all_honest[group_size:]
    return group_1, group_2

def getByzantineVector(n,f):
    byzantine_nodes = random.sample(range(0, n), f)
    vec = [0 for _ in range(n)]
    for val in byzantine_nodes:
        vec[val] = 1
    return byzantine_nodes, vec

def getByzantineSender(vec):
    return random.choice(vec)

def getGlobalDelays_equal(n):
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

def getGlobalDelays_distribution(n):
    delays = {}
    counter = 0
    for i in range(n):
        delays[str(i)] = {}
        for j in range(n):
            delays[str(i)][str(j)] = round(random.uniform(0.05, 2.0),2)
    return delays


for alg, alg_class in alg_list:
    file_name = f"{alg}.json"
    file_path = dir_file_json / alg_class / file_name  
    
    experiments = {} 
    experiments["experiments"] = []
    
    for n in n_list:
        for f in f_list:
            for p in p_list:
                exp = {}
                
                exp["parameters"] = {}
                exp["distribution"] = {}
                exp["topology"] = {}
                
                parameters = exp["parameters"]
                parameters["debug_prints"] = False
                parameters["n"] = n
                parameters["f"] = f
                parameters["percentage"] = p
                byz_vec, vec = getByzantineVector(n,f)
                parameters["byzantine_nodes"] = vec
                parameters["sender"] = getByzantineSender(byz_vec)
                honest_group_0, honest_group_1 = get_honest_groups(n, p, byz_vec)
                parameters["honest_group_0"] = honest_group_0
                parameters["honest_group_1"] = honest_group_1

                distribution = exp["distribution"]
                distribution["type"] = "GEOMETRIC"
                #distribution["type"] = "UNIFORM"
                distribution["maxDelay"] = 10
                distribution["global_delay"] = 5
                distribution["global_delays_setting"] = "uniform"
                distribution["min_lambda"] = 0.05
                distribution["max_lambda"] = 2
                distribution["n"] = n
                distribution["links_delay"] = {}
                #distribution["links_delay"] = getGlobalDelays_distribution(n) 
                
                topology = exp["topology"]
                topology["type"] = "fullyComplete"
                topology["initialPeers"] = n
                topology["totalPeers"] = n 

                exp["logFile"] = f"results_all/{alg}/n{n}_f{f}_p{p}.json"
                file_path_on_disk = results_dir / alg / f"n{n}_f{f}_p{p}.json"
                os.makedirs(os.path.dirname(file_path_on_disk), exist_ok=True)  
                with open(file_path_on_disk, 'w') as log_file:
                    json.dump({}, log_file)
                exp["tests"] = 100
                exp["rounds"] = 1000
                    
                experiments["experiments"].append(exp)
    
    print(alg, alg_class)
    file_name = f"{alg}.json"
    file_path = dir_file_json / alg_class / file_name  
    os.makedirs(os.path.dirname(file_path), exist_ok=True)  
    with open(file_path, 'w') as f:
        json.dump(experiments, f, indent=4)
    


"""
for alg, alg_class in alg_list:
    (results_dir / alg).mkdir(parents=True, exist_ok=True)
    experiments = {} 
    experiments["experiments"] = []
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
                parameters["debug_prints"] = False
                parameters["n"] = n
                parameters["f"] = f
                parameters["percentage"] = p
                byz_vec, vec = getByzantineVector(n,f)
                parameters["byzantine_nodes"] = vec
                parameters["sender"] = getByzantineSender(byz_vec)
                honest_group_0, honest_group_1 = get_honest_groups(n, p, byz_vec)
                parameters["honest_group_0"] = honest_group_0
                parameters["honest_group_1"] = honest_group_1

                distribution = exp["distribution"]
                distribution["type"] = "GEOMETRIC"
                #distribution["type"] = "UNIFORM"
                distribution["maxDelay"] = 10
                distribution["global_delay"] = 5
                distribution["global_delays_setting"] = "uniform"
                distribution["min_lambda"] = 0.05
                distribution["max_lambda"] = 2.0
                distribution["n"] = n
                distribution["links_delay"] = {}
                #distribution["links_delay"] = getGlobalDelays_distribution(n) 
                
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

    print(alg, alg_class)
    file_name = f"{alg}.json"
    file_path = dir_file_json / alg_class / file_name       
    with open(file_path, 'w') as f:
        json.dump(experiments, f, indent=4)
"""