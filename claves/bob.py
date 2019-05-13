from socket import *
from random import randint
import sys


sock = socket(AF_INET, SOCK_STREAM)
server_address = ('', 9999)
sock.bind(server_address)
sock.listen(1)

print("Escuchando en 0.0.0.0:9999")
conexion, addr = sock.accept()
data = conexion.recv(8192)
data = data.decode('utf-8')

p = open("../p.txt", "r").read()
p = int(p)
g = open("../g.txt", "r").read()
g = int(g)
b = randint(2, p-2)
print("b generado:", b)
b_mayus = pow(g,b,p)
print("B generado:", b_mayus)

conexion.sendall(str(b_mayus).encode())
conexion.close()

K = pow(int(data),b,p)
print("K generado:", K)
