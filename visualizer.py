import numpy as np 
import matplotlib.pyplot as plt 
  

datas = [[0,0],
         [0,0],
         [0,0],
         [0,0]]

datas[0][0] = (np.loadtxt("data/1_1_16_256_200_5.txt"), 200, 5)  
datas[1][0] = (np.loadtxt("data/1_1_128_256_100_5.txt"), 100, 5)
datas[2][0] = (np.loadtxt("data/1_1_16_1024_100_5.txt"), 100, 5)
datas[3][0] = (np.loadtxt("data/1_1_128_1024_10_10.txt"), 10, 10)
datas[0][1] = (np.loadtxt("data/4_4_16_256_200_5.txt"), 200, 5)  
datas[1][1] = (np.loadtxt("data/4_4_128_256_100_5.txt"), 100, 5)
datas[2][1] = (np.loadtxt("data/4_4_16_1024_100_5.txt"), 100, 5)
datas[3][1] = (np.loadtxt("data/4_4_128_1024_10_10.txt"), 10, 10)

X = ['16 - 256','128 - 256','16 - 1024','128 - 1024']
  
# X_axis = np.array([16*256*256, 128*256*256, 16*1024*1024, 128*1024*1024] )
X_axis = np.arange(len(X))

t1 = [datas[x][0][0][-1]/(datas[x][0][1]*datas[x][0][2]) for x in range(4)]
t2 = [datas[x][1][0][-1]/(datas[x][1][1]*datas[x][1][2]) for x in range(4)]

m1 = [[] for i in range(4)]; m2 = [[] for i in range(4)]
for i in range(4):
    r = datas[i][0][1]
    t = datas[i][0][2]
    for j in range(r*t):
        m1[i].append((datas[i][0][0][j] + (j*1000/r))/j)
        m2[i].append((datas[i][1][0][j] + (j*1000/r))/j)

#print(m1)

err1 = [np.std(m1[i]) for i in range(4)]
err2 = [np.std(m2[i]) for i in range(4)]

print(np.diff(X_axis))

plt.bar(X_axis - 0.2, t1, 0.4, label = '1 thread - 1 core')
plt.bar(X_axis + 0.2, t2, 0.4, label = '4 threads - 4 cores')
plt.yscale('log')
plt.xticks(X_axis, X)
plt.xlabel("Key Size - File Size")
plt.ylabel("Avg server process time (ms)")
plt.title("Server process time")
plt.legend()
plt.show()