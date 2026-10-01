/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*******************************************************************************
 * Copyright (C) 2016-2018, Allwinner Technology CO., LTD.
 * Author: zhuxianbin <zhuxianbin@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 ******************************************************************************/
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/sizes.h>
#include <linux/device.h>
#include <asm/cacheflush.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>

#include <sunxi-iommu.h>
#include "sun55i-iommu.h"

/*
 * Register of IOMMU device
 */
#define IOMMU_VERSION_REG			0x0000
#define IOMMU_RESET_REG				0x0010
#define IOMMU_ENABLE_REG			0x0020
#define IOMMU_BYPASS_REG			0x0030
#define IOMMU_AUTO_GATING_REG			0x0040
#define IOMMU_WBUF_CTRL_REG			0x0044
#define IOMMU_OOO_CTRL_REG			0x0048
#define IOMMU_4KB_BDY_PRT_CTRL_REG		0x004C
#define IOMMU_TTB_REG				0x0050
#define IOMMU_TLB_ENABLE_REG			0x0060
#define IOMMU_TLB_PREFETCH_REG			0x0070
#define IOMMU_TLB_FLUSH_ENABLE_REG		0x0080
#define IOMMU_TLB_IVLD_MODE_SEL_REG		0x0084
#define IOMMU_TLB_IVLD_START_ADDR_REG		0x0088
#define IOMMU_TLB_IVLD_END_ADDR_REG		0x008C
#define IOMMU_TLB_IVLD_ADDR_REG			0x0090
#define IOMMU_TLB_IVLD_ADDR_MASK_REG		0x0094
#define IOMMU_TLB_IVLD_ENABLE_REG		0x0098
#define IOMMU_PC_IVLD_MODE_SEL_REG		0x009C
#define IOMMU_PC_IVLD_ADDR_REG			0x00A0
#define IOMMU_PC_IVLD_START_ADDR_REG		0x00A4
#define IOMMU_PC_IVLD_ENABLE_REG		0x00A8
#define IOMMU_PC_IVLD_END_ADDR_REG		0x00Ac
#define IOMMU_DM_AUT_CTRL_REG0			0x00B0
#define IOMMU_DM_AUT_CTRL_REG1			0x00B4
#define IOMMU_DM_AUT_CTRL_REG2			0x00B8
#define IOMMU_DM_AUT_CTRL_REG3			0x00BC
#define IOMMU_DM_AUT_CTRL_REG4			0x00C0
#define IOMMU_DM_AUT_CTRL_REG5			0x00C4
#define IOMMU_DM_AUT_CTRL_REG6			0x00C8
#define IOMMU_DM_AUT_CTRL_REG7			0x00CC
#define IOMMU_DM_AUT_OVWT_REG			0x00D0
#define IOMMU_INT_ENABLE_REG			0x0100
#define IOMMU_INT_CLR_REG			0x0104
#define IOMMU_INT_STA_REG			0x0108
#define IOMMU_INT_ERR_ADDR_REG0			0x0110

#define IOMMU_INT_ERR_ADDR_REG1			0x0114
#define IOMMU_INT_ERR_ADDR_REG2			0x0118

#define IOMMU_INT_ERR_ADDR_REG3			0x011C
#define IOMMU_INT_ERR_ADDR_REG4			0x0120
#define IOMMU_INT_ERR_ADDR_REG5			0x0124

#define IOMMU_INT_ERR_ADDR_REG6			0x0128
#define IOMMU_INT_ERR_ADDR_REG7			0x0130
#define IOMMU_INT_ERR_ADDR_REG8			0x0134

#define IOMMU_INT_ERR_DATA_REG0			0x0150
#define IOMMU_INT_ERR_DATA_REG1			0x0154
#define IOMMU_INT_ERR_DATA_REG2			0x0158
#define IOMMU_INT_ERR_DATA_REG3			0x015C
#define IOMMU_INT_ERR_DATA_REG4			0x0160
#define IOMMU_INT_ERR_DATA_REG5			0x0164

#define IOMMU_INT_ERR_DATA_REG6			0x0168
#define IOMMU_INT_ERR_DATA_REG7			0x0170
#define IOMMU_INT_ERR_DATA_REG8			0x0174

#define IOMMU_L1PG_INT_REG			0x0180
#define IOMMU_L2PG_INT_REG			0x0184
#define IOMMU_VA_REG				0x0190
#define IOMMU_VA_DATA_REG			0x0194
#define IOMMU_VA_CONFIG_REG			0x0198
#define IOMMU_PMU_ENABLE_REG			0x0200
#define IOMMU_PMU_CLR_REG			0x0210
#define IOMMU_PMU_ACCESS_LOW_REG0		0x0230
#define IOMMU_PMU_ACCESS_HIGH_REG0		0x0234
#define IOMMU_PMU_HIT_LOW_REG0			0x0238
#define IOMMU_PMU_HIT_HIGH_REG0			0x023C
#define IOMMU_PMU_ACCESS_LOW_REG1		0x0240
#define IOMMU_PMU_ACCESS_HIGH_REG1		0x0244
#define IOMMU_PMU_HIT_LOW_REG1			0x0248
#define IOMMU_PMU_HIT_HIGH_REG1			0x024C
#define IOMMU_PMU_ACCESS_LOW_REG2		0x0250
#define IOMMU_PMU_ACCESS_HIGH_REG2		0x0254
#define IOMMU_PMU_HIT_LOW_REG2			0x0258
#define IOMMU_PMU_HIT_HIGH_REG2			0x025C
#define IOMMU_PMU_ACCESS_LOW_REG3		0x0260
#define IOMMU_PMU_ACCESS_HIGH_REG3		0x0264
#define IOMMU_PMU_HIT_LOW_REG3			0x0268
#define IOMMU_PMU_HIT_HIGH_REG3			0x026C
#define IOMMU_PMU_ACCESS_LOW_REG4		0x0270
#define IOMMU_PMU_ACCESS_HIGH_REG4		0x0274
#define IOMMU_PMU_HIT_LOW_REG4			0x0278
#define IOMMU_PMU_HIT_HIGH_REG4			0x027C
#define IOMMU_PMU_ACCESS_LOW_REG5		0x0280
#define IOMMU_PMU_ACCESS_HIGH_REG5		0x0284
#define IOMMU_PMU_HIT_LOW_REG5			0x0288
#define IOMMU_PMU_HIT_HIGH_REG5			0x028C

#define IOMMU_PMU_ACCESS_LOW_REG6		0x0290
#define IOMMU_PMU_ACCESS_HIGH_REG6		0x0294
#define IOMMU_PMU_HIT_LOW_REG6			0x0298
#define IOMMU_PMU_HIT_HIGH_REG6			0x029C
#define IOMMU_PMU_ACCESS_LOW_REG7		0x02D0
#define IOMMU_PMU_ACCESS_HIGH_REG7		0x02D4
#define IOMMU_PMU_HIT_LOW_REG7			0x02D8
#define IOMMU_PMU_HIT_HIGH_REG7			0x02DC
#define IOMMU_PMU_ACCESS_LOW_REG8		0x02E0
#define IOMMU_PMU_ACCESS_HIGH_REG8		0x02E4
#define IOMMU_PMU_HIT_LOW_REG8			0x02E8
#define IOMMU_PMU_HIT_HIGH_REG8			0x02EC

#define IOMMU_PMU_TL_LOW_REG0			0x0300
#define IOMMU_PMU_TL_HIGH_REG0			0x0304
#define IOMMU_PMU_ML_REG0			0x0308

#define IOMMU_PMU_TL_LOW_REG1			0x0310
#define IOMMU_PMU_TL_HIGH_REG1			0x0314
#define IOMMU_PMU_ML_REG1			0x0318

#define IOMMU_PMU_TL_LOW_REG2			0x0320
#define IOMMU_PMU_TL_HIGH_REG2			0x0324
#define IOMMU_PMU_ML_REG2			0x0328

#define IOMMU_PMU_TL_LOW_REG3			0x0330
#define IOMMU_PMU_TL_HIGH_REG3			0x0334
#define IOMMU_PMU_ML_REG3			0x0338

#define IOMMU_PMU_TL_LOW_REG4			0x0340
#define IOMMU_PMU_TL_HIGH_REG4			0x0344
#define IOMMU_PMU_ML_REG4			0x0348

#define IOMMU_PMU_TL_LOW_REG5			0x0350
#define IOMMU_PMU_TL_HIGH_REG5			0x0354
#define IOMMU_PMU_ML_REG5			0x0358

#define IOMMU_PMU_TL_LOW_REG6			0x0360
#define IOMMU_PMU_TL_HIGH_REG6			0x0364
#define IOMMU_PMU_ML_REG6			0x0368

#define IOMMU_RESET_SHIFT   31
#define IOMMU_RESET_MASK (1 << IOMMU_RESET_SHIFT)
#define IOMMU_RESET_SET (0 << 31)
#define IOMMU_RESET_RELEASE (1 << 31)

/*
 * IOMMU enable register field
 */
#define IOMMU_ENABLE	0x1

/*
 * IOMMU interrupt id mask
 */
#define MICRO_TLB0_INVALID_INTER_MASK   0x1
#define MICRO_TLB1_INVALID_INTER_MASK   0x2
#define MICRO_TLB2_INVALID_INTER_MASK   0x4
#define MICRO_TLB3_INVALID_INTER_MASK   0x8
#define MICRO_TLB4_INVALID_INTER_MASK   0x10
#define MICRO_TLB5_INVALID_INTER_MASK   0x20
#define MICRO_TLB6_INVALID_INTER_MASK   0x40

#define L1_PAGETABLE_INVALID_INTER_MASK   0x10000
#define L2_PAGETABLE_INVALID_INTER_MASK   0x20000

/**
 * sun8iw15p1
 *	DE :		masterID 0
 *	E_EDMA:		masterID 1
 *	E_FE:		masterID 2
 *	VE:		masterID 3
 *	CSI:		masterID 4
 *	G2D:		masterID 5
 *	E_BE:		masterID 6
 *
 * sun50iw9p1:
 *	DE :		masterID 0
 *	DI:			masterID 1
 *	VE_R:		masterID 2
 *	VE:			masterID 3
 *	CSI0:		masterID 4
 *	CSI1:		masterID 5
 *	G2D:		masterID 6
 * sun8iw19p1:
 *	DE :>--->-------masterID 0
 *	EISE:		masterID 1
 *	AI:		masterID 2
 *	VE:>---->-------masterID 3
 *	CSI:	>-->----masterID 4
 *	ISP:>-->------	masterID 5
 *	G2D:>--->-------masterID 6
 * sun8iw21:
 *	VE :			masterID 0
 *	CSI:			masterID 1
 *	DE:				masterID 2
 *	G2D:			masterID 3
 *	ISP:			masterID 4
 *	RISCV:			masterID 5
 *	NPU:			masterID 6
 */
#define DEFAULT_BYPASS_VALUE     0x7f
static const u32 master_id_bitmap[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40};

#define sunxi_wait_when(COND, MS) ({ \
	unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1;	\
	int ret__ = 0;							\
	while ((COND)) {						\
		if (time_after(jiffies, timeout__)) {			\
			ret__ = (!COND) ? 0 : -ETIMEDOUT;		\
			break;						\
		}							\
		udelay(1);					\
	}								\
	ret__;								\
})

/*
 * The format of device tree, and client device how to use it.
 *
 * /{
 *	....
 *	smmu: iommu@xxxxx {
 *		compatible = "allwinner,iommu";
 *		reg = <xxx xxx xxx xxx>;
 *		interrupts = <GIC_SPI xxx IRQ_TYPE_LEVEL_HIGH>;
 *		interrupt-names = "iommu-irq";
 *		clocks = <&iommu_clk>;
 *		clock-name = "iommu-clk";
 *		#iommu-cells = <1>;
 *		status = "enabled";
 *	};
 *
 *	de@xxxxx {
 *		.....
 *		iommus = <&smmu ID>;
 *	};
 *
 * }
 *
 * Here, ID number is 0 ~ 5, every client device have a unique id.
 * Every id represent a micro TLB, also represent a master device.
 *
 */

enum sunxi_iommu_version {
	IOMMU_VERSION_V10 = 0x10,
	IOMMU_VERSION_V11,
	IOMMU_VERSION_V12,
	IOMMU_VERSION_V13,
	IOMMU_VERSION_V14,
};

struct sunxi_iommu_plat_data {
	u32 version;
	u32 tlb_prefetch;
	u32 tlb_invalid_mode;
	u32 ptw_invalid_mode;
	const char *master[8];
};

struct sunxi_iommu_dev {
	struct iommu_device iommu;
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	int irq;
	u32 bypass;
	spinlock_t iommu_lock;
	struct list_head rsv_list;
	const struct sunxi_iommu_plat_data *plat_data;
	struct iommu_domain *identity_domain;
	struct sunxi_iommu_domain *debug_domain;
};

struct sunxi_iommu_domain {
	unsigned int *pgtable;		/* first page directory, size is 16KB */
	u32 *sg_buffer;
	struct spinlock  dt_lock;	/* lock for modifying page table @ pgtable */
	struct dma_iommu_mapping *mapping;
	struct iommu_domain domain;
	/* struct iova_domain iovad; */
	/* list of master device, it represent a micro TLB */
	struct list_head mdevs;
	spinlock_t lock;
};

/*
 * sunxi master device which use iommu.
 */
struct sunxi_mdev {
	struct list_head node;	/* for sunxi_iommu mdevs list */
	struct device *dev;	/* the master device */
	unsigned int tlbid;	/* micro TLB id, distinguish device by it */
	bool flag;
};

struct sunxi_iommu_owner {
	unsigned int tlbid;
	bool flag;
	struct sunxi_iommu_dev *data;
	struct device *dev;
	struct dma_iommu_mapping *mapping;
};

#define _max(x, y) (((u64)(x) > (u64)(y)) ? (x) : (y))

static struct kmem_cache *iopte_cache;
static struct sunxi_iommu_dev *global_iommu_dev;
static bool iommu_hw_init_flag;
static struct device *dma_dev;

static sunxi_iommu_fault_cb sunxi_iommu_fault_notify_cbs[7];
u32 sunxi_iommu_dump_rsv_list(struct list_head *rsv_list, ssize_t len,
                  char *buf, size_t buf_len, bool for_sysfs_show);
int sunxi_iommu_check_cmd(struct device *dev, void *data);

void sun55i_iommu_register_fault_cb(sunxi_iommu_fault_cb cb, unsigned int master_id)
{
	if (master_id >= ARRAY_SIZE(sunxi_iommu_fault_notify_cbs))
		return;
	sunxi_iommu_fault_notify_cbs[master_id] = cb;
}
EXPORT_SYMBOL_GPL(sun55i_iommu_register_fault_cb);

static inline u32 sunxi_iommu_read(struct sunxi_iommu_dev *iommu,
				   u32 offset)
{
	return readl(iommu->base + offset);
}

static inline void sunxi_iommu_write(struct sunxi_iommu_dev *iommu,
				     u32 offset, u32 value)
{
	writel(value, iommu->base + offset);
}

void sun55i_reset_device_iommu(unsigned int master_id)
{
	unsigned int regval;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;

	if (master_id >= 7)
		return;

	if (!iommu)
		return;

	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, regval & (~(1 << master_id)));
	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG);
	if (!(regval & ((1 << master_id)))) {
		sunxi_iommu_write(iommu, IOMMU_RESET_REG, regval | ((1 << master_id)));
	}
}
EXPORT_SYMBOL(sun55i_reset_device_iommu);

void sun55i_enable_device_iommu(struct sunxi_iommu_dev *iommu, unsigned int master_id, bool flag)
{
	unsigned long mflag;

	if (!iommu)
		return;

	if (master_id >= ARRAY_SIZE(master_id_bitmap))
		return;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	if (flag)
		iommu->bypass &= ~(master_id_bitmap[master_id]);
	else
		iommu->bypass |= master_id_bitmap[master_id];
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
}
EXPORT_SYMBOL(sun55i_enable_device_iommu);

static int sun55i_tlb_flush(struct sunxi_iommu_dev *iommu)
{
	int ret;

	/* enable the maximum number(7) of master to fit all platform */
	sunxi_iommu_write(iommu, IOMMU_TLB_FLUSH_ENABLE_REG, 0x0003007f);
	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_FLUSH_ENABLE_REG)), 2);
	if (ret)
		dev_err(iommu->dev, "Enable flush all request timed out\n");

	return ret;
}

static int sun55i_iommu_hw_init(struct sunxi_iommu_dev *iommu, struct sunxi_iommu_domain *sunxi_domain)
{
	int ret = 0;
	int iommu_enable = 0;
	unsigned long mflag;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);

	if (sunxi_domain) {
		phys_addr_t dte_addr = __pa(sunxi_domain->pgtable);
		sunxi_iommu_write(iommu, IOMMU_TTB_REG, dte_addr);
	}

	/*
	 * set preftech functions, including:
	 * master prefetching and only prefetch valid page to TLB/PTW
	 */
	sunxi_iommu_write(iommu, IOMMU_TLB_PREFETCH_REG, plat_data->tlb_prefetch);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_MODE_SEL_REG, plat_data->tlb_invalid_mode);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_MODE_SEL_REG, plat_data->ptw_invalid_mode);

	/* disable interrupt of prefetch */
	sunxi_iommu_write(iommu, IOMMU_INT_ENABLE_REG, 0x3003f);
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);

	ret = sun55i_tlb_flush(iommu);
	if (ret) {
		dev_err(iommu->dev, "Enable flush all request timed out\n");
		goto out;
	}
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);
	sunxi_iommu_write(iommu, IOMMU_ENABLE_REG, IOMMU_ENABLE);
	iommu_enable = sunxi_iommu_read(iommu, IOMMU_ENABLE_REG);
	if (iommu_enable != 0x1) {
		iommu_enable = sunxi_iommu_read(iommu, IOMMU_ENABLE_REG);
		if (iommu_enable != 0x1) {
			dev_err(iommu->dev, "iommu enable failed! No iommu in bitfile!\n");
			ret = -ENODEV;
			goto out;
		}
	}
	iommu_hw_init_flag = true;

out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static int sun55i_tlb_invalid(dma_addr_t iova, dma_addr_t iova_mask)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	dma_addr_t iova_end = iova_mask;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	/* new TLB invalid function: use range(start, end) to invalid TLB page */
	pr_debug("iommu: TLB invalid:0x%x-0x%x\n", (unsigned int)iova,
		(unsigned int)iova_end);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_START_ADDR_REG, iova);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_END_ADDR_REG, iova_end);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);

	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "TLB cache invalid timed out\n");
	}
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static int sun55i_ptw_cache_invalid(dma_addr_t iova_start, dma_addr_t iova_end)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	/* new PTW invalid function: use range(start, end) to invalid PTW page */
	pr_debug("iommu: PTW invalid:0x%x-0x%x\n", (unsigned int)iova_start,
		 (unsigned int)iova_end);
	WARN_ON(iova_end == 0);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_START_ADDR_REG, iova_start);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_END_ADDR_REG, iova_end);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);

	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "PTW cache invalid timed out\n");
		goto out;
	}

out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static void sun55i_zap_tlb(unsigned long iova, size_t size)
{
	sun55i_tlb_invalid(iova, iova + 2 * SPAGE_SIZE);
	sun55i_tlb_invalid(iova + size - SPAGE_SIZE, iova + size + 8 * SPAGE_SIZE);
	sun55i_ptw_cache_invalid(iova, iova + SPD_SIZE);
	sun55i_ptw_cache_invalid(iova + size - SPD_SIZE, iova + size);

	return;
}

static int sun55i_iommu_map(struct iommu_domain *domain, unsigned long iova,
			   phys_addr_t paddr, size_t size, size_t count, int prot,
			   gfp_t gfp, size_t *mapped)
{
	struct sunxi_iommu_domain *sunxi_domain;
	size_t iova_start, iova_end;
	unsigned long total_size = size * count;
	int ret;
	unsigned long flags;

	sunxi_domain = container_of(domain, struct sunxi_iommu_domain, domain);
	WARN_ON(sunxi_domain->pgtable == NULL);

	iova_start = iova & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + total_size);

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);

	ret = sunxi_pgtable_prepare_l1_tables(sunxi_domain->pgtable, iova_start,
					      iova_end, prot);
	if (ret) {
		spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
		if (mapped)
			*mapped = 0;
		return -ENOMEM;
	}

	sunxi_pgtable_prepare_l2_tables(sunxi_domain->pgtable,
					iova_start, iova_end, paddr, prot);

	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	if (mapped)
		*mapped = total_size;

	return 0;
}

static size_t sun55i_iommu_unmap(struct iommu_domain *domain, unsigned long iova,
			       size_t size, size_t count,
			       struct iommu_iotlb_gather *gather)
{
	struct sunxi_iommu_domain *sunxi_domain;
	const struct sunxi_iommu_plat_data *plat_data;
	size_t iova_start, iova_end;
	unsigned long total_size = size * count;
	int iova_tail_size;
	unsigned long flags;

	sunxi_domain = container_of(domain, struct sunxi_iommu_domain, domain);
	plat_data = global_iommu_dev->plat_data;
	WARN_ON(sunxi_domain->pgtable == NULL);

	iova_start = iova & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + total_size);

	if (gather) {
		if (gather->start > iova_start)
			gather->start = iova_start;
		if (gather->end < iova_end)
			gather->end = iova_end;
	}

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);

	sun55i_tlb_invalid(iova_start, iova_end);
	sun55i_ptw_cache_invalid(iova_start, iova_end);

	for (; iova_start < iova_end; ) {
		iova_tail_size = sunxi_pgtable_delete_l2_tables(
				sunxi_domain->pgtable, iova_start, iova_end);
		if (iova_tail_size < 0) {
			spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
			return 0;
		}
		if (iova_tail_size == 0)
			break;

		sun55i_ptw_cache_invalid(iova_start, iova_start + iova_tail_size);
		iova_start += iova_tail_size;
	}
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return total_size;
}

static int sun55i_iommu_iotlb_sync_map(struct iommu_domain *domain,
				      unsigned long iova, size_t size)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	unsigned long flags;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sun55i_zap_tlb(iova, size);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return 0;
}

static phys_addr_t sun55i_iommu_iova_to_phys(struct iommu_domain *domain,
					    dma_addr_t iova)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	phys_addr_t ret = 0;
	unsigned long flags;

	WARN_ON(sunxi_domain->pgtable == NULL);
	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	ret = sunxi_pgtable_iova_to_phys(sunxi_domain->pgtable, iova);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return ret;
}

static struct iommu_domain *sun55i_iommu_domain_alloc_paging(struct device *dev)
{
	struct sunxi_iommu_domain *sunxi_domain;

	sunxi_domain = kzalloc(sizeof(*sunxi_domain), GFP_KERNEL);
	if (!sunxi_domain)
		return NULL;

	sunxi_domain->pgtable = sunxi_pgtable_alloc();
	if (!sunxi_domain->pgtable) {
		pr_err("sunxi domain get pgtable failed\n");
		goto err_page;
	}

	sunxi_domain->sg_buffer = (unsigned int *)__get_free_pages(
				GFP_KERNEL, get_order(MAX_SG_TABLE_SIZE));
	if (!sunxi_domain->sg_buffer) {
		pr_err("sunxi domain get sg_buffer failed\n");
		goto err_sg_buffer;
	}

	sunxi_domain->domain.geometry.aperture_start = 0;
	sunxi_domain->domain.geometry.aperture_end	 = (1ULL << 32) - 1;
	sunxi_domain->domain.geometry.force_aperture = true;
	spin_lock_init(&sunxi_domain->dt_lock);

	if (global_iommu_dev)
		global_iommu_dev->debug_domain = sunxi_domain;

	if (!iommu_hw_init_flag) {
		if (sun55i_iommu_hw_init(global_iommu_dev, sunxi_domain))
			pr_err("sunxi iommu hardware init failed\n");
	}

	return &sunxi_domain->domain;

err_sg_buffer:
	sunxi_pgtable_free(sunxi_domain->pgtable);
	sunxi_domain->pgtable = NULL;
err_page:
	kfree(sunxi_domain);

	return NULL;
}

static void sun55i_iommu_domain_free(struct iommu_domain *domain)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	unsigned long flags;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_pgtable_clear(sunxi_domain->pgtable);
	sun55i_tlb_flush(global_iommu_dev);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
	sunxi_pgtable_free(sunxi_domain->pgtable);
	sunxi_domain->pgtable = NULL;
	free_pages((unsigned long)sunxi_domain->sg_buffer,
						get_order(MAX_SG_TABLE_SIZE));
	sunxi_domain->sg_buffer = NULL;
	kfree(sunxi_domain);
}

static int sun55i_iommu_attach_dev(struct iommu_domain *domain,
				   struct device *dev)
{
	return 0;
}

static void sun55i_iommu_probe_device_finalize(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

	WARN(!dev->dma_mask || *dev->dma_mask == 0, "NULL or 0 dma mask will fail iommu setup\n");
	iommu_setup_dma_ops(dev);

	sun55i_enable_device_iommu(owner->data, owner->tlbid, owner->flag);
}

static struct iommu_device *sun55i_iommu_probe_device(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

	if (!owner) /* Not a iommu client device */
		return ERR_PTR(-ENODEV);

	return &owner->data->iommu;
}

static void sun55i_iommu_release_device(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

	if (!owner)
		return;

	sun55i_enable_device_iommu(owner->data, owner->tlbid, false);
	dev->iommu_group = NULL;
	devm_kfree(dev, dev->dma_parms);
	dev->dma_parms = NULL;
	kfree(owner);
	owner = NULL;
	dev_iommu_priv_set(dev, NULL);
}

static int sun55i_iommu_of_xlate(struct device *dev,
				 const struct of_phandle_args *args)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);
	struct platform_device *sysmmu = of_find_device_by_node(args->np);
	struct sunxi_iommu_dev *data;

	if (!sysmmu)
		return -ENODEV;

	data = platform_get_drvdata(sysmmu);
	if (data == NULL)
		return -ENODEV;

	if (!owner) {
		owner = kzalloc(sizeof(*owner), GFP_KERNEL);
		if (!owner)
			return -ENOMEM;
		owner->tlbid = args->args[0];
		if (args->args_count > 1)
			owner->flag = args->args[1];
		else
			owner->flag = 0;
		owner->data = data;
		owner->dev = dev;
		dev_iommu_priv_set(dev, owner);
	}

	return 0;
}

static irqreturn_t sunxi_iommu_irq(int irq, void *dev_id)
{
	u32 inter_status_reg = 0;
	u32 addr_reg = 0;
	u32	int_masterid_bitmap = 0;
	u32	data_reg = 0;
	u32	l1_pgint_reg = 0;
	u32	l2_pgint_reg = 0;
	u32	master_id = 0;
	unsigned long mflag;
	struct sunxi_iommu_dev *iommu = dev_id;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	inter_status_reg = sunxi_iommu_read(iommu, IOMMU_INT_STA_REG) & 0x3ffff;
	l1_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L1PG_INT_REG);
	l2_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L2PG_INT_REG);
	int_masterid_bitmap = inter_status_reg | l1_pgint_reg | l2_pgint_reg;

	if (inter_status_reg & MICRO_TLB0_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[0]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG0);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG0);
	} else if (inter_status_reg & MICRO_TLB1_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[1]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG1);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG1);
	} else if (inter_status_reg & MICRO_TLB2_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[2]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG2);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG2);
	} else if (inter_status_reg & MICRO_TLB3_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[3]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG3);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG3);
	} else if (inter_status_reg & MICRO_TLB4_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[4]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG4);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG4);
	} else if (inter_status_reg & MICRO_TLB5_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[5]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG5);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG5);
	} else if (inter_status_reg & MICRO_TLB6_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[6]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG6);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG6);
	} else if (inter_status_reg & L1_PAGETABLE_INVALID_INTER_MASK) {
		/* It's OK to prefetch an invalid page, no need to print msg for debug. */
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L1 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG7);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG7);
	} else if (inter_status_reg & L2_PAGETABLE_INVALID_INTER_MASK) {
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L2 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG8);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG8);
	} else
		pr_err("sunxi iommu int error!!!\n");

	if (!(int_masterid_bitmap & (1U << 31))) {
		int_masterid_bitmap &= 0xffff;

		if (int_masterid_bitmap) {
			master_id = __ffs(int_masterid_bitmap);
			pr_err("Bug is in %s module, invalid address: 0x%x, data:0x%x, id:0x%x\n",
				plat_data->master[master_id], addr_reg, data_reg,
				int_masterid_bitmap);

			if (sunxi_iommu_fault_notify_cbs[master_id])
				sunxi_iommu_fault_notify_cbs[master_id]();
		} else {
			pr_err("Bug in unknown module (id=0), invalid address: 0x%x, data:0x%x\n",
				addr_reg, data_reg);
		}
	}

	/* invalid TLB */
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_START_ADDR_REG, addr_reg);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_END_ADDR_REG, addr_reg + 4 * SPAGE_SIZE);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG) & 0x1)
		;

	/* invalid PTW */
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_START_ADDR_REG, addr_reg);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_END_ADDR_REG, addr_reg + 2 * SPD_SIZE);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG) & 0x1)
		;

	sunxi_iommu_write(iommu, IOMMU_INT_CLR_REG, inter_status_reg);
	inter_status_reg |= (l1_pgint_reg | l2_pgint_reg);
	inter_status_reg &= 0xffff;
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, ~inter_status_reg);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, 0xffffffff);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return IRQ_HANDLED;
}

static ssize_t sunxi_iommu_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	u32 data;

	spin_lock(&iommu->iommu_lock);
	data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
	spin_unlock(&iommu->iommu_lock);

	return scnprintf(buf, PAGE_SIZE,
		"enable = %d\n", data & 0x1 ? 1 : 0);
}

static ssize_t sunxi_iommu_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long val;
	u32 data;
	int retval;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val) {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data | 0x1);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		spin_unlock(&iommu->iommu_lock);
	} else {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data & ~0x1);
		spin_unlock(&iommu->iommu_lock);
	}

	return count;
}

static ssize_t sunxi_iommu_profilling_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	struct {
		u64 macrotlb_access_count;
		u64 macrotlb_hit_count;
		u64 ptwcache_access_count;
		u64 ptwcache_hit_count;
		struct {
			u64 access_count;
			u64 hit_count;
			u64 latency;
			u32 max_latency;
		} micro_tlb[7];
	} *iommu_profile;
	iommu_profile = kmalloc(sizeof(*iommu_profile), GFP_KERNEL);
	if (!iommu_profile)
		return 0;
	int len;
	spin_lock(&iommu->iommu_lock);

	iommu_profile->micro_tlb[0].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG0) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG0);
	iommu_profile->micro_tlb[0].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG0) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG0);

	iommu_profile->micro_tlb[1].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG1) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG1);
	iommu_profile->micro_tlb[1].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG1) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG1);

	iommu_profile->micro_tlb[2].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG2) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG2);
	iommu_profile->micro_tlb[2].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG2) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG2);

	iommu_profile->micro_tlb[3].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG3) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG3);
	iommu_profile->micro_tlb[3].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG3) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG3);

	iommu_profile->micro_tlb[4].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG4) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG4);
	iommu_profile->micro_tlb[4].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG4) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG4);

	iommu_profile->micro_tlb[5].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG5) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG5);
	iommu_profile->micro_tlb[5].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG5) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG5);

	iommu_profile->micro_tlb[6].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG6) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG6);
	iommu_profile->micro_tlb[6].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG6) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG6);

	iommu_profile->macrotlb_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG7) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG7);
	iommu_profile->macrotlb_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG7) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG7);

	iommu_profile->ptwcache_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG8) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG8);
	iommu_profile->ptwcache_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG8) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG8);

	iommu_profile->micro_tlb[0].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG0) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG0);
	iommu_profile->micro_tlb[1].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG1) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG1);
	iommu_profile->micro_tlb[2].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG2) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG2);
	iommu_profile->micro_tlb[3].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG3) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG3);
	iommu_profile->micro_tlb[4].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG4) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG4);
	iommu_profile->micro_tlb[5].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG5) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG5);

	iommu_profile->micro_tlb[6].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG6) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG6);

	iommu_profile->micro_tlb[0].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG0);
	iommu_profile->micro_tlb[1].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG1);
	iommu_profile->micro_tlb[2].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG2);
	iommu_profile->micro_tlb[3].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG3);
	iommu_profile->micro_tlb[4].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG4);
	iommu_profile->micro_tlb[5].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG5);
	iommu_profile->micro_tlb[6].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG6);

	spin_unlock(&iommu->iommu_lock);
err:
	return 0;

	len = scnprintf(
		buf, PAGE_SIZE,
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"macrotlb_access_count = 0x%llx\n"
		"macrotlb_hit_count = 0x%llx\n"
		"ptwcache_access_count = 0x%llx\n"
		"ptwcache_hit_count = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n",
		plat_data->master[0], iommu_profile->micro_tlb[0].access_count,
		plat_data->master[0], iommu_profile->micro_tlb[0].hit_count,
		plat_data->master[1], iommu_profile->micro_tlb[1].access_count,
		plat_data->master[1], iommu_profile->micro_tlb[1].hit_count,
		plat_data->master[2], iommu_profile->micro_tlb[2].access_count,
		plat_data->master[2], iommu_profile->micro_tlb[2].hit_count,
		plat_data->master[3], iommu_profile->micro_tlb[3].access_count,
		plat_data->master[3], iommu_profile->micro_tlb[3].hit_count,
		plat_data->master[4], iommu_profile->micro_tlb[4].access_count,
		plat_data->master[4], iommu_profile->micro_tlb[4].hit_count,
		plat_data->master[5], iommu_profile->micro_tlb[5].access_count,
		plat_data->master[5], iommu_profile->micro_tlb[5].hit_count,
		plat_data->master[6], iommu_profile->micro_tlb[6].access_count,
		plat_data->master[6], iommu_profile->micro_tlb[6].hit_count,
		iommu_profile->macrotlb_access_count,
		iommu_profile->macrotlb_hit_count,
		iommu_profile->ptwcache_access_count,
		iommu_profile->ptwcache_hit_count, plat_data->master[0],
		iommu_profile->micro_tlb[0].latency, plat_data->master[1],
		iommu_profile->micro_tlb[1].latency, plat_data->master[2],
		iommu_profile->micro_tlb[2].latency, plat_data->master[3],
		iommu_profile->micro_tlb[3].latency, plat_data->master[4],
		iommu_profile->micro_tlb[4].latency, plat_data->master[5],
		iommu_profile->micro_tlb[5].latency, plat_data->master[6],
		iommu_profile->micro_tlb[6].latency, plat_data->master[0],
		iommu_profile->micro_tlb[0].max_latency, plat_data->master[1],
		iommu_profile->micro_tlb[1].max_latency, plat_data->master[2],
		iommu_profile->micro_tlb[2].max_latency, plat_data->master[3],
		iommu_profile->micro_tlb[3].max_latency, plat_data->master[4],
		iommu_profile->micro_tlb[4].max_latency, plat_data->master[5],
		iommu_profile->micro_tlb[5].max_latency, plat_data->master[6],
		iommu_profile->micro_tlb[6].max_latency);
	kfree(iommu_profile);
	return len;
}

static u32 __print_rsv_region(char *buf, size_t buf_len, ssize_t len,
			      struct dump_region *active_region,
			      bool for_sysfs_show)
{
	if (active_region->type == DUMP_REGION_RESERVE) {
		if (for_sysfs_show) {
			len += sysfs_emit_at(
				buf, len,
				"iova:%pad                            size:0x%zx\n",
				&active_region->iova, active_region->size);
		} else {
			len += scnprintf(
				buf + len, buf_len - len,
				"iova:%pad                            size:0x%zx\n",
				&active_region->iova, active_region->size);
		}
	}
	return len;
}

u32 sunxi_iommu_dump_rsv_list(struct list_head *rsv_list, ssize_t len,
			      char *buf, size_t buf_len, bool for_sysfs_show)
{
	struct iommu_resv_region *resv;
	struct dump_region active_region;
	if (for_sysfs_show) {
		len += sysfs_emit_at(buf, len, "reserved\n");
	} else {
		len += scnprintf(buf + len, buf_len - len, "reserved\n");
	}
	list_for_each_entry(resv, rsv_list, list) {
		active_region.access_mask = 0;
		active_region.iova = resv->start;
		active_region.type = DUMP_REGION_RESERVE;
		active_region.size = resv->length;
		len = __print_rsv_region(buf, buf_len, len, &active_region,
					 for_sysfs_show);
	}
	return len;
}

static ssize_t sun55i_iommu_dump_pgtable(struct sunxi_iommu_dev *iommu, char *buf, size_t buf_len,
                       bool for_sysfs_show)
{
	struct sunxi_iommu_domain *sunxi_domain = iommu->debug_domain;
	ssize_t len = 0;

	len = sunxi_iommu_dump_rsv_list(&iommu->rsv_list, len, buf,
                      buf_len, for_sysfs_show);

	if (sunxi_domain && sunxi_domain->pgtable) {
		len = sunxi_pgtable_dump(sunxi_domain->pgtable, len, buf, buf_len,
                       for_sysfs_show);
	} else {
		if (for_sysfs_show) {
			len += sysfs_emit_at(buf, len, "no active domain to dump\n");
		} else {
			len += scnprintf(buf + len, buf_len - len, "no active domain to dump\n");
		}
	}

	return len;
}

static ssize_t sun55i_iommu_map_show(struct device *dev,
                       struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu_dev *iommu = dev_get_drvdata(dev);

	if (!iommu)
		return -ENODEV;

	return sun55i_iommu_dump_pgtable(iommu, buf, PAGE_SIZE, true);
}

static struct device_attribute sunxi_iommu_enable_attr =
	__ATTR(enable, 0644, sunxi_iommu_enable_show,
	sunxi_iommu_enable_store);
static struct device_attribute sunxi_iommu_profilling_attr =
	__ATTR(profilling, 0444, sunxi_iommu_profilling_show, NULL);
static struct device_attribute sun55i_iommu_map_attr =
	__ATTR(page_debug, 0444, sun55i_iommu_map_show, NULL);

static void sun55i_iommu_sysfs_create(struct platform_device *_pdev,
				struct sunxi_iommu_dev *sunxi_iommu)
{
	device_create_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_create_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
	device_create_file(&_pdev->dev, &sun55i_iommu_map_attr);
}

static void sun55i_iommu_sysfs_remove(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
	device_remove_file(&_pdev->dev, &sun55i_iommu_map_attr);
}

int sunxi_iommu_check_cmd(struct device *dev, void *data)
{
	struct iommu_resv_region *region;
	int prot = IOMMU_WRITE | IOMMU_READ;
	struct list_head *rsv_list = data;
	struct {
		const char *name;
		u32 region_type;
	} supported_region[2] = { { "sunxi-iova-reserve", IOMMU_RESV_RESERVED },
				  { "sunxi-iova-premap", IOMMU_RESV_DIRECT } };
	int i, j;
#define REGION_CNT_MAX (8)
	struct {
		u64 array[REGION_CNT_MAX * 2];
		int count;
	} *tmp_data;

	tmp_data = kzalloc(sizeof(*tmp_data), GFP_KERNEL);
	if (!tmp_data)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(supported_region); i++) {
		/* search all supported argument */
		if (!of_find_property(dev->of_node, supported_region[i].name,
				      NULL))
			continue;

		tmp_data->count = of_property_read_variable_u64_array(
			dev->of_node, supported_region[i].name, tmp_data->array,
			0, REGION_CNT_MAX);
		if (tmp_data->count <= 0)
			continue;
		if ((tmp_data->count & 1) != 0) {
			dev_err(dev, "size %d of array %s should be even\n",
				tmp_data->count, supported_region[i].name);
			continue;
		}

		/* two u64 describe one region */
		tmp_data->count /= 2;

		/* prepared reserve region data */
		for (j = 0; j < tmp_data->count; j++) {
			region = iommu_alloc_resv_region(
				tmp_data->array[j * 2],
				tmp_data->array[j * 2 + 1], prot,
				supported_region[i].region_type,
				GFP_KERNEL);
			if (!region) {
				dev_err(dev, "no memory for iova rsv region");
			} else {
				struct iommu_resv_region *walk;
				/* warn on region overlaps */
				list_for_each_entry(walk, rsv_list, list) {
					phys_addr_t walk_end =
						walk->start + walk->length;
					phys_addr_t region_end =
						region->start + region->length;
					if (!(walk->start >
						      region->start +
							      region->length ||
					      walk->start + walk->length <
						      region->start)) {
						dev_warn(
							dev,
							"overlap on iova-reserve %pap~%pap with %pap~%pap",
							&walk->start, &walk_end,
							&region->start,
							&region_end);
					}
				}
				list_add_tail(&region->list, rsv_list);
			}
		}
	}
	kfree(tmp_data);
#undef REGION_CNT_MAX

	return 0;
}

static int __init_reserve_mem(struct sunxi_iommu_dev *dev)
{
	return bus_for_each_dev(&platform_bus_type, NULL, &dev->rsv_list,
			sunxi_iommu_check_cmd);
}

static const struct iommu_ops sunxi_iommu_ops = {
	.domain_alloc_paging	= sun55i_iommu_domain_alloc_paging,
	.probe_device	    = sun55i_iommu_probe_device,
	.probe_finalize = sun55i_iommu_probe_device_finalize,
	.release_device	    = sun55i_iommu_release_device,
	.device_group	    = generic_device_group,
	.of_xlate	    = sun55i_iommu_of_xlate,
	.owner		    = THIS_MODULE,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev		= sun55i_iommu_attach_dev,
		.map_pages		= sun55i_iommu_map,
		.unmap_pages	= sun55i_iommu_unmap,
		.iotlb_sync_map = sun55i_iommu_iotlb_sync_map,
		.iova_to_phys	= sun55i_iommu_iova_to_phys,
		.free			= sun55i_iommu_domain_free,
	}
};

static int sun55i_iommu_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct device *dev = &pdev->dev;
	struct sunxi_iommu_dev *sunxi_iommu;
	struct resource *res;

	iopte_cache = sunxi_pgtable_alloc_pte_cache();
	if (!iopte_cache) {
		pr_err("%s: Failed to create sunx-iopte-cache.\n", __func__);
		return -ENOMEM;
	}

	sunxi_iommu = devm_kzalloc(dev, sizeof(*sunxi_iommu), GFP_KERNEL);
	if (!sunxi_iommu) {
		kmem_cache_destroy(iopte_cache);
		iopte_cache = NULL;
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_dbg(dev, "Unable to find resource region\n");
		ret = -ENOENT;
		goto err_res;
	}

	sunxi_iommu->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sunxi_iommu->base)) {
		dev_dbg(dev, "Unable to map IOMEM @ PA:%pa\n", &res->start);
		ret = PTR_ERR(sunxi_iommu->base);
		goto err_res;
	}

	sunxi_iommu->bypass = DEFAULT_BYPASS_VALUE;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_dbg(dev, "Unable to find IRQ resource\n");
		ret = -ENOENT;
		goto err_res;
	}
	pr_info("sunxi iommu: irq = %d\n", irq);

	ret = devm_request_irq(dev, irq, sunxi_iommu_irq, 0,
			dev_name(dev), (void *)sunxi_iommu);
	if (ret < 0) {
		dev_dbg(dev, "Unabled to register interrupt handler\n");
		goto err_res;
	}

	sunxi_iommu->irq = irq;

	sunxi_iommu->clk = devm_clk_get(dev, "iommu");
	if (IS_ERR(sunxi_iommu->clk)) {
		ret = PTR_ERR(sunxi_iommu->clk);
		goto err_res;
	}

	ret = clk_prepare_enable(sunxi_iommu->clk);
	if (ret)
		goto err_res;

	platform_set_drvdata(pdev, sunxi_iommu);
	sunxi_iommu->dev = dev;
	spin_lock_init(&sunxi_iommu->iommu_lock);
	global_iommu_dev = sunxi_iommu;
	sunxi_iommu->plat_data = of_device_get_match_data(dev);

	if (sunxi_iommu->plat_data->version !=
			sunxi_iommu_read(sunxi_iommu, IOMMU_VERSION_REG)) {
		dev_err(dev, "iommu version mismatch, please check and reconfigure\n");

		ret = -ENODEV;
		goto err_clk;
	}

	sun55i_iommu_sysfs_create(pdev, sunxi_iommu);
	ret = iommu_device_sysfs_add(&sunxi_iommu->iommu, dev, NULL,
				     dev_name(dev));
	if (ret) {
		dev_err(dev, "Failed to register iommu in sysfs\n");
		goto err_clk;
	}

	ret = iommu_device_register(&sunxi_iommu->iommu, &sunxi_iommu_ops, dev);
	if (ret) {
		dev_err(dev, "Failed to register iommu\n");
		goto err_sysfs_remove;
	}

	INIT_LIST_HEAD(&sunxi_iommu->rsv_list);
	__init_reserve_mem(sunxi_iommu);

	sunxi_iommu->identity_domain = sun55i_iommu_domain_alloc_paging(&pdev->dev);
	if (!sunxi_iommu->identity_domain) {
		dev_err(dev, "Failed to allocate identity domain\n");
		ret = -ENOMEM;
		goto err_iommu_unregister;
	}

	if (!list_empty(&sunxi_iommu->rsv_list)) {
		struct iommu_resv_region *entry;

		dev_info(dev, "Mapping %zu reserved regions for identity domain\n",
			 list_count_nodes(&sunxi_iommu->rsv_list));

		list_for_each_entry(entry, &sunxi_iommu->rsv_list, list) {
			size_t size = entry->length;
			phys_addr_t phys = entry->start;

			if (sun55i_iommu_map(sunxi_iommu->identity_domain, phys, phys, size, 1, entry->prot, GFP_KERNEL, NULL)) {
				dev_err(dev, "Failed to map reserved region %pa [%zx]\n",
					&phys, size);
			}
		}
	}

	if (!dma_dev) {
		dma_dev = &pdev->dev;
		sunxi_pgtable_set_dma_dev(dma_dev);
	}

	return 0;

err_iommu_unregister:
	iommu_device_unregister(&sunxi_iommu->iommu);
err_sysfs_remove:
	iommu_device_sysfs_remove(&sunxi_iommu->iommu);
err_clk:
	global_iommu_dev = NULL;
	clk_disable_unprepare(sunxi_iommu->clk);
err_res:
	sunxi_pgtable_free_pte_cache(iopte_cache);

	return dev_err_probe(dev, ret, "Failed to initialize\n");
}

static void sun55i_iommu_remove(struct platform_device *pdev)
{
	struct sunxi_iommu_dev *sunxi_iommu = platform_get_drvdata(pdev);
	struct iommu_resv_region *entry, *next;

	sunxi_pgtable_free_pte_cache(iopte_cache);
	if (!list_empty(&sunxi_iommu->rsv_list)) {
		list_for_each_entry_safe (entry, next, &sunxi_iommu->rsv_list,
					  list)
			kfree(entry);
	}
	devm_free_irq(sunxi_iommu->dev, sunxi_iommu->irq, sunxi_iommu);
	devm_iounmap(sunxi_iommu->dev, sunxi_iommu->base);
	sun55i_iommu_sysfs_remove(pdev);
	iommu_device_sysfs_remove(&sunxi_iommu->iommu);
	iommu_device_unregister(&sunxi_iommu->iommu);
	clk_disable_unprepare(sunxi_iommu->clk);
	global_iommu_dev = NULL;

	return;
}

static int sun55i_iommu_suspend(struct device *dev)
{
	clk_disable_unprepare(global_iommu_dev->clk);

	return 0;
}

static int sun55i_iommu_resume(struct device *dev)
{
	struct sunxi_iommu_dev *iommu = dev_get_drvdata(dev);
	int ret;

	ret = clk_prepare_enable(iommu->clk);
	if (ret)
		return ret;

	ret = sun55i_iommu_hw_init(iommu, NULL);
	if (ret)
		clk_disable_unprepare(iommu->clk);

	return ret;
}

static const struct dev_pm_ops sunxi_iommu_pm_ops = {
	.suspend	= sun55i_iommu_suspend,
	.resume		= sun55i_iommu_resume,
};

static const struct sunxi_iommu_plat_data iommu_v15_sun55iw3_data = {
	.version = 0x15,
	/* disable preftech to test display rcq bug */
	.tlb_prefetch = 0x30000,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"ISP", "CSI", "VE0", "VE1", "G2D", "DE",
			"DI", "DEBUG_MODE"},
};

static const struct of_device_id sunxi_iommu_dt_ids[] = {
	{ .compatible = "allwinner,sun55i-a523-iommu", .data = &iommu_v15_sun55iw3_data},
	{ /* sentinel */ },
};

static struct platform_driver sunxi_iommu_driver = {
	.probe		= sun55i_iommu_probe,
	.remove		= sun55i_iommu_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "sunxi-iommu",
		.pm		= &sunxi_iommu_pm_ops,
		.of_match_table = sunxi_iommu_dt_ids,
	}
};

static int __init sunxi_iommu_init(void)
{
	return platform_driver_register(&sunxi_iommu_driver);
}

static void __exit sunxi_iommu_exit(void)
{
	return platform_driver_unregister(&sunxi_iommu_driver);
}

subsys_initcall(sunxi_iommu_init);
module_exit(sunxi_iommu_exit);
