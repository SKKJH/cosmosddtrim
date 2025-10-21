################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/target/cosmos_plus/cosmos_plus_system.c 

OBJS += \
./src/target/cosmos_plus/cosmos_plus_system.o 

C_DEPS += \
./src/target/cosmos_plus/cosmos_plus_system.d 


# Each subdirectory must supply rules for building sources it contributes
src/target/cosmos_plus/cosmos_plus_system.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/target/cosmos_plus/cosmos_plus_system.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


