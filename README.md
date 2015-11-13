[![nanoinit logo](http://nano-assets.gopagoda.io/readme-headers/nanoinit.png)](http://nanobox.io/open-source#nanoinit)
 [![Build Status](https://travis-ci.org/nanopack/nanoinit.svg)](https://travis-ci.org/nanopack/nanoinit)

# Nanoinit

A small, proper, init process for docker containers.

## Why nanoinit?

Docker containers don't usually run with a true init process, and often times suffer from having un-reaped processes sitting around.
There are other init processes that can take care of this.
One of which is my_init which is written in Python.
It works well, but the only issue is that it uses the Python runtime which uses a little more ram than we would like.
Writing the nanoinit in C helps reduce the memory footprint, allowing more docker containers to run on a system.

## Usage:
    nanoinit [options...] [init command to run]
        Options:
        --quiet                 Set log level to warning
        --no-killall            Skip killall on shutdown
        --skip-startup-files    Skip /etc/rc.local and /etc/nanoinit.d/* files

If no command is specified, it defaults to:
    /opt/gonano/sbin/runsvdir -P /etc/service

Before running the init command, nanoinit will run:
* /etc/nanoinit.d/*
* /etc/rc.local

## TODO
- documentation

### Contributing

Contributions to the nanoinit project are welcome and encouraged. Nanoinit is a [Nanobox](https://nanobox.io) project and contributions should follow the [Nanobox Contribution Process & Guidelines](https://docs.nanobox.io/contributing/).

### Licence

Mozilla Public License Version 2.0

[![open source](http://nano-assets.gopagoda.io/open-src/nanobox-open-src.png)](http://nanobox.io/open-source)
