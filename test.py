import numpy as np
s=np.asarray([1,2,3,4])
A=s[:2]
B=s[2:]
print(A.join(B))