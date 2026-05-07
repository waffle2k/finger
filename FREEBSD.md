# Running finger on FreeBSD

Tested on FreeBSD 15.0-RELEASE using clang 19, meson, and ninja inside a Bastille thin jail.

## Dependencies

```sh
pkg install git meson ninja pkgconf boost-all googletest
```

## Build

```sh
git clone https://github.com/waffle2k/finger /usr/local/src/finger
cd /usr/local/src/finger
meson setup builddir
meson compile -C builddir
meson install -C builddir
```

`meson install` places the binary at `/usr/local/bin/finger`.

## Plan Files

Create one file per username under `/var/finger/users/`. The file contents are returned when that user is fingered.

```sh
mkdir -p /var/finger/users
echo "Out to lunch." > /var/finger/users/pete
```

Test locally:

```sh
printf 'pete\r\n' | nc -w 2 127.0.0.1 79
```

## RC Service

Create `/usr/local/etc/rc.d/fingerd`:

```sh
#!/bin/sh
# PROVIDE: fingerd
# REQUIRE: NETWORKING
# KEYWORD: shutdown

. /etc/rc.subr

name="fingerd"
rcvar="fingerd_enable"
command="/usr/local/bin/finger"
pidfile="/var/run/fingerd.pid"
command_interpreter=""

start_cmd="fingerd_start"
stop_cmd="fingerd_stop"
status_cmd="fingerd_status"

fingerd_start() {
    echo "Starting ${name}."
    /usr/sbin/daemon -p ${pidfile} -f ${command}
}

fingerd_stop() {
    if [ -f ${pidfile} ]; then
        echo "Stopping ${name}."
        kill $(cat ${pidfile}) && rm -f ${pidfile}
    else
        echo "${name} is not running."
    fi
}

fingerd_status() {
    if [ -f ${pidfile} ] && kill -0 $(cat ${pidfile}) 2>/dev/null; then
        echo "${name} is running as pid $(cat ${pidfile})."
    else
        echo "${name} is not running."
    fi
}

load_rc_config $name
run_rc_command "$1"
```

```sh
chmod +x /usr/local/etc/rc.d/fingerd
```

Enable at boot and start:

```sh
sysrc fingerd_enable=YES
service fingerd start
```

## Bastille Jail Setup (optional)

If running inside a Bastille thin jail, bind-mount the plan files directory from the host so they can be managed without entering the jail.

Host fstab (`/usr/local/bastille/jails/finger/fstab`):

```
/srv/finger/users  /usr/local/bastille/jails/finger/root/var/finger/users  nullfs  rw  0  0
```

To redirect external port 79 into the jail, add to `/usr/local/bastille/jails/finger/rdr.conf`:

```
dual re0 any any tcp 79 79
```

pf will apply this as:

```
rdr pass on re0 inet proto tcp from any to any port = finger -> <jail-ip> port 79
```

Rebuild after source changes:

```sh
bastille cmd finger sh -c 'cd /usr/local/src/finger && meson compile -C builddir && meson install -C builddir'
```
