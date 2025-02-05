import tkinter
import screeninfo


root = tkinter.Tk()
root.wm_title("Embedding in Tk")
root["bd"] = 5
root["bg"] = "#fbe5d6"

width = 400
height = 150

# screeninfo.get_monitors()

monitors = screeninfo.get_monitors()
# for m in monitors:
#     print(m)
if 1 < len(monitors):
    xpos = monitors[1].x + (monitors[1].width - width) // 2
    ypos = monitors[1].y + (monitors[1].height - height) // 2
else:
    xpos = (root.winfo_screenwidth() - width) // 2
    ypos = (root.winfo_screenheight() - height) // 2

# xpos =  (1280 - width) // 2
# ypos = (720 - height) // 2

root.geometry(f"{width}x{height}+{xpos}+{ypos}")
# root.geometry(f"{1300}x{800}")

tkinter.mainloop()
