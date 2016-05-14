import random

f = open("../inputs/medium", "w")
chars = ('a', 'c', 'g', 't', 'n')
for i in xrange(150000):
    c = random.randint(0,len(chars)-1)
    f.write(chars[c])
    
f.close()
