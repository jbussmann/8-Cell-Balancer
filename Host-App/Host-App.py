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

    def __init__(self, event_loop):
        super().__init__()
        self.title("8-Cell Balancer")
        self.wm_iconphoto(True, tk.PhotoImage(file="icon_20.png"))
        width = 600
        height = 400

        self.btn_pwm_pressed = False
        self.app_exit_flag = False
        self.is_values_curr_ready = False
        self.is_values_volt_ready = False
        self.counter_seconds = 0
        self.device = None
        self.file = None
        self.protocol("WM_DELETE_WINDOW", self.close)
        self.event_loop = event_loop
        self.gui_task = self.event_loop.create_task(self.gui_loop())
        self.connection_task = self.event_loop.create_task(self.connection_loop())

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
        self.figure = Figure(constrained_layout=True)
        axes_volt, axes_curr = self.figure.subplots(2, 1)
        self.values = {
            "voltage_12h": None,
            "voltage_1h": None,
            "voltage_2min": None,
            "current_12h": None,
            "current_1h": None,
            "current_2min": None
        }
        for i in self.values:
            self.values[i] = [[None] * 121 for _ in range(8)]

        colors = seaborn.color_palette("tab10", 8)
        self.lines_volt = [None] * 8
        self.lines_curr = [None] * 8

        for i in range(8):
            self.lines_volt[i], = axes_volt.plot(self.values["voltage_2min"][i], label=f'Cell {i+1}', color=colors[i])
            self.lines_curr[i], = axes_curr.plot(self.values["current_2min"][i], label=f'Cell {i+1}', color=colors[i])

        axes_volt.set_xlim(-1, 121)
        axes_volt.set_ylim(3.15, 3.7)
        axes_volt.grid()

        axes_curr.set_xlim(-1, 121)
        axes_curr.set_ylim(-0.02, 0.25)
        axes_curr.grid()

        self.legend = self.figure.legend(handles=self.lines_volt, loc='outside center right', labelspacing=1.3, handlelength=0, labelcolor='linecolor', edgecolor='None')
        
        # place plot
        self.canvas = FigureCanvasTkAgg(self.figure, master=self)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(row=0, column=0, columnspan=3, padx='5', pady='5', sticky='nesw')
        
        self.plot_range = tk.StringVar()
        self.plot_range.set("2min")
        
        self.rbt_12h = tk.Radiobutton(self, text="12h", value="12h", variable=self.plot_range)
        self.rbt_12h.grid(row=1, column=0, padx='5', pady='5', sticky='ew')

        self.rbt_1h = tk.Radiobutton(self, text="1h", value="1h", variable=self.plot_range)
        self.rbt_1h.grid(row=1, column=1, padx='5', pady='5', sticky='ew')

        self.rbt_2min = tk.Radiobutton(self, text="2min", value="2min", variable=self.plot_range)
        self.rbt_2min.grid(row=1, column=2, padx='5', pady='5', sticky='ew')

        self.lbl_text = tk.Label(self, text="")
        self.lbl_text.grid(row=2, column=0, columnspan=3, padx='5', pady='5', sticky='nesw')

        self.lbl_pwm = tk.Label(self, text="0-100%")
        self.lbl_pwm.grid(row=3, column=0, padx='5', pady='5', sticky='ew')

        self.etr_pwm = tk.Entry(self, text="", justify="right", width=10)
        self.etr_pwm.insert(0, '  0,  0,  0,  0,  0,  0,  0,  0')
        self.etr_pwm.grid(row=3, column=1, padx='5', pady='5', sticky='ew')

        self.btn_pwm_set = tk.Button(self, text="Set PWM", state="disabled")
        self.btn_pwm_set.config(command=lambda: self.button_callback(self.btn_pwm_set))
        self.btn_pwm_set.grid(row=3, column=2, padx='5', pady='5', sticky='ew')
        
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
            self.values["voltage_2min"][i].pop(0)
            self.values["voltage_2min"][i].append(float(string_split[i])/1000)
        self.is_values_volt_ready = True
        # self.canvas.draw()
        
    def current_callback(self, sender, data):
        string = data.decode()
        print("current: " + string)
        string_split = string.split(',')
        # self.file.write(f"{string}\n")
        # self.lbl_text.config(text=string)

        for i in range(8):
            self.values["current_2min"][i].pop(0)
            self.values["current_2min"][i].append(float(string_split[i])/1000)
            self.lines_curr[i].set_ydata(self.values["current_2min"][i])
        self.is_values_curr_ready = True
        # self.canvas.draw()

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
            val = self.etr_pwm.get()
            await self.client.write_gatt_char(uuids['pwm_set'], val.encode())

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
        self.event_loop.stop()
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
            if self.is_values_volt_ready and self.is_values_curr_ready:
                self.is_values_volt_ready = False
                self.is_values_curr_ready = False
                self.counter_seconds += 1
                plot_range = self.plot_range.get()
                for i in range(8):
                    self.lines_volt[i].set_ydata(self.values[f"voltage_{plot_range}"][i])
                    self.lines_curr[i].set_ydata(self.values[f"current_{plot_range}"][i])
                self.canvas.draw()
                if self.counter_seconds%30 == 0:
                    for i in range(8):
                        mean = sum(self.values["voltage_2min"][i][-30:])/30
                        self.values["voltage_1h"][i].pop(0)
                        self.values["voltage_1h"][i].append(mean)
                        mean = sum(self.values["current_2min"][i][-30:])/30
                        self.values["current_1h"][i].pop(0)
                        self.values["current_1h"][i].append(mean)
                    print(f"{self.counter_seconds}s: 2min->1h")
                if self.counter_seconds%360 == 0:
                    for i in range(8):
                        mean = sum(self.values["voltage_1h"][i][-12:])/12
                        self.values["voltage_12h"][i].pop(0)
                        self.values["voltage_12h"][i].append(mean)
                        mean = sum(self.values["current_1h"][i][-12:])/12
                        self.values["current_12h"][i].pop(0)
                        self.values["current_12h"][i].append(mean)
                    print(f"{self.counter_seconds}s: 1h->12h")
            await asyncio.sleep(0.1)

    def close(self):
        self.app_exit_flag = True
        self.lbl_text.config(text="closing")
        # self.gui_task.cancel()
        # self.connection_task.cancel()
        # self.loop.stop()


# Main function, executed when file is invoked directly.
if __name__ == "__main__":
    event_loop = asyncio.get_event_loop()
    MainWindow(event_loop)
    event_loop.run_forever()
    event_loop.close()
