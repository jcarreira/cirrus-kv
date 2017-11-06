import random
import string

# ITERS = 200000000
ITERS = 1000000

f = open('terasort_data.txt', 'w')
for j in range(ITERS):
	if j % 10000 == 0:
		print j
	rand_str = ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for _ in range(100))
	f.write(rand_str + '\n')
f.close()
