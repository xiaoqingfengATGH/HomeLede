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

#include <linux/iommu.h>
#include <linux/slab.h>
#include "sun55i-iommu.h"
#include <sunxi-iommu.h>

#define NUM_ENTRIES_PDE (1UL << (IOMMU_VA_BITS - IOMMU_PD_SHIFT))
#define NUM_ENTRIES_PTE (1UL << (IOMMU_PD_SHIFT - IOMMU_PT_SHIFT))
#define PD_SIZE (NUM_ENTRIES_PDE * sizeof(u32))
#define PT_SIZE (NUM_ENTRIES_PTE * sizeof(u32))

#define PAGE_OFFSET_MASK ((1UL << IOMMU_PT_SHIFT) - 1)
#define IOPTE_BASE_MASK (~(PT_SIZE - 1))

/*
 * Page Directory Entry Control Bits
 */
#define DENT_VALID 0x01
#define DENT_PTE_SHFIT 10
#define DENT_WRITABLE BIT(3)
#define DENT_READABLE BIT(2)

/*
 * Page Table Entry Control Bits
 */
#define SUNXI_PTE_PAGE_WRITABLE BIT(3)
#define SUNXI_PTE_PAGE_READABLE BIT(2)
#define SUNXI_PTE_PAGE_VALID BIT(1)

#define IS_VALID(x) (((x)&0x03) == DENT_VALID)

#define IOPDE_INDEX(va) (((va) >> IOMMU_PD_SHIFT) & (NUM_ENTRIES_PDE - 1))
#define IOPTE_INDEX(va) (((va) >> IOMMU_PT_SHIFT) & (NUM_ENTRIES_PTE - 1))

#define IOPTE_BASE(ent) ((ent)&IOPTE_BASE_MASK)

#define IOPTE_TO_PFN(ent) ((*ent) & IOMMU_PT_MASK)
#define IOVA_PAGE_OFT(va) ((va)&PAGE_OFFSET_MASK)

/* IO virtual address start page frame number */
#define IOVA_START_PFN (1)
#define IOVA_PFN(addr) ((addr) >> PAGE_SHIFT)

/* TLB Invalid ALIGN */
#define IOVA_4M_ALIGN(iova) ((iova) & (~0x3fffff))

struct sunxi_pgtable_t {
	unsigned int *pgtable;
	struct kmem_cache *iopte_cache;
	struct device *dma_dev;
} sunxi_pgtable_params;

/* pointer to l1 table entry */
static inline u32 *iopde_offset(u32 *iopd, dma_addr_t iova)
{
	return iopd + IOPDE_INDEX(iova);
}

/* pointer to l2 table entry */
static inline u32 *iopte_offset(u32 *ent, dma_addr_t iova)
{
	u64 iopte_base = 0;

	iopte_base = IOPTE_BASE(*ent);
	iopte_base = iommu_phy_to_cpu_phy(iopte_base);

	return (u32 *)__va(iopte_base) + IOPTE_INDEX(iova);
}

static int sunxi_alloc_iopte(u32 *sent, int prot)
{
	u32 *pent;
	u32 flags = 0;

	flags |= (prot & IOMMU_READ) ? DENT_READABLE : 0;
	flags |= (prot & IOMMU_WRITE) ? DENT_WRITABLE : 0;

	pent = kmem_cache_zalloc(sunxi_pgtable_params.iopte_cache, GFP_ATOMIC);
	WARN_ON((unsigned long)pent & (PT_SIZE - 1));
	if (!pent) {
		pr_err("%s, %d, kmalloc failed!\n", __func__, __LINE__);
		return 0;
	}
	dma_sync_single_for_cpu(sunxi_pgtable_params.dma_dev,
				virt_to_phys(sent), sizeof(*sent),
				DMA_TO_DEVICE);
	*sent = cpu_phy_to_iommu_phy(__pa(pent)) | DENT_VALID;
	dma_sync_single_for_device(sunxi_pgtable_params.dma_dev,
				   virt_to_phys(sent), sizeof(*sent),
				   DMA_TO_DEVICE);

	return 1;
}

static void sunxi_free_iopte(u32 *pent)
{
	kmem_cache_free(sunxi_pgtable_params.iopte_cache, pent);
}

static inline u32 sunxi_mk_pte(phys_addr_t page, int prot)
{
	u32 flags = 0;
	u32 high_addr = 0;

	flags |= (prot & IOMMU_READ) ? SUNXI_PTE_PAGE_READABLE : 0;
	flags |= (prot & IOMMU_WRITE) ? SUNXI_PTE_PAGE_WRITABLE : 0;
	page &= IOMMU_PT_MASK;

	return page | high_addr | flags | SUNXI_PTE_PAGE_VALID;
}

int sunxi_pgtable_prepare_l1_tables(unsigned int *pgtable,
				    dma_addr_t iova_start, dma_addr_t iova_end,
				    int prot)
{
	u32 *dent;
	for (; iova_start <= iova_end; iova_start += SPD_SIZE) {
		dent = iopde_offset(pgtable, iova_start);
		if (!IS_VALID(*dent) && !sunxi_alloc_iopte(dent, prot)) {
			return -ENOMEM;
		}
	}
	return 0;
}

int sunxi_pgtable_prepare_l2_tables(unsigned int *pgtable,
				    dma_addr_t iova_start, dma_addr_t iova_end,
				    phys_addr_t paddr, int prot)
{
	size_t paddr_start;
	u32 *dent, *pent;
	u32 iova_tail_count, iova_tail_size;
	u32 pent_val;
	int i;
	paddr = cpu_phy_to_iommu_phy(paddr);
	paddr_start = paddr & IOMMU_PT_MASK;
	for (; iova_start < iova_end;) {
		iova_tail_count = NUM_ENTRIES_PTE - IOPTE_INDEX(iova_start);
		iova_tail_size = iova_tail_count * SPAGE_SIZE;
		if (iova_start + iova_tail_size > iova_end) {
			iova_tail_size = iova_end - iova_start;
			iova_tail_count = iova_tail_size / SPAGE_SIZE;
		}

		dent = iopde_offset(pgtable, iova_start);
		pent = iopte_offset(dent, iova_start);
		pent_val = sunxi_mk_pte(paddr_start, prot);
		for (i = 0; i < iova_tail_count; i++) {
			WARN_ON(*pent);
			*pent = pent_val + SPAGE_SIZE * i;
			pent++;
		}

		dma_sync_single_for_device(
			sunxi_pgtable_params.dma_dev,
			virt_to_phys(iopte_offset(dent, iova_start)),
			iova_tail_count << 2, DMA_TO_DEVICE);
		iova_start += iova_tail_size;
		paddr_start += iova_tail_size;
	}
	return 0;
}

int sunxi_pgtable_delete_l2_tables(unsigned int *pgtable, dma_addr_t iova_start,
				   dma_addr_t iova_end)
{
	u32 *dent, *pent;
	u32 iova_tail_count, iova_tail_size;
	iova_tail_count = NUM_ENTRIES_PTE - IOPTE_INDEX(iova_start);
	iova_tail_size = iova_tail_count * SPAGE_SIZE;
	if (iova_start + iova_tail_size > iova_end) {
		iova_tail_size = iova_end - iova_start;
		iova_tail_count = iova_tail_size / SPAGE_SIZE;
	}

	dent = iopde_offset(pgtable, iova_start);
	if (!IS_VALID(*dent))
		return -EINVAL;
	pent = iopte_offset(dent, iova_start);
	memset(pent, 0, iova_tail_count * sizeof(u32));
	dma_sync_single_for_device(sunxi_pgtable_params.dma_dev,
				   virt_to_phys(iopte_offset(dent, iova_start)),
				   iova_tail_count << 2, DMA_TO_DEVICE);

	if (iova_tail_size == SPD_SIZE) {
		*dent = 0;
		dma_sync_single_for_device(sunxi_pgtable_params.dma_dev,
					   virt_to_phys(dent), sizeof(*dent),
					   DMA_TO_DEVICE);
		sunxi_free_iopte(pent);
	}
	return iova_tail_size;
}

phys_addr_t sunxi_pgtable_iova_to_phys(unsigned int *pgtable, dma_addr_t iova)
{
	u32 *dent, *pent;
	phys_addr_t ret = 0;
	dent = iopde_offset(pgtable, iova);
	if (IS_VALID(*dent)) {
		pent = iopte_offset(dent, iova);
		if (*pent) {
			ret = IOPTE_TO_PFN(pent) + IOVA_PAGE_OFT(iova);
			ret = iommu_phy_to_cpu_phy(ret);
		}
	}
	return ret;
}

int sunxi_pgtable_invalid_helper(unsigned int *pgtable, dma_addr_t iova)
{
	u32 *pte_addr, *dte_addr;

	dte_addr = iopde_offset(pgtable, iova);
	if ((*dte_addr & 0x3) != 0x1) {
		pr_err("0x%pad is not mapped!\n", &iova);
		return 1;
	}
	pte_addr = iopte_offset(dte_addr, iova);
	if ((*pte_addr & 0x2) == 0) {
		pr_err("0x%pad is not mapped!\n", &iova);
		return 1;
	}
	pr_err("0x%pad is mapped!\n", &iova);

	return 0;
}

void sunxi_pgtable_clear(unsigned int *pgtable)
{
	int i = 0;
	u32 *dent, *pent;
	size_t iova;

	for (i = 0; i < NUM_ENTRIES_PDE; ++i) {
		dent = pgtable + i;
		iova = (unsigned long)i << IOMMU_PD_SHIFT;
		if (IS_VALID(*dent)) {
			pent = iopte_offset(dent, iova);
			dma_sync_single_for_cpu(sunxi_pgtable_params.dma_dev,
						virt_to_phys(pent), PT_SIZE,
						DMA_TO_DEVICE);
			memset(pent, 0, PT_SIZE);
			dma_sync_single_for_device(sunxi_pgtable_params.dma_dev,
						   virt_to_phys(pent), PT_SIZE,
						   DMA_TO_DEVICE);
			dma_sync_single_for_cpu(sunxi_pgtable_params.dma_dev,
						virt_to_phys(dent), PT_SIZE,
						DMA_TO_DEVICE);
			*dent = 0;
			dma_sync_single_for_device(sunxi_pgtable_params.dma_dev,
						   virt_to_phys(dent),
						   sizeof(*dent),
						   DMA_TO_DEVICE);
			sunxi_free_iopte(pent);
		}
	}
}

unsigned int *sunxi_pgtable_alloc(void)
{
	unsigned int *pgtable;
	pgtable = (unsigned int *)__get_free_pages(GFP_KERNEL,
						   get_order(PD_SIZE));

	if (pgtable != NULL) {
		memset(pgtable, 0, PD_SIZE);
	}
	sunxi_pgtable_params.pgtable = pgtable;
	return pgtable;
}

void sunxi_pgtable_free(unsigned int *pgtable)
{
	free_pages((unsigned long)pgtable, get_order(PD_SIZE));
	sunxi_pgtable_params.pgtable = NULL;
}

static inline bool __region_ended(u32 pent)
{
	return !(pent & SUNXI_PTE_PAGE_VALID);
}

static inline bool __access_mask_changed(u32 pent, u32 old_mask)
{
	return old_mask !=
	       (pent & (SUNXI_PTE_PAGE_READABLE | SUNXI_PTE_PAGE_WRITABLE));
}

static u32 __print_region(char *buf, size_t buf_len, ssize_t len,
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
	} else {
		if (for_sysfs_show) {
			len += sysfs_emit_at(
				buf, len,
				"iova:%pad phys:%pad %s%s size:0x%zx\n",
				&active_region->iova, &active_region->phys,
				active_region->access_mask &
						SUNXI_PTE_PAGE_READABLE ?
					"R" :
					" ",
				active_region->access_mask &
						SUNXI_PTE_PAGE_WRITABLE ?
					"W" :
					" ",
				active_region->size);
		} else {
			len += scnprintf(
				buf + len, buf_len - len,
				"iova:%pad phys:%pad %s%s size:0x%zx\n",
				&active_region->iova, &active_region->phys,
				active_region->access_mask &
						SUNXI_PTE_PAGE_READABLE ?
					"R" :
					" ",
				active_region->access_mask &
						SUNXI_PTE_PAGE_WRITABLE ?
					"W" :
					" ",
				active_region->size);
		}
	}
	return len;
}

ssize_t sunxi_pgtable_dump(unsigned int *pgtable, ssize_t len, char *buf,
			   size_t buf_len, bool for_sysfs_show)
{
	/* walk and dump */
	int i, j;
	u32 *dent, *pent;
	struct dump_region active_region;

	if (for_sysfs_show) {
		len += sysfs_emit_at(buf, len, "mapped\n");
	} else {
		len += scnprintf(buf + len, buf_len - len, "mapped\n");
	}

	dent = pgtable;
	active_region.type = DUMP_REGION_MAP;
	active_region.size = 0;
	active_region.access_mask = 0;
	for (i = 0; i < NUM_ENTRIES_PDE; i++) {
		j = 0;
		if (!IS_VALID(dent[i])) {
			/* empty dentry measn ended of region, print it*/
			if (active_region.size) {
				len = __print_region(buf, buf_len, len,
						     &active_region,
						     for_sysfs_show);
				/* prepare next region */
				active_region.size = 0;
				active_region.access_mask = 0;
			}
			continue;
		}
		/* iova here use for l1 idx, safe to pass 0 to get entry for 1st page(idx 0)*/
		pent = iopte_offset(dent + i, 0);
		for (; j < NUM_ENTRIES_PTE; j++) {
			if (active_region.size) {
				/* looks like we are counting something, check if it need printing */
				if (__region_ended(pent[j]) /* not contiguous */
				    ||
				    (active_region.access_mask &&
				     __access_mask_changed(
					     pent[j],
					     active_region
						     .access_mask)) /* different access */
				) {
					len = __print_region(buf, buf_len, len,
							     &active_region,
							     for_sysfs_show);

					/* prepare next region */
					active_region.size = 0;
					active_region.access_mask = 0;
				}
			}

			if (pent[j] & SUNXI_PTE_PAGE_VALID) {
				/* no on count region, mark start address */
				if (active_region.size == 0) {
					active_region.iova =
						((dma_addr_t)i
						 << IOMMU_PD_SHIFT) +
						((dma_addr_t)j
						 << IOMMU_PT_SHIFT);
					active_region.phys =
						iommu_phy_to_cpu_phy(
							IOPTE_TO_PFN(&pent[j]));
					active_region.access_mask =
						(pent[j] &
						 (SUNXI_PTE_PAGE_READABLE |
						  SUNXI_PTE_PAGE_WRITABLE));
				}
				active_region.size += 1 << IOMMU_PT_SHIFT;
			}
		}
	}
	//dump last region (if any)
	if (active_region.size) {
		len = __print_region(buf, buf_len, len, &active_region,
				     for_sysfs_show);
	}
	return len;
}

struct kmem_cache *sunxi_pgtable_alloc_pte_cache(void)
{
	struct kmem_cache *cache;
	cache = kmem_cache_create("sunxi-iopte-cache", PT_SIZE, PT_SIZE,
				  SLAB_HWCACHE_ALIGN, NULL);
	sunxi_pgtable_params.iopte_cache = cache;
	return cache;
}

void sunxi_pgtable_free_pte_cache(struct kmem_cache *iopte_cache)
{
	kmem_cache_destroy(iopte_cache);
}

void sunxi_pgtable_set_dma_dev(struct device *dma_dev)
{
	sunxi_pgtable_params.dma_dev = dma_dev;
}
