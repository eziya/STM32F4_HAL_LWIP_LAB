################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.c \
../Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.c 

OBJS += \
./Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.o \
./Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.o 

C_DEPS += \
./Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.d \
./Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.o: ../Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet" -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100" -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DHCP" -I../Drivers/CMSIS/Include -I../Core/Inc -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DNS" -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/netutil.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.o: ../Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet" -I"../Middlewares/Third_Party/ioLibrary_Driver/Ethernet/W5100" -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DHCP" -I../Drivers/CMSIS/Include -I../Core/Inc -I"../Middlewares/Third_Party/ioLibrary_Driver/Internet/DNS" -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/Third_Party/ioLibrary_Driver/Internet/TFTP/tftp.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

