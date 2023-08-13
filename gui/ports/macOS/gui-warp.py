#!/usr/bin/env python3

"""
------------------------------------------------------------------------------------------------------------------------
GUI frontend for TS-Warp - Transparent proxy server and traffic wrapper (macOS app)
------------------------------------------------------------------------------------------------------------------------

Copyright (c) 2022-2023 Mikhail Zakharov <zmey20000@yahoo.com>

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
   disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

# -------------------------------------------------------------------------------------------------------------------- #
# TODO:
#   1. Rewrite ugly code
#   2. Redo "refresh" timers
#   3. Implement "tail" for the LOG-file
#   4. Does the full featured configuration editor with input fields and etc is required?

# -------------------------------------------------------------------------------------------------------------------- #
import os
import sys
import tkinter as tk
import tkinter.ttk as ttk
import subprocess
import configparser
import shutil


# -------------------------------------------------------------------------------------------------------------------- #
class App:
    def __init__(self, width=800, height=560,
                 runcmd='/usr/local/etc/ts-warp.sh',
                 inifile='/usr/local/etc/ts-warp.ini',
                 fwfile='/usr/local/etc/ts-warp_pf.conf',
                 logfile='/usr/local/var/log/ts-warp.log',
                 pidfile='/usr/local/var/run/ts-warp.pid'):

        self.password = ''

        self.version = 'v1.0.14-mac'
        self.width = width
        self.height = height

        # Tk widgets formatting variables
        self._padx = 2
        self._pady = 4
        self._btnw = 2                                                  # Button width

        # -- GUI ----------------------------------------------------------------------------------------------------- #
        self.root = tk.Tk()

        # Password dialog as modal window
        if os.geteuid() != 0:
            self.ask_password()

        self.root.title(f'GUI-warp {self.version}')
        self.root.geometry(f'{self.width}x{self.height}')
        self.root.resizable(width=True, height=True)

        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=0)
        self.root.rowconfigure(1, weight=1)

        # -- Top button frame ---------------------------------------------------------------------------------------- #
        lfrm_top = tk.LabelFrame(self.root, height=40, relief=tk.FLAT, padx=self._padx, pady=self._pady)
        lfrm_top.grid(column=0, row=0, sticky=tk.EW)

        btn_rld1 = ttk.Button(lfrm_top, width=self._btnw, text='⟳')
        btn_rld1.grid(column=0, row=0, sticky=tk.W, padx=self._padx)
        btn_rld1['command'] = lambda: self.run_script('reload')

        btn_run = ttk.Button(lfrm_top, width=self._btnw, text='▶')
        btn_run.grid(column=1, row=0, sticky=tk.W, padx=self._padx)
        btn_run['command'] = lambda: self.run_script('stop') if btn_run['text'] == '■' else self.run_script('start')

        ttk.Label(lfrm_top, text='Log-level:').grid(column=2, row=0, sticky=tk.W)
        self.cmb_lvl = ttk.Combobox(lfrm_top, state="readonly", values=[1, 2, 3, 4], width=1)
        self.cmb_lvl.current(1)
        self.cmb_lvl.grid(column=3, row=0, sticky=tk.W, padx=self._padx)

        ttk.Label(lfrm_top, text='Options:').grid(column=4, row=0, sticky=tk.W)
        lfrm_top.columnconfigure(5, weight=1)
        self.tsw_opts = tk.StringVar()
        self.ent_opt = ttk.Entry(lfrm_top, textvariable=self.tsw_opts).grid(column=5, row=0, padx=3, sticky=tk.EW)

        # -- Display ini/fw/log pane --------------------------------------------------------------------------------- #
        tabControl = ttk.Notebook(self.root)
        tab_ini = ttk.Frame(tabControl)
        tab_log = ttk.Frame(tabControl)

        tabControl.add(tab_log, text='Log')
        tabControl.add(tab_ini, text='INI')
        tabControl.grid(column=0, row=1, sticky=tk.NSEW)

        # -- Tab Log ------------------------------------------------------------------------------------------------- #
        tab_log.columnconfigure(0, weight=1)
        tab_log.rowconfigure(1, weight=1)

        ttk.Label(tab_log, text='Log auto-refresh:').grid(column=0, row=0, sticky=tk.E)

        btn_pause = ttk.Button(tab_log, width=self._btnw, text='■')
        btn_pause.grid(column=1, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        self.pause = False
        btn_pause['command'] = lambda: self.pauselog(btn_pause, log_txt, logfile)

        log_txt = tk.Text(tab_log, highlightthickness=0, state='disabled')
        log_txt.grid(column=0, row=1, columnspan=2, sticky=tk.NSEW)
        tab_log.bind("<Visibility>", self.readfile(log_txt, logfile, refresh=True))

        scroll_log = ttk.Scrollbar(tab_log, orient=tk.VERTICAL)
        scroll_log.grid(column=2, row=1, sticky=tk.NSEW)
        scroll_log.config(command=log_txt.yview)
        log_txt.config(yscrollcommand=scroll_log.set)

        # -- Tab INI ------------------------------------------------------------------------------------------------- #
        tab_ini.columnconfigure(0, weight=1)
        tab_ini.rowconfigure(1, weight=1)

        frm_tab_ini_top = ttk.Frame(tab_ini)
        frm_tab_ini_top.grid(column=0, row=0, sticky=tk.EW)
        frm_tab_ini_top.columnconfigure(0, weight=0)
        frm_tab_ini_top.columnconfigure(1, weight=1)
        frm_tab_ini_top.columnconfigure(2, weight=0)
        frm_tab_ini_top.columnconfigure(3, weight=0)
        frm_tab_ini_top.rowconfigure(1, weight=1)

        btn_tswhash = ttk.Button(frm_tab_ini_top, text='Password hash')
        btn_tswhash.grid(column=0, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        self.tswhash = tk.StringVar()
        self.ent_tswhash = ttk.Entry(frm_tab_ini_top, textvariable=self.tswhash).grid(column=1, row=0, padx=3, sticky=tk.EW)
        btn_tswhash['command'] = lambda: self.tswhash.set(
            'tsw01:' + subprocess.Popen(['./ts-pass', self.tswhash.get().encode()],
                                        stdout=subprocess.PIPE).stdout.read().decode().strip('\n\r'))

        ttk.Label(frm_tab_ini_top, text='Save changes:').grid(column=2, row=0, sticky=tk.E)
        btn_save_ini = ttk.Button(frm_tab_ini_top, width=self._btnw, text='▲')
        btn_save_ini.grid(column=3, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        btn_save_ini['command'] = lambda: self.saveini(ini_txt, inifile)

        ini_txt = tk.Text(tab_ini, highlightthickness=0)
        ini_txt.grid(column=0, row=1, columnspan=2, sticky=tk.NSEW)
        tab_ini.bind("<Visibility>", self.readfile(ini_txt, inifile, refresh=False))

        scroll_ini = ttk.Scrollbar(tab_ini, orient=tk.VERTICAL)
        scroll_ini.grid(column=2, row=1, sticky=tk.NSEW)
        scroll_ini.config(command=ini_txt.yview)
        ini_txt.config(yscrollcommand=scroll_ini.set)

        # -- Status bar ---------------------------------------------------------------------------------------------- #
        lfrm_bottom = tk.LabelFrame(self.root, relief=tk.FLAT, padx=self._padx)
        lfrm_bottom.grid(column=0, row=2, sticky=tk.EW)
        lfrm_bottom.columnconfigure(0, weight=1)

        lbl_stat = ttk.Label(lfrm_bottom, text='■ running', foreground='green')
        lbl_stat.grid(row=0, column=0, sticky=tk.E)
        self.status(lbl_stat, btn_run, pidfile)

        self.root.mainloop()

    def readfile(self, t_widget, filename, refresh=False):
        t_widget.config(state='normal')
        f = open(filename, 'r')
        t_widget.delete(1.0, tk.END)
        t_widget.insert(tk.END, ''.join(f.readlines()))
        t_widget.see(tk.END)
        f.close()
        if refresh:
            t_widget.config(state='disabled')
            if not self.pause:
                self.root.after(3000, self.readfile, t_widget, filename, refresh)

    def saveini(self, t_widget, filename):
        f = open(filename, 'w')
        f.write(t_widget.get('1.0', tk.END))
        f.close()
        # Rebuild ts-warp_pf.conf when saving the INI-file
        with open(fwfile, "w") as outfw:
            subprocess.run(['./ts-warp_autofw.sh', prefix], stdout=outfw)

    def pauselog(self, btn, txt, filename):
        if self.pause:
            self.pause = False
            btn['text'] = '■'                                           # Pause log auto-refresh
            self.readfile(txt, filename, refresh=True)
        else:
            self.pause = True
            btn['text'] = '↭'                                           # Enable auto-refresh

    def status(self, lbl, btn, pidfile):
        pf = None
        try:
            pf = open(pidfile, 'r')
        except:
            lbl['text'] = '■ Stopped'
            lbl['foreground'] = 'red'
            btn['text'] = '▶'

        if pf:
            lbl['text'] = '■ Running: ' + pf.readline()[:-1]
            lbl['foreground'] = 'green'
            btn['text'] = '■'
            pf.close()

        self.root.after(10000, self.status, lbl, btn, pidfile)

    def run_script(self, command):
        if os.geteuid() != 0:
            gwp = subprocess.Popen(
                ['sudo', '-S', '-b', runcmd, command, prefix, '-v', self.cmb_lvl.get(), self.tsw_opts.get()],
                stdin=subprocess.PIPE)
            sout, serr = gwp.communicate(self.password)
        else:
            gwp = subprocess.Popen([runcmd, command, prefix + '/etc', '-v', self.cmb_lvl.get(), self.tsw_opts.get()])

    def ask_password(self):
        padx = 10
        pady = 10

        self.win_pwd = tk.Toplevel(self.root)
        self.win_pwd.protocol("WM_DELETE_WINDOW", lambda: sys.exit(0))
        self.win_pwd.title('Starting TS-Warp GUI-frontend...')
        self.win_pwd.resizable(width=False, height=False)

        self.win_pwd.columnconfigure(0, weight=1)
        self.win_pwd.rowconfigure(0, weight=0)

        # Window modal settings (macOS specific ?)
        self.win_pwd.wait_visibility()
        self.win_pwd.grab_set()
        self.win_pwd.transient(self.root)

        if not shutil.which('sudo'):
            # If sudo is not installed, show error label and exit
            self.lbl_error = ttk.Label(self.win_pwd, text='FATAL: "Sudo" command is not found')
            self.lbl_error.grid(column=0, row=0, sticky=tk.EW, padx=padx, pady=pady)

            self.btn_ok = ttk.Button(self.win_pwd, text="OK")
            self.btn_ok['command'] = lambda: sys.exit(1)
            self.btn_ok.grid(column=1, row=0, sticky=tk.EW, padx=padx, pady=pady)
        else:
            # Ask for password and launch gui-warp.py
            self.lbl_pwd = ttk.Label(self.win_pwd, text='Your password:')
            self.lbl_pwd.grid(column=0, row=0, sticky=tk.EW, padx=padx, pady=pady)

            self.pwd = tk.StringVar()
            self.ent_pwd = ttk.Entry(self.win_pwd, textvariable=self.pwd, show='*')
            self.ent_pwd.bind('<Return>', lambda x: self.get_password())
            self.ent_pwd.grid(column=1, row=0, sticky=tk.EW, padx=padx, pady=pady)

            self.btn_enter = ttk.Button(self.win_pwd, text="Enter")
            self.btn_enter['command'] = lambda: self.get_password()
            self.btn_enter.grid(column=2, row=0, sticky=tk.EW, padx=padx, pady=pady)

    def get_password(self):
        gwp = subprocess.Popen(['sudo', '-S', '-v'], stdin=subprocess.PIPE)
        sout, serr = gwp.communicate(self.pwd.get().encode())

        if gwp.returncode == 1:
            self.password = ''
            self.ent_pwd.config(show='')
            self.pwd.set('Wrong password')
            self.root.after(1000, lambda: self.pwd.set(''))
            self.root.after(1000, lambda: self.ent_pwd.config(show='*'))
        else:
            self.password = self.pwd.get().encode()
            self.win_pwd.destroy()


# -------------------------------------------------------------------------------------------------------------------- #
if __name__ == "__main__":
    ini = configparser.ConfigParser()

    ulocal_prefix = '/usr/local/'
    home_prefix = os.path.expanduser("~/ts-warp/")

    prefix = home_prefix if os.path.exists(home_prefix) and os.path.isdir(home_prefix) else ulocal_prefix

    runcmd = './ts-warp.sh'				                # Use the one included with the app
    ini.read(prefix + 'etc/gui-warp.ini')
    # NB! Prefix could == ulocal_prefix
    inifile = prefix + 'etc/ts-warp.ini'
    fwfile = prefix + 'etc/ts-warp_pf.conf'
    logfile = prefix + 'var/log/ts-warp.log'
    pidfile = prefix + 'var/run/ts-warp.pid'

    if 'GUI-WARP' in ini.sections():
        runcmd = ini['GUI-WARP']['prefix'] + ini['GUI-WARP']['runcmd']
        inifile = ini['GUI-WARP']['prefix'] + ini['GUI-WARP']['inifile']
        fwfile = ini['GUI-WARP']['prefix'] + ini['GUI-WARP']['fwfile']
        logfile = ini['GUI-WARP']['prefix'] + ini['GUI-WARP']['logfile']
        pidfile = ini['GUI-WARP']['prefix'] + ini['GUI-WARP']['pidfile']

    if not os.path.exists(inifile):                     # Failback to the home directory and reset paths
        prefix = home_prefix
        inifile = prefix + 'etc/ts-warp.ini'
        fwfile = prefix + 'etc/ts-warp_pf.conf'
        logfile = prefix + 'var/log/ts-warp.log'
        pidfile = prefix + 'var/run/ts-warp.pid'

        if not os.path.exists(prefix):                  # Create ts-warp dir + subdirs in home
            os.makedirs(prefix + 'etc/')
            os.makedirs(prefix + 'var/log/')
            os.makedirs(prefix + 'var/run/')
        shutil.copyfile('./ts-warp.ini', inifile)
        os.chmod(inifile, 0o600)
    if not os.path.exists(fwfile):
        with open(fwfile, "w") as outfw:
            subprocess.run(['./ts-warp_autofw.sh', prefix], stdout=outfw)
    if not os.path.exists(logfile):
        open(logfile, 'a').close()

    app = App(runcmd=runcmd, inifile=inifile, fwfile=fwfile, logfile=logfile, pidfile=pidfile)
