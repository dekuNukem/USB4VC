import os
import time
from datetime import datetime
import webbrowser
# import check_update
from tkinter import *
from tkinter import filedialog
from tkinter import simpledialog
from tkinter import messagebox
import urllib.request

THIS_VERSION_NUMBER = '0.1.0'
MAIN_WINDOW_WIDTH = 800
MAIN_WINDOW_HEIGHT = 600
PADDING = 10
HEIGHT_CONNECT_LF = 60

root = Tk()
root.title("USB4VC Configurator v" + THIS_VERSION_NUMBER)
root.geometry(str(MAIN_WINDOW_WIDTH) + "x" + str(MAIN_WINDOW_HEIGHT))
root.resizable(width=FALSE, height=FALSE)

connection_lf = LabelFrame(root, text="Dashboard", width=MAIN_WINDOW_WIDTH - PADDING*2, height=HEIGHT_CONNECT_LF)
connection_lf.place(x=PADDING, y=0)

def open_user_manual_url():
    webbrowser.open('https://github.com/dekuNukem/bob_cassette_rewinder/blob/master/user_manual.md')

def open_discord_link():
    try:
        webbrowser.open(str(urllib.request.urlopen(discord_link_url).read().decode('utf-8')).split('\n')[0])
    except Exception as e:
        messagebox.showerror("Error", "Failed to open discord link!\n"+str(e))

user_manual_button = Button(connection_lf, text="User Manual", command=open_user_manual_url)
user_manual_button.place(x=570, y=5, width=100)

discord_button = Button(connection_lf, text="Discord", command=open_discord_link)
discord_button.place(x=680, y=5, width=80)

open_button = Button(connection_lf, text="Open...", command=None)
open_button.place(x=10, y=5, width=80)

info_lf = LabelFrame(root, text="Status", width=MAIN_WINDOW_WIDTH - PADDING*2, height=120)
info_lf.place(x=PADDING, y=70)

mappings_lf = LabelFrame(root, text="Custom Mappings", width=MAIN_WINDOW_WIDTH - PADDING*2, height=390)
mappings_lf.place(x=PADDING, y=200)

root.update()

root.mainloop()
