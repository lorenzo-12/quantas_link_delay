import pathlib 
import matplotlib.pyplot as plt
import numpy as np
import json
from scipy import stats
from utils import *


def plots_single_algorithm(alg):
    results = get_all_results(alg)
    for combination in results:
        comb = results[combination]
        
        # Symmetric, tiny horizontal offsets
        offsets = np.linspace(-0.9, 0.9, len(F_VALUES))

        fig, axes = plt.subplots(3, 3, figsize=(14, 8), sharex=True)
        axes = axes.ravel()
            
        for idx, info in enumerate(INFOS):
            ax = axes[idx]
            for i, f in enumerate(F_VALUES):
                x = [p + offsets[i] for p in P_VALUES]
                y = [comb[f][p][info][0] for p in P_VALUES]
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
                ax.set_ylabel(f"{info}")
            if idx >= 3:
                ax.set_xlabel("message diversity (p)")
                    
                    
        handles, labels = axes[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc='lower center', ncol=len(F_VALUES), frameon=False)
        fig.suptitle(f"Comparison of all information of algorithm: {alg}", y=0.98)
        fig.tight_layout(rect=[0, 0.08, 1, 0.95])
        #plt.show()
        plt.savefig(DIR_IMG / f"comparison_info_{alg}_{combination}.png", bbox_inches='tight', dpi=900)
        plt.close(fig)
            

plots_single_algorithm("alg23")     
