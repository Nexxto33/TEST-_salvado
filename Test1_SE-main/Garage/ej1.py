import RPi.GPIO as GPIO
import time
import serial
import random
import threading

TRIG = 22  
ECHO = 26
IN1 = 5
IN2 = 6
ENA = 13
button1 = 16

#Bandera para habilitar o deshabilitar la bandera de temperatura
enviar_temp = True

GPIO.setmode(GPIO.BCM)  
GPIO.setup(TRIG, GPIO.OUT)  
GPIO.setup(ECHO, GPIO.IN)
GPIO.setup(IN1, GPIO.OUT)
GPIO.setup(IN2, GPIO.OUT)
GPIO.setup(ENA, GPIO.OUT)
GPIO.setup(button1, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Configuración del UART
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
ser.reset_input_buffer()
time.sleep(2)
print("UART conectado en /dev/ttyACM0")

# Inicializar PWM
pwm = GPIO.PWM(ENA, 1000)
pwm.start(0)
GPIO.output(IN1, GPIO.LOW)
GPIO.output(IN2, GPIO.HIGH)

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
        temperatura = random.randint(-5, 45)
        print(f"Temperatura generada: {temperatura} °C")
        try:
            with open("temp.txt", "r") as f:
                temp_str = f.read().strip()
                temp = float(temp_str)
                print(f"Umbral leído desde archivo: {temp} °C")
        except Exception as e:
            print("Error leyendo temp.txt")
            temp = 20

        if temp == 1:

            if temperatura < 20:
                ser.write("C\n".encode('utf-8'))
                print("Comando enviado: C")
                
            else:
                ser.write("D\n".encode('utf-8'))
                print("Comando enviado: D")

        # Intentar leer respuesta de la TIVA después de enviar el comando
        respuesta = ''
        timeout = time.time() + 3
        while time.time() < timeout:
            if ser.in_waiting > 0:
                respuesta = ser.readline().decode('utf-8').strip()
                if respuesta:
                    print("Respuesta de la TIVA:", respuesta)
                    break
            time.sleep(0.1)

        if not respuesta:
            print("No se recibió respuesta de la TIVA")

        time.sleep(1)

#Parte del codigo para abrir 2 hilos al mismo tiempo(ejecutando el envio de temperatura en segundo plano)
temp_thread = threading.Thread(target=enviar_temperatura, daemon=True)
temp_thread.start()

# Programa principal
try:
    while True:
        distancia = medir_distancia()
        print(f"Distancia medida: {distancia} cm")
        if GPIO.input(button1) == GPIO.LOW:
            ser.write("A\n".encode('utf-8'))
            enviar_temp = False
            time.sleep(0.1)
            print("Boton")
            enviar_temp = True

            # Leer respuesta de la TIVA
            respuesta = ''
            timeout = time.time() + 2  # Timeout de 2 segundos
            while time.time() < timeout:
                if ser.in_waiting > 0:  # Verifica si hay datos en el buffer
                    respuesta = ser.readline().decode('utf-8').strip()
                    print("Respuesta de la TIVA:", respuesta)
                    
                    # Asegurarse de que la respuesta no esté vacía
                    if respuesta:
                        break  # Rompe el bucle y hace la siguiente medición
                time.sleep(0.1)  # Esperar un poco antes de verificar nuevamente
            
            # Si no se recibe respuesta en el tiempo esperado
            if not respuesta:
                print("No se recibió respuesta de la TIVA")
        # Si la distancia es menor a 8 cm, enviar el comando 'A'
        if distancia < 8:
            ser.write("B\n".encode('utf-8'))
            pwm.ChangeDutyCycle(100)
            enviar_temp = False
            time.sleep(0.1)  # Enviar comando a la TIVA
            enviar_temp = True
            
            # Leer respuesta de la TIVA
            respuesta = ''
            timeout = time.time() + 2  # Timeout de 2 segundos
            while time.time() < timeout:
                if ser.in_waiting > 0:  # Verifica si hay datos en el buffer
                    respuesta = ser.readline().decode('utf-8').strip()
                    print("Respuesta de la TIVA:", respuesta)
                    
                    # Asegurarse de que la respuesta no esté vacía
                    if respuesta:
                        break  # Rompe el bucle y hace la siguiente medición
                time.sleep(0.1)  # Esperar un poco antes de verificar nuevamente
            
            # Si no se recibe respuesta en el tiempo esperado
            if not respuesta:
                print("No se recibió respuesta de la TIVA")
        else:
            pwm.ChangeDutyCycle(0)
except KeyboardInterrupt:
    print("Medición interrumpida por el usuario")

finally:
    GPIO.cleanup()  # Limpiar los pines GPIO
    ser.close()  # Cerrar la conexión serial