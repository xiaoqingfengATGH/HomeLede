/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's pgtable controler
 *
 * Copyright (c) 2023, ouyangkun <ouyangkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef __SUNXI_IOMMU_PGTALBE__
#define __SUNXI_IOMMU_PGTALBE__
#include <linux/iommu.h>

#define SUNXI_PHYS_OFFSET 0x40000000UL

#define IOMMU_VA_BITS 32

#define IOMMU_PD_SHIFT 20
#define IOMMU_PD_MASK (~((1UL << IOMMU_PD_SHIFT) - 1))

#define IOMMU_PT_SHIFT 12
#define IOMMU_PT_MASK (~((1UL << IOMMU_PT_SHIFT) - 1))

#define SPAGE_SIZE (1 << IOMMU_PT_SHIFT)
#define SPD_SIZE (1 << IOMMU_PD_SHIFT)
#define SPAGE_ALIGN(addr) ALIGN(addr, SPAGE_SIZE)
#define SPDE_ALIGN(addr) ALIGN(addr, SPD_SIZE)

/*
 * This version Hardware just only support 4KB page. It have
 * a two level page table structure, where the first level has
 * 4096 entries, and the second level has 256 entries. And, the
 * first level is "Page Directory(PG)", every entry include a
 * Page Table base address and a few of control bits. Second
 * level is "Page Table(PT)", every entry include a physical
 * page address and a few of control bits. Each entry is one
 * 32-bit word. Most of the bits in the second level entry are
 * used by hardware.
 *
 * Virtual Address Format:
 *     31              20|19        12|11     0
 *     +-----------------+------------+--------+
 *     |    PDE Index    |  PTE Index | offset |
 *     +-----------------+------------+--------+
 *
 * Table Layout:
 *
 *      First Level         Second Level
 *   (Page Directory)       (Page Table)
 *   ----+---------+0
 *    ^  |  PDE   |   ---> -+--------+----
 *    |  ----------+1       |  PTE   |  ^
 *    |  |        |         +--------+  |
 *       ----------+2       |        |  1K
 *   16K |        |         +--------+  |
 *       ----------+3       |        |  v
 *    |  |        |         +--------+----
 *    |  ----------
 *    |  |        |
 *    v  |        |
 *   ----+--------+
 *
 * IOPDE:
 * 31                     10|9       0
 * +------------------------+--------+
 * |   PTE Base Address     |CTRL BIT|
 * +------------------------+--------+
 *
 * IOPTE:
 * 31                  12|11         0
 * +---------------------+-----------+
 * |  Phy Page Address   |  CTRL BIT |
 * +---------------------+-----------+
 *
 * cpu phy 0x0000 0000 ~ 0x4000 0000 is reserved for IO access,
 * iommu phy in between 0x0000 0000 ~ 0x4000 0000 should not used
 * as cpu phy directly, move this address space beyond iommu
 * phy max, so iommu phys 0x0000 0000 ~ 0x4000 0000 shoule be
 * iommu_phy_max + 0x0000 0000 ~ iommu_phy_max + 0x4000 0000(as
 * spec said)
 */

static inline dma_addr_t iommu_phy_to_cpu_phy(dma_addr_t iommu_phy)
{
	return iommu_phy < SUNXI_PHYS_OFFSET ?
		       iommu_phy + (1ULL << IOMMU_VA_BITS) :
		       iommu_phy;
}

static inline dma_addr_t cpu_phy_to_iommu_phy(dma_addr_t cpu_phy)
{
	return cpu_phy > (1ULL << IOMMU_VA_BITS) ?
		       cpu_phy - (1ULL << IOMMU_VA_BITS) :
		       cpu_phy;
}

int sunxi_pgtable_prepare_l1_tables(unsigned int *pgtable,
				    dma_addr_t iova_start, dma_addr_t iova_end,
				    int prot);
int sunxi_pgtable_prepare_l2_tables(unsigned int *pgtable,
				    dma_addr_t iova_start, dma_addr_t iova_end,
				    phys_addr_t paddr, int prot);
int sunxi_pgtable_delete_l2_tables(unsigned int *pgtable, dma_addr_t iova_start,
				   dma_addr_t iova_end);
phys_addr_t sunxi_pgtable_iova_to_phys(unsigned int *pgtable, dma_addr_t iova);
int sunxi_pgtable_invalid_helper(unsigned int *pgtable, dma_addr_t iova);
void sunxi_pgtable_clear(unsigned int *pgtable);
unsigned int *sunxi_pgtable_alloc(void);
void sunxi_pgtable_free(unsigned int *pgtable);
ssize_t sunxi_pgtable_dump(unsigned int *pgtable, ssize_t len, char *buf,
			   size_t buf_len, bool for_sysfs_show);
struct kmem_cache *sunxi_pgtable_alloc_pte_cache(void);
void sunxi_pgtable_free_pte_cache(struct kmem_cache *iopte_cache);
void sunxi_pgtable_set_dma_dev(struct device *dma_dev);

#endif
