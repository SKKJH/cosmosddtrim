################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../.metadata/.plugins/org.eclipse.cdt.make.core/specs.c 

CPP_SRCS += \
../.metadata/.plugins/org.eclipse.cdt.make.core/specs.cpp 

OBJS += \
./.metadata/.plugins/org.eclipse.cdt.make.core/specs.o 

C_DEPS += \
./.metadata/.plugins/org.eclipse.cdt.make.core/specs.d 

CPP_DEPS += \
./.metadata/.plugins/org.eclipse.cdt.make.core/specs.d 


# Each subdirectory must supply rules for building sources it contributes
.metadata/.plugins/org.eclipse.cdt.make.core/%.o: ../.metadata/.plugins/org.eclipse.cdt.make.core/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -DCOSMOS_PLUS=1 -DDFTL=1 -Wall -O0 -g3 -I../../DFTL_bsp/ps7_cortexa9_0/include -I../../../../common -I../../../../fil -I../../../../ftl -I../../../../ftl/dftl -I../../../../hil -I../../../../hil/nvme -I../../../../target -I../../../../target/cosmos_plus -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

.metadata/.plugins/org.eclipse.cdt.make.core/%.o: ../.metadata/.plugins/org.eclipse.cdt.make.core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -DCOSMOS_PLUS=1 -DDFTL=1 -Wall -O0 -g3 -I../../DFTL_bsp/ps7_cortexa9_0/include -I../../../../common -I../../../../fil -I../../../../ftl -I../../../../ftl/dftl -I../../../../hil -I../../../../hil/nvme -I../../../../target -I../../../../target/cosmos_plus -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


