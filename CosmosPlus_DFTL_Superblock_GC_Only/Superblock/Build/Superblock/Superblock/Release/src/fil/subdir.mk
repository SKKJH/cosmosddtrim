################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil_nand.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil_request.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/nsc_driver.c 

OBJS += \
./src/fil/fil.o \
./src/fil/fil_nand.o \
./src/fil/fil_request.o \
./src/fil/nsc_driver.o 

C_DEPS += \
./src/fil/fil.d \
./src/fil/fil_nand.d \
./src/fil/fil_request.d \
./src/fil/nsc_driver.d 


# Each subdirectory must supply rules for building sources it contributes
src/fil/fil.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/fil/fil_nand.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil_nand.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/fil/fil_request.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/fil_request.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/fil/nsc_driver.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/fil/nsc_driver.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


