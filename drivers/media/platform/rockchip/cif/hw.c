// SPDX-License-Identifier: GPL-2.0
/*
 * Rockchip CIF Driver
 *
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regmap.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-fwnode.h>
#include <linux/iommu.h>
#include <dt-bindings/soc/rockchip-system-status.h>
#include <soc/rockchip/rockchip-system-status.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include "dev.h"

static const struct cif_reg px30_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_LINE_NUM_ADDR] = CIF_REG(CIF_LINE_NUM_ADDR),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_SCM_ADDR_Y] = CIF_REG(CIF_SCM_ADDR_Y),
	[CIF_REG_DVP_SCM_ADDR_U] = CIF_REG(CIF_SCM_ADDR_U),
	[CIF_REG_DVP_SCM_ADDR_V] = CIF_REG(CIF_SCM_ADDR_V),
	[CIF_REG_DVP_WB_UP_FILTER] = CIF_REG(CIF_WB_UP_FILTER),
	[CIF_REG_DVP_WB_LOW_FILTER] = CIF_REG(CIF_WB_LOW_FILTER),
	[CIF_REG_DVP_WBC_CNT] = CIF_REG(CIF_WBC_CNT),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_SCL_CTRL] = CIF_REG(CIF_SCL_CTRL),
	[CIF_REG_DVP_SCL_DST] = CIF_REG(CIF_SCL_DST),
	[CIF_REG_DVP_SCL_FCT] = CIF_REG(CIF_SCL_FCT),
	[CIF_REG_DVP_SCL_VALID_NUM] = CIF_REG(CIF_SCL_VALID_NUM),
	[CIF_REG_DVP_LINE_LOOP_CTRL] = CIF_REG(CIF_LINE_LOOP_CTR),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
};

static const char * const px30_cif_clks[] = {
	"aclk_cif",
	"hclk_cif",
	"pclk_cif",
	"cif_out",
};

static const char * const px30_cif_rsts[] = {
	"rst_cif_a",
	"rst_cif_h",
	"rst_cif_pclkin",
};

static const struct cif_reg rk1808_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_DMA_IDLE_REQ] = CIF_REG(CIF_DMA_IDLE_REQ),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_LINE_INT_NUM] = CIF_REG(CIF_LINE_INT_NUM),
	[CIF_REG_DVP_LINE_CNT] = CIF_REG(CIF_LINE_CNT),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_PATH_SEL] = CIF_REG(CIF_PATH_SEL),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
	[CIF_REG_MIPI_LVDS_ID0_CTRL0] = CIF_REG(CIF_CSI_ID0_CTRL0),
	[CIF_REG_MIPI_LVDS_ID0_CTRL1] = CIF_REG(CIF_CSI_ID0_CTRL1),
	[CIF_REG_MIPI_LVDS_ID1_CTRL0] = CIF_REG(CIF_CSI_ID1_CTRL0),
	[CIF_REG_MIPI_LVDS_ID1_CTRL1] = CIF_REG(CIF_CSI_ID1_CTRL1),
	[CIF_REG_MIPI_LVDS_ID2_CTRL0] = CIF_REG(CIF_CSI_ID2_CTRL0),
	[CIF_REG_MIPI_LVDS_ID2_CTRL1] = CIF_REG(CIF_CSI_ID2_CTRL1),
	[CIF_REG_MIPI_LVDS_ID3_CTRL0] = CIF_REG(CIF_CSI_ID3_CTRL0),
	[CIF_REG_MIPI_LVDS_ID3_CTRL1] = CIF_REG(CIF_CSI_ID3_CTRL1),
	[CIF_REG_MIPI_WATER_LINE] = CIF_REG(CIF_CSI_WATER_LINE),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_INTEN] = CIF_REG(CIF_CSI_INTEN),
	[CIF_REG_MIPI_LVDS_INTSTAT] = CIF_REG(CIF_CSI_INTSTAT),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID0_1] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID2_3] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID2_3),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID0_1] = CIF_REG(CIF_CSI_LINE_CNT_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID2_3] = CIF_REG(CIF_CSI_LINE_CNT_ID2_3),
	[CIF_REG_MIPI_LVDS_ID0_CROP_START] = CIF_REG(CIF_CSI_ID0_CROP_START),
	[CIF_REG_MIPI_LVDS_ID1_CROP_START] = CIF_REG(CIF_CSI_ID1_CROP_START),
	[CIF_REG_MIPI_LVDS_ID2_CROP_START] = CIF_REG(CIF_CSI_ID2_CROP_START),
	[CIF_REG_MIPI_LVDS_ID3_CROP_START] = CIF_REG(CIF_CSI_ID3_CROP_START),
	[CIF_REG_MMU_DTE_ADDR] = CIF_REG(CIF_MMU_DTE_ADDR),
	[CIF_REG_MMU_STATUS] = CIF_REG(CIF_MMU_DTE_ADDR),
	[CIF_REG_MMU_COMMAND] = CIF_REG(CIF_MMU_COMMAND),
	[CIF_REG_MMU_PAGE_FAULT_ADDR] = CIF_REG(CIF_MMU_PAGE_FAULT_ADDR),
	[CIF_REG_MMU_ZAP_ONE_LINE] = CIF_REG(CIF_MMU_ZAP_ONE_LINE),
	[CIF_REG_MMU_INT_RAWSTAT] = CIF_REG(CIF_MMU_INT_RAWSTAT),
	[CIF_REG_MMU_INT_CLEAR] = CIF_REG(CIF_MMU_INT_CLEAR),
	[CIF_REG_MMU_INT_MASK] = CIF_REG(CIF_MMU_INT_MASK),
	[CIF_REG_MMU_INT_STATUS] = CIF_REG(CIF_MMU_INT_STATUS),
	[CIF_REG_MMU_AUTO_GATING] = CIF_REG(CIF_MMU_AUTO_GATING),
};

static const char * const rk1808_cif_clks[] = {
	"aclk_cif",
	"dclk_cif",
	"hclk_cif",
	"sclk_cif_out",
	/* "pclk_csi2host" */
};

static const char * const rk1808_cif_rsts[] = {
	"rst_cif_a",
	"rst_cif_h",
	"rst_cif_i",
	"rst_cif_d",
	"rst_cif_pclkin",
};

static const struct cif_reg rk3128_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_SCL_CTRL] = CIF_REG(CIF_SCL_CTRL),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
};

static const char * const rk3128_cif_clks[] = {
	"aclk_cif",
	"hclk_cif",
	"sclk_cif_out",
};

static const char * const rk3128_cif_rsts[] = {
	"rst_cif",
};

static const struct cif_reg rk3288_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_SCL_CTRL] = CIF_REG(CIF_SCL_CTRL),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
};

static const char * const rk3288_cif_clks[] = {
	"aclk_cif0",
	"hclk_cif0",
	"cif0_in",
};

static const char * const rk3288_cif_rsts[] = {
	"rst_cif",
};

static const struct cif_reg rk3328_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_SCL_CTRL] = CIF_REG(CIF_SCL_CTRL),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
};

static const char * const rk3328_cif_clks[] = {
	"aclk_cif",
	"hclk_cif",
};

static const char * const rk3328_cif_rsts[] = {
	"rst_cif_a",
	"rst_cif_p",
	"rst_cif_h",
};

static const char * const rk3368_cif_clks[] = {
	"pclk_cif",
	"aclk_cif0",
	"hclk_cif0",
	"cif0_in",
};

static const char * const rk3368_cif_rsts[] = {
	"rst_cif",
};

static const struct cif_reg rk3368_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_DMA_IDLE_REQ] = CIF_REG(CIF_DMA_IDLE_REQ),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_CROP] = CIF_REG(CIF_CROP),
	[CIF_REG_DVP_SCL_CTRL] = CIF_REG(CIF_SCL_CTRL),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(CIF_LAST_PIX),
};

static const char * const rv1126_cif_clks[] = {
	"aclk_cif",
	"hclk_cif",
	"dclk_cif",
};

static const char * const rv1126_cif_rsts[] = {
	"rst_cif_a",
	"rst_cif_h",
	"rst_cif_d",
	"rst_cif_p",
	"rst_cif_i",
	"rst_cif_rx_p",
};

static const struct cif_reg rv1126_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_MULTI_ID] = CIF_REG(CIF_MULTI_ID),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_LINE_INT_NUM] = CIF_REG(CIF_LINE_INT_NUM),
	[CIF_REG_DVP_LINE_CNT] = CIF_REG(CIF_LINE_CNT),
	[CIF_REG_DVP_CROP] = CIF_REG(RV1126_CIF_CROP),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(RV1126_CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(RV1126_CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(RV1126_CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(RV1126_CIF_LAST_PIX),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID1] = CIF_REG(CIF_FRM0_ADDR_Y_ID1),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID1] = CIF_REG(CIF_FRM0_ADDR_UV_ID1),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID1] = CIF_REG(CIF_FRM1_ADDR_Y_ID1),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID1] = CIF_REG(CIF_FRM1_ADDR_UV_ID1),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID2] = CIF_REG(CIF_FRM0_ADDR_Y_ID2),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID2] = CIF_REG(CIF_FRM0_ADDR_UV_ID2),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID2] = CIF_REG(CIF_FRM1_ADDR_Y_ID2),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID2] = CIF_REG(CIF_FRM1_ADDR_UV_ID2),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID3] = CIF_REG(CIF_FRM0_ADDR_Y_ID3),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID3] = CIF_REG(CIF_FRM0_ADDR_UV_ID3),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID3] = CIF_REG(CIF_FRM1_ADDR_Y_ID3),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID3] = CIF_REG(CIF_FRM1_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_ID0_CTRL0] = CIF_REG(CIF_CSI_ID0_CTRL0),
	[CIF_REG_MIPI_LVDS_ID0_CTRL1] = CIF_REG(CIF_CSI_ID0_CTRL1),
	[CIF_REG_MIPI_LVDS_ID1_CTRL0] = CIF_REG(CIF_CSI_ID1_CTRL0),
	[CIF_REG_MIPI_LVDS_ID1_CTRL1] = CIF_REG(CIF_CSI_ID1_CTRL1),
	[CIF_REG_MIPI_LVDS_ID2_CTRL0] = CIF_REG(CIF_CSI_ID2_CTRL0),
	[CIF_REG_MIPI_LVDS_ID2_CTRL1] = CIF_REG(CIF_CSI_ID2_CTRL1),
	[CIF_REG_MIPI_LVDS_ID3_CTRL0] = CIF_REG(CIF_CSI_ID3_CTRL0),
	[CIF_REG_MIPI_LVDS_ID3_CTRL1] = CIF_REG(CIF_CSI_ID3_CTRL1),
	[CIF_REG_MIPI_LVDS_CTRL] = CIF_REG(CIF_CSI_MIPI_LVDS_CTRL),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_INTEN] = CIF_REG(CIF_CSI_INTEN),
	[CIF_REG_MIPI_LVDS_INTSTAT] = CIF_REG(CIF_CSI_INTSTAT),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID0_1] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID2_3] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID2_3),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID0_1] = CIF_REG(CIF_CSI_LINE_CNT_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID2_3] = CIF_REG(CIF_CSI_LINE_CNT_ID2_3),
	[CIF_REG_MIPI_LVDS_ID0_CROP_START] = CIF_REG(CIF_CSI_ID0_CROP_START),
	[CIF_REG_MIPI_LVDS_ID1_CROP_START] = CIF_REG(CIF_CSI_ID1_CROP_START),
	[CIF_REG_MIPI_LVDS_ID2_CROP_START] = CIF_REG(CIF_CSI_ID2_CROP_START),
	[CIF_REG_MIPI_LVDS_ID3_CROP_START] = CIF_REG(CIF_CSI_ID3_CROP_START),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID0),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID0),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID0),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID0),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID1),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID1),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID1),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID1),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID2),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID2),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID2),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID2),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID3),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID3),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID3),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID3),
	[CIF_REG_Y_STAT_CONTROL] = CIF_REG(CIF_Y_STAT_CONTROL),
	[CIF_REG_Y_STAT_VALUE] = CIF_REG(CIF_Y_STAT_VALUE),
	[CIF_REG_GRF_CIFIO_CON] = CIF_REG(CIF_GRF_CIFIO_CON),
};

static const char * const rv1126_cif_lite_clks[] = {
	"aclk_cif_lite",
	"hclk_cif_lite",
	"dclk_cif_lite",
};

static const char * const rv1126_cif_lite_rsts[] = {
	"rst_cif_lite_a",
	"rst_cif_lite_h",
	"rst_cif_lite_d",
	"rst_cif_lite_rx_p",
};

static const struct cif_reg rv1126_cif_lite_regs[] = {
	[CIF_REG_MIPI_LVDS_ID0_CTRL0] = CIF_REG(CIF_CSI_ID0_CTRL0),
	[CIF_REG_MIPI_LVDS_ID0_CTRL1] = CIF_REG(CIF_CSI_ID0_CTRL1),
	[CIF_REG_MIPI_LVDS_ID1_CTRL0] = CIF_REG(CIF_CSI_ID1_CTRL0),
	[CIF_REG_MIPI_LVDS_ID1_CTRL1] = CIF_REG(CIF_CSI_ID1_CTRL1),
	[CIF_REG_MIPI_LVDS_ID2_CTRL0] = CIF_REG(CIF_CSI_ID2_CTRL0),
	[CIF_REG_MIPI_LVDS_ID2_CTRL1] = CIF_REG(CIF_CSI_ID2_CTRL1),
	[CIF_REG_MIPI_LVDS_ID3_CTRL0] = CIF_REG(CIF_CSI_ID3_CTRL0),
	[CIF_REG_MIPI_LVDS_ID3_CTRL1] = CIF_REG(CIF_CSI_ID3_CTRL1),
	[CIF_REG_MIPI_LVDS_CTRL] = CIF_REG(CIF_CSI_MIPI_LVDS_CTRL),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_INTEN] = CIF_REG(CIF_CSI_INTEN),
	[CIF_REG_MIPI_LVDS_INTSTAT] = CIF_REG(CIF_CSI_INTSTAT),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID0_1] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID2_3] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID2_3),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID0_1] = CIF_REG(CIF_CSI_LINE_CNT_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID2_3] = CIF_REG(CIF_CSI_LINE_CNT_ID2_3),
	[CIF_REG_MIPI_LVDS_ID0_CROP_START] = CIF_REG(CIF_CSI_ID0_CROP_START),
	[CIF_REG_MIPI_LVDS_ID1_CROP_START] = CIF_REG(CIF_CSI_ID1_CROP_START),
	[CIF_REG_MIPI_LVDS_ID2_CROP_START] = CIF_REG(CIF_CSI_ID2_CROP_START),
	[CIF_REG_MIPI_LVDS_ID3_CROP_START] = CIF_REG(CIF_CSI_ID3_CROP_START),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID0),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID0),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID0),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID0] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID0),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID1),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID1),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID1),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID1] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID1),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID2),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID2),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID2),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID2] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID2),
	[CIF_REG_LVDS_SAV_EAV_ACT0_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_ACT0_ID3),
	[CIF_REG_LVDS_SAV_EAV_BLK0_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_BLK0_ID3),
	[CIF_REG_LVDS_SAV_EAV_ACT1_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_ACT1_ID3),
	[CIF_REG_LVDS_SAV_EAV_BLK1_ID3] = CIF_REG(CIF_LVDS_SAV_EAV_BLK1_ID3),
	[CIF_REG_Y_STAT_CONTROL] = CIF_REG(CIF_Y_STAT_CONTROL),
	[CIF_REG_Y_STAT_VALUE] = CIF_REG(CIF_Y_STAT_VALUE),
};

static const char * const rk3568_cif_clks[] = {
	"aclk_cif",
	"hclk_cif",
	"dclk_cif",
	"iclk_cif_g",
};

static const char * const rk3568_cif_rsts[] = {
	"rst_cif_a",
	"rst_cif_h",
	"rst_cif_d",
	"rst_cif_p",
	"rst_cif_i",
};

static const struct cif_reg rk3568_cif_regs[] = {
	[CIF_REG_DVP_CTRL] = CIF_REG(CIF_CTRL),
	[CIF_REG_DVP_INTEN] = CIF_REG(CIF_INTEN),
	[CIF_REG_DVP_INTSTAT] = CIF_REG(CIF_INTSTAT),
	[CIF_REG_DVP_FOR] = CIF_REG(CIF_FOR),
	[CIF_REG_DVP_MULTI_ID] = CIF_REG(CIF_MULTI_ID),
	[CIF_REG_DVP_FRM0_ADDR_Y] = CIF_REG(CIF_FRM0_ADDR_Y),
	[CIF_REG_DVP_FRM0_ADDR_UV] = CIF_REG(CIF_FRM0_ADDR_UV),
	[CIF_REG_DVP_FRM1_ADDR_Y] = CIF_REG(CIF_FRM1_ADDR_Y),
	[CIF_REG_DVP_FRM1_ADDR_UV] = CIF_REG(CIF_FRM1_ADDR_UV),
	[CIF_REG_DVP_VIR_LINE_WIDTH] = CIF_REG(CIF_VIR_LINE_WIDTH),
	[CIF_REG_DVP_SET_SIZE] = CIF_REG(CIF_SET_SIZE),
	[CIF_REG_DVP_LINE_INT_NUM] = CIF_REG(CIF_LINE_INT_NUM),
	[CIF_REG_DVP_LINE_CNT] = CIF_REG(CIF_LINE_CNT),
	[CIF_REG_DVP_CROP] = CIF_REG(RV1126_CIF_CROP),
	[CIF_REG_DVP_FIFO_ENTRY] = CIF_REG(RK3568_CIF_FIFO_ENTRY),
	[CIF_REG_DVP_FRAME_STATUS] = CIF_REG(RV1126_CIF_FRAME_STATUS),
	[CIF_REG_DVP_CUR_DST] = CIF_REG(RV1126_CIF_CUR_DST),
	[CIF_REG_DVP_LAST_LINE] = CIF_REG(RV1126_CIF_LAST_LINE),
	[CIF_REG_DVP_LAST_PIX] = CIF_REG(RV1126_CIF_LAST_PIX),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID1] = CIF_REG(CIF_FRM0_ADDR_Y_ID1),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID1] = CIF_REG(CIF_FRM0_ADDR_UV_ID1),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID1] = CIF_REG(CIF_FRM1_ADDR_Y_ID1),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID1] = CIF_REG(CIF_FRM1_ADDR_UV_ID1),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID2] = CIF_REG(CIF_FRM0_ADDR_Y_ID2),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID2] = CIF_REG(CIF_FRM0_ADDR_UV_ID2),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID2] = CIF_REG(CIF_FRM1_ADDR_Y_ID2),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID2] = CIF_REG(CIF_FRM1_ADDR_UV_ID2),
	[CIF_REG_DVP_FRM0_ADDR_Y_ID3] = CIF_REG(CIF_FRM0_ADDR_Y_ID3),
	[CIF_REG_DVP_FRM0_ADDR_UV_ID3] = CIF_REG(CIF_FRM0_ADDR_UV_ID3),
	[CIF_REG_DVP_FRM1_ADDR_Y_ID3] = CIF_REG(CIF_FRM1_ADDR_Y_ID3),
	[CIF_REG_DVP_FRM1_ADDR_UV_ID3] = CIF_REG(CIF_FRM1_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_ID0_CTRL0] = CIF_REG(CIF_CSI_ID0_CTRL0),
	[CIF_REG_MIPI_LVDS_ID0_CTRL1] = CIF_REG(CIF_CSI_ID0_CTRL1),
	[CIF_REG_MIPI_LVDS_ID1_CTRL0] = CIF_REG(CIF_CSI_ID1_CTRL0),
	[CIF_REG_MIPI_LVDS_ID1_CTRL1] = CIF_REG(CIF_CSI_ID1_CTRL1),
	[CIF_REG_MIPI_LVDS_ID2_CTRL0] = CIF_REG(CIF_CSI_ID2_CTRL0),
	[CIF_REG_MIPI_LVDS_ID2_CTRL1] = CIF_REG(CIF_CSI_ID2_CTRL1),
	[CIF_REG_MIPI_LVDS_ID3_CTRL0] = CIF_REG(CIF_CSI_ID3_CTRL0),
	[CIF_REG_MIPI_LVDS_ID3_CTRL1] = CIF_REG(CIF_CSI_ID3_CTRL1),
	[CIF_REG_MIPI_LVDS_CTRL] = CIF_REG(CIF_CSI_MIPI_LVDS_CTRL),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID0] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID0] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID0),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID1] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID1] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID1),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID2] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID2] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID2),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_Y_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM0_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_ADDR_UV_ID3] = CIF_REG(CIF_CSI_FRM1_ADDR_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_Y_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_Y_ID3),
	[CIF_REG_MIPI_LVDS_FRAME0_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM0_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_FRAME1_VLW_UV_ID3] = CIF_REG(CIF_CSI_FRM1_VLW_UV_ID3),
	[CIF_REG_MIPI_LVDS_INTEN] = CIF_REG(CIF_CSI_INTEN),
	[CIF_REG_MIPI_LVDS_INTSTAT] = CIF_REG(CIF_CSI_INTSTAT),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID0_1] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_INT_NUM_ID2_3] = CIF_REG(CIF_CSI_LINE_INT_NUM_ID2_3),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID0_1] = CIF_REG(CIF_CSI_LINE_CNT_ID0_1),
	[CIF_REG_MIPI_LVDS_LINE_LINE_CNT_ID2_3] = CIF_REG(CIF_CSI_LINE_CNT_ID2_3),
	[CIF_REG_MIPI_LVDS_ID0_CROP_START] = CIF_REG(CIF_CSI_ID0_CROP_START),
	[CIF_REG_MIPI_LVDS_ID1_CROP_START] = CIF_REG(CIF_CSI_ID1_CROP_START),
	[CIF_REG_MIPI_LVDS_ID2_CROP_START] = CIF_REG(CIF_CSI_ID2_CROP_START),
	[CIF_REG_MIPI_LVDS_ID3_CROP_START] = CIF_REG(CIF_CSI_ID3_CROP_START),
	[CIF_REG_MIPI_FRAME_NUM_VC0] = CIF_REG(CIF_CSI_FRAME_NUM_VC0),
	[CIF_REG_MIPI_FRAME_NUM_VC1] = CIF_REG(CIF_CSI_FRAME_NUM_VC1),
	[CIF_REG_MIPI_FRAME_NUM_VC2] = CIF_REG(CIF_CSI_FRAME_NUM_VC2),
	[CIF_REG_MIPI_FRAME_NUM_VC3] = CIF_REG(CIF_CSI_FRAME_NUM_VC3),
	[CIF_REG_Y_STAT_CONTROL] = CIF_REG(CIF_Y_STAT_CONTROL),
	[CIF_REG_Y_STAT_VALUE] = CIF_REG(CIF_Y_STAT_VALUE),
	[CIF_REG_MMU_DTE_ADDR] = CIF_REG(CIF_MMU_DTE_ADDR),
	[CIF_REG_MMU_STATUS] = CIF_REG(CIF_MMU_STATUS),
	[CIF_REG_MMU_COMMAND] = CIF_REG(CIF_MMU_COMMAND),
	[CIF_REG_MMU_PAGE_FAULT_ADDR] = CIF_REG(CIF_MMU_PAGE_FAULT_ADDR),
	[CIF_REG_MMU_ZAP_ONE_LINE] = CIF_REG(CIF_MMU_ZAP_ONE_LINE),
	[CIF_REG_MMU_INT_RAWSTAT] = CIF_REG(CIF_MMU_INT_RAWSTAT),
	[CIF_REG_MMU_INT_CLEAR] = CIF_REG(CIF_MMU_INT_CLEAR),
	[CIF_REG_MMU_INT_MASK] = CIF_REG(CIF_MMU_INT_MASK),
	[CIF_REG_MMU_INT_STATUS] = CIF_REG(CIF_MMU_INT_STATUS),
	[CIF_REG_MMU_AUTO_GATING] = CIF_REG(CIF_MMU_AUTO_GATING),
	[CIF_REG_GRF_CIFIO_CON] = CIF_REG(CIF_GRF_VI_CON0),
	[CIF_REG_GRF_CIFIO_CON1] = CIF_REG(CIF_GRF_VI_CON1),
};

static const struct rkcif_hw_match_data px30_cif_match_data = {
	.chip_id = CHIP_PX30_CIF,
	.clks = px30_cif_clks,
	.clks_num = ARRAY_SIZE(px30_cif_clks),
	.rsts = px30_cif_rsts,
	.rsts_num = ARRAY_SIZE(px30_cif_rsts),
	.cif_regs = px30_cif_regs,
};

static const struct rkcif_hw_match_data rk1808_cif_match_data = {
	.chip_id = CHIP_RK1808_CIF,
	.clks = rk1808_cif_clks,
	.clks_num = ARRAY_SIZE(rk1808_cif_clks),
	.rsts = rk1808_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk1808_cif_rsts),
	.cif_regs = rk1808_cif_regs,
};

static const struct rkcif_hw_match_data rk3128_cif_match_data = {
	.chip_id = CHIP_RK3128_CIF,
	.clks = rk3128_cif_clks,
	.clks_num = ARRAY_SIZE(rk3128_cif_clks),
	.rsts = rk3128_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk3128_cif_rsts),
	.cif_regs = rk3128_cif_regs,
};

static const struct rkcif_hw_match_data rk3288_cif_match_data = {
	.chip_id = CHIP_RK3288_CIF,
	.clks = rk3288_cif_clks,
	.clks_num = ARRAY_SIZE(rk3288_cif_clks),
	.rsts = rk3288_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk3288_cif_rsts),
	.cif_regs = rk3288_cif_regs,
};

static const struct rkcif_hw_match_data rk3328_cif_match_data = {
	.chip_id = CHIP_RK3328_CIF,
	.clks = rk3328_cif_clks,
	.clks_num = ARRAY_SIZE(rk3328_cif_clks),
	.rsts = rk3328_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk3328_cif_rsts),
	.cif_regs = rk3328_cif_regs,
};

static const struct rkcif_hw_match_data rk3368_cif_match_data = {
	.chip_id = CHIP_RK3368_CIF,
	.clks = rk3368_cif_clks,
	.clks_num = ARRAY_SIZE(rk3368_cif_clks),
	.rsts = rk3368_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk3368_cif_rsts),
	.cif_regs = rk3368_cif_regs,
};

static const struct rkcif_hw_match_data rv1126_cif_match_data = {
	.chip_id = CHIP_RV1126_CIF,
	.clks = rv1126_cif_clks,
	.clks_num = ARRAY_SIZE(rv1126_cif_clks),
	.rsts = rv1126_cif_rsts,
	.rsts_num = ARRAY_SIZE(rv1126_cif_rsts),
	.cif_regs = rv1126_cif_regs,
};

static const struct rkcif_hw_match_data rv1126_cif_lite_match_data = {
	.chip_id = CHIP_RV1126_CIF_LITE,
	.clks = rv1126_cif_lite_clks,
	.clks_num = ARRAY_SIZE(rv1126_cif_lite_clks),
	.rsts = rv1126_cif_lite_rsts,
	.rsts_num = ARRAY_SIZE(rv1126_cif_lite_rsts),
	.cif_regs = rv1126_cif_lite_regs,
};

static const struct rkcif_hw_match_data rk3568_cif_match_data = {
	.chip_id = CHIP_RK3568_CIF,
	.clks = rk3568_cif_clks,
	.clks_num = ARRAY_SIZE(rk3568_cif_clks),
	.rsts = rk3568_cif_rsts,
	.rsts_num = ARRAY_SIZE(rk3568_cif_rsts),
	.cif_regs = rk3568_cif_regs,
};


static const struct of_device_id rkcif_plat_of_match[] = {
#ifdef CONFIG_CPU_PX30
	{
		.compatible = "rockchip,px30-cif",
		.data = &px30_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK1808
	{
		.compatible = "rockchip,rk1808-cif",
		.data = &rk1808_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK312X
	{
		.compatible = "rockchip,rk3128-cif",
		.data = &rk3128_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK3288
	{
		.compatible = "rockchip,rk3288-cif",
		.data = &rk3288_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK3328
	{
		.compatible = "rockchip,rk3328-cif",
		.data = &rk3328_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK3368
	{
		.compatible = "rockchip,rk3368-cif",
		.data = &rk3368_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RK3568
	{
		.compatible = "rockchip,rk3568-cif",
		.data = &rk3568_cif_match_data,
	},
#endif
#ifdef CONFIG_CPU_RV1126
	{
		.compatible = "rockchip,rv1126-cif",
		.data = &rv1126_cif_match_data,
	},
	{
		.compatible = "rockchip,rv1126-cif-lite",
		.data = &rv1126_cif_lite_match_data,
	},
#endif
	{},
};

static irqreturn_t rkcif_irq_handler(int irq, void *ctx)
{
	struct device *dev = ctx;
	struct rkcif_hw *cif_hw = dev_get_drvdata(dev);
	int i;
	struct rkcif_device *tmp_dev = NULL;

	for (i = 0; i < cif_hw->dev_num; i++) {
		tmp_dev = cif_hw->cif_dev[i];
		if (tmp_dev->isr_hdl &&
		    (atomic_read(&tmp_dev->pipe.stream_cnt) != 0))
			tmp_dev->isr_hdl(irq, tmp_dev);
	}

	return IRQ_HANDLED;
}

void rkcif_disable_sys_clk(struct rkcif_hw *cif_hw)
{
	int i;

	for (i = cif_hw->clk_size - 1; i >= 0; i--)
		clk_disable_unprepare(cif_hw->clks[i]);
}

int rkcif_enable_sys_clk(struct rkcif_hw *cif_hw)
{
	int i, ret = -EINVAL;

	for (i = 0; i < cif_hw->clk_size; i++) {
		ret = clk_prepare_enable(cif_hw->clks[i]);

		if (ret < 0)
			goto err;
	}

	write_cif_reg_and(cif_hw->base_addr, CIF_CSI_INTEN, 0x0);
	return 0;

err:
	for (--i; i >= 0; --i)
		clk_disable_unprepare(cif_hw->clks[i]);

	return ret;
}

static void rkcif_iommu_cleanup(struct rkcif_hw *cif_hw)
{
	if (cif_hw->domain)
		iommu_detach_device(cif_hw->domain, cif_hw->dev);
}

static void rkcif_iommu_enable(struct rkcif_hw *cif_hw)
{
	if (!cif_hw->domain)
		cif_hw->domain = iommu_get_domain_for_dev(cif_hw->dev);

	if (cif_hw->domain)
		iommu_attach_device(cif_hw->domain, cif_hw->dev);
}

static inline bool is_iommu_enable(struct device *dev)
{
	struct device_node *iommu;

	iommu = of_parse_phandle(dev->of_node, "iommus", 0);
	if (!iommu) {
		dev_info(dev, "no iommu attached, using non-iommu buffers\n");
		return false;
	} else if (!of_device_is_available(iommu)) {
		dev_info(dev, "iommu is disabled, using non-iommu buffers\n");
		of_node_put(iommu);
		return false;
	}
	of_node_put(iommu);

	return true;
}

void rkcif_hw_soft_reset(struct rkcif_hw *cif_hw, bool is_rst_iommu)
{
	unsigned int i;

	if (cif_hw->iommu_en && is_rst_iommu)
		rkcif_iommu_cleanup(cif_hw);

	for (i = 0; i < ARRAY_SIZE(cif_hw->cif_rst); i++)
		if (cif_hw->cif_rst[i])
			reset_control_assert(cif_hw->cif_rst[i]);
	udelay(5);
	for (i = 0; i < ARRAY_SIZE(cif_hw->cif_rst); i++)
		if (cif_hw->cif_rst[i])
			reset_control_deassert(cif_hw->cif_rst[i]);

	if (cif_hw->iommu_en && is_rst_iommu)
		rkcif_iommu_enable(cif_hw);
}

static char *rkcif_get_monitor_mode(enum rkcif_monitor_mode mode)
{
	switch (mode) {
	case RKCIF_MONITOR_MODE_IDLE:
		return "idle";
	case RKCIF_MONITOR_MODE_CONTINUE:
		return "continue";
	case RKCIF_MONITOR_MODE_TRIGGER:
		return "trigger";
	case RKCIF_MONITOR_MODE_HOTPLUG:
		return "hotplug";
	default:
		return "unknown";
	}
}

static void rkcif_init_reset_timer(struct rkcif_hw *hw)
{
	struct device_node *node = hw->dev->of_node;
	struct rkcif_hw_timer *hw_timer = &hw->hw_timer;
	u32 para[8];
	int i;

	if (!of_property_read_u32_array(node,
					OF_CIF_MONITOR_PARA,
					para,
					CIF_MONITOR_PARA_NUM)) {
		for (i = 0; i < CIF_MONITOR_PARA_NUM; i++) {
			if (i == 0) {
				hw_timer->monitor_mode = para[0];
				dev_info(hw->dev,
					 "%s: timer monitor mode:%s\n",
					 __func__, rkcif_get_monitor_mode(hw_timer->monitor_mode));
			}

			if (i == 1) {
				hw_timer->monitor_cycle = para[1];
				dev_info(hw->dev,
					 "timer of monitor cycle:%d\n",
					 hw_timer->monitor_cycle);
			}

			if (i == 2) {
				hw_timer->err_time_interval = para[2];
				dev_info(hw->dev,
					 "timer err time for keeping:%d ms\n",
					 hw_timer->err_time_interval);
			}

			if (i == 3) {
				hw_timer->err_ref_cnt = para[3];
				dev_info(hw->dev,
					 "timer err ref val for resetting:%d\n",
					 hw_timer->err_ref_cnt);
			}

			if (i == 4) {
				hw_timer->is_reset_by_user = para[4];
				dev_info(hw->dev,
					 "reset by user:%d\n",
					 hw_timer->is_reset_by_user);
			}
		}
	} else {
		hw_timer->monitor_mode = RKCIF_MONITOR_MODE_IDLE;
		hw_timer->err_time_interval = 0xffffffff;
		hw_timer->monitor_cycle = 0xffffffff;
		hw_timer->err_ref_cnt = 0xffffffff;
		hw_timer->is_reset_by_user = 0;
	}

	hw_timer->is_running = false;
	spin_lock_init(&hw_timer->timer_lock);
	hw->reset_info.is_need_reset = 0;
	timer_setup(&hw_timer->timer, rkcif_reset_watchdog_timer_handler, 0);
}

static int rkcif_plat_hw_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct rkcif_hw *cif_hw;
	struct rkcif_device *cif_dev;
	const struct rkcif_hw_match_data *data;
	struct resource *res;
	int i, ret, irq;

	match = of_match_node(rkcif_plat_of_match, node);
	if (IS_ERR(match))
		return PTR_ERR(match);
	data = match->data;

	cif_hw = devm_kzalloc(dev, sizeof(*cif_hw), GFP_KERNEL);
	if (!cif_hw)
		return -ENOMEM;

	dev_set_drvdata(dev, cif_hw);
	cif_hw->dev = dev;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(dev, irq, rkcif_irq_handler,
			       IRQF_SHARED,
			       dev_driver_string(dev), dev);
	if (ret < 0) {
		dev_err(dev, "request irq failed: %d\n", ret);
		return ret;
	}

	cif_hw->irq = irq;
	cif_hw->match_data = data;
	cif_hw->chip_id = data->chip_id;
	if (data->chip_id == CHIP_RK1808_CIF ||
	    data->chip_id == CHIP_RV1126_CIF ||
	    data->chip_id == CHIP_RV1126_CIF_LITE ||
	    data->chip_id == CHIP_RK3568_CIF) {
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM,
						   "cif_regs");
		cif_hw->base_addr = devm_ioremap_resource(dev, res);
		if (PTR_ERR(cif_hw->base_addr) == -EBUSY) {
			resource_size_t offset = res->start;
			resource_size_t size = resource_size(res);

			cif_hw->base_addr = devm_ioremap(dev, offset, size);
			if (IS_ERR(cif_hw->base_addr)) {
				dev_err(dev, "ioremap failed\n");
				return PTR_ERR(cif_hw->base_addr);
			}
		}
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		cif_hw->base_addr = devm_ioremap_resource(dev, res);
		if (IS_ERR(cif_hw->base_addr))
			return PTR_ERR(cif_hw->base_addr);
	}

	cif_hw->grf = syscon_regmap_lookup_by_phandle(np, "rockchip,grf");
	if (IS_ERR(cif_hw->grf))
		dev_warn(dev, "unable to get rockchip,grf\n");

	if (data->clks_num > RKCIF_MAX_BUS_CLK ||
	    data->rsts_num > RKCIF_MAX_RESET) {
		dev_err(dev, "out of range: clks(%d %d) rsts(%d %d)\n",
			data->clks_num, RKCIF_MAX_BUS_CLK,
			data->rsts_num, RKCIF_MAX_RESET);
		return -EINVAL;
	}

	for (i = 0; i < data->clks_num; i++) {
		struct clk *clk = devm_clk_get(dev, data->clks[i]);

		if (IS_ERR(clk)) {
			dev_err(dev, "failed to get %s\n", data->clks[i]);
			return PTR_ERR(clk);
		}
		cif_hw->clks[i] = clk;
	}
	cif_hw->clk_size = data->clks_num;

	for (i = 0; i < data->rsts_num; i++) {
		struct reset_control *rst = NULL;

		if (data->rsts[i])
			rst = devm_reset_control_get(dev, data->rsts[i]);
		if (IS_ERR(rst)) {
			dev_err(dev, "failed to get %s\n", data->rsts[i]);
			return PTR_ERR(rst);
		}
		cif_hw->cif_rst[i] = rst;
	}

	cif_hw->cif_regs = data->cif_regs;

	cif_hw->iommu_en = is_iommu_enable(dev);
	if (!cif_hw->iommu_en) {
		ret = of_reserved_mem_device_init(dev);
		if (ret)
			dev_info(dev, "No reserved memory region assign to CIF\n");
	}

	if (data->chip_id != CHIP_RK1808_CIF &&
	    data->chip_id != CHIP_RV1126_CIF &&
	    data->chip_id != CHIP_RV1126_CIF_LITE &&
	    data->chip_id != CHIP_RK3568_CIF) {
		cif_dev = devm_kzalloc(dev, sizeof(*cif_dev), GFP_KERNEL);
		if (!cif_dev)
			return -ENOMEM;

		cif_dev->dev = dev;
		cif_dev->hw_dev = cif_hw;
		cif_dev->chip_id = cif_hw->chip_id;
		cif_hw->cif_dev[0] = cif_dev;
		cif_hw->dev_num = 1;
		ret = rkcif_plat_init(cif_dev, node, RKCIF_DVP);
		if (ret)
			return ret;
	}

	rkcif_hw_soft_reset(cif_hw, true);

	mutex_init(&cif_hw->dev_lock);

	pm_runtime_enable(&pdev->dev);
	rkcif_init_reset_timer(cif_hw);

	if (data->chip_id == CHIP_RK1808_CIF ||
	    data->chip_id == CHIP_RV1126_CIF ||
	    data->chip_id == CHIP_RK3568_CIF) {
		platform_driver_register(&rkcif_plat_drv);
		platform_driver_register(&rkcif_subdev_driver);
	}

	return 0;
}

static int rkcif_plat_remove(struct platform_device *pdev)
{
	struct rkcif_hw *cif_hw = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	if (cif_hw->iommu_en)
		rkcif_iommu_cleanup(cif_hw);

	mutex_destroy(&cif_hw->dev_lock);
	if (cif_hw->chip_id != CHIP_RK1808_CIF &&
	    cif_hw->chip_id != CHIP_RV1126_CIF &&
	    cif_hw->chip_id != CHIP_RV1126_CIF_LITE &&
	    cif_hw->chip_id != CHIP_RK3568_CIF)
		rkcif_plat_uninit(cif_hw->cif_dev[0]);
	del_timer_sync(&cif_hw->hw_timer.timer);
	return 0;
}

static int __maybe_unused rkcif_runtime_suspend(struct device *dev)
{
	struct rkcif_hw *cif_hw = dev_get_drvdata(dev);

	rkcif_disable_sys_clk(cif_hw);

	return pinctrl_pm_select_sleep_state(dev);
}

static int __maybe_unused rkcif_runtime_resume(struct device *dev)
{
	struct rkcif_hw *cif_hw = dev_get_drvdata(dev);
	int ret;

	ret = pinctrl_pm_select_default_state(dev);
	if (ret < 0)
		return ret;
	rkcif_enable_sys_clk(cif_hw);

	return 0;
}

static const struct dev_pm_ops rkcif_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(rkcif_runtime_suspend, rkcif_runtime_resume, NULL)
};

static struct platform_driver rkcif_hw_plat_drv = {
	.driver = {
		.name = RKCIF_HW_DRIVER_NAME,
		.of_match_table = of_match_ptr(rkcif_plat_of_match),
		.pm = &rkcif_plat_pm_ops,
	},
	.probe = rkcif_plat_hw_probe,
	.remove = rkcif_plat_remove,
};

static int __init rk_cif_plat_drv_init(void)
{
	int ret;

	ret = platform_driver_register(&rkcif_hw_plat_drv);
	if (ret)
		return ret;
	return rkcif_csi2_plat_drv_init();
}

static void __exit rk_cif_plat_drv_exit(void)
{
	platform_driver_unregister(&rkcif_hw_plat_drv);
	rkcif_csi2_plat_drv_exit();
}

module_init(rk_cif_plat_drv_init);
module_exit(rk_cif_plat_drv_exit);

MODULE_AUTHOR("Rockchip Camera/ISP team");
MODULE_DESCRIPTION("Rockchip CIF platform driver");
MODULE_LICENSE("GPL v2");