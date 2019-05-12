from socket import *
from random import randint
import sys
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from Crypto.Hash import SHA256

puerto = sys.argv[1]

sock = socket(AF_INET, SOCK_STREAM)
server_address = ('', int(puerto))
sock.bind(server_address)
sock.listen(1)

print("Escuchando en 0.0.0.0:%s" %(puerto))
conexion, addr = sock.accept()
data = conexion.recv(8192)
data = data.decode('utf-8')

p = open("p.txt", "r").read()
p = int(p)
g = open("g.txt", "r").read()
g = int(g)
b = randint(0, p-1)
print("b generado:", b)
b_mayus = pow(g,b,p)
print("B generado:", b_mayus)

conexion.sendall(str(b_mayus).encode())

K = pow(int(data),b,p)
print("K generado:", K)

h = SHA256.new(str(K).encode())
key = h.digest()
cifrador = AES.new(key, AES.MODE_ECB)
data = conexion.recv(8192)
print("Cifrado recibido: ", data)
data = unpad(cifrador.decrypt(data), AES.block_size)
print("Recibido en claro: ", data)

mensaje = b"Genial te hare el ingreso luego :D"
print("Texto en claro: ", mensaje)
mensaje_cifrado = cifrador.encrypt(pad(mensaje, AES.block_size))
print("Texto cifrado: ", mensaje_cifrado)
conexion.sendall(mensaje_cifrado)
conexion.close()
