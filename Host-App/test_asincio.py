import asyncio
import time
import tkinter as tk

def gui():
    root = tk.Tk()
    timer = tk.Button(root, text="Timer", command=wait)
    timer.pack()
    root.mainloop()

def wait():
    start = time.time()
    asyncio.run(sleep())
    print(f'Elapsed: {time.time() - start}')

async def sleep():
    await asyncio.sleep(1)

def main():
    wait()

main()
gui()