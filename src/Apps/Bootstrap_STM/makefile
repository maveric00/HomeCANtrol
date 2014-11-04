################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
Bootstrap_STM.hex \

SECONDARY_LIST += \
Bootstrap_STM.lst \

SECONDARY_SIZE += \
Bootstrap_STM.siz \


# All Target
all: Bootstrap_STM.elf secondary-outputs

# Tool invocations
Bootstrap_STM.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Linux GCC C Linker'
	arm-none-eabi-gcc -T"stm32_flash.ld" -nostartfiles -Wl,--gc-sections  -Wl,-Map,Bootstrap_STM.map -mcpu=cortex-m4 -mthumb -g3 -gdwarf-2 -o "Bootstrap_STM.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

Bootstrap_STM.hex: Bootstrap_STM.elf
	@echo 'Invoking: ARM Sourcery Linux GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex Bootstrap_STM.elf  "Bootstrap_STM.hex"
	@echo 'Finished building: $@'
	@echo ' '

Bootstrap_STM.lst: Bootstrap_STM.elf
	@echo 'Invoking: ARM Sourcery Linux GNU Create Listing'
	arm-none-eabi-objdump -h -S Bootstrap_STM.elf > "Bootstrap_STM.lst"
	@echo 'Finished building: $@'
	@echo ' '

Bootstrap_STM.siz: Bootstrap_STM.elf
	@echo 'Invoking: ARM Sourcery Linux GNU Print Size'
	arm-none-eabi-size  --format=berkeley Bootstrap_STM.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) Bootstrap_STM.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
