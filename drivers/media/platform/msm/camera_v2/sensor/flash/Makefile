ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/cci
ccflags-y += -Idrivers/media/platform/msm/camera_v2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/io
obj-$(CONFIG_MSMB_CAMERA) += msm_led_flash.o
obj-$(CONFIG_MSMB_CAMERA) += msm_led_trigger.o
obj-$(CONFIG_MSMB_CAMERA) += msm_led_i2c_trigger.o
#ifndef CONFIG_MACH_VIVO
//obj-$(CONFIG_MSMB_CAMERA) += adp1660.o
//obj-$(CONFIG_MSMB_CAMERA) += bd7710.o
#endif
obj-$(CONFIG_MSMB_CAMERA) += msm_led_torch.o
#ifndef CONFIG_MACH_VIVO
//obj-$(CONFIG_MSMB_CAMERA) += msm_flash.o
//obj-$(CONFIG_MSMB_CAMERA) += lm3642.o
#endif
#ifdef CONFIG_MACH_VIVO
obj-$(CONFIG_MSMB_CAMERA) += tps61310.o
#endif

