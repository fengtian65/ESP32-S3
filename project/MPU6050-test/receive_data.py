#!/usr/bin/env python3
import socket
import csv
from datetime import datetime

HOST = '0.0.0.0'
PORT = 8888
CSV_FILENAME = f'mpu6050_data_{datetime.now().strftime("%Y%m%d_%H%M%S")}.csv'

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print(f"Server listening on {HOST}:{PORT}")
        print(f"Data will be saved to: {CSV_FILENAME}")
        
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            
            with open(CSV_FILENAME, 'w', newline='') as csvfile:
                fieldnames = ['timestamp', 'ax', 'ay', 'az', 'gx', 'gy', 'gz', 'temp']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                writer.writeheader()
                
                buffer = b''
                try:
                    while True:
                        data = conn.recv(1024)
                        if not data:
                            break
                        buffer += data
                        
                        while b'\n' in buffer:
                            line, buffer = buffer.split(b'\n', 1)
                            try:
                                line_str = line.decode('utf-8').strip()
                                if not line_str:
                                    continue
                                values = line_str.split(',')
                                if len(values) == 7:
                                    ax, ay, az, gx, gy, gz, temp = map(float, values)
                                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                                    writer.writerow({
                                        'timestamp': timestamp,
                                        'ax': ax,
                                        'ay': ay,
                                        'az': az,
                                        'gx': gx,
                                        'gy': gy,
                                        'gz': gz,
                                        'temp': temp
                                    })
                                    csvfile.flush()
                                    print(f"Received: {timestamp} | Accel: {ax:.3f}, {ay:.3f}, {az:.3f} | Gyro: {gx:.3f}, {gy:.3f}, {gz:.3f} | Temp: {temp:.2f}°C")
                            except Exception as e:
                                print(f"Error processing line: {e}")
                                continue
                except KeyboardInterrupt:
                    print("\nServer stopped by user")
                except Exception as e:
                    print(f"Error: {e}")

if __name__ == "__main__":
    main()
