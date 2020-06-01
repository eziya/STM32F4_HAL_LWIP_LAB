################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.c 

OBJS += \
./Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.o 

C_DEPS += \
./Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.o: ../Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet" -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100" -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DHCP" -I../Drivers/CMSIS/Include -I../Core/Inc -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DNS" -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100S/w5100s.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

