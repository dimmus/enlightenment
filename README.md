# Enlightenment

![Screenshot](/data/readme/screenshot.png)

#### Status

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FDimmus%2Fenlightenment.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FDimmus%2Fenlightenment?ref=badge_shield)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/26863/badge.svg)](https://scan.coverity.com/projects/dimmus-enlightenment)
[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/Dimmus/enlightenment)
[![contributions welcome](https://img.shields.io/static/v1.svg?label=Contributions&message=Welcome&color=0059b3&style=flat-square)](https://github.com/Dimmus/enlightenment/blob/master/CONTRIBUTING.md)
![GitHub repo size](https://img.shields.io/github/repo-size/Dimmus/enlightenment?color=red&style=flat-square)
[![Packaging status](https://repology.org/badge/tiny-repos/enlightenment.svg)](https://repology.org/project/enlightenment/versions)
[![latest packaged version(s)](https://repology.org/badge/latest-versions/enlightenment.svg)](https://repology.org/project/enlightenment/versions)

*Please report bugs/issues at*
[git.enlightenment.org](https://git.enlightenment.org/enlightenment/enlightenment/issues)

Enlightenment is a Window Manager, Compositor and basic "Desktop
Shell". It replaces your GUI environment that runs your day to day
graphical Desktop. It includes a built-in File Manager, Application
Launcher, Shelves, Settings Dialogs, Menus, Audio Mixer Controls, Network
Control front-end (for Connman), Battery Monitoring, CPU Frequency
Controls, Screen Blanking and Backlight controls, Screenshotting and
editing, Clock and Calendar, Temperature Guages, Mpris2 Music Controls,
Packagekit Update Front-end, Bluetooth controls for BlueZ 5, Screen
resolution and layout controls and much much more.

Enlightenment is one of the lowest resource environments around
relative to its featureset.

Enlightenment is primarly developed for X11, but does have an
experimental Wayland mode that will have issues, so only try it if
you are adventurous or willing to work on it.

For more information please see
[Enlightenment's About Page](https://www.enlightenment.org/about-enlightenment).

![Screenshot 2](/data/readme/screenshot2.png)
![Screenshot 3](/data/readme/screenshot3.png)
![Screenshot 4](/data/readme/screenshot4.png)
![Screenshot 5](/data/readme/screenshot5.png)
![Screenshot 6](/data/readme/screenshot6.png)

-----

Things that need doing:

[TODO](TODO.md)

-----

## Requirements

Must:

* [efl](https://git.enlightenment.org/enlightenment/efl)
* libpam (Required on Linux, not BSD)

Highly suggested:

* libexif (exif metadata display support)
* bluez5 (BT support and l2ping tool)
* connman
* bc (For everything calculator)
* pulseaudio
* acpid (Unless your system doesn't have ACPI at all)
* packagekit (For packagekit module updates status)
* udisks2
* gdb (If you want automatic bactraces in ~/.e-crashdump.txt)

-----

## Compiling and Installing

Meson is the build system used for this project. For more information
please see [mesonbuild.com](https://mesonbuild.com)

You will need normal build tooling installed such as a compiler (gcc
or clang for example), pkg-config, ninja, any relevant package-dev or
package-devel packages if your distribution splits out development
headers (e.g. libc6-dev) etc.

Depending on where dependencies (like efl) are installed, you might have to
set your `PKG_CONFIG_PATH` environment variable like:
```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
```

Also note that some distributions like to add extra arch directories
to your library locations so you might have to have more like:
```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig
```

You will need to ensure that the destination library directory (e.g.
`/usr/local/lib` is in your /etc/ld.so.conf or /etc/ld.so.conf.d/
files and after installing anything that installs libraries you
re-run `ldconfig`. Please see relevant documentation on ldconfig and
ld.so.conf for your distribution.

You might also want to add the destination bin dir to your environment
variable PATH (see documentation on your shell PATH variable) such as:
```sh
export PATH=/usr/local/bin:/usr/bin:/bin
```

Normal compilation in /usr/local:
```sh
meson . build
ninja -C build
sudo ninja -C build install
```

For meson build generic options:
```sh
meson --help
```

For a list of project specific options supported:
```sh
cat meson_options.txt
```

To set 1 or more project specific options:
```sh
meson --prefix=/path/to -Doption=value [-Dother=value2] [...] . build
```

To display current configuration:
```sh
meson configure build
```

The above will only work after at least the following is done:
```sh
meson . build
```

### Quick build help

How to clean out the build and config and start fresh:
```sh
rm -rf build
```

How to make a dist tarball and check its build:
(must do it from git tree clone and commit all changes to git first)
```sh
ninja -C build dist
```

How to change prefix:
```sh
meson --prefix=/path/to/prefix . build
```

How to install in a specific destination directory for packaging:
```sh
DESTDIR=/path/to/destdir ninja -C build install
```

How to build with verbose output (full commands run):
```sh
ninja -C build -v
```
-----

**NOTE:** Once Enlightenment is started by `enlightenment_start`, all
output logs are put in `~/.e-log.log`. The previous log for the
previous execution of Enlightenment will be moved to `~/.e-log.log.old`.

-----

**NOTE:** If you install enlightenment to the default compile prefix
(`/usr/local`) then you might want to ensure the session file is
accessible to your login manager (assuming you use one) by doing
something like:

```sh
sudo ln -s /usr/local/share/xsessions/enlightenment.desktop /usr/share/xsessions/enlightenment.desktop
```

You can also just set up a custom launcher script. Edit the
`~/.xinitrc` file in your home directory and have it have this content:

```sh
#!/bin/sh
exec /usr/local/bin/enlightenment_start >& ~/.xsession-errors
```

(`exec` tells the shell to replace itself with the following command
and `>& ~/.xsession-errors` ensures all output is logged to the
`~/.xsession-errors` file).

You may also want your `~/.xsession` file to be the same, so make a link:

```sh
ln -s .xinitrc .xsession
```

This assumes `/usr/local` as well and just modify based on your
install prefix location.

If you do not use a login manager you can just log into a text console
and run `startx` after the above `.xinitrc` file is set up.

-----

**NOTE:** If you do not want security issues make sure `sysactions.conf` is in
`/etc/enlightenment` (not `PREFIX/etc/enlightenment`) as this is the first
place it looks at. This file is intended to be customized by packagers and
system integrators to match your policies and scripts/tools. This also
applies to `system.conf` in the same directory.

-----

**NOTE:** To enable Wayland support (still considered experimental and not for
regular end users) use the meson `-Dwl=true option`. To run Enlightenment in
Wayland mode, just login on any text VT and run `enlightenment_start`. If you
wish to debug and see all the output try using something like screen then
attaching to the remote screen session by sshing in etc.

-----

**NOTE:** Nvidia support - do *not* enable Wayland support if you have
Nvidia cards and use Nvidia drivers. You will need to use **full**
OpenGL+GLX with EFL instead of the default OpenGL-ES/EGL for those to not
have problems and this conflicts with any **DRM** and **Wayland** support.
You will



## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FDimmus%2Fenlightenment.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FDimmus%2Fenlightenment?ref=badge_large)

[![Packaging status](https://repology.org/badge/vertical-allrepos/enlightenment.svg)](https://repology.org/project/enlightenment/versions)
