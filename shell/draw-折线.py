import matplotlib.pyplot as plt
import numpy as np
# x=[384,768,1536,3072,6144,12288,24576,49152,98304,196608,393216,786432]
# x2=[384,768,1536,3072,6144,12288,24576,49152,98304,196608,393216]
x = [1, 2, 4, 8, 16, 24]
# for i in x:
#     i = i/192
# for i in x2:
#     i = i/192

# xlog = np.log2(x)
# x2log = np.log2(x2)

data1=[552.000,281.000,164.000,90.000,70.000,65.300]
data2=[293.0000, 154.00, 82.000, 40.000, 23.0, 12.0]
data3=[293.0000, 154.00, 82.000, 40.000, 23.0, 12.0]
[0.53, 0.54, 0.44, 0.328, 0.183]

for i in range(0, len(data1)):
    data3[i] = data2[i]/data1[i]
print(data3) 
# data1=[5.500,12.000000,16.666667,21.600000,25.777778,38.230769,43.200000,66.735294,78.387097,88.367521,93.500000,126.060440]
# data2=[1.466667,3.142857,4.000000,7.000000,11.318182,12.775000,21.215686,17.823077,12.109453,7.047782,4.419414,2.299945]
# data3=[1.060870,1.126050,1.358974,1.918699,2.450000,3.413408,3.478261,2.362919,1.417894,0.789573,0.419155]
# data4=[1.051724,1.166667,1.377193,1.844262,2.503650,3.435028,3.375358,2.358899,1.153286,0.704493,0.404292]
# y3=[1.660198,2.189551,2.720566,3.049943,3.812188]
# y4=[0.987527,0.890520,0.958986,0.933661,1.010783]

# plt.ylim(1, 6)    
# plt.xlim(1, 24)
plt.xticks(x)

# plt.yticks(data1)
print(len(x))
plt.scatter(x, data1)
plt.scatter(x, data2)
# plt.scatter(xlog, data2)
# plt.scatter(x2log, data3)
# plt.scatter(x2log, data4)
# plt.scatter(x, y3)
# plt.scatter(x, y4)
# plt.title("performance of bitonic sort with different thread")
plt.plot(x,data1)
# plt.bar(x, data3, align='center', alpha=0.5)

# for i in range(len(x)):
#     plt.bar(x[i],data3[i])
    
plt.plot(x,data2)
plt.plot(x,data1,label='total time')
plt.plot(x,data2,label='mt3k time')
# plt.plot(x2log,data3,label='1 cluster; allocate threads count; initial data in mem')
# plt.plot(x2log,data4,label='4 cluster; allocate threads count; initial data in mem')
# plt.plot(x,y3,label='thread=4')
# plt.plot(x,y4,label='thread=8')
plt.xlabel('dsp num')
plt.ylabel('sort nums/sec')

plt.legend()
plt.show()