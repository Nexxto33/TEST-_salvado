import RPi.GPIO as GPIO
import time
import serial
import random
import threading

#Definiciones para el ultrasonico y boton
TRIG = 22  
ECHO = 26 
button1 = 16

#Bandera para habilitar o deshabilitar la bandera de temperatura
enviar_temp = True

GPIO.setmode(GPIO.BCM)  
GPIO.setup(TRIG, GPIO.OUT)  
GPIO.setup(ECHO, GPIO.IN)  
GPIO.setup(button1, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Configuración del UART
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
ser.reset_input_buffer()
time.sleep(2)
print("UART conectado en /dev/ttyACM0")

# Función para medir distancia
def medir_distancia():
    GPIO.output(TRIG, GPIO.LOW) 
    time.sleep(0.1)  
    GPIO.output(TRIG, GPIO.HIGH) 
    time.sleep(0.00001)  
    GPIO.output(TRIG, GPIO.LOW)  

    # Medir el tiempo de duración del pulso
    while GPIO.input(ECHO) == GPIO.LOW:
        inicio = time.time() 

    while GPIO.input(ECHO) == GPIO.HIGH:
        fin = time.time()  

    duracion = fin - inicio  # Duración del pulso
    distancia = duracion * 34300  # Calcular la distancia en cm
    return int(distancia)

def enviar_temperatura():
    while enviar_temp:
        temperatura = random.randint(-5, 25)
        print(f"Temperatura generada: {temperatura} °C")

        if temperatura < 20:
            ser.write("C".encode('utf-8'))
            print("Comando enviado: C")
        else:
            ser.write("D".encode('utf-8'))
            print("Comando enviado: D")

        # Intentar leer respuesta de la TIVA después de enviar el comando
        respuesta = ''
        timeout = time.time() + 3  # Esperar máximo 2 segundos
        while time.time() < timeout:
            if ser.in_waiting > 0:
                respuesta = ser.readline().decode('utf-8').strip()
                if respuesta:
                    print("Respuesta de la TIVA:", respuesta)
                    break
            time.sleep(0.1)

        if not respuesta:
            print("No se recibió respuesta de la TIVA")

        time.sleep(0.5)

#Parte del codigo para abrir 2 hilos al mismo tiempo(ejecutando el envio de temperatura en segundo plano)
temp_thread = threading.Thread(target=enviar_temperatura, daemon=True)
temp_thread.start()

# Programa principal
try:
    while True:
        distancia = medir_distancia()
        print(f"Distancia medida: {distancia} cm")
        
        if GPIO.input(button1) == GPIO.LOW:
            ser.write("A".encode('utf-8'))
            #Se va a simular una interrupcion para que no se manden datos mientras se apreta el boton
            enviar_temp = False
            print("Boton")
            enviar_temp = True

        # Si la distancia es menor a 8 cm, enviar el comando 'A'
        if distancia < 8:
            ser.write("B".encode('utf-8'))  # Enviar comando a la TIVA
            enviar_temp = False
            time.sleep(0.1)
            enviar_temp = True

except KeyboardInterrupt:
    print("Medición interrumpida por el usuario")

finally:
    GPIO.cleanup()  # Limpiar los pines GPIO
    ser.close()  # Cerrar la conexión serial