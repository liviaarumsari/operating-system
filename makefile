# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
LIBRARY_FOLDER = lib-source
OUTPUT_FOLDER = bin
ISO_NAME      = os2023

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run: all
	@qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel



kernel:
# Compile Assembly source file
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o

# Compile C files	
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(LIBRARY_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(LIBRARY_FOLDER)/stdmem.c -o $(OUTPUT_FOLDER)/stdmem.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(LIBRARY_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(LIBRARY_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
	
# Link object files
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/portio
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/stdmem
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/framebuffer
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/gdt

	@echo Linking object files and generate elf32...
	@rm -f *.o

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
