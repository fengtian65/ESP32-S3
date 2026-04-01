#!/usr/bin/env python3
import socket
import csv
import threading
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
from datetime import datetime

HOST = '0.0.0.0'
PORT = 8888

class MPU6050Visualizer:
    def __init__(self, root):
        self.root = root
        self.root.title("MPU6050 数据可视化")
        self.root.geometry("1000x600")
        
        self.running = False
        self.data_buffer = []
        self.max_points = 100
        self.csv_file = None
        self.csv_writer = None
        self.server_thread = None
        self.server_socket = None
        self.client_socket = None
        
        self.setup_ui()
        self.setup_matplotlib()
        
    def setup_ui(self):
        # 控制按钮
        control_frame = ttk.Frame(self.root, padding="10")
        control_frame.pack(fill=tk.X, side=tk.TOP)
        
        self.start_button = ttk.Button(control_frame, text="开始", command=self.start)
        self.start_button.pack(side=tk.LEFT, padx=5)
        
        self.stop_button = ttk.Button(control_frame, text="暂停", command=self.stop, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=5)
        
        self.exit_button = ttk.Button(control_frame, text="退出", command=self.exit_app)
        self.exit_button.pack(side=tk.LEFT, padx=5)
        
        # 状态标签
        self.status_var = tk.StringVar(value="就绪")
        status_label = ttk.Label(control_frame, textvariable=self.status_var)
        status_label.pack(side=tk.RIGHT)
        
        # 数据显示
        data_frame = ttk.Frame(self.root, padding="10")
        data_frame.pack(fill=tk.BOTH, expand=True)
        
        # 波形图
        self.figure = plt.Figure(figsize=(10, 6), dpi=100)
        self.ax = self.figure.add_subplot(111)
        self.canvas = FigureCanvasTkAgg(self.figure, data_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
    def setup_matplotlib(self):
        self.ax.set_title('MPU6050 实时数据')
        self.ax.set_xlabel('时间')
        self.ax.set_ylabel('数值')
        self.ax.grid(True)
        
        # 初始化数据
        self.timestamps = np.array([])
        self.accel_data = {'x': np.array([]), 'y': np.array([]), 'z': np.array([])}
        self.gyro_data = {'x': np.array([]), 'y': np.array([]), 'z': np.array([])}
        self.temp_data = np.array([])
        
    def start(self):
        if not self.running:
            self.running = True
            self.start_button.config(state=tk.DISABLED)
            self.stop_button.config(state=tk.NORMAL)
            self.status_var.set("正在接收数据...")
            
            # 创建CSV文件
            csv_filename = f'mpu6050_data_{datetime.now().strftime("%Y%m%d_%H%M%S")}.csv'
            self.csv_file = open(csv_filename, 'w', newline='')
            fieldnames = ['timestamp', 'ax', 'ay', 'az', 'gx', 'gy', 'gz', 'temp']
            self.csv_writer = csv.DictWriter(self.csv_file, fieldnames=fieldnames)
            self.csv_writer.writeheader()
            
            # 启动服务器线程
            self.server_thread = threading.Thread(target=self.server_loop)
            self.server_thread.daemon = True
            self.server_thread.start()
            
            # 启动数据更新线程
            self.update_thread = threading.Thread(target=self.update_plot)
            self.update_thread.daemon = True
            self.update_thread.start()
    
    def stop(self):
        if self.running:
            self.running = False
            self.start_button.config(state=tk.NORMAL)
            self.stop_button.config(state=tk.DISABLED)
            self.status_var.set("已暂停")
            
            # 关闭CSV文件
            if self.csv_file:
                self.csv_file.close()
                self.csv_file = None
            
            # 关闭客户端连接
            if self.client_socket:
                try:
                    self.client_socket.close()
                except:
                    pass
            
            # 关闭服务器
            if self.server_socket:
                try:
                    self.server_socket.close()
                except:
                    pass
    
    def exit_app(self):
        """彻底退出应用程序，清理所有资源"""
        self.status_var.set("正在退出...")
        self.root.update()
        
        # 先停止数据接收
        if self.running:
            self.stop()
        
        # 等待线程结束
        if self.server_thread and self.server_thread.is_alive():
            import time
            time.sleep(0.5)
        
        # 关闭所有文件
        if self.csv_file:
            try:
                self.csv_file.close()
            except:
                pass
        
        # 关闭所有网络连接
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
        
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
        
        # 退出应用
        self.root.quit()
        self.root.destroy()
    
    def server_loop(self):
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.bind((HOST, PORT))
            self.server_socket.listen()
            print(f"Server listening on {HOST}:{PORT}")
            
            while self.running:
                try:
                    self.client_socket, addr = self.server_socket.accept()
                    print(f"Connected by {addr}")
                    self.status_var.set(f"已连接: {addr}")
                    
                    buffer = b''
                    while self.running:
                        data = self.client_socket.recv(1024)
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
                                    timestamp = datetime.now()
                                    
                                    # 保存到CSV
                                    if self.csv_writer:
                                        self.csv_writer.writerow({
                                            'timestamp': timestamp.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3],
                                            'ax': ax, 'ay': ay, 'az': az,
                                            'gx': gx, 'gy': gy, 'gz': gz,
                                            'temp': temp
                                        })
                                        self.csv_file.flush()
                                    
                                    # 添加到数据缓冲区
                                    self.data_buffer.append({
                                        'timestamp': timestamp,
                                        'ax': ax, 'ay': ay, 'az': az,
                                        'gx': gx, 'gy': gy, 'gz': gz,
                                        'temp': temp
                                    })
                            except Exception as e:
                                print(f"Error processing line: {e}")
                                continue
                except Exception as e:
                    print(f"Server error: {e}")
                    if not self.running:
                        break
        except Exception as e:
            print(f"Server setup error: {e}")
        finally:
            # 确保关闭服务器socket
            if hasattr(self, 'server_socket') and self.server_socket:
                try:
                    self.server_socket.close()
                except:
                    pass
                self.server_socket = None
    
    def update_plot(self):
        while self.running:
            if self.data_buffer:
                # 更新数据
                new_data = self.data_buffer.copy()
                self.data_buffer.clear()
                
                for item in new_data:
                    # 添加新数据点
                    self.timestamps = np.append(self.timestamps, item['timestamp'].timestamp())
                    self.accel_data['x'] = np.append(self.accel_data['x'], item['ax'])
                    self.accel_data['y'] = np.append(self.accel_data['y'], item['ay'])
                    self.accel_data['z'] = np.append(self.accel_data['z'], item['az'])
                    self.gyro_data['x'] = np.append(self.gyro_data['x'], item['gx'])
                    self.gyro_data['y'] = np.append(self.gyro_data['y'], item['gy'])
                    self.gyro_data['z'] = np.append(self.gyro_data['z'], item['gz'])
                    self.temp_data = np.append(self.temp_data, item['temp'])
                
                # 保持数据点数量在限制内
                if len(self.timestamps) > self.max_points:
                    self.timestamps = self.timestamps[-self.max_points:]
                    for key in self.accel_data:
                        self.accel_data[key] = self.accel_data[key][-self.max_points:]
                    for key in self.gyro_data:
                        self.gyro_data[key] = self.gyro_data[key][-self.max_points:]
                    self.temp_data = self.temp_data[-self.max_points:]
                
                # 绘制图形
                self.ax.clear()
                self.ax.set_title('MPU6050 实时数据')
                self.ax.set_xlabel('时间')
                self.ax.set_ylabel('数值')
                self.ax.grid(True)
                
                # 绘制加速度数据
                self.ax.plot(self.timestamps, self.accel_data['x'], 'r-', label='Accel X')
                self.ax.plot(self.timestamps, self.accel_data['y'], 'g-', label='Accel Y')
                self.ax.plot(self.timestamps, self.accel_data['z'], 'b-', label='Accel Z')
                
                # 绘制陀螺仪数据（缩放）
                self.ax.plot(self.timestamps, self.gyro_data['x']/10, 'r--', label='Gyro X (/10)')
                self.ax.plot(self.timestamps, self.gyro_data['y']/10, 'g--', label='Gyro Y (/10)')
                self.ax.plot(self.timestamps, self.gyro_data['z']/10, 'b--', label='Gyro Z (/10)')
                
                # 绘制温度数据
                self.ax.plot(self.timestamps, self.temp_data-20, 'y-', label='Temp (-20)')
                
                self.ax.legend()
                self.canvas.draw()
            
            # 短暂休眠
            import time
            time.sleep(0.1)

if __name__ == "__main__":
    root = tk.Tk()
    app = MPU6050Visualizer(root)
    root.mainloop()
