#ifndef DT_BINDINGS_MEMORY_TEGRA186_MC_H
#define DT_BINDINGS_MEMORY_TEGRA186_MC_H

/* special clients */
#define TEGRA186_SID_INVALID		0x00
#define TEGRA186_SID_PASSTHROUGH	0x7f

/* host1x clients */
#define TEGRA186_SID_HOST1X		0x01
#define TEGRA186_SID_CSI		0x02
#define TEGRA186_SID_VIC		0x03
#define TEGRA186_SID_VI			0x04
#define TEGRA186_SID_ISP		0x05
#define TEGRA186_SID_NVDEC		0x06
#define TEGRA186_SID_NVENC		0x07
#define TEGRA186_SID_NVJPG		0x08
#define TEGRA186_SID_NVDISPLAY		0x09
#define TEGRA186_SID_TSEC		0x0a
#define TEGRA186_SID_TSECB		0x0b
#define TEGRA186_SID_SE			0x0c
#define TEGRA186_SID_SE1		0x0d
#define TEGRA186_SID_SE2		0x0e
#define TEGRA186_SID_SE3		0x0f

/* GPU clients */
#define TEGRA186_SID_GPU		0x10

/* other SoC clients */
#define TEGRA186_SID_AFI		0x11
#define TEGRA186_SID_HDA		0x12
#define TEGRA186_SID_ETR		0x13
#define TEGRA186_SID_EQOS		0x14
#define TEGRA186_SID_UFSHC		0x15
#define TEGRA186_SID_AON		0x16
#define TEGRA186_SID_SDMMC4		0x17
#define TEGRA186_SID_SDMMC3		0x18
#define TEGRA186_SID_SDMMC2		0x19
#define TEGRA186_SID_SDMMC1		0x1a
#define TEGRA186_SID_XUSB_HOST		0x1b
#define TEGRA186_SID_XUSB_DEV		0x1c
#define TEGRA186_SID_SATA		0x1d
#define TEGRA186_SID_APE		0x1e
#define TEGRA186_SID_SCE		0x1f

/* GPC DMA clients */
#define TEGRA186_SID_GPCDMA_0		0x20
#define TEGRA186_SID_GPCDMA_1		0x21
#define TEGRA186_SID_GPCDMA_2		0x22
#define TEGRA186_SID_GPCDMA_3		0x23
#define TEGRA186_SID_GPCDMA_4		0x24
#define TEGRA186_SID_GPCDMA_5		0x25
#define TEGRA186_SID_GPCDMA_6		0x26
#define TEGRA186_SID_GPCDMA_7		0x27

/* APE DMA clients */
#define TEGRA186_SID_APE_1		0x28
#define TEGRA186_SID_APE_2		0x29

/* camera RTCPU */
#define TEGRA186_SID_RCE		0x2a

/* camera RTCPU on host1x address space */
#define TEGRA186_SID_RCE_1X		0x2b

/* APE DMA clients */
#define TEGRA186_SID_APE_3		0x2c

/* camera RTCPU running on APE */
#define TEGRA186_SID_APE_CAM		0x2d
#define TEGRA186_SID_APE_CAM_1X		0x2e

/*
 * The BPMP has its SID value hardcoded in the firmware. Changing it requires
 * considerable effort.
 */
#define TEGRA186_SID_BPMP		0x32

/* for SMMU tests */
#define TEGRA186_SID_SMMU_TEST		0x33

/* host1x virtualization channels */
#define TEGRA186_SID_HOST1X_CTX0	0x38
#define TEGRA186_SID_HOST1X_CTX1	0x39
#define TEGRA186_SID_HOST1X_CTX2	0x3a
#define TEGRA186_SID_HOST1X_CTX3	0x3b
#define TEGRA186_SID_HOST1X_CTX4	0x3c
#define TEGRA186_SID_HOST1X_CTX5	0x3d
#define TEGRA186_SID_HOST1X_CTX6	0x3e
#define TEGRA186_SID_HOST1X_CTX7	0x3f

/* host1x command buffers */
#define TEGRA186_SID_HOST1X_VM0		0x40
#define TEGRA186_SID_HOST1X_VM1		0x41
#define TEGRA186_SID_HOST1X_VM2		0x42
#define TEGRA186_SID_HOST1X_VM3		0x43
#define TEGRA186_SID_HOST1X_VM4		0x44
#define TEGRA186_SID_HOST1X_VM5		0x45
#define TEGRA186_SID_HOST1X_VM6		0x46
#define TEGRA186_SID_HOST1X_VM7		0x47

/* SE data buffers */
#define TEGRA186_SID_SE_VM0		0x48
#define TEGRA186_SID_SE_VM1		0x49
#define TEGRA186_SID_SE_VM2		0x4a
#define TEGRA186_SID_SE_VM3		0x4b
#define TEGRA186_SID_SE_VM4		0x4c
#define TEGRA186_SID_SE_VM5		0x4d
#define TEGRA186_SID_SE_VM6		0x4e
#define TEGRA186_SID_SE_VM7		0x4f

#endif
