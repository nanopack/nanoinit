                                                       o                 o     o     
                                                     _<|>_             _<|>_  <|>    
                                                                              < >    
     \o__ __o      o__ __o/  \o__ __o     o__ __o      o    \o__ __o     o     |     
      |     |>    /v     |    |     |>   /v     v\    <|>    |     |>   <|>    o__/_ 
     / \   / \   />     / \  / \   / \  />       <\   / \   / \   / \   / \    |     
     \o/   \o/   \      \o/  \o/   \o/  \         /   \o/   \o/   \o/   \o/    |     
      |     |     o      |    |     |    o       o     |     |     |     |     o     
     / \   / \    <\__  / \  / \   / \   <\__ __/>    / \   / \   / \   / \    <\__  
 
# Nanoinit

A combination of Nano and Init

* Nano - denoting a very small item.
* Init - Initialization process

## Why nanoinit?
Docker containers don't usually run with a true init process, and often times suffer from having unreaped processes sitting around. 
There are other init processes that can take care of this.
One of which is my_init which is written in Python.
It works well, but the only issue is that it uses the Python runtime which uses a little more ram than we would like.
Writing the nanoinit in C helps reduce the memory footprint, allowing more docker containers to run on a system.

### TODO: list of things that my_init.py has that nanoinit doesn't
* Custom options:
 - custom command for service management
 - enable insecure ssh keys
 - skip startup files
 - no killall on exit
 - quiet mode
* output environment variables to json
* run /etc/rc.local
* run other startup files/scripts (my_init.py uses /etc/my_init.d/*)
* stop runit services before shutting down
