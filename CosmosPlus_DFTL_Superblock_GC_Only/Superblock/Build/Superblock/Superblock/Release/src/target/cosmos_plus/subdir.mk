################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/target/cosmos_plus/cosmos_plus_system.c 

OBJS += \
./src/target/cosmos_plus/cosmos_plus_system.o 

C_DEPS += \
./src/target/cosmos_plus/cosmos_plus_system.d 


# Each subdirectory must supply rules for building sources it contributes
src/target/cosmos_plus/cosmos_plus_system.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/target/cosmos_plus/cosmos_plus_system.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


