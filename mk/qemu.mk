QEMU_STM32 ?= ../qemu_stm32-stm32_v0.1.3/arm-softmmu/qemu-system-arm

qemu: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 -nographic\
		-cpu cortex-m3 \
		-kernel $(OUTDIR)/$(TARGET).bin -semihosting

qemud: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-gdb tcp::3333 -S \
		-kernel $(OUTDIR)/$(TARGET).bin -semihosting

qemugrasp: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	bash tool/emulate_grasp.sh $(OUTDIR)/$(TARGET).bin
	python log2grasp.py
	../grasp_linux/grasp sched.grasp

run:
	$(CROSS_COMPILE)gdb -x $(TOOLDIR)/gdbscript

qemunographic: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-monitor stdio -nographic \
		-kernel $(OUTDIR)/$(TARGET).bin -semihosting

qemudbg: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-monitor stdio \
		-gdb tcp::3333 -S \
		-kernel $(OUTDIR)/$(TARGET).bin -semihosting 2>&1>/dev/null & \
	echo $$! > $(OUTDIR)/qemu_pid && \
	$(CROSS_COMPILE)gdb -x $(TOOLDIR)/gdbscript && \
	cat $(OUTDIR)/qemu_pid | `xargs kill 2>/dev/null || test true` && \
	rm -f $(OUTDIR)/qemu_pid

qemu_remote: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 -kernel $(OUTDIR)/$(TARGET).bin -vnc :1

qemudbg_remote: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-gdb tcp::3333 -S \
		-kernel $(OUTDIR)/$(TARGET).bin \
		-vnc :1

qemu_remote_bg: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-kernel $(OUTDIR)/$(TARGET).bin \
		-vnc :1 &

qemudbg_remote_bg: $(OUTDIR)/$(TARGET).bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-gdb tcp::3333 -S \
		-kernel $(OUTDIR)/$(TARGET).bin \
		-vnc :1 &

emu: $(OUTDIR)/$(TARGET).bin
	bash $(TOOLDIR)/emulate.sh $(OUTDIR)/$(TARGET).bin

qemuauto: $(OUTDIR)/$(TARGET).bin $(TOOLDIR)gdbscript
	bash $(TOOLDIR)emulate.sh $(OUTDIR)/$(TARGET).bin &
	sleep 1
	$(CROSS_COMPILE)gdb -x gdbscript&
	sleep 5

qemuauto_remote: $(OUTDIR)/$(TARGET).bin $(TOOLDIR)gdbscript
	bash $(TOOLDIR)emulate_remote.sh $(OUTDIR)/$(TARGET).bin &
	sleep 1
	$(CROSS_COMPILE)gdb -x gdbscript&
	sleep 5
