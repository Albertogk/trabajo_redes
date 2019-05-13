from socket import *
from random import randint
import sys
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from Crypto.Hash import SHA256

p = open("../p.txt", "r").read()
p = int(p)
g = open("../g.txt", "r").read()
g = int(g)
a = randint(2, p-2)
print("a generado:", a)
a_mayus = pow(g,a,p)
print("A generado:", a_mayus)

ip = sys.argv[1]

sock = socket(AF_INET, SOCK_STREAM)
sock.connect((ip, 8888))
sock.sendall(str(a_mayus).encode())


data = sock.recv(8192)

K = pow(int(data),a,p)
print("K generado:", K)

h = SHA256.new(str(K).encode())
key = h.digest()
mensaje = b"Hola! Mis detalles de cuenta bancaria son los siguientes: blablabla"
print("Texto en claro: ", mensaje)
cifrador = AES.new(key, AES.MODE_ECB)
mensaje_cifrado = cifrador.encrypt(pad(mensaje, AES.block_size))
print("Texto cifrado: ", mensaje_cifrado)
sock.sendall(mensaje_cifrado)
data = sock.recv(8192)
print("Cifrado recibido: ", data)
data = unpad(cifrador.decrypt(data), AES.block_size)
print("Recibido en claro: ", data)
