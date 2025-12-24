import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# График Ципфа
df = pd.read_csv('zipf_analysis.csv')
plt.figure(figsize=(10, 6))
plt.scatter(df['log_rank'], df['log_freq'], alpha=0.5, label='Данные')
plt.plot(df['log_rank'], df['zipf_prediction'], 'r-', label='Закон Ципфа')
plt.xlabel('log(ранг)')
plt.ylabel('log(частота)')
plt.title('Распределение Ципфа')
plt.legend()
plt.grid(True)
plt.savefig('zipf_plot.png')
plt.show()