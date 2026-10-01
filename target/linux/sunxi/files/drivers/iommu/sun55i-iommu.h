/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi iommu: main structures
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Written by Hiroshi DOYU <Hiroshi.DOYU@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include "sun55i-iommu-pgtable.h"

//iommu domain have seperate ops
#define SEPERATE_DOMAIN_API
//dma-iommu is enclosed into iommu-core
#define DMA_IOMMU_IN_IOMMU
//not used anywhere since refactoring
#define GROUP_NOTIFIER_DEPRECATED
//iommu now have correct probe order
//no more need bus set op as workaround
#define BUS_SET_OP_DEPRECATED
//dma cookie handled by iommu core, not driver
#define COOKIE_HANDLE_BY_CORE
//iommu resv region allocation require gfp flags
#define RESV_REGION_NEED_GFP_FLAG

#ifdef DMA_IOMMU_IN_IOMMU
#include <linux/iommu.h>
/*
 * by design iommu driver should be part of iommu
 * and get to it by ../../dma-iommu.h
 * sunxi bsp have seperate root, use different path
 * to reach dma-iommu.h
 */
#include <../drivers/iommu/dma-iommu.h>
#else
#include <linux/dma-iommu.h>
#endif

#define MAX_SG_SIZE (128 << 20)
#define MAX_SG_TABLE_SIZE ((MAX_SG_SIZE / SPAGE_SIZE) * sizeof(u32))
#define DUMP_REGION_MAP 0
#define DUMP_REGION_RESERVE 1
struct dump_region {
	u32 access_mask;
	size_t size;
	u32 type;
	dma_addr_t phys, iova;
};
struct sunxi_iommu_dev;
void sun55i_reset_device_iommu(unsigned int master_id);
void sun55i_enable_device_iommu(struct sunxi_iommu_dev *iommu, unsigned int master_id, bool flag);
