# sdat2img
This is a c++ equivalent of the original sdat2img by xpirt - luxi78 - howellzhu which was written in python


## Usage
```
sdat2img <transfer_list> <system_new_file> [system_img] [-q]
```
- `<transfer_list>` = input, system.transfer.list from rom zip
- `<system_new_file>` = input, system.new.dat from rom zip
- `[system_img]` = output ext4 raw image file
- `[-q]` = quiet mode


## Example
This is a simple example on a Linux system: 
```
~$ ./sdat2img system.transfer.list system.new.dat system.img
```
