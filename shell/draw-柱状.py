# -*- coding:utf-8 -*-
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
matplotlib.rcParams['font.sans-serif'] = ['SimHei']  # 用黑体显示中文
 
# 构建数据
x = [1, 2 ,3 ,4 ,5]
y = [0.8, 0.88, 0.825, 0.76, 0.91]

z = [0.53, 0.54, 0.44, 0.328, 0.183]
u = [0.53, 0.54, 0.44, 0.328, 0.183]
for i in range(0, len(z)):
    u[i] = 1

 
 
# 绘柱状图
plt.bar(x=x, height=u, label='cpu', color='LemonChiffon', alpha=0.8)
plt.bar(x=x, height=z, label='mt3k', color='Coral', alpha=0.8)
# 在左侧显示图例
plt.legend(loc="upper left")
 
# 设置标题
# plt.title("Detection results")
# 为两条坐标轴设置名称
# plt.xlabel("Component number")
# plt.ylabel("Number of seam")
 
 
# 画折线图
# ax2 = plt.twinx()
# ax2.set_ylabel("mt3k & cpu")
# 设置坐标轴范围
# ax2.set_ylim([0.5, 1.05]);
# plt.plot(x, y, "r", marker='.', c='r', ms=5, linewidth='1', label="mt3k & cpu")
# 显示数字
# for a, b in zip(x, y):
#     plt.text(a, b, b, ha='center', va='bottom', fontsize=8)
# 在右侧显示图例
plt.legend(loc="upper right")
plt.savefig("recall.jpg")
 
plt.show()
 