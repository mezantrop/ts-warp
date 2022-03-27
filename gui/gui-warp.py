#!/usr/bin/env python3

"""
--------------------------------------------------------------------------------
GUI frontend for TS-Warp - Transparent SOCKS protocol Wrapper
--------------------------------------------------------------------------------

Copyright (c) 2022, Mikhail Zakharov <zmey20000@yahoo.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

# TODO:
#   1. Create INI-configuration editor
#   2. Create Firewall configuration editor
#   3. In editors/viewers add buttons: save, refresh, pause

import tkinter as tk
import tkinter.ttk as ttk
import subprocess

import gui_conf


class App:
    def __init__(self, root,
                 runcmd='/usr/local/etc/ts-warp.sh',
                 inifile='/usr/local/etc/ts-warp.ini',
                 logfile='/usr/local/var/log/ts-warp.log',
                 pidfile='/usr/local/var/run/ts-warp.pid'):

        ini_data = None

        root.title("TS-WARP GUI v0.1")
        width = 800
        height = 560
        root.geometry(f'{width}x{height}')
        root.resizable(width=True, height=True)

        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=0)
        root.rowconfigure(1, weight=1)
        root.rowconfigure(2, weight=0)

        # Top button frame
        lfrm_top = tk.LabelFrame(root, height=40, relief=tk.FLAT, padx=2, pady=4)
        lfrm_top.grid(column=0, row=0, sticky=tk.N + tk.W + tk.E)
        lfrm_top.columnconfigure(0, weight=0)
        lfrm_top.columnconfigure(1, weight=0)
        lfrm_top.columnconfigure(2, weight=0)

        btn_run = tk.Button(lfrm_top, width=2, height=1, text='▶')
        btn_run.grid(row=0, column=0, sticky=tk.W, padx=2)
        btn_run['command'] = lambda: self.startstop(btn_run, runcmd)

        btn_rld = tk.Button(lfrm_top, width=2, height=1, text='⟳')
        btn_rld.grid(row=0, column=1, sticky=tk.W, padx=2)
        btn_rld['command'] = lambda: subprocess.run([runcmd, 'reload'])

        # Display config/log pane
        tabControl = ttk.Notebook(root)
        tab_ini = ttk.Frame(tabControl)
        tab_log = ttk.Frame(tabControl)

        tabControl.add(tab_ini, text='INI')
        tabControl.add(tab_log, text='Log')
        tabControl.grid(column=0, row=1, sticky=tk.NSEW)

        # Tab INI/Config
        tab_ini.columnconfigure(0, weight=1)
        tab_ini.rowconfigure(0, weight=1)

        ini_txt = tk.Text(tab_ini)
        ini_txt.grid(column=0, row=0, sticky=tk.NSEW)
        tab_ini.bind("<Visibility>", self.readfile(ini_txt, inifile, refresh=False))

        scroll_ini = tk.Scrollbar(tab_ini, orient=tk.VERTICAL)
        scroll_ini.grid(column=1, row=0, sticky=tk.NSEW)
        scroll_ini.config(command=ini_txt.yview)
        ini_txt.config(yscrollcommand=scroll_ini.set)

        # Tab Log
        tab_log.columnconfigure(0, weight=1)
        tab_log.rowconfigure(0, weight=1)

        log_txt = tk.Text(tab_log)
        log_txt.grid(column=0, row=0, sticky=tk.NSEW)
        tab_log.bind("<Visibility>", self.readfile(log_txt, logfile, refresh=True))

        scroll_log = tk.Scrollbar(tab_log, orient=tk.VERTICAL)
        scroll_log.grid(column=1, row=0, sticky=tk.NSEW)
        scroll_log.config(command=log_txt.yview)
        log_txt.config(yscrollcommand=scroll_log.set)

        # Status bar
        lfrm_bottom = tk.LabelFrame(root, relief=tk.FLAT, padx=2)
        lfrm_bottom.grid(column=0, row=2, sticky=tk.S + tk.W + tk.E)
        lfrm_bottom.columnconfigure(0, weight=1)

        lbl_stat = tk.Label(lfrm_bottom, text='■ running', fg='green')
        lbl_stat.grid(row=0, column=0, sticky=tk.E)
        self.status(lbl_stat, btn_run, pidfile)

    def readfile(self, t_widget, filename, refresh=False):
        t_widget.config(state='normal')
        lf = open(filename, 'r')
        t_widget.delete(1.0, tk.END)
        t_widget.insert(tk.END, lf.read())
        t_widget.see(tk.END)
        lf.close()
        if refresh:
            t_widget.config(state='disabled')
            root.after(1000, self.readfile, t_widget, filename, refresh)

    def status(self, lbl, btn, pidfile):
        pf = None
        try:
            pf = open(pidfile, 'r')
        except:
            lbl['text'] = '■ Stopped'
            lbl['fg'] = 'red'
            btn['text'] = '▶'

        if pf:
            lbl['text'] = '■ Running: ' + pf.readline()[:-1]
            lbl['fg'] = 'green'
            btn['text'] = '■'
            pf.close()

        root.after(1000, self.status, lbl, btn, pidfile)

    def startstop(self, t_widget, runcmd):
        if t_widget['text'] == '■':
            subprocess.run([runcmd, 'stop'])
        else:
            subprocess.run([runcmd, 'start'])


if __name__ == "__main__":
    root = tk.Tk()
    app = App(root,
              runcmd=gui_conf.runcmd,
              inifile=gui_conf.inifile,
              logfile=gui_conf.logfile,
              pidfile=gui_conf.pidfile)
    root.mainloop()
