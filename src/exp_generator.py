import numpy as np
import sys

t = int(sys.argv[1])
print("avg wait time", t)

exit(int(np.random.exponential(scale=t)))