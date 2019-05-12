from socket import *
from random import randint
import sys

p = open("p.txt", "r").read()
p = int(p)
g = open("g.txt", "r").read()
g = int(g)
a = randint(0, p-1)
print("a generado:", a)
a_mayus = pow(g,a,p)
print("A generado:", a_mayus)

ip = sys.argv[1]
port = sys.argv[2]

sock = socket(AF_INET, SOCK_STREAM)
sock.connect((ip, int(port)))
sock.sendall(str(a_mayus).encode())

data = ''
while 1:
    data_aux = sock.recv(1024)
    if not data_aux:
        break
    data = data + data_aux.decode('utf-8')

K = pow(int(data),a,p)
print("K generado:", K)
