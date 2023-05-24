import matplotlib.pyplot as plt

fig, (ax1,ax2) = plt.subplots(2,1,sharex=True)

# fruits = ['apple', 'blueberry', 'cherry', 'orange','apple', 'blueberry', 'cherry', 'orange']
# counts = [40, 100, 30, 55,40, 100, 30, 55]
# bar_labels = ['red', 'blue', '_red', 'orange']

type = ["AM"," DDR"," gsm"," DDR load to AM","gsm load to AM"]
type2 = []

bandwith = [900.145721,4.875888,7.144939,33.326332,201.85799]



bar_colors = ['tab:red', 'tab:blue', 'tab:red', 'tab:orange']

ax1.bar(type, bandwith, width=0.5)#,  color=bar_colors)
ax2.bar(type, bandwith, width=0.5)#,  color=bar_colors)
# ax.bar(SDGs, v1, label=SDGs)


ax1.get_xaxis().set_visible(False)
ax1.spines['bottom'].set_visible(False)#关闭子图1中底部脊
ax2.spines['top'].set_visible(False)##关闭子图2中顶部脊


d = .85  #设置倾斜度
#绘制断裂处的标记
kwargs = dict(marker=[(-1, -d), (1, d)], markersize=15,
              linestyle='none', color='b', mec='r', mew=1, clip_on=False)
ax1.plot([0, 1], [0, 0],transform=ax1.transAxes, **kwargs)
ax2.plot([0, 1], [1, 1], transform=ax2.transAxes, **kwargs)

ax1.set_ylim(800, 910)
ax2.set_ylim(0,210)
plt.xticks(rotation=350)
# ax.set_ylabel('fruit supply')
# ax.set_title('Fruit supply by kind and color')
# ax.legend(title='Fruit color')
# plt.grid()

plt.show()