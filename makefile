# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
LIBRARY_FOLDER = lib/lib-source
OUTPUT_FOLDER = bin
ISO_NAME      = os2023
FRAMEBUFFER_FOLDER = framebuffer
GDT_FOLDER = GDT
INTERRUPT_FOLDER = interrupt
DISK_NAME = storage
FILESYSTEM_FOLDER = filesystem
KEYBOARD_FOLDER = keyboard
PAGING_FOLDER = paging
INSERTER_FOLDER = inserter
UTILITY_SHELL_FOLDER = utility_shell

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run: all
	@qemu-system-i386 -s -S -drive file=$(OUTPUT_FOLDER)/storage.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso

all: build

build: iso

clean:
	rm -rf $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/*.iso $(OUTPUT_FOLDER)/*.bin $(OUTPUT_FOLDER)/kernel $(OUTPUT_FOLDER)/inserter $(OUTPUT_FOLDER)/shell

reset: clean disk insert-shell

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g \
		$(LIBRARY_FOLDER)/stdmem.c $(SOURCE_FOLDER)/$(FILESYSTEM_FOLDER)/fat32.c \
		$(SOURCE_FOLDER)/$(INSERTER_FOLDER)/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

# User mode
user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/user-entry.s -o $(OUTPUT_FOLDER)/user-entry.o

	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/user-shell.c -o $(OUTPUT_FOLDER)/user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(LIBRARY_FOLDER)/string.c -o $(OUTPUT_FOLDER)/string.o
	@$(CC)  $(CFLAGS) -fno-pie $(LIBRARY_FOLDER)/stdmem.c -o $(OUTPUT_FOLDER)/stdmem.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/$(UTILITY_SHELL_FOLDER)/utility_shell.c -o $(OUTPUT_FOLDER)/utility_shell.o

	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 \
		$(OUTPUT_FOLDER)/user-entry.o $(OUTPUT_FOLDER)/stdmem.o $(OUTPUT_FOLDER)/user-shell.o $(OUTPUT_FOLDER)/utility_shell.o $(OUTPUT_FOLDER)/string.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...

	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386\
		$(OUTPUT_FOLDER)/user-entry.o $(OUTPUT_FOLDER)/stdmem.o $(OUTPUT_FOLDER)/user-shell.o $(OUTPUT_FOLDER)/utility_shell.o $(OUTPUT_FOLDER)/string.o -o $(OUTPUT_FOLDER)/shell_elf
	@echo Linking object shell object files and generate ELF32 for debugging...

	@size --target=binary bin/shell
	@rm -f $(OUTPUT_FOLDER)/*.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory... 
	@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin

kernel:
# Compile Assembly source file
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o

# Compile C files	
	@$(CC) $(CFLAGS) $(LIBRARY_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(LIBRARY_FOLDER)/stdmem.c -o $(OUTPUT_FOLDER)/stdmem.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FRAMEBUFFER_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(GDT_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/idt.c -o $(OUTPUT_FOLDER)/idt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FILESYSTEM_FOLDER)/disk.c -o $(OUTPUT_FOLDER)/disk.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FILESYSTEM_FOLDER)/fat32.c -o $(OUTPUT_FOLDER)/fat32.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(KEYBOARD_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(PAGING_FOLDER)/paging.c -o $(OUTPUT_FOLDER)/paging.o
	
# Link object files
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel

	@echo Linking object files and generate elf32...
	@rm -f ${OUTPUT_FOLDER}/*.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/

# Create ISO image
	@genisoimage -R			\
	-b boot/grub/grub1		\
	-no-emul-boot			\
	-boot-load-size 4 		\
	-A os					\
	-input-charset utf-8	\
	-quiet					\
	-boot-info-table		\
	-o $(OUTPUT_FOLDER)/$(ISO_NAME).iso \
	$(OUTPUT_FOLDER)/iso

	@rm -r $(OUTPUT_FOLDER)/iso/
