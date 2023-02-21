import matplotlib.pyplot as plt

fig, ax = plt.subplots()

# fruits = ['apple', 'blueberry', 'cherry', 'orange','apple', 'blueberry', 'cherry', 'orange']
# counts = [40, 100, 30, 55,40, 100, 30, 55]
# bar_labels = ['red', 'blue', '_red', 'orange']

SDGs =["Pov","Hug","CM","Ed","WP","Wat","AE","YU","Int","GINI","San","AP","CO2","PS","PL","Vio"]


v1 = [4,5,1,6,11,2,12,15,7,13,3,8,9,14,16,10]
v2 = [5,6,2,1,12,3,10,15,7,12,4,8,9,14,16,11]
delta = [0]*16
for i in range(1,16,1):
    delta[abs(v2[i]-v1[i])] = delta[abs(v2[i]-v1[i])] + 1
bar_colors = ['tab:red', 'tab:blue', 'tab:red', 'tab:orange']

ax.bar(SDGs, delta)#,  color=bar_colors)
# ax.bar(SDGs, v1, label=SDGs)

ax.set_ylabel('fruit supply')
ax.set_title('Fruit supply by kind and color')
ax.legend(title='Fruit color')

plt.show()