// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022 Allwinner Co., Ltd.
 *
 * The pcie_dma_chnl_request() is used to apply for pcie DMA channels;
 * The pcie_dma_mem_xxx() is to initiate DMA read and write operations;
 *
 */

#define SUNXI_MODNAME "pcie-edma"
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_pci.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/reset.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "pcie-sunxi-dma.h"

static struct dma_trx_obj *obj_global;

sunxi_pci_edma_chan_t *sunxi_pcie_dma_chan_request(enum dma_dir dma_trx, void *cb, void *data)
{
	struct sunxi_pcie *pci = dev_get_drvdata(obj_global->dev);
	sunxi_pci_edma_chan_t *edma_chan = NULL;
	u32 free_chan;

	if (dma_trx == PCIE_DMA_WRITE) {
		free_chan = find_first_zero_bit(pci->wr_edma_map, pci->num_edma);

		if (free_chan >= pci->num_edma) {
			dev_err(pci->dev, "No free pcie edma write channel.\n");
			return NULL;
		}

		set_bit(free_chan, pci->wr_edma_map);

		edma_chan = &pci->dma_wr_chn[free_chan];

		edma_chan->dma_trx = PCIE_DMA_WRITE;
		edma_chan->chnl_num = free_chan;
		edma_chan->callback = cb;
		edma_chan->callback_param = data;

		return edma_chan;
	} else if (dma_trx == PCIE_DMA_READ) {
		free_chan = find_first_zero_bit(pci->rd_edma_map, pci->num_edma);

		if (free_chan >= pci->num_edma) {
			dev_err(pci->dev, "No free pcie edma read channel.\n");
			return NULL;
		}

		set_bit(free_chan, pci->rd_edma_map);

		edma_chan = &pci->dma_rd_chn[free_chan];

		edma_chan->dma_trx = PCIE_DMA_READ;
		edma_chan->chnl_num = free_chan;
		edma_chan->callback = cb;
		edma_chan->callback_param = data;

		return edma_chan;
	} else {
		dev_err(pci->dev, "ERR: unsupported type:%d \n", dma_trx);
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(sunxi_pcie_dma_chan_request);

int sunxi_pcie_dma_chan_release(struct sunxi_pci_edma_chan *edma_chan, enum dma_dir dma_trx)
{
	struct sunxi_pcie *pci = dev_get_drvdata(obj_global->dev);

	if (edma_chan->chnl_num >= pci->num_edma) {
		dev_err(pci->dev, "ERR: the channel num:%d is error\n", edma_chan->chnl_num);
		return -1;
	}

	if (PCIE_DMA_WRITE == dma_trx) {
		edma_chan->callback = NULL;
		edma_chan->callback_param = NULL;
		clear_bit(edma_chan->chnl_num, pci->wr_edma_map);
	} else if (PCIE_DMA_READ == dma_trx) {
		edma_chan->callback = NULL;
		edma_chan->callback_param = NULL;
		clear_bit(edma_chan->chnl_num, pci->rd_edma_map);
	} else {
		dev_err(pci->dev, "ERR: unsupported type:%d \n", dma_trx);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_pcie_dma_chan_release);

static int sunxi_pcie_init_edma_map(struct sunxi_pcie *pci)
{
	pci->rd_edma_map = devm_bitmap_zalloc(pci->dev, pci->num_edma, GFP_KERNEL);
	if (!pci->rd_edma_map)
		return -ENOMEM;

	pci->wr_edma_map = devm_bitmap_zalloc(pci->dev, pci->num_edma, GFP_KERNEL);
	if (!pci->wr_edma_map)
		return -ENOMEM;

	return 0;
}

int sunxi_pcie_dma_get_chan(struct platform_device *pdev)
{
	struct sunxi_pcie *pci = platform_get_drvdata(pdev);
	sunxi_pci_edma_chan_t *edma_chan = NULL;
	int ret, i;

	ret = of_property_read_u32(pdev->dev.of_node, "num-edma", &pci->num_edma);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse the number of edma\n");
		return -EINVAL;
	} else {
		ret = sunxi_pcie_init_edma_map(pci);
		if (ret)
			return -EINVAL;
	}

	pci->dma_wr_chn = devm_kcalloc(&pdev->dev, pci->num_edma, sizeof(sunxi_pci_edma_chan_t), GFP_KERNEL);
	pci->dma_rd_chn = devm_kcalloc(&pdev->dev, pci->num_edma, sizeof(sunxi_pci_edma_chan_t), GFP_KERNEL);
	if (!pci->dma_wr_chn || !pci->dma_rd_chn) {
		dev_err(&pdev->dev, "PCIe edma init failed\n");
		return -EINVAL;
	}

	for (i = 0; i < pci->num_edma; i++) {
		edma_chan = &pci->dma_wr_chn[i];
		spin_lock_init(&edma_chan->lock);
	}

	for (i = 0; i < pci->num_edma; i++) {
		edma_chan = &pci->dma_rd_chn[i];
		spin_lock_init(&edma_chan->lock);
	}

	return 0;
}

struct dma_trx_obj *sunxi_pcie_dma_obj_probe(struct device *dev)
{
	struct dma_trx_obj *obj;

	obj = devm_kzalloc(dev, sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return ERR_PTR(-ENOMEM);

	obj_global = obj;
	obj->dev = dev;

	INIT_LIST_HEAD(&obj->dma_list);
	spin_lock_init(&obj->dma_list_lock);

	mutex_init(&obj->count_mutex);

	return obj;
}
EXPORT_SYMBOL_GPL(sunxi_pcie_dma_obj_probe);

int sunxi_pcie_dma_obj_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_pcie *pci = platform_get_drvdata(pdev);

	memset(pci->dma_wr_chn, 0, sizeof(sunxi_pci_edma_chan_t) * pci->num_edma);
	memset(pci->dma_rd_chn, 0, sizeof(sunxi_pci_edma_chan_t) * pci->num_edma);

	obj_global->dma_list.next = NULL;
	obj_global->dma_list.prev = NULL;
	mutex_destroy(&obj_global->count_mutex);

	obj_global = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_pcie_dma_obj_remove);
