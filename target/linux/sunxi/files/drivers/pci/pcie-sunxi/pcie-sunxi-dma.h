/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * allwinner PCIe dma driver
 *
 * Copyright (C) 2022 allwinner Co., Ltd.
 *
 * Author: songjundong <songjundong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _PCIE_SUNXI_DMA_H
#define _PCIE_SUNXI_DMA_H

#include <linux/debugfs.h>
#include <linux/platform_device.h>

#include "pcie-sunxi.h"

#define PCIE_DMA_TABLE_NUM		8
#define PCIE_DMA_TRX_TYPE_NUM		3

#define PCIE_WEIGHT			0x1f
/*
 * MASK_DONE_CNT_xx and MASK_ABORT_CNT_xx used in dma interrupt
 */
#define MASK_DONE_CNT_WR   ((2 << (PCIE_DMA_WR_CHN_CNT - 1)) - 1)
#define MASK_DONE_CNT_RD   ((2 << (PCIE_DMA_RD_CHN_CNT - 1)) - 1)

#define MASK_ABORD_CNT_WR  (((2 << (PCIE_DMA_WR_CHN_CNT - 1)) - 1))
#define MASK_ABORD_CNT_RD  (((2 << (PCIE_DMA_RD_CHN_CNT - 1)) - 1))

#define PCIE_DMA_OFFSET			0x380000

#define PCIE_DMA_WR_ENB			0xc
#define PCIE_DMA_WR_CTRL_LO		0x200
#define PCIE_DMA_WR_CTRL_HI		0x204
#define PCIE_DMA_WR_XFERSIZE		0x208
#define PCIE_DMA_WR_SAR_LO		0x20c
#define PCIE_DMA_WR_SAR_HI		0x210
#define PCIE_DMA_WR_DAR_LO		0x214
#define PCIE_DMA_WR_DAR_HI		0x218
#define PCIE_DMA_WR_WEILO		0x18
#define PCIE_DMA_WR_WEIHI		0x1c
#define PCIE_DMA_WR_DOORBELL		0x10
#define PCIE_DMA_WR_INT_STATUS		0x4c
#define PCIE_DMA_WR_INT_MASK		0x54
#define PCIE_DMA_WR_INT_CLEAR		0x58

#define PCIE_DMA_RD_ENB			0x2c
#define PCIE_DMA_RD_CTRL_LO		0x300
#define PCIE_DMA_RD_CTRL_HI		0x304
#define PCIE_DMA_RD_XFERSIZE		0x308
#define PCIE_DMA_RD_SAR_LO		0x30c
#define PCIE_DMA_RD_SAR_HI		0x310
#define PCIE_DMA_RD_DAR_LO		0x314
#define PCIE_DMA_RD_DAR_HI		0x318
#define PCIE_DMA_RD_WEILO		0x38
#define PCIE_DMA_RD_WEIHI		0x3c
#define PCIE_DMA_RD_DOORBELL		0x30
#define PCIE_DMA_RD_INT_STATUS		0xa0
#define PCIE_DMA_RD_INT_MASK		0xa8
#define PCIE_DMA_RD_INT_CLEAR		0xac

#define PCIE_DMA_INT_MASK		0xf000f

enum dma_dir {
	PCIE_DMA_WRITE = 0,
	PCIE_DMA_READ,
};

typedef void (*sunxi_pcie_edma_callback)(void *param);

typedef struct sunxi_pci_edma_chan {
	u32		chnl_num;
	spinlock_t	lock;
	bool		cookie;
	phys_addr_t	src_addr;
	phys_addr_t	dst_addr;
	u32		size;
	enum dma_dir	dma_trx;
	void		*callback_param;
	sunxi_pcie_edma_callback callback;
} sunxi_pci_edma_chan_t;

/*
 * The Channel Control Register for read and write.
 */
union chan_ctrl_lo {
	struct {
		u32	cb		:1;	/* 0 bit */
		u32	tcb		:1;	/* 1	 */
		u32	llp		:1;	/* 2	 */
		u32	lie		:1;	/* 3	 */
		u32	rie		:1;	/* 4	 */
		u32	cs		:2;	/* 5:6   */
		u32	rsvd1		:1;	/* 7	 */
		u32	ccs		:1;	/* 8	 */
		u32	llen		:1;	/* 9	 */
		u32	b_64s		:1;	/* 10	 */
		u32	b_64d		:1;	/* 11	 */
		u32	fn		:5;	/* 12:16 */
		u32	rsvd2		:7;	/* 17:23 */
		u32	ns		:1;	/* 24	 */
		u32	ro		:1;	/* 25	 */
		u32	td		:1;	/* 26	 */
		u32	tc		:3;	/* 27:29 */
		u32	at		:2;	/* 30:31 */
	};
	u32 dword;
};

/*
 * The Channel Control Register high part for read and write.
 * Note: depend on CX_SRIOV_ENABLE
 * Note: Need to confirm the difference between PCIe 2.0 with 3.0
 */
union chan_ctrl_hi {
	struct {
		u32	vfenb		:1;	/* 0 bit */
		u32	vfunc		:8;	/* 1-8	 */
		u32	rsvd0		:23;	/* 9-31  */
	};
	u32 dword;
};

struct ctx_reg {
	union chan_ctrl_lo		ctrllo;
	union chan_ctrl_hi		ctrlhi;
	u32				xfersize;
	u32				sarptrlo;
	u32				sarptrhi;
	u32				darptrlo;
	u32				darptrhi;
};

/*
 * The Channel Weight Register for read and write.
 *
 * weight_lo->weight0 means set channel 0
 * weight_hi->weight0 means set channel 4;
 *
 * Example:
 * write channel #0 weight to 32
 * write channel #1 weight to 16
 *
 * Then the DMA will issue 32 MRd requests for #0,followed by 16 MRd requests for #1,
 * followed by the 32 MRd requests for #0 and so on...
 */
union weight {
	struct {
		u32	weight0		:5;	/* 0:4 bit */
		u32	weight1		:5;	/* 5:9	   */
		u32	weight2		:5;	/* 10:14   */
		u32	weight3		:5;	/* 15:19   */
		u32	rsvd		:12;	/* 20:31   */
	};
	u32 dword;
};

/*
 * The Doorbell Register for read and write.
 * if is read  db: you need write 0x0 for that channel
 * if is write db: you need write channel number for that channel.
 */
union db {
	struct {
		u32	chnl		:3;	/* 0 bit */
		u32	rsvd		:28;	/* 3:30  */
		u32	stop		:1;	/* 31    */
	};
	u32 dword;
};

/*
 * The Enable VIEWPORT Register for read and write.
 */
union enb {
	struct {
		u32	enb		:1;	/* 0 bit */
		u32	rsvd		:31;	/* 1:31  */
	};
	u32 dword;
};

/*
 * The Interrupt Status Register for read and write.
 */
union int_status {
	struct {
		u32	done		:8;	/* 0:7 bit */
		u32	rsvd0		:8;	/* 8:15    */
		u32	abort		:8;	/* 16:23   */
		u32	rsvd1		:8;	/* 24:31   */
	};
	u32 dword;
};

/*
 * The Interrupt Status Register for read and write.
 */
union int_clear {
	struct {
		u32	doneclr		:8;	/* 0:7 bit */
		u32	rsvd0		:8;	/* 8:15    */
		u32	abortclr	:8;	/* 16:23   */
		u32	rsvd1		:8;	/* 24:31   */
	};
	u32 dword;
};

/*
 * The Context Registers for read and write.
 */
struct ctx_regs {
	union chan_ctrl_lo		ctrllo;
	union chan_ctrl_hi		ctrlhi;
	u32				xfersize;
	u32				sarptrlo;
	u32				sarptrhi;
	u32				darptrlo;
	u32				darptrhi;
};

struct dma_table {
	u32				*descs;
	int				chn;
	phys_addr_t			phys_descs;
	enum dma_dir			dir;
	u32				type;
	struct list_head		dma_tbl;
	union enb			enb;
	struct ctx_regs			ctx_reg;
	union weight			weilo;
	union weight			weihi;
	union db			start;
	phys_addr_t			local;
	phys_addr_t			bus;
	size_t				size;
};

struct dma_trx_obj {
	struct device			*dev;
	void				*mem_base;
	phys_addr_t			mem_start;
	size_t				mem_size;
	int				dma_free;
	spinlock_t			dma_list_lock; /* lock dma table */
	struct list_head		dma_list;
	struct work_struct		dma_trx_work;
	wait_queue_head_t		event_queue;
	struct workqueue_struct		*dma_trx_wq;
	struct dma_table		*table[PCIE_DMA_TABLE_NUM];
	struct task_struct		*scan_thread;
	struct hrtimer			scan_timer;
	void				*priv;
	struct completion		done;
	int				ref_count;
	struct mutex			count_mutex;
	unsigned long			irq_num;
	struct dentry			*pcie_root;
	struct pcie_misc_dev		*pcie_dev;
	void				(*start_dma_trx_func)(struct dma_table *table, struct dma_trx_obj *obj);
	int				(*config_dma_trx_func)(struct dma_table *table, phys_addr_t sar_addr, phys_addr_t dar_addr,
							unsigned int size, enum dma_dir dma_trx, sunxi_pci_edma_chan_t *edma_chn);
};

struct dma_trx_obj *sunxi_pcie_dma_obj_probe(struct device *dev);
int sunxi_pcie_dma_obj_remove(struct device *dev);
sunxi_pci_edma_chan_t *sunxi_pcie_dma_chan_request(enum dma_dir dma_trx, void *cb, void *data);
int sunxi_pcie_dma_chan_release(struct sunxi_pci_edma_chan *edma_chan, enum dma_dir dma_trx);

int sunxi_pcie_dma_get_chan(struct platform_device *pdev);

#endif
