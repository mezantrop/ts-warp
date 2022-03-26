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

# TODO: Create INI-configuration editor

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

        tabControl.add(tab_ini, text='Configuration')
        tabControl.add(tab_log, text='Log')
        tabControl.grid(column=0, row=1, sticky=tk.NSEW)

        # Tab INI/Config
        tab_ini.columnconfigure(0, weight=1)
        tab_ini.rowconfigure(0, weight=1)

        ini_cnv = tk.Canvas(tab_ini)
        ini_cnv.grid(column=0, row=0, sticky=tk.NSEW)

        scroll_ini = tk.Scrollbar(tab_ini, orient=tk.VERTICAL)
        scroll_ini.grid(column=1, row=0, sticky=tk.NSEW)
        scroll_ini.config(command=ini_cnv.yview)
        ini_cnv.config(yscrollcommand=scroll_ini.set)

        ini_frm = tk.Frame(ini_cnv)
        ini_cnv.create_window((0, 0), window=ini_frm, anchor='nw')

        ini_frm.columnconfigure(0, weight=1)

        ini_f = open(inifile)
        ini_data = ini_f.readlines()

        ini_wgts = []
        n = 0
        for ln in ini_data:
            ln = ln.split('#', 1)[0]
            ln = ln.split(';', 1)[0]
            ln = ln.rstrip()
            ini_frm.rowconfigure(n, weight=1)
            n += 1

            if '[' and ']' in ln:
                ini_section = tk.Entry(ini_frm, width=100)
                ini_section.insert(tk.END, ln)
                ini_section.grid(column=0, row=n, sticky=tk.NSEW)
                ini_wgts.append(ini_section)

            if '=' in ln:
                ini_variable = tk.Entry(ini_frm, width=100)
                ini_variable.insert(tk.END, ln)
                ini_variable.grid(column=0, row=n, sticky=tk.NSEW)
                ini_wgts.append(ini_variable)

        # Tab Log
        tab_log.columnconfigure(0, weight=1)
        tab_log.rowconfigure(0, weight=1)

        log_txt = tk.Text(tab_log)
        log_txt.grid(column=0, row=0, sticky=tk.NSEW)
        log_txt.config(state='disabled')
        tab_log.bind("<Visibility>", self.readfile(log_txt, logfile))

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

    def readfile(self, t_widget, logfile):
        t_widget.config(state='normal')
        lf = open(logfile, 'r')
        t_widget.delete(1.0, tk.END)
        t_widget.insert(tk.END, lf.read())
        t_widget.see(tk.END)
        lf.close()
        t_widget.config(state='disabled')
        root.after(1000, self.readfile, t_widget, logfile)

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
