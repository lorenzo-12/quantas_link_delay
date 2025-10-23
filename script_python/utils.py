import pathlib 
import matplotlib.pyplot as plt
import numpy as np
import json
from scipy import stats


DIR_RESULTS = pathlib.Path(__file__).parent.parent / "results_all"
DIR_IMG = pathlib.Path(__file__).parent.parent / "img"

ALGORITHM = ["alg23", "alg24", "bracha", "imbsraynal"]
F_VALUES = [19, 20, 25, 33, 40]
P_VALUES = [50, 60, 70, 80, 90, 100]
INFOS = ["delivery_nodes", "delivery_time", "termination_rate", "disagreement", "disagreement_frequency", "finishing_steps", "total_msgs_sent"]

def get_param(file_name):
    file_name = file_name.replace(".json","").replace("n","").replace("f","").replace("p","")
    n, f, p = file_name.split("_")
    return int(n), int(f), int(p)

def get_data(file_name):
    with open(file_name, "r") as f:
        data = json.load(f)
    return data["Results"]

def confidence_interval(data, confidence: float = 0.95):
    data = np.array(data)
    n = len(data)
    if n < 2:
        raise ValueError("At least two data points are required to compute a confidence interval.")
    mean = np.mean(data)
    sem = stats.sem(data)  # standard error of the mean
    margin = sem * stats.t.ppf((1 + confidence) / 2.0, n - 1)
    return mean - margin, mean + margin

def get_all_tests(alg):
    d = {}
    dir_alg = DIR_RESULTS / alg
    sub_dirs = [subdir for subdir in dir_alg.iterdir() if subdir.is_dir()]
    for combination in sub_dirs:
        d[combination.name] = []
        json_files = combination.glob("*.json")
        for json_file in json_files:
            d[combination.name].append(json_file)
    return d

def get_all_results(alg):
    results = get_all_tests(alg)
    all_results = {}
    for combination in results:
        comb = results[combination]
        all_results[combination] = {}
        for f in F_VALUES:
            all_results[combination][f] = {}
            for p in P_VALUES:
                all_results[combination][f][p] = {}
        for json_file in comb:
            data = get_single_result(json_file)
            n, f, p = get_param(json_file.name)
            for info in INFOS:
                all_results[combination][f][p][info] = [data[info][0], data[info][1], data[info][2]]
    return all_results

def get_single_result(file_name):
    data = get_data(file_name)
    result = {}
    
    lower_ci, upper_ci = confidence_interval(data["delivery_nodes"])
    delivery_nodes = [sum(data["delivery_nodes"])/len(data["delivery_nodes"]), lower_ci, upper_ci]
    result["delivery_nodes"] = delivery_nodes
    
    lower_ci, upper_ci = confidence_interval(data["delivery_time"])
    delivery_time = [sum(data["delivery_time"])/len(data["delivery_time"]), lower_ci, upper_ci]
    result["delivery_time"] = delivery_time

    lower_ci, upper_ci = confidence_interval(data["disagreement"])
    disagreement = [sum(data["disagreement"])/len(data["disagreement"]), lower_ci, upper_ci]
    result["disagreement"] = disagreement

    lower_ci, upper_ci = confidence_interval(data["finishing_steps"])
    finishing_steps = [sum(data["finishing_steps"])/len(data["finishing_steps"]), lower_ci, upper_ci]
    result["finishing_steps"] = finishing_steps

    lower_ci, upper_ci = confidence_interval(data["termination_rate"])
    termination_rate = [sum(data["termination_rate"])/len(data["termination_rate"])*100, lower_ci*100, upper_ci*100]
    result["termination_rate"] = termination_rate

    lower_ci, upper_ci = confidence_interval(data["total_msgs_sent"])
    total_msgs_sent = [sum(data["total_msgs_sent"])/len(data["total_msgs_sent"]), lower_ci, upper_ci]
    result["total_msgs_sent"] = total_msgs_sent

    x = 0
    for val in data["disagreement"]:
        if val > 0:
            x += 1
            
    disagreement_frequency = x*100 / len(data["disagreement"]) if data["disagreement"] else 0.0
    result["disagreement_frequency"] = [disagreement_frequency, disagreement_frequency, disagreement_frequency]
    
    result["name"] = file_name.name.replace(".json","")

    return result
            

""" x = get_single_result("/Users/lorenzo/Github/quantas_comparing_BRB/results_all/alg23/opposite/n100_f40_p50.json")
for key in x:
    print(key, x[key]) """

""" x = get_all_tests("alg23")
for key in x:
    print(key) """