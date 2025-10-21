################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/host_lld.c \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_admin_cmd.c \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_identify.c \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_io_cmd.c \
C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_main.c 

OBJS += \
./src/hil/nvme/host_lld.o \
./src/hil/nvme/nvme_admin_cmd.o \
./src/hil/nvme/nvme_identify.o \
./src/hil/nvme/nvme_io_cmd.o \
./src/hil/nvme/nvme_main.o 

C_DEPS += \
./src/hil/nvme/host_lld.d \
./src/hil/nvme/nvme_admin_cmd.d \
./src/hil/nvme/nvme_identify.d \
./src/hil/nvme/nvme_io_cmd.d \
./src/hil/nvme/nvme_main.d 


# Each subdirectory must supply rules for building sources it contributes
src/hil/nvme/host_lld.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/host_lld.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hil/nvme/nvme_admin_cmd.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_admin_cmd.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hil/nvme/nvme_identify.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_identify.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hil/nvme/nvme_io_cmd.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_io_cmd.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hil/nvme/nvme_main.o: C:/CosmosPlus_DFTL_KJH/cosmos_plus_DFTL/hil/nvme/nvme_main.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


