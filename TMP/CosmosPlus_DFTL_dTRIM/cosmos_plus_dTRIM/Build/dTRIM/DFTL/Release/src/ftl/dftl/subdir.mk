################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_activeblock.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_block.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_bufferpool.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_external_interface.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_garbagecollector.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_global.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_hdma.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_meta.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_profile.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_gc.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_hil.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_meta.cpp \
C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_vnand.cpp 

OBJS += \
./src/ftl/dftl/dftl_activeblock.o \
./src/ftl/dftl/dftl_block.o \
./src/ftl/dftl/dftl_bufferpool.o \
./src/ftl/dftl/dftl_external_interface.o \
./src/ftl/dftl/dftl_garbagecollector.o \
./src/ftl/dftl/dftl_global.o \
./src/ftl/dftl/dftl_hdma.o \
./src/ftl/dftl/dftl_meta.o \
./src/ftl/dftl/dftl_profile.o \
./src/ftl/dftl/dftl_request.o \
./src/ftl/dftl/dftl_request_gc.o \
./src/ftl/dftl/dftl_request_hil.o \
./src/ftl/dftl/dftl_request_meta.o \
./src/ftl/dftl/dftl_vnand.o 

CPP_DEPS += \
./src/ftl/dftl/dftl_activeblock.d \
./src/ftl/dftl/dftl_block.d \
./src/ftl/dftl/dftl_bufferpool.d \
./src/ftl/dftl/dftl_external_interface.d \
./src/ftl/dftl/dftl_garbagecollector.d \
./src/ftl/dftl/dftl_global.d \
./src/ftl/dftl/dftl_hdma.d \
./src/ftl/dftl/dftl_meta.d \
./src/ftl/dftl/dftl_profile.d \
./src/ftl/dftl/dftl_request.d \
./src/ftl/dftl/dftl_request_gc.d \
./src/ftl/dftl/dftl_request_hil.d \
./src/ftl/dftl/dftl_request_meta.d \
./src/ftl/dftl/dftl_vnand.d 


# Each subdirectory must supply rules for building sources it contributes
src/ftl/dftl/dftl_activeblock.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_activeblock.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_block.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_block.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_bufferpool.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_bufferpool.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_external_interface.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_external_interface.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_garbagecollector.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_garbagecollector.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_global.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_global.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_hdma.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_hdma.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_meta.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_meta.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_profile.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_profile.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_request.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_request_gc.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_gc.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_request_hil.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_hil.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_request_meta.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_request_meta.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ftl/dftl/dftl_vnand.o: C:/CosmosPlus_DFTL_TRIM/CosmosPlus_DFTL_dTRIM/cosmos_plus_dTRIM/ftl/dftl/dftl_vnand.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../DFTL_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


