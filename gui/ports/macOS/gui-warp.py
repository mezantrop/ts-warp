#!/usr/bin/env python3

"""
------------------------------------------------------------------------------------------------------------------------
GUI frontend for TS-Warp - Transparent proxy server and traffic wrapper (macOS app)
------------------------------------------------------------------------------------------------------------------------

Copyright (c) 2022-2024 Mikhail Zakharov <zmey20000@yahoo.com>

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
#   * Rewrite ugly code
#   * Does the full featured configuration editor with input fields and etc is required?

# -------------------------------------------------------------------------------------------------------------------- #
import os
import sys
import tkinter as tk
import tkinter.ttk as ttk
import subprocess
import configparser
import shutil

import webbrowser
import urllib
import urllib.request

# -------------------------------------------------------------------------------------------------------------------- #
class App:
    """
    The GUI-Warp main class
    """

    def __init__(self, width=800, height=560,
                 runcmd='/usr/local/etc/ts-warp.sh',
                 inifile='/usr/local/etc/ts-warp.ini',
                 daemon_options="",
                 fwfile='/usr/local/etc/ts-warp_pf.conf',
                 logfile='/usr/local/var/log/ts-warp.log',
                 pidfile='/usr/local/var/run/ts-warp.pid',
                 url_new_vesrsion=''):

        self.password = ''

        self.version = 'v1.0.30-mac'
        self.width = width
        self.height = height

        # Tk widgets formatting variables
        self._padx = 2
        self._pady = 4
        self._btnw = 2                                                  # Button width

        self.log_size = 0                                               # logfile and pidfile size / timestamps
        self.pid_time = 0                                               # for file-refreshing

        # -- GUI ----------------------------------------------------------------------------------------------------- #
        self.root = tk.Tk()
        self.root.createcommand('tk::mac::ReopenApplication', self.root.deiconify)

        # Password dialog as modal window
        if os.geteuid() != 0:
            self.ask_password()

        self.root.title(f'GUI-Warp {self.version}')
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
        self.cmb_lvl = ttk.Combobox(lfrm_top, state="readonly", values=[1, 2, 3, 4], width=2)
        self.cmb_lvl.current(1)
        self.cmb_lvl.grid(column=3, row=0, sticky=tk.W, padx=self._padx)
        self.cmb_lvl.bind("<FocusIn>", self.root.focus_set())

        ttk.Label(lfrm_top, text='Options:').grid(column=4, row=0, sticky=tk.W)
        lfrm_top.columnconfigure(5, weight=1)
        self.tsw_opts = tk.StringVar(value=daemon_options)
        ttk.Entry(lfrm_top, textvariable=self.tsw_opts).grid(column=5, row=0, padx=3, sticky=tk.EW)

        # -- Display INI/FW/LOG/ACT pane ----------------------------------------------------------------------------- #
        tab_ctrl = ttk.Notebook(self.root)
        tab_about = ttk.Frame(tab_ctrl)
        tab_ini = ttk.Frame(tab_ctrl)
        tab_log = ttk.Frame(tab_ctrl)
        tab_act = ttk.Frame(tab_ctrl)

        tab_ctrl.add(tab_about, text='About')
        tab_ctrl.add(tab_log, text='Log')
        tab_ctrl.add(tab_ini, text='INI')
        tab_ctrl.add(tab_act, text='ACT')

        tab_ctrl.grid(column=0, row=1, sticky=tk.NSEW)

        # -- Tab LOG ------------------------------------------------------------------------------------------------- #
        tab_log.columnconfigure(0, weight=1)
        tab_log.rowconfigure(1, weight=1)

        ttk.Label(tab_log, text='Log auto-refresh:').grid(column=0, row=0, sticky=tk.E)

        btn_pause = ttk.Button(tab_log, width=self._btnw, text='■')
        btn_pause.grid(column=1, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        self.pause_log = False
        btn_pause['command'] = lambda: self.pauselog(btn_pause, log_txt, logfile)

        log_txt = tk.Text(tab_log, highlightthickness=0, state='disabled')
        log_txt.grid(column=0, row=1, columnspan=2, sticky=tk.NSEW)
        tab_log.bind("<Visibility>", self.readfile_log(log_txt, logfile, refresh=True))

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
        self.ent_tswhash = ttk.Entry(frm_tab_ini_top, textvariable=self.tswhash)
        self.ent_tswhash.grid(column=1, row=0, padx=3, sticky=tk.EW)
        btn_tswhash['command'] = lambda: self.tswhash.set(
            'tsw01:' + subprocess.Popen(['./ts-pass', self.tswhash.get().encode()],
                                        stdout=subprocess.PIPE).stdout.read().decode().strip('\n\r'))

        ttk.Label(frm_tab_ini_top, text='Save changes:').grid(column=2, row=0, sticky=tk.E)
        btn_save_ini = ttk.Button(frm_tab_ini_top, width=self._btnw, text='▲')
        btn_save_ini.grid(column=3, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        btn_save_ini['command'] = lambda: self.saveini(ini_txt, inifile)

        ini_txt = tk.Text(tab_ini, highlightthickness=0)
        ini_txt.grid(column=0, row=1, columnspan=2, sticky=tk.NSEW)
        tab_ini.bind("<Visibility>", self.readfile_ini(ini_txt, inifile))

        scroll_ini = ttk.Scrollbar(tab_ini, orient=tk.VERTICAL)
        scroll_ini.grid(column=2, row=1, sticky=tk.NSEW)
        scroll_ini.config(command=ini_txt.yview)
        ini_txt.config(yscrollcommand=scroll_ini.set)

        # -- Tab ACT ------------------------------------------------------------------------------------------------- #
        tab_act.columnconfigure(0, weight=1)
        tab_act.rowconfigure(1, weight=1)

        ttk.Label(tab_act, text='ACT auto-refresh:').grid(column=0, row=0, sticky=tk.E)

        btn_act = ttk.Button(tab_act, width=self._btnw, text='▶')
        btn_act.grid(column=1, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        self.pause_act = True
        btn_act['command'] = lambda: self.pauseact(btn_act, tree_act, actfile)

        cols_act = ('Time', 'PID', 'Status', 'Section', 'Client', 'Client bytes', 'Target', 'Target bytes')
        tree_act = ttk.Treeview(tab_act, columns=cols_act, show='headings')

        for col in cols_act:
            tree_act.heading(col, text=col)
            tree_act.column(col, width=100)

        tree_act.grid(row=1, column=0, columnspan=2, sticky=tk.NSEW)

        # -- Tab About ----------------------------------------------------------------------------------------------- #
        tab_about.columnconfigure(0, weight=1)
        tab_about.rowconfigure(0, weight=0)
        tab_about.rowconfigure(1, weight=0)
        tab_about.rowconfigure(2, weight=0)
        tab_about.rowconfigure(3, weight=0)
        tab_about.rowconfigure(4, weight=1)
        tab_about.rowconfigure(5, weight=0)

        style_url = ttk.Style()
        style_url.configure("URL.TLabel", foreground="orange")

        about_text = 'TS-Warp - Transparent proxy server and traffic wrapper\n\
It is a free and open-source software, but if you want to support it, please do'

        lbl_about = ttk.Label(tab_about, text=about_text)
        lbl_about.grid(column=0, row=0, sticky=tk.EW, padx=self._padx, pady=self._pady)

        img_bmcoffee = tk.PhotoImage(file="bmcoffee.png")

        lbl_coffe = ttk.Label(tab_about, text='Buy me a coffee', cursor='hand2', image=img_bmcoffee)
        lbl_coffe.grid(column=1, row=0, sticky=tk.W, padx=self._padx, pady=self._pady)
        lbl_coffe.bind("<Button-1>", lambda e: webbrowser.open_new(url_supportus))

        ttk.Separator(tab_about, orient='horizontal').grid(column=0, row=1, sticky=tk.EW, columnspan=2, pady=self._pady)

        btn_newv = ttk.Button(tab_about, text='Check for updates')
        btn_newv.grid(column=0, row=2, sticky=tk.W)

        lbl_newv = ttk.Label(tab_about, text='Not checked', cursor='hand2', style='URL.TLabel')
        lbl_newv.grid(column=1, row=2, sticky=tk.W, padx=self._padx, pady=self._pady)
        lbl_newv.bind("<Button-1>", lambda e: webbrowser.open_new(url_repository))

        btn_newv['command'] = lambda: self.check_new_version(url_new_vesrsion, lbl_newv)

        ttk.Separator(tab_about, orient='horizontal').grid(column=0, row=3, sticky=tk.EW, columnspan=2, pady=self._pady)

        release_txt = tk.Text(tab_about, highlightthickness=0, state='disabled')
        release_txt.grid(column=0, row=4, columnspan=2, sticky=tk.NSEW, pady=self._pady)
        tab_about.bind("<Visibility>", self.readfile_ini(release_txt, 'CHANGELOG.md'))
        release_txt.config(state='disabled')
        release_txt.see('1.0')

        scroll_release = ttk.Scrollbar(tab_about, orient=tk.VERTICAL)
        scroll_release.grid(column=2, row=4, sticky=tk.NSEW, pady=self._pady)
        scroll_release.config(command=release_txt.yview)
        release_txt.config(yscrollcommand=scroll_release.set)

        lbl_contact_txt = ttk.Label(tab_about, text='Mikhail Zakharov, 2021-2024, BSD-2-Clause license')
        lbl_contact_txt.grid(column=0, row=5, sticky=tk.SW, padx=self._padx, pady=self._pady)
        lbl_contact_url = ttk.Label(tab_about, text='zmey20000@yahoo.com', style='URL.TLabel')
        lbl_contact_url.grid(column=1, row=5, sticky=tk.SE, padx=self._padx, pady=self._pady)
        lbl_contact_url.bind("<Button-1>", lambda e: webbrowser.open_new(url_contact))

        # -- Status bar ---------------------------------------------------------------------------------------------- #
        lfrm_bottom = tk.LabelFrame(self.root, relief=tk.FLAT, padx=self._padx)
        lfrm_bottom.grid(column=0, row=2, sticky=tk.EW)
        lfrm_bottom.columnconfigure(0, weight=1)

        lbl_stat = ttk.Label(lfrm_bottom, text='■ running', foreground='green')
        lbl_stat.grid(row=0, column=0, sticky=tk.E)
        self.status(lbl_stat, btn_run, pidfile)

        self.root.mainloop()

    # ---------------------------------------------------------------------------------------------------------------- #
    def check_new_version(self, rvurl, t_widget):
        """
        Check new version
        """

        if rvurl.lower().startswith('https'):
            try:
                with urllib.request.urlopen(rvurl) as f:
                    rverl = [int(l.split()[2].strip('"'))
                            for l in f.read().decode('utf-8').splitlines() if '#define PROG_VERSION_' in l]

                    rveri = int(f'{rverl[0]:03d}{rverl[1]:03d}{rverl[2]:03d}')
            except Exception:
                t_widget['text'] = 'Failed to get information'

            cver = os.popen('./ts-warp -h').read().splitlines()
            for n, l in enumerate(cver):
                if l == 'Version:':
                    lverl = cver[n+1].split('-')[2].split('.')
                    lveri = int(f'{int(lverl[0]):03d}{int(lverl[1]):03d}{int(lverl[2]):03d}')
                    break

            t_widget['text'] = 'Click here to download' if rveri - lveri > 0 else 'No update required'
        else:
            t_widget['text'] = 'Broken update application URL'

    # ---------------------------------------------------------------------------------------------------------------- #
    def read_file_tree(self, t_widget, filename, refresh=False):
        """
        Read contents of a file into a widget
        """

        if not self.pause_act:
            try:
                with open(pidfile, 'r', encoding='utf-8') as pf:
                    subprocess.Popen(['sudo', 'kill', '-USR2', pf.readline()[:-1]])
            except Exception:
                pass

            for item in t_widget.get_children():
                t_widget.delete(item)

            with open(filename, 'r', encoding='utf-8') as f:
                f.readline()
                while True:
                    l = f.readline()
                    if l == '\n':
                        break
                    t_widget.insert('', tk.END, values=l.split(','))

        if refresh:
            self.root.after(5000, self.read_file_tree, t_widget, filename, refresh)

    # ---------------------------------------------------------------------------------------------------------------- #
    def readfile_ini(self, t_widget, filename):
        """
        Read contents of the INI-file
        """

        t_widget.config(state='normal')
        with open(filename, 'r', encoding='utf-8') as f:
            t_widget.delete(1.0, tk.END)
            t_widget.insert(tk.END, ''.join(f.readlines()))
            t_widget.see(tk.END)

    # ---------------------------------------------------------------------------------------------------------------- #
    def readfile_log(self, t_widget, filename, refresh=False):
        """
        Read contents of the LOG-file
        """

        t_widget.config(state='normal')
        with open(filename, 'r', encoding='utf-8') as f:
            sz = os.path.getsize(filename)
            if sz > self.log_size:
                self.log_size = sz
                t_widget.delete(1.0, tk.END)
                t_widget.insert(tk.END, ''.join(f.readlines()))
                t_widget.see(tk.END)

        if refresh:
            t_widget.config(state='disabled')
            if not self.pause_log:
                self.root.after(500, self.readfile_log, t_widget, filename, refresh)

    # ---------------------------------------------------------------------------------------------------------------- #
    def saveini(self, t_widget, filename):
        """
        Save INI-file
        """

        with open(filename, 'w', encoding='utf8') as f:
            f.write(t_widget.get('1.0', tk.END)[:-1])                   # Strip extra newline

        # Rebuild ts-warp_pf.conf when saving the INI-file
        with open(fwfile, 'w', encoding='utf8') as outfw:
            subprocess.run(['./ts-warp_autofw.sh', prefix], stdout=outfw, check=False)

    # ---------------------------------------------------------------------------------------------------------------- #
    def pauselog(self, btn, txt, filename):
        """
        Pause LOG
        """

        if self.pause_log:
            self.pause_log = False
            btn['text'] = '■'                                           # Pause log auto-refresh
            self.readfile_log(txt, filename, refresh=True)
        else:
            self.pause_log = True
            btn['text'] = '↭'                                           # Enable auto-refresh

    # ---------------------------------------------------------------------------------------------------------------- #
    def pauseact(self, btn, tree, filename):
        """
        Pause ACT
        """

        if self.pause_act:
            self.pause_act = False
            btn['text'] = '■'                                           # Pause act auto-refresh
            self.read_file_tree(tree, filename, refresh=True)
        else:
            self.pause_act = True
            btn['text'] = '▶'                                           # Enable auto-refresh
            self.read_file_tree(tree, filename, refresh=False)

    # ---------------------------------------------------------------------------------------------------------------- #
    def status(self, lbl, btn, pidfile):
        """
        Statusbar message
        """

        pf = None

        try:
            pf = open(pidfile, 'r', encoding='utf8')
        except Exception:
            lbl['text'] = '■ Stopped'
            lbl['foreground'] = 'red'
            btn['text'] = '▶'

        if pf:
            pt = os.path.getmtime(pidfile)
            if pt > self.pid_time:
                self.pid_time = pt

                lbl['text'] = '■ Running: ' + pf.readline()[:-1]
                lbl['foreground'] = 'green'
                btn['text'] = '■'
            pf.close()

        self.root.after(1000, self.status, lbl, btn, pidfile)

    # ---------------------------------------------------------------------------------------------------------------- #
    def run_script(self, command):
        """
        Run a script as root
        """

        if os.geteuid() != 0:
            gwp = subprocess.Popen(
                ['sudo', '-S', '-b', runcmd, command, prefix, '-v', self.cmb_lvl.get(), self.tsw_opts.get()],
                stdin=subprocess.PIPE)
            gwp.communicate(self.password)
        else:
            subprocess.Popen([runcmd, command, prefix, '-v', self.cmb_lvl.get(), self.tsw_opts.get()])

    # ---------------------------------------------------------------------------------------------------------------- #
    def ask_password(self):
        """
        Password dialog
        """

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
            self.lbl_pwd = ttk.Label(self.win_pwd, text='OS user password:')
            self.lbl_pwd.grid(column=0, row=0, sticky=tk.EW, padx=padx, pady=pady)

            self.pwd = tk.StringVar()
            self.ent_pwd = ttk.Entry(self.win_pwd, textvariable=self.pwd, show='*')
            self.ent_pwd.bind('<Return>', lambda x: self.get_password())
            self.ent_pwd.grid(column=1, row=0, sticky=tk.EW, padx=padx, pady=pady)
            self.ent_pwd.focus_set()

            self.btn_enter = ttk.Button(self.win_pwd, text="Enter")
            self.btn_enter['command'] = lambda: self.get_password()
            self.btn_enter.grid(column=2, row=0, sticky=tk.EW, padx=padx, pady=pady)

    # ---------------------------------------------------------------------------------------------------------------- #
    def get_password(self):
        """
        Check password via sudo
        """

        gwp = subprocess.Popen(['sudo', '-S', '-v'], stdin=subprocess.PIPE)
        gwp.communicate(self.pwd.get().encode())

        if gwp.returncode == 1:
            self.password = ''
            self.ent_pwd.config(show='')
            self.pwd.set('Wrong password')
            self.root.after(1000, lambda: self.pwd.set(''))
            self.root.after(1000, lambda: self.ent_pwd.config(show='*'))
        else:
            self.password = self.pwd.get().encode()
            self.win_pwd.destroy()
            self.root.focus_set()

# -------------------------------------------------------------------------------------------------------------------- #
def dedupch(s, c='/'):
    """
    Remove duplicated characters: c from the string: s
    """

    return c.join([x for x in s.split(c) if x != ''])

# -------------------------------------------------------------------------------------------------------------------- #


if __name__ == "__main__":
    url_new_vesrsion = 'https://raw.githubusercontent.com/mezantrop/ts-warp/master/version.h'
    url_repository = 'https://github.com/mezantrop/ts-warp/releases/latest/download/GUI-Warp.dmg'
    url_supportus = 'https://www.buymeacoffee.com/mezantrop'
    url_contact = 'mailto:zmey20000@yahoo.com'

    runcmd = './ts-warp.sh'
    prefix = '/' + dedupch(os.path.expanduser("~/ts-warp/")) + '/'
    inifile = prefix + 'etc/ts-warp.ini'
    fwfile = prefix + 'etc/ts-warp_pf.conf'
    logfile = prefix + 'var/log/ts-warp.log'
    pidfile = prefix + 'var/run/ts-warp.pid'
    actfile = prefix + 'var/spool/ts-warp/ts-warp.act'
    daemon_options = ''

    # Override defaults by gui-warp.ini
    try:
        gui_ini = configparser.ConfigParser()
        gui_ini.read(prefix + 'etc/gui-warp.ini')
        if 'GUI-WARP' in gui_ini.sections():
            if 'prefix' in gui_ini['GUI-WARP'].keys():
                prefix = '/' + dedupch(os.path.expanduser(gui_ini['GUI-WARP']['prefix'])) + '/'
            if 'runcmd' in gui_ini['GUI-WARP'].keys():
                runcmd = '/' + dedupch(prefix + gui_ini['GUI-WARP']['runcmd'])
            if 'inifile' in gui_ini['GUI-WARP'].keys():
                inifile = '/' + dedupch(prefix + gui_ini['GUI-WARP']['inifile'])
            if 'fwfile' in gui_ini['GUI-WARP'].keys():
                fwfile = '/' + dedupch(prefix + gui_ini['GUI-WARP']['fwfile'])
            if 'logfile' in gui_ini['GUI-WARP'].keys():
                logfile = '/' + dedupch(prefix + gui_ini['GUI-WARP']['logfile'])
            if 'pidfile' in gui_ini['GUI-WARP'].keys():
                pidfile = '/' + dedupch(prefix + gui_ini['GUI-WARP']['pidfile'])
            if 'actfile' in gui_ini['GUI-WARP'].keys():
                actfile = '/' + dedupch(prefix + gui_ini['GUI-WARP']['actfile'])
            if 'daemon_options' in gui_ini['GUI-WARP'].keys():
                daemon_options = gui_ini['GUI-WARP']['daemon_options']
    except Exception:
        gui_ini = None

    if not os.path.exists(prefix):                      # Create ts-warp dir + subdirs in home
        os.makedirs(prefix + 'etc/')
        shutil.copyfile('./ts-warp.ini', inifile)       # Install sample INI
        os.chmod(inifile, 0o600)
        if gui_ini:
            # Install sample gui-warp.ini
            shutil.copyfile('./gui-warp.ini', prefix + 'etc/gui-warp.ini')
            os.chmod(prefix + 'etc/gui-warp.ini', 0o600)
    if not os.path.exists(prefix + 'etc/'):
        os.makedirs(prefix + 'etc/')
    if not os.path.exists(prefix + 'var/log/'):
        os.makedirs(prefix + 'var/log/')
    if not os.path.exists(prefix + 'var/run/'):
        os.makedirs(prefix + 'var/run/')
    if not os.path.exists(prefix + 'var/spool/ts-warp/'):
        os.makedirs(prefix + 'var/spool/ts-warp/')

    if not os.path.exists(fwfile):
        with open(fwfile, 'w', encoding='utf8') as outfw:
            subprocess.run(['./ts-warp_autofw.sh', prefix], stdout=outfw, check=False)
    if not os.path.exists(logfile):
        open(logfile, 'a', encoding='utf8').close()

    app = App(runcmd=runcmd, daemon_options=daemon_options,
              inifile=inifile, fwfile=fwfile, logfile=logfile, pidfile=pidfile,
              url_new_vesrsion=url_new_vesrsion)
