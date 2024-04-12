from UUIDs import uuids
import tkinter as tk
import asyncio
from bleak import BleakClient, BleakScanner
import time
import seaborn
from datetime import datetime

from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.figure import Figure

        
class MainWindow(tk.Tk):

    def __init__(self, loop):
        super().__init__()
        self.title("8-Cell Balancer")
        self.wm_iconphoto(True, tk.PhotoImage(file="icon_20.png"))
        width = 600
        height = 400

        self.btn_pwm_pressed = False
        self.app_exit_flag = False
        self.device = None
        self.file = None
        self.protocol("WM_DELETE_WINDOW", self.close)
        self.loop = loop
        self.gui_task = self.loop.create_task(self.gui_loop())
        self.connection_task = self.loop.create_task(self.connection_loop())

        # Place in the middle of the screen
        xpos = (self.winfo_screenwidth() - width) // 2
        ypos = (self.winfo_screenheight() - height) // 2
        # self.geometry(f"{width}x{height}+{xpos}+{ypos}")
        self.geometry(f"{width}x{height}+{xpos+400}+{-600}")
        self.resizable(False, False)
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.columnconfigure(2, weight=1)

        self['bg'] = '#fbe5d6'
        self['bd'] = 5

        # matpotlib figure for plotting 
        self.figure = Figure(tight_layout=True)
        axes_volt, axes_curr = self.figure.subplots(2, 1)
        self.plot_values_volt = [[None] * 31 for _ in range(8)]
        self.plot_values_curr = [[None] * 31 for _ in range(8)]

        colors = seaborn.color_palette("tab10", 8)
        self.lines_volt = [None] * 8
        self.lines_curr = [None] * 8

        for i in range(8):
            self.lines_volt[i], = axes_volt.plot(self.plot_values_volt[i], label=f'Cell {i+1}', color=colors[i])
            self.lines_curr[i], = axes_curr.plot(self.plot_values_curr[i], label=f'Cell {i+1}', color=colors[i])

        axes_volt.set_xlim(-1, 31)
        axes_volt.set_ylim(-0.2, 5.2)
        # axes_volt.legend(loc='center left', ncol=8, bbox_to_anchor=(1.02, 0.5))
        axes_volt.grid()

        axes_curr.set_xlim(-1, 31)
        axes_curr.set_ylim(-0.1, 1.1)
        # axes_curr.legend(loc='center left', ncol=8, bbox_to_anchor=(1.02, 0.5))
        axes_curr.grid()

        # place plot
        self.canvas = FigureCanvasTkAgg(self.figure, master=self)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(row=0, column=0, columnspan=3, padx='5', pady='5', sticky='nesw')

        self.lbl_text = tk.Label(self, text="")
        self.lbl_text.grid(row=1, column=0, columnspan=3, padx='5', pady='5', sticky='nesw')

        self.lbl_pwm = tk.Label(self, text="0-100%")
        self.lbl_pwm.grid(row=2, column=0, padx='5', pady='5', sticky='ew')

        self.etr_pwm = tk.Entry(self, text="", justify="right", width=10)
        self.etr_pwm.insert(0, '  0,  0,  0,  0,  0,  0,  0,  0')
        self.etr_pwm.grid(row=2, column=1, padx='5', pady='5', sticky='ew')

        self.btn_pwm_set = tk.Button(self, text="Set PWM", state="disabled")
        self.btn_pwm_set.config(command=lambda: self.button_callback(self.btn_pwm_set))
        self.btn_pwm_set.grid(row=2, column=2, padx='5', pady='5', sticky='ew')
        
        # self.btn_voltage = tk.Button(self, text="Read Voltage", state="disabled")
        # self.btn_voltage.config(command=lambda: self.button_callback(self.btn_voltage))
        # self.btn_voltage.grid(row=3, column=0, padx='5', pady='5', sticky='ew')

        # self.btn_current = tk.Button(self, text="Read Current", state="disabled")
        # self.btn_current.config(command=lambda: self.button_callback(self.btn_current))
        # self.btn_current.grid(row=3, column=1, padx='5', pady='5', sticky='ew')
        
        self.btn_close = tk.Button(self, text="Close", command=self.close)
        self.btn_close.grid(row=3, column=0, columnspan=3, padx='5', pady='5', sticky='ew')

    def button_callback(self, button):
        if button == self.btn_pwm_set:
            self.btn_pwm_pressed = True

    def string_callback(self, sender, data):
        string = data.decode()
        string_split = string.split(',')
        print(string)
        self.file.write(f"{string}\n")
        self.lbl_text.config(text=string)

        self.plot_values_pow.pop(0)
        self.plot_values_vdc.pop(0)
        self.plot_values_idc.pop(0)
        self.plot_values_pow.append(float(string_split[0])/1000)
        self.plot_values_vdc.append(float(string_split[1])/1000)
        self.plot_values_idc.append(float(string_split[2])/1000)
        self.line_pow.set_ydata(self.plot_values_pow)
        self.line_vdc.set_ydata(self.plot_values_vdc)
        self.line_idc.set_ydata(self.plot_values_idc)
        self.canvas.draw()


    def voltage_callback(self, sender, data):
        string = data.decode()
        print("voltage: " + string)
        string_split = string.split(',')
        # self.file.write(f"{string}\n")
        # self.lbl_text.config(text=string)

        for i in range(8):
            self.plot_values_volt[i].pop(0)
            self.plot_values_volt[i].append(float(string_split[i])/1000)
            self.lines_volt[i].set_ydata(self.plot_values_volt[i])
        self.canvas.draw()
        
    def current_callback(self, sender, data):
        string = data.decode()
        print("current: " + string)
        string_split = string.split(',')
        # self.file.write(f"{string}\n")
        # self.lbl_text.config(text=string)

        for i in range(8):
            self.plot_values_curr[i].pop(0)
            self.plot_values_curr[i].append(float(string_split[i])/1000)
            self.lines_curr[i].set_ydata(self.plot_values_curr[i])
        self.canvas.draw()

    # def raw_callback(self, sender, data):
    #     number = data.pop(0)
    #     data.pop(0)
        
    #     values = [int.from_bytes(data[i:i+2], 'little', signed=True) for i in range(0, len(data), 2)]
    #     for i in range(0, len(values), 8):
    #         self.file.write(f"{values[i]*values[i+1]//1000:5},"
    #                         f"{values[i]:5},{values[i+1]:5},"
    #                         f"{values[i+2]:6},{values[i+3]:6},{values[i+4]:6},"
    #                         f"{values[i+5]:6},{values[i+6]:6},{values[i+7]:6}\n")

    #     self.file.flush()

    #     if number == 0:
    #         self.start = time.time()
    #     if number == 249:
    #         end = time.time()
    #         print(f"Data transmitted in {end - self.start:.1f}s")
    #     self.lbl_text.config(text=f"{number + 1}/250 Messages")

    # async def run_voltage_button(self):
    #     if self.btn_voltage_pressed:
    #         self.btn_voltage_pressed = False
            
            # if not self.reading_active:
            #     self.reading_active = True
            #     self.btn_voltage.config(text="Stop")
            #     self.btn_current.config(state="disabled")

            #     self.file = open(f"buffer_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.csv", 'x')
            #     data = await self.client.read_gatt_char(uuids['string'])
            #     self.file.write(f"{data.decode()}\n")

            #     await self.client.start_notify(uuids['raw'], self.raw_callback)
            # else:
            #     self.reading_active = False
            #     self.btn_voltage.config(text="Read Buffer")
            #     self.btn_current.config(state="normal")
            #     await self.client.stop_notify(uuids['raw'])
            #     self.file.close()
            #     self.file = None

    # async def run_current_button(self):
    #     if self.btn_current_pressed:
    #         self.btn_current_pressed = False
    #         await self.client.start_notify(uuids['current'], self.current_callback)
            # if not self.logging_active:
            #     self.logging_active = True
            #     self.btn_current.config(text="Stop Logging")
            #     self.btn_voltage.config(state="disabled")

            #     timestamp = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
            #     self.file = open("log_" + timestamp + ".csv", 'x')
            #     data = await self.client.read_gatt_char(uuids['string'])
            #     print(data.decode())
            #     self.file.write(f"{data.decode()}\n")

            #     await self.client.start_notify(uuids['string'], self.string_callback)
            # else:
            #     self.logging_active = False
            #     self.btn_current.config(text="Start Logging")
            #     self.btn_voltage.config(state="normal")
            #     await self.client.stop_notify(uuids['string'])
            #     self.file.close()
            #     self.file = None

    async def run_pwm_button(self):
        if self.btn_pwm_pressed:
            self.btn_pwm_pressed = False
            val = int(self.etr_pwm.get())
            await self.client.write_gatt_char(uuids['iset'], val.to_bytes(2, 'little'))

    async def gui_loop(self):
        while True:
            self.update()
            await asyncio.sleep(0.1)

    async def connection_loop(self):
        while not self.app_exit_flag:
            self.lbl_text.config(text="scanning")
            self.device = await BleakScanner.find_device_by_name("8-Cell Balancer", 1)
            print("scan timed out, restarting")
            if self.device:
                self.lbl_text.config(text="connecting")
                await self.connect_device()
                self.device = None

        if self.file:
            print("force close file")
            self.file.close()
        
        print("stopping loop")
        self.loop.stop()
        # self.gui_task.cancel()
        # self.connection_task.cancel()

    async def connect_device(self):
        self.client = BleakClient(self.device)
        try:
            await self.client.connect()
            while not self.app_exit_flag:
                self.lbl_text.config(text="connected")
                self.btn_pwm_set.config(state="normal")
                await self.client.start_notify(uuids['voltage'], self.voltage_callback)
                await self.client.start_notify(uuids['current'], self.current_callback)
                await self.application_loop()
        except Exception as e:
            print(f"Terminating with Exception {e}")
        finally:
            self.btn_pwm_set.config(state="disabled")
            self.lbl_text.config(text="disconnecting")
            await self.client.disconnect()

    async def application_loop(self):
        while not self.app_exit_flag:
            await self.run_pwm_button()
            await asyncio.sleep(0.1)

    def close(self):
        self.app_exit_flag = True
        self.lbl_text.config(text="closing")
        # self.gui_task.cancel()
        # self.connection_task.cancel()
        # self.loop.stop()


# Main function, executed when file is invoked directly.
if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    MainWindow(loop)
    loop.run_forever()
    loop.close()
