from Crypto.Util.number import getPrime
from math import gcd
from random import *

p = getPrime(680)
print("p:", p)
file = open("p.txt", 'w')
file.write(str(p))
file.close()
while 1:
    gaux = randint(2, p)
    if gcd(gaux, p) == 1:
        g = gaux
        break
print("g:", g)
file = open("g.txt", "w")
file.write(str(g))
file.close()
