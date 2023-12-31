# Operating System Course Project: sOS
This repository contains the source code for a kernel as a completion for an Operating System Course Project.

<img src="https://github.com/Sister20/if2230-2023-sos-k2/blob/main/assets/image1.png" width="752" />

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
### **Milestone 3**
* Paging
* User Mode
* Shell: implemented cp and rm for folder

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
2. If it's your first time running the application or you want to make a new storage for file system, run this command
```sh 
make disk
```
3. Insert user program to the system by this command
```sh 
make insert-shell
```
4. Run the application using this command
```sh 
make run
```

## **Program Structure**
```
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
│       paging.h
│       utility_shell.h
│
├───lib
│   ├───lib-header
│   │       portio.h
│   │       stdmem.h
│   │       stdtype.h
│   │       string.h
│   │
│   └───lib-source
│           portio.c
│           stdmem.c
│           string.c
│
├───other
│       grub1
│
└───src
    │   init_storage.c
    │   kernel.c
    │   kernel_loader.s
    │   linker.ld
    │   menu.lst
    │   user-entry.s
    │   user-linker.ld
    │   user-shell.c
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
    ├───inserter
    │       external-inserter.c
    │
    ├───interrupt
    │       idt.c
    │       interrupt.c
    │       intsetup.s
    │
    ├───keyboard
    │       keyboard.c
    │
    ├───paging
    │       paging.c
    │
    └───utility_shell
            utility_shell.c

```

## Authors

|  **NIM** | **Github**    |          **Nama**         |
|:--------:|---------------|:-------------------------:|
| 13521094 | [liviaarumsari](https://github.com/liviaarumsari) | Angela Livia Arumsari     |
| 13521096 | [noelsimbolon](https://github.com/noelsimbolon)  | Noel Christoffel Simbolon |
| 13521134 | [Rinaldy-Adin](https://github.com/Rinaldy-Adin)  | Rinaldy Adin              |
| 13521142 | [AlifioDitya](https://github.com/AlifioDitya)   | Enrique Alifio Ditya      |
