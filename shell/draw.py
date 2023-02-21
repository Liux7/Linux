import matplotlib.pyplot as plt

fig, ax = plt.subplots()

counts = [1.032, 1.58,2.009, 2.319,2.698,2.734,3.394, 4.895, 5.017, 5.358, 5.613,5.828, 5.914,6.579,6.628, 6.736]
bar_labels = ['red', 'blue', '_red', 'orange']
bar_colors = ['tab:red', 'tab:blue', 'tab:red', 'tab:orange']
index = ['YU' ,'Vio','PL','GIN','PS','WP','AE','CO2','Hug','Pov','San','Ed','Wat','AP','CM','Int']






























ax.bar(index, counts, label=bar_labels)

ax.set_ylabel('Impact')
# ax.set_title('SDGs')
# ax.legend(title='Fruit color')

plt.show()