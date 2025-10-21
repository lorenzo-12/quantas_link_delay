import pathlib 
import os
import json 
import random


dir_file_json = pathlib.Path(__file__).parent / "quantas" 
base_file = pathlib.Path(__file__).parent / "base.json"
results_dir = pathlib.Path(__file__).parent / "results_all"

alg_list = [("bracha","BrachaPeer"), ("imbsraynal", "ImbsRaynalPeer"), ("alg24", "Alg24Peer"), ("alg23", "Alg23Peer")]
n_list = [100]
f_list = [19, 20, 25, 33, 40]
p_list = [100, 90, 80, 70, 60, 50]
tests = 1000 

comb = {
    'alg23': {'msg_type': ['ack'], 'combinations': ['same', 'silent', 'opposite']}, 
    'alg24': {'msg_type': ['ack', 'vote1', 'vote2'], 'combinations': [['silent', 'opposite', 'silent'], ['same', 'silent', 'opposite'], ['silent', 'opposite', 'opposite'], ['opposite', 'opposite', 'same'], ['silent', 'same', 'silent'], ['opposite', 'silent', 'silent'], ['opposite', 'same', 'same'], ['opposite', 'silent', 'opposite'], ['silent', 'same', 'opposite'], ['same', 'silent', 'same'], ['same', 'opposite', 'silent'], ['silent', 'opposite', 'same'], ['same', 'same', 'silent'], ['same', 'same', 'opposite'], ['silent', 'silent', 'silent'], ['same', 'opposite', 'opposite'], ['opposite', 'silent', 'same'], ['silent', 'same', 'same'], ['silent', 'silent', 'opposite'], ['opposite', 'opposite', 'silent'], ['opposite', 'opposite', 'opposite'], ['same', 'same', 'same'], ['same', 'opposite', 'same'], ['opposite', 'same', 'silent'], ['opposite', 'same', 'opposite'], ['same', 'silent', 'silent'], ['silent', 'silent', 'same']]}, 
    'bracha': {'msg_type': ['echo', 'ready'], 'combinations': [['same', 'silent'], ['silent', 'opposite'], ['silent', 'same'], ['opposite', 'opposite'], ['same', 'opposite'], ['silent', 'silent'], ['opposite', 'same'], ['same', 'same'], ['opposite', 'silent']]}, 
    'imbsraynal': {'msg_type': ['witness'], 'combinations': ['same', 'silent', 'opposite']}
}

def combination_to_string(l):
    if type(l) == str:
        return l
    x = ""
    for i in range(len(l)-1):
        x+=l[i]+"_"
    x+= l[-1]
    return x


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
    n = n_list[0]

    for combination in comb[alg]['combinations']:
        experiments = {} 
        experiments["experiments"] = []
        file_name = f"{alg}_{combination_to_string(combination)}.json"
        file_path = dir_file_json / alg_class / file_name
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
                parameters["combination"] = combination

                distribution = exp["distribution"]
                distribution["type"] = "GEOMETRIC"
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

                exp["logFile"] = f"results_all/{alg}/{combination_to_string(combination)}/n{n}_f{f}_p{p}.json"
                file_path_on_disk = results_dir / alg / combination_to_string(combination) / f"n{n}_f{f}_p{p}.json"
                os.makedirs(os.path.dirname(file_path_on_disk), exist_ok=True)  
                with open(file_path_on_disk, 'w') as log_file:
                    json.dump({}, log_file)
                exp["tests"] = 100
                exp["rounds"] = 1000
                exp["algorithm"] = alg
                exp["output_status"] = file_name
                experiments["experiments"].append(exp)
    
        print(alg, alg_class)
        os.makedirs(os.path.dirname(file_path), exist_ok=True)  
        with open(file_path, 'w') as f:
            json.dump(experiments, f, indent=4)
    


