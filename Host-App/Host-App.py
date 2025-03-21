from UUIDs import uuids
import tkinter as tk
import asyncio
from bleak import BleakClient, BleakScanner
import struct
import seaborn
import screeninfo

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.figure import Figure


class MainWindow(tk.Tk):

    def __init__(self, event_loop, device_name):
        super().__init__()
        scaling = 1
        self.title("8-Cell Balancer")
        # self.wm_iconphoto(True, tk.PhotoImage(file="icon_20.png"))
        self.tk.call('tk', 'scaling', scaling)
        width = 600*scaling
        height = 400*scaling

        self.device_name = device_name

        self.btn_pwm_pressed = False
        self.rbt_2min_pressed = False
        self.rbt_1h_pressed = None
        self.rbt_12h_pressed = None
        self.app_exit_flag = False
        self.is_values_ready = False
        self.is_deviations_ready = False
        self.counter_seconds = 0
        self.device = None
        self.file = None
        self.protocol("WM_DELETE_WINDOW", self.close)
        self.event_loop = event_loop
        self.gui_task = self.event_loop.create_task(self.gui_loop())
        self.connection_task = self.event_loop.create_task(self.connection_loop())

        # Place in the middle of the screen
        monitors = screeninfo.get_monitors()
        # print(monitors)
        if 1 < len(monitors):
            xpos = monitors[1].x + (monitors[1].width - width) // 2
            ypos = monitors[1].y + (monitors[1].height - height) // 2
        else:
            xpos = (self.winfo_screenwidth() - width) // 2
            ypos = (self.winfo_screenheight() - height) // 2
        self.geometry(f"{width}x{height}+{xpos}+{ypos}")
        self.resizable(False, False)
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.columnconfigure(2, weight=1)

        self["bg"] = "#fbe5d6"
        self["bd"] = 5

        # matpotlib figure for plotting
        self.figure = Figure(constrained_layout=True, dpi=100*scaling)
        axes_volt, axes_curr = self.figure.subplots(2, 1)
        self.values = {
            "voltage_12h": None,
            "voltage_1h": None,
            "voltage_2min": None,
            "current_12h": None,
            "current_1h": None,
            "current_2min": None,
        }
        for i in self.values:
            self.values[i] = [[None] * 121 for _ in range(8)]

        colors = seaborn.color_palette("tab10", 8)
        self.lines_volt = [None] * 8
        self.lines_curr = [None] * 8

        for i in range(8):
            (self.lines_volt[i],) = axes_volt.plot(
                self.values["voltage_2min"][i], label=f"Cell {i+1}", color=colors[i]
            )
            (self.lines_curr[i],) = axes_curr.plot(
                self.values["current_2min"][i], label=f"Cell {i+1}", color=colors[i]
            )

        axes_volt.set_xlim(-1, 121)
        # axes_volt.set_ylim(3.15, 3.7)
        axes_volt.set_ylim(3.15, 4.15)
        axes_volt.grid()

        axes_curr.set_xlim(-1, 121)
        axes_curr.set_ylim(-0.02, 0.25)
        axes_curr.grid()

        self.legend = self.figure.legend(
            handles=self.lines_volt,
            loc="outside center right",
            labelspacing=1.3,
            handlelength=0,
            labelcolor="linecolor",
            edgecolor="None",
        )

        # place plot
        self.canvas = FigureCanvasTkAgg(self.figure, master=self)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(
            row=0, column=0, columnspan=3, padx="5", pady="5", sticky="nesw"
        )

        self.plot_range = tk.StringVar()
        self.plot_range.set("2min")

        self.rbt_12h = tk.Radiobutton(
            self, text="12h", value="12h", variable=self.plot_range
        )
        self.rbt_12h.config(command=lambda: self.button_callback(self.rbt_12h))
        self.rbt_12h.grid(row=1, column=0, padx="5", pady="5", sticky="ew")

        self.rbt_1h = tk.Radiobutton(
            self, text="1h", value="1h", variable=self.plot_range
        )
        self.rbt_1h.config(command=lambda: self.button_callback(self.rbt_1h))
        self.rbt_1h.grid(row=1, column=1, padx="5", pady="5", sticky="ew")

        self.rbt_2min = tk.Radiobutton(
            self, text="2min", value="2min", variable=self.plot_range
        )
        self.rbt_2min.config(command=lambda: self.button_callback(self.rbt_2min))
        self.rbt_2min.grid(row=1, column=2, padx="5", pady="5", sticky="ew")

        self.lbl_text = tk.Label(self, text="")
        self.lbl_text.grid(
            row=2, column=0, columnspan=3, padx="5", pady="5", sticky="nesw"
        )

        self.lbl_pwm = tk.Label(self, text="0-100%")
        self.lbl_pwm.grid(row=3, column=0, padx="5", pady="5", sticky="ew")

        self.etr_pwm = tk.Entry(self, text="", justify="right", width=10)
        self.etr_pwm.insert(0, " 00, 00, 00, 00, 00, 00, 00, 00")
        self.etr_pwm.grid(row=3, column=1, padx="5", pady="5", sticky="ew")

        self.btn_pwm_set = tk.Button(self, text="Set PWM", state="disabled")
        self.btn_pwm_set.config(command=lambda: self.button_callback(self.btn_pwm_set))
        self.btn_pwm_set.grid(row=3, column=2, padx="5", pady="5", sticky="ew")

    def button_callback(self, button):
        if button == self.btn_pwm_set:
            self.btn_pwm_pressed = True
        elif button == self.rbt_2min:
            self.rbt_2min_pressed = True
        elif button == self.rbt_1h:
            if self.rbt_1h_pressed is None:
                self.rbt_1h_pressed = "first"
            else:
                self.rbt_1h_pressed = True
        elif button == self.rbt_12h:
            if self.rbt_12h_pressed is None:
                self.rbt_12h_pressed = "first"
            else:
                self.rbt_12h_pressed = True

    def values_callback(self, sender, data):
        values = struct.unpack(f"< {len(data)//2}h", data)
        print(str(values)[1:-1])

        for i in range(8):
            self.values["voltage_2min"][i].pop(0)
            self.values["current_2min"][i].pop(0)
            self.values["voltage_2min"][i].append(float(values[2 * i]) / 1000)
            self.values["current_2min"][i].append(float(values[(2 * i) + 1]) / 1000)
        self.is_values_ready = True
        # self.canvas.draw()

    def deviations_callback(self, sender, data):
        string = data.decode()
        print("dev: " + string)
        # string_split = string.split(',')
        # # self.file.write(f"{string}\n")
        # # self.lbl_text.config(text=string)

        # for i in range(8):
        #     self.values["current_2min"][i].pop(0)
        #     self.values["current_2min"][i].append(float(string_split[i])/1000)
        #     self.lines_curr[i].set_ydata(self.values["current_2min"][i])
        # self.is_values_curr_ready = True
        # self.canvas.draw()

    def history_callback(self, sender, data):
        if sender.uuid == uuids["history_1h"]:
            type = "1h"
        elif sender.uuid == uuids["history_12h"]:
            type = "12h"
        else:
            type = "undefined"
        print(f"callback is type: {type}")

        values_packed = struct.unpack(f"< {len(data)//2}h", data)

        for i in range(len(values_packed) // 16):
            values = values_packed[16 * i : 16 * i + 16]

            if 0 < values[0]:
                for k in range(8):
                    self.values[f"voltage_{type}"][k].pop(0)
                    self.values[f"current_{type}"][k].pop(0)
                    self.values[f"voltage_{type}"][k].append(
                        float(values[2 * k]) / 1000
                    )
                    self.values[f"current_{type}"][k].append(
                        float(values[(2 * k) + 1]) / 1000
                    )

            # print(f"{i}: {values}")

    async def run_buttons(self):
        if self.btn_pwm_pressed:
            self.btn_pwm_pressed = False
            val = self.etr_pwm.get()
            await self.client.write_gatt_char(uuids["pwm_set"], val.encode())

        if self.rbt_2min_pressed:
            self.rbt_2min_pressed = False
            self.draw_plot()

        if type(self.rbt_1h_pressed) is str:
            self.rbt_1h_pressed = False
            await self.client.start_notify(uuids["history_1h"], self.history_callback)
        elif self.rbt_1h_pressed is True:
            self.rbt_1h_pressed = False
            self.draw_plot()

        if type(self.rbt_12h_pressed) is str:
            self.rbt_12h_pressed = False
            await self.client.start_notify(uuids["history_12h"], self.history_callback)
        elif self.rbt_12h_pressed is True:
            self.rbt_12h_pressed = False
            self.draw_plot()

    def draw_plot(self):
        plot_range = self.plot_range.get()
        for i in range(8):
            self.lines_volt[i].set_ydata(self.values[f"voltage_{plot_range}"][i])
            self.lines_curr[i].set_ydata(self.values[f"current_{plot_range}"][i])
        self.canvas.draw()

    async def gui_loop(self):
        while True:
            self.update()
            await asyncio.sleep(0.1)

    async def connection_loop(self):
        while not self.app_exit_flag:
            self.lbl_text.config(text="scanning")
            self.device = await BleakScanner.find_device_by_name(self.device_name, 1)
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
                await self.client.start_notify(uuids["values"], self.values_callback)
                await self.application_loop()
        except Exception as e:
            print(f"Terminating with Exception {e}")
        finally:
            self.btn_pwm_set.config(state="disabled")
            self.lbl_text.config(text="disconnecting")
            await self.client.disconnect()

    async def application_loop(self):
        while not self.app_exit_flag:
            await self.run_buttons()
            if self.is_values_ready:
                self.is_values_ready = False
                self.draw_plot()
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
    MainWindow(event_loop, "8-Cell Balancer")
    event_loop.run_forever()
    event_loop.close()
