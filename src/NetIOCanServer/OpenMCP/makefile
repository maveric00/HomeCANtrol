# WinAVR Sample makefile written by Eric B. Weddington, Jörg Wunsch, et al.
# Released to the Public Domain
# Please read the make user manual!
#
# Additional material for this makefile was submitted by:
#  Tim Henigan
#  Peter Fleury
#  Reiner Patommel
#  Sander Pool
#  Frederik Rouleau
#  Markus Pfaff
#
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make coff = Convert ELF to AVR COFF (for use with AVR Studio 3.x or VMLAB).
#
# make extcoff = Convert ELF to AVR Extended COFF (for use with AVR Studio
#                4.07 or greater).
#
# make program = Download the hex file to the device, using avrdude.  Please
#                customize the avrdude settings below first!
#
# make filename.s = Just compile filename.c into the assembler code only
#
# To rebuild project do "make clean" then "make all".
#

# MCU name
# MCU = atmega2561
MCU = atmega644
#MCU = atmega644p

# legt das System fest unter dem übersetzt wird
# SYSTEM = Linux
SYSTEM = Windows
# SYSTEM = MacOSX

# CPU-Frequenz
# F_CPU = 16000000
F_CPU = 16000000

# Hardwareplatform
# HARDWARE = OpenMCP
# HARDWARE = AVRNETIO
HARDWARE = AVRNETIO

# Output format. (can be srec, ihex, binary)
FORMAT = ihex

# Target file name (without extension).
TARGET = main

# Optimization level, can be [0, 1, 2, 3, s]. 0 turns off optimization.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
OPT = s

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
hardware/ext_int/ext_int.c system/clock/clock.c hardware/timer1/timer1.c hardware/spi/spi_core.c hardware/uart/uart.c \
hardware/adc/adc.c  hardware/gpio/gpio_out.c hardware/gpio/gpio_in.c hardware/network/enc28j60.c hardware/1wire/1wire.c hardware/1wire/DS16xxx.c \
hardware/twi/twi.c hardware/twi/lm75.c hardware/pcint/pcint.c hardware/memory/xram.c hardware/led/led_core.c hardware/led/led_0.c \
hardware/led/led_1.c hardware/led/led_2.c hardware/spi/spi_0.c hardware/spi/spi_1.c hardware/spi/spi_2.c hardware/vs10xx/vs10xx.c \
hardware/timer0/timer0.c hardware/hal.c \
system/net/icmp.c system/net/udp.c system/net/tcp.c system/net/ethernet.c system/net/arp.c system/net/ip.c \
system/net/dhcpc.c system/net/ntp.c system/net/dns.c system/net/network.c system/math/math.c system/math/checksum.c \
system/stdout/stdout.c system/config/config.c system/softreset/softreset.c system/buffer/fifo.c system/init.c \
system/base64/base64.c system/net/twitter.c system/net/dyndns.c system/led-tafel/tafel.c system/thread/thread.c \
system/string/string.c system/sensor/temp.c system/nano_DB/nano_db.c \
apps/httpd/httpd2.c apps/httpd/httpd2_pharse.c apps/httpd/cgibin/cgi-bin.c apps/httpd/files.c apps/telnet/telnet.c \
apps/cron/cron.c apps/apps_init.c apps/modul_init.c \
apps/modules/cmd_arp.c apps/modules/cmd_stats.c apps/modules/cmd_reset.c apps/modules/cmd_dns.c apps/modules/cmd_adc.c \
apps/modules/cmd_gpio.c apps/modules/cmd_ifconfig.c apps/modules/cmd_ntp.c apps/modules/cmd_dyndns.c apps/modules/cmd_temp.c \
apps/modules/cmd_cron.c apps/modules/cmd_twitter.c apps/modules/cmd_eemem.c apps/modules/cmd_twi.c apps/modules/cmd_onewire.c \
apps/modules/cmd_stream.c apps/modules/cmd_tafel.c apps/modules/impulse.c apps/modules/hal.c apps/modules/temp_json.c \
apps/mp3-streamingclient/mp3-streaming.c apps/mp3-streamingclient/mp3-clientserver.c

SRC += hardware/CAN_Lib/mcp2515.c
SRC += hardware/CAN_Lib/mcp2515_buffer.c
SRC += hardware/CAN_Lib/mcp2515_get_message.c
SRC += hardware/CAN_Lib/mcp2515_send_message.c
SRC += hardware/CAN_Lib/mcp2515_set_dyn_filter.c
SRC += hardware/CAN_Lib/mcp2515_get_dyn_filter.c
SRC += hardware/CAN_Lib/mcp2515_static_filter.c
SRC += hardware/CAN_Lib/mcp2515_write_id.c
SRC += hardware/CAN_Lib/mcp2515_read_id.c
SRC += hardware/CAN_Lib/mcp2515_error_register.c
SRC += hardware/CAN_Lib/mcp2515_regdump.c
SRC += hardware/CAN_Lib/mcp2515_set_mode.c
SRC += hardware/CAN_Lib/spi.c
SRC += hardware/CAN_Lib/can_buffer.c
SRC += apps/can_relay/can_relay.c


# Wenn ATmega2561, dann mach mal noch nen paar Modules mit rein
ifeq ($(MCU),atmega2561)
	SRC += 
endif
ifeq ($(MCU),atmega644)
	SRC += hardware/sd_raw/sd_raw.c system/filesystem/fat.c system/filesystem/partition.c system/filesystem/byteordering.c system/filesystem/filesystem.c
endif
ifeq ($(MCU),atmega644p)
	SRC += apps/modules/lcd_info.c
	SRC += hardware/sd_raw/sd_raw.c system/filesystem/fat.c system/filesystem/partition.c system/filesystem/byteordering.c system/filesystem/filesystem.c
	SRC += hardware/lcd/lcd.c
endif

# List Assembler source files here.
# Make them always end in a capital .S.  Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = 


# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
# EXTRAINCDIRS = /opt/cross/lib/gcc/avr/4.1.3/include


# Optional compiler flags.
#  -g:        generate debugging information (for GDB, or for COFF conversion)
#  -O*:       optimization level
#  -f...:     tuning, see gcc manual and avr-libc documentation
#  -Wall...:  warning level
#  -Wa,...:   tell GCC to pass this to the assembler.
#    -ahlms:  create assembler listing

CFLAGS = -g -O$(OPT) \
	-funsigned-char \
	-funsigned-bitfields \
	-fpack-struct \
	-fshort-enums \
	-Wstrict-prototypes \
	-Wuninitialized \
	-Wa,-adhlns=$(<:.c=.lst) \
	$(patsubst %,-I%,$(EXTRAINCDIRS)) \
	$(MY_MAC_ADRESS) $(MY_IP_ADDRESS) $(MY_IP_NETWORK_MASK) \
	-D__heap_end=0x80ffff -DF_CPU=$(F_CPU) -D$(HARDWARE) \
	-mcall-prologues \
	-fdata-sections \
	-ffunction-sections \
	-fmerge-constants \
	-mno-tablejump

# Set a "language standard" compiler flag.
#   Unremark just one line below to set the language standard to use.
#   gnu99 = C99 + GNU extensions. See GCC manual for more information.
CFLAGS += -std=gnu99

# Optional assembler flags.
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -ahlms:    create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 

# Optional linker flags.
#  -Wl,...:   tell GCC to pass this to linker.
#  -Map:      create map file
#  --cref:    add cross reference to  map file
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref,--gc-sections

# Wenn ATmega2561, dann externes RAM verwenden für Heap und Variablen, interes ist dann nur Stack
ifeq ($(MCU),atmega2561)
	LDFLAGS += -Wl,--section-start,.data=0x802200,--defsym=__heap_end=0x80ffff
endif

# Additional libraries

# Minimalistic printf version
# LDFLAGS += -Wl,-u,vfprintf -lprintf_min

# Floating point printf version (requires -lm below)
# LDFLAGS += -Wl,-u,vfprintf -lprintf_flt

# -lm = math library
# LDFLAGS += -lm

# Programming support using avrdude. Settings and variables.

# Programming hardware: alf avr910 avrisp bascom bsd 
# dt006 pavr picoweb pony-stk200 sp12 stk200 stk500
#
# Type: avrdude -c ?
# to get a full listing.
#
AVRDUDE_PROGRAMMER = avrisp2

AVRDUDE_PORT = usb	   # programmer connected to serial device
# AVRDUDE_PORT = /dev/ttyUSB0	# programmer connected to parallel port

AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
#AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
AVRDUDE_SAVE_EEPROM = -U eeprom:r:temp.hex:i
AVRDUDE_RESTORE_EEPROM = -U eeprom:w:temp.hex

AVRDUDE_FUSE = -i 15

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

# Uncomment the following if you want avrdude's erase cycle counter.
# Note that this counter needs to be initialized first using -Yn,
# see avrdude manual.
#AVRDUDE_ERASE += -y

# Uncomment the following if you do /not/ wish a verification to be
# performed after programming the device.
AVRDUDE_FLAGS += -V

# Increase verbosity level.  Please use this when submitting bug
# reports about avrdude. See <http://savannah.nongnu.org/projects/avrdude> 
# to submit bug reports.
#AVRDUDE_FLAGS += -v -v

# ---------------------------------------------------------------------------

# Define directories, if needed for linux
ifeq ($(SYSTEM),Linux)
	DIRAVRPREFIX = /usr/local/avr/
	DIRAVRBIN = $(DIRAVRPREFIX)bin/
	DIRAVRUTILS = $(DIRAVRPREFIX)utils/bin
	DIRINC = $(DIRAVRPREFIX)avr/lib
	DIRLIB = $(DIRAVRPREFIX)avr/lib
	SHELL = sh
	CC =  $(DIRAVRBIN)avr-gcc-4.2.4
	OBJCOPY =$(DIRAVRBIN)avr-objcopy
	OBJDUMP = $(DIRAVRBIN)avr-objdump
	SIZE = $(DIRAVRBIN)avr-size
	AVRDUDE = avrdude
	REMOVE = rm -f
	COPY = cp
endif

# Define programs and commands.for Windows
ifeq ($(SYSTEM),Windows)
	SHELL = sh
	CC = avr-gcc
	OBJCOPY = avr-objcopy
	OBJDUMP = avr-objdump
	SIZE = avr-size
	NM = avr-nm
	AVRDUDE = avrdude
	REMOVE = rm -f
	REMOVEDIR = rm -rf
	COPY = cp
	WINSHELL = cmd
endif

# Define directories for MacOSX
ifeq ($(SYSTEM),MacOSX)
	DIRAVRPREFIX = /usr/local/CrossPack-AVR/
	DIRAVRBIN = $(DIRAVRPREFIX)bin/
	DIRAVRUTILS = $(DIRAVRPREFIX)utils/bin
	DIRINC = $(DIRAVRPREFIX)avr/lib
	DIRLIB = $(DIRAVRPREFIX)avr/lib
	SHELL = sh
	CC =  $(DIRAVRBIN)avr-gcc
	OBJCOPY =$(DIRAVRBIN)avr-objcopy
	OBJDUMP = $(DIRAVRBIN)avr-objdump
	SIZE = $(DIRAVRBIN)avr-size
	AVRDUDE = avrdude
	REMOVE = rm -f
	COPY = cp
endif

HEXSIZE = $(SIZE) --target=$(FORMAT) $(TARGET).hex
ELFSIZE = $(SIZE) -A $(TARGET).elf

# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before: 
MSG_SIZE_AFTER = Size after:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:

# Define all object files.
OBJ = $(SRC:.c=.o) $(ASRC:.S=.o) 

# Define all listing files.
LST = $(ASRC:.S=.lst) $(SRC:.c=.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)

# Default target.
all: begin gccversion sizebefore $(TARGET).elf $(TARGET).hex $(TARGET).eep \
	$(TARGET).lss $(TARGET).sym sizeafter finished end

fuse:
ifeq ($(MCU),atmega2561)
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U lfuse:w:0xe6:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U hfuse:w:0xd1:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U efuse:w:0xfd:m
	PROGRAM_F=ok
endif
ifeq ($(MCU),atmega644)
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U lfuse:w:0xe6:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U hfuse:w:0xd1:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U efuse:w:0xfd:m
	PROGRAM_F=ok
endif
ifeq ($(MCU),atmega644p)
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U lfuse:w:0xe6:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U hfuse:w:0xd1:m
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_FUSE) -q -U efuse:w:0xfd:m
	PROGRAM_F=ok
endif

ifneq ($(PROGRAM_F),ok)
	@echo $(MCU) wird nicht unterstuetzt.
endif

# Eye candy.
# AVR Studio 3.x does not check make's exit code but relies on
# the following magic strings to be generated by the compile job.
begin:
	@echo
	@echo $(MSG_BEGIN)

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)
	@echo


# Display size of file.
sizebefore:
	@if [ -f $(TARGET).elf ]; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); echo; fi

sizeafter:
	@if [ -f $(TARGET).elf ]; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); echo; fi



# Display compiler version information.
gccversion : 
	@$(CC) --version




# Convert ELF to COFF for use in debugging / simulating in
# AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
	--change-section-address .data-0x800000 \
	--change-section-address .bss-0x800000 \
	--change-section-address .noinit-0x800000 \
	--change-section-address .eeprom-0x810000 


coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-avr $< $(TARGET).cof


extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof




# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	# $(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_SAVE_EEPROM)
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)
	# $(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_RESTORE_EEPROM)

# Program the device.  
install: $(TARGET).hex $(TARGET).eep
	# $(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_SAVE_EEPROM)
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)
	# $(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_RESTORE_EEPROM)



# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(DIRAVRBIN)avr-nm -n $< > $@



# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(ALL_CFLAGS) $(OBJ) --output $@ $(LDFLAGS)


# Compile: create object files from C source files.
%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@


# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@






# Target: clean project.
clean: begin clean_list finished end

clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).eep
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).cof
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).a90
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lnk
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(OBJ)
	$(REMOVE) $(LST)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)


# Automatically generate C source code dependencies. 
# (Code originally taken from the GNU make user manual and modified 
# (See README.txt Credits).)
#
# Note that this will work with sh (bash) and sed that is shipped with WinAVR
# (see the SHELL variable defined above).
# This may not work with other shells or other seds.
#
#%.d: %.c
#	set -e; $(CC) -MM $(ALL_CFLAGS) $< \
#	| sed 's,\(.*\)\.o[ :]*,\1.o \1.d : ,g' > $@; \
#	[ -s $@ ] || rm -f $@


# Remove the '-' if you want to see the dependency files generated.
#-include $(SRC:.c=.d)



# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion coff extcoff \
	clean clean_list program
