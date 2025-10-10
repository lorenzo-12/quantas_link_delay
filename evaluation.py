import pathlib 
import matplotlib.pyplot as plt
import numpy as np
import json


dir_img = pathlib.Path(__file__).parent / "img"
dir_results = pathlib.Path(__file__).parent / "results"
dir_alg = dir_img / "<alg>"

dict_results = {}
algorithms = ["alg23", "alg24", "bracha", "imbsraynal"]

def get_param(file_name):
    file_name = file_name.replace(".json","").replace("n","").replace("f","").replace("p","")
    n, f, p = file_name.split("_")
    return int(n), int(f), int(p)

def get_disagreement(a, b):
    if a==0 and b==0:
        return 0
    m = min(a,b)
    x = float(m*100)/(a+b)
    return round(float(x), 2)

def get_data(file_name):
    with open(file_name, "r") as f:
        data = json.load(f)
    return data["Results"]


def draw_plots_info(alg):
    f_values = [20, 30, 35, 40, 45, 50]
    p_values = [50, 60, 70, 80, 90, 100]
    possible_info = ["avg_delivery", "avg_delivery_time", "avg_disagreement", "disagreement_frequency", "termination_rate"]

    # Symmetric, tiny horizontal offsets
    offsets = np.linspace(-0.9, 0.9, len(f_values))

    fig, axes = plt.subplots(2, 3, figsize=(14, 8), sharex=True, sharey=True)
    axes = axes.ravel()

    for idx, info in enumerate(possible_info):
        ax = axes[idx]
        for i, f in enumerate(f_values):
            x = [p + offsets[i] for p in p_values]
            y = [dict_results[alg][f][p][info] for p in p_values]
            ax.plot(
                x, y,
                marker='o',
                linewidth=2,
                markerfacecolor='white',
                markeredgewidth=1.5,
                label=f'{f} byzantine nodes'
            )

        ax.set_title(f"{info}")
        ax.grid(True, linestyle=':', alpha=0.4)
        if idx % 3 == 0:
            ax.set_ylabel(info)
        if idx >= 3:
            ax.set_xlabel("message diversity (p)")

    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=len(f_values), frameon=False)
    fig.suptitle(f"Comparison of all information of algorithm: {alg}", y=0.98)
    fig.tight_layout(rect=[0, 0.08, 1, 0.95])
    #plt.show()
    plt.savefig(dir_img / f"comparison_info_{alg}.png", bbox_inches='tight', dpi=900)
    plt.close(fig)
    


def draw_plots_alg(info):
    f_values = [20, 30, 35, 40, 45, 50]
    p_values = [50, 60, 70, 80, 90, 100]

    # Symmetric, tiny horizontal offsets (total width < spacing=10)
    offsets = np.linspace(-0.9, 0.9, len(f_values))  # tweak 0.9 -> 1.2 if you want more separation

    # Create a 2x2 grid of subplots, one for each algorithm
    fig, axes = plt.subplots(2, 2, figsize=(12, 9), sharex=True)
    axes = axes.ravel()

    for idx, alg in enumerate(algorithms):
        ax = axes[idx]
        for i, f in enumerate(f_values):
            x = [p + offsets[i] for p in p_values]
            y = [dict_results[alg][f][p][info] for p in p_values]
            ax.plot(
                x, y,
                marker='o',
                linewidth=2,
                markerfacecolor='white',
                markeredgewidth=1.5,
                label=f'{f} byzantine nodes'
            )

        # Titles/labels per subplot
        ax.set_title(f"{alg} (100 nodes)")
        ax.grid(True, linestyle=':', alpha=0.4)
        if idx % 2 == 0:
            ax.set_ylabel(info)
        if idx >= 2:
            ax.set_xlabel("message diversity (p)")

    # Single shared legend at the bottom
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=len(f_values), frameon=False)

    fig.suptitle(f"Comparison across algorithms • metric: {info}", y=0.98)
    fig.tight_layout(rect=[0, 0.08, 1, 0.95])  # leave space for legend and suptitle
    #plt.show()
    plt.savefig(dir_img / f"comparison_{info}.png", bbox_inches='tight', dpi=900)
    plt.close(fig)


def draw_plots_byzantine(info):
    f_values = [20, 30, 35, 40, 45, 50]
    p_values = [50, 60, 70, 80, 90, 100]

    # Symmetric, tiny horizontal offsets
    offsets = np.linspace(-0.9, 0.9, len(algorithms))

    fig, axes = plt.subplots(2, 3, figsize=(14, 8), sharex=True, sharey=True)
    axes = axes.ravel()

    for idx, f in enumerate(f_values):
        ax = axes[idx]
        for i, alg in enumerate(algorithms):
            x = [p + offsets[i] for p in p_values]
            y = [dict_results[alg][f][p][info] for p in p_values]
            ax.plot(
                x, y,
                marker='o',
                linewidth=2,
                markerfacecolor='white',
                markeredgewidth=1.5,
                label=f'{alg}'
            )

        ax.set_title(f"{f} byzantine nodes")
        ax.grid(True, linestyle=':', alpha=0.4)
        if idx % 3 == 0:
            ax.set_ylabel(info)
        if idx >= 3:
            ax.set_xlabel("message diversity (p)")

    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=len(algorithms), frameon=False)
    fig.suptitle(f"Comparison across byzantine configurations • metric: {info}", y=0.98)
    fig.tight_layout(rect=[0, 0.08, 1, 0.95])
    #plt.show()
    plt.savefig(dir_img / f"comparison_byzantine_{info}.png", bbox_inches='tight', dpi=900)
    plt.close(fig)

for alg in algorithms:
    dir_path = dir_results / alg 
    for file in dir_path.glob("*.json"):
        n, f, p = get_param(file.name)
        if alg not in dict_results:
            dict_results[alg] = {}
        if f not in dict_results[alg]:
            dict_results[alg][f] = {}
        if p not in dict_results[alg][f]:
            dict_results[alg][f][p] = {}
        data = get_data(file)
        x = dict_results[alg][f][p]
        x["avg_delivery"] = data["avg_delivery"]
        x["avg_delivery_time"] = data["avg_delivery_time"]
        x["avg_disagreement"] = data["avg_disagreement"]
        x["disagreement_frequency"] = data["disagreement_frequency"]
        
        tmp = data["termination_rate"].split("%")[0]
        x["termination_rate"] = float(tmp)

draw_plots_alg("avg_delivery")
draw_plots_alg("avg_delivery_time")
draw_plots_alg("avg_disagreement")
draw_plots_alg("disagreement_frequency")
draw_plots_alg("termination_rate")

draw_plots_byzantine("avg_delivery")
draw_plots_byzantine("avg_delivery_time")
draw_plots_byzantine("avg_disagreement")
draw_plots_byzantine("disagreement_frequency")
draw_plots_byzantine("termination_rate")

for alg in algorithms:
    draw_plots_info(alg)




    
    
