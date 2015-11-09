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
