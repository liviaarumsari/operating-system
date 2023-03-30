# Operating System Course Project: sOS
This repository contains the source code for a kernel as a completion for an Operating System Course Project.

## **Table of Contents**
* [General Information](#general-information)
* [Milestone](#milestone)
* [Requirements](#requirements)
* [How to Run](#how-to-run)
* [Program Structure](#program-structure)
* [Author](#author)

## **General Information**
This repository contains code to build an operating system in x86 32 bit architecture which will be run by QEMU emulator. The progress is divided into several milestones.

## **Milestone**
### **Milestone 1**
* Set up tools and repository
* Build script making
* Run the operating system
* Make output with text
* Enter the protected mode
### **Milestone 2**
* Interrupt
* Keyboard device driver
* File system


## **Requirements**
To use this program, you will need to install these programs:
* Netwide assembler
* GNU C Compiler
* GNU Linker
* QEMU - System i386
* GNU Make
* genisoimage

By using Ubuntu 20.04 operating system, you can use this command to install all the requirements
```sh 
sudo apt update
sudo apt install gcc nasm make qemu-system-x86 genisoimage
```

## **How to Run**
### **Run**
You can directly use VSCode to run and debug or follow these steps

1. Make sure you are in the root folder <br>
2. Run the application using this command
```sh 
make run
```

## **Program Structure**
```
.
│   makefile
│   README.md
│
├───.vscode
│       launch.json
│       settings.json
│       tasks.json
│
├───bin
│       .gitignore
│
├───include
│       disk.h
│       fat32.h
│       framebuffer.h
│       gdt.h
│       idt.h
│       interrupt.h
│       kernel_loader.h
│       keyboard.h
│
├───lib
│   ├───lib-header
│   │       portio.h
│   │       stdmem.h
│   │       stdtype.h
│   │
│   └───lib-source
│           portio.c
│           stdmem.c
│
├───other
│       grub1
│
└───src
    │   kernel.c
    │   kernel_loader.s
    │   linker.ld
    │   menu.lst
    │
    ├───filesystem
    │       disk.c
    │       fat32.c
    │
    ├───framebuffer
    │       framebuffer.c
    │
    ├───GDT
    │       gdt.c
    │
    ├───interrupt
    │       idt.c
    │       interrupt.c
    │       intsetup.s
    │
    └───keyboard
            keyboard.c

```

## Authors

|  **NIM** | **Github**    |          **Nama**         |
|:--------:|---------------|:-------------------------:|
| 13521094 | [liviaarumsari](https://github.com/liviaarumsari) | Angela Livia Arumsari     |
| 13521096 | [noelsimbolon](https://github.com/noelsimbolon)  | Noel Christoffel Simbolon |
| 13521134 | [Rinaldy-Adin](https://github.com/Rinaldy-Adin)  | Rinaldy Adin              |
| 13521142 | [AlifioDitya](https://github.com/AlifioDitya)   | Enrique Alifio Ditya      |
