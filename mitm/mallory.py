from socket import *
from random import randint
import sys
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from Crypto.Hash import SHA256


sock = socket(AF_INET, SOCK_STREAM)
server_address = ('', 8888)
sock.bind(server_address)
sock.listen(1)

print("Conexión interceptada")
conexion, addr = sock.accept()
data = conexion.recv(8192)
data = data.decode('utf-8')

p = open("../p.txt", "r").read()
p = int(p)
g = open("../g.txt", "r").read()
g = int(g)
c = randint(2, p-2)
print("c generado:", c)
c_mayus = pow(g,c,p)
print("C generado:", c_mayus)

conexion.sendall(str(c_mayus).encode())

K = pow(int(data),c,p)
print("K generado:", K)

h = SHA256.new(str(K).encode())
key = h.digest()
cifrador = AES.new(key, AES.MODE_ECB)
data = conexion.recv(8192)
print("Cifrado recibido: ", data)
data = unpad(cifrador.decrypt(data), AES.block_size)
print("Recibido en claro: ", data)

mensaje = b"Okey ;)"
print("Texto en claro: ", mensaje)
mensaje_cifrado = cifrador.encrypt(pad(mensaje, AES.block_size))
print("Texto cifrado: ", mensaje_cifrado)
conexion.sendall(mensaje_cifrado)
conexion.close()

print("\n*****Bob******\n")

ip = sys.argv[1]

sock = socket(AF_INET, SOCK_STREAM)
sock.connect((ip, 9999))
sock.sendall(str(c_mayus).encode())


data = sock.recv(8192)

K = pow(int(data),c,p)
print("K generado:", K)

h = SHA256.new(str(K).encode())
key = h.digest()
mensaje = b"Hola! Mis detalles de cuenta bancaria son los siguientes: blublublu"
print("Texto en claro: ", mensaje)
cifrador = AES.new(key, AES.MODE_ECB)
mensaje_cifrado = cifrador.encrypt(pad(mensaje, AES.block_size))
print("Texto cifrado: ", mensaje_cifrado)
sock.sendall(mensaje_cifrado)
data = sock.recv(8192)
print("Cifrado recibido: ", data)
data = unpad(cifrador.decrypt(data), AES.block_size)
print("Recibido en claro: ", data)
