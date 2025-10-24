################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/list.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/random.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/statistics.c \
C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/util.c 

OBJS += \
./src/common/list.o \
./src/common/random.o \
./src/common/statistics.o \
./src/common/util.o 

C_DEPS += \
./src/common/list.d \
./src/common/random.d \
./src/common/statistics.d \
./src/common/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/common/list.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/list.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/common/random.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/random.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/common/statistics.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/statistics.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/common/util.o: C:/COSMOS_KJH/CosmosPlus_DFTL_Superblock_GC_Only/Superblock/common/util.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../Superblock_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


