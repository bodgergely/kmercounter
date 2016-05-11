import random

f = open("../inputs/100mb", "w")
chars = ('a', 'c', 'g', 't', 'n')
for i in xrange(100000000):
    c = random.randint(0,len(chars)-1)
    f.write(chars[c])
    
f.close()
