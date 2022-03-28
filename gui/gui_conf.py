"""
GUI-WARP configuration file
"""

runcmd = '/usr/local/etc/ts-warp.sh'                # ts-warp daemon management script

inifile = '/usr/local/etc/ts-warp.ini'              # INI-file location
fwfile = '/usr/local/etc/ts-warp_pf.conf'           # Firewall rules, either PF or Iptables
logfile = '/usr/local/var/log/ts-warp.log'          # LOG-file
pidfile = '/usr/local/var/run/ts-warp.pid'          # Daemon PID-file
