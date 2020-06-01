################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/httpServer/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/SNTP/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/SNMP/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/MQTT/MQTTPacket/src/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/MQTT/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/FTPServer/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/FTPClient/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/DNS/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Internet/DHCP/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5500/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5300/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5200/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Ethernet/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Application/multicast/subdir.mk
-include Middlewares/Third_Party/ioLibrary_Driver/Application/loopback/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.list \

OBJCOPY_BIN += \
STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.bin \


# All Target
all: STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.elf secondary-outputs

# Tool invocations
STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.elf: $(OBJS) $(USER_OBJS) E:\Workspace\STM\ETH\STM32F4_HAL_LWIP_LAB\STM32F4_HAL_ETH_WIZ5100_TCPCLIENT\STM32F429ZITX_FLASH.ld
	arm-none-eabi-gcc -o "STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"E:\Workspace\STM\ETH\STM32F4_HAL_LWIP_LAB\STM32F4_HAL_ETH_WIZ5100_TCPCLIENT\STM32F429ZITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.list"
	@echo 'Finished building: $@'
	@echo ' '

STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "STM32F4_HAL_ETH_WIZ5100_TCPCLIENT.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
