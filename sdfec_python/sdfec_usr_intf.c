/*******************************************************************************
 *
 * Copyright (C) 2016 - 2020 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

/* Kernel Headers */
#include <sys/ioctl.h>
/* User Headers */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
/* Local Headers */

#include "sdfec_usr_intf.h"

#define LDPC_TABLE_DATA_SIZE (4)

int open_xsdfec(const char *dev_name)
{
	int fd = -1;
	if (!dev_name) {
		fprintf(stderr, "%s : Null file name", __func__);
		return -ENOENT;
	}

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s : Failed to open %s error = %s\n", __func__,
			dev_name, strerror(errno));
		return -1;
	}

	return fd;
}

int close_xsdfec(int fd)
{
	int rval;
	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}
	rval = close(fd);
	if (rval < 0) {
		fprintf(stderr, "%s : Close failed with %s", __func__,
			strerror(errno));
		return (errno);
	}
	return 0;
}

int get_config_xsdfec(int fd, struct xsdfec_config *config)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!config) {
		fprintf(stderr, "%s : NULL config pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_GET_CONFIG, config);
	if (rval < 0) {
		fprintf(stderr, "%s failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int get_status_xsdfec(int fd, struct xsdfec_status *status)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!status) {
		fprintf(stderr, "%s : NULL status pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_GET_STATUS, status);
	if (rval < 0) {
		fprintf(stderr, "%s failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

void print_stats_xsdfec(struct xsdfec_stats *stats,
			const char *dev_name)
{
	if (!stats) {
		fprintf(stderr, "Stats is NULL ... exiting\n");
		return;
	}

	printf("-------- %s Stats ---------\n", dev_name);
	/* Print State */
	printf("ISR Error Count               = %u\n", stats->isr_err_count);
	printf("Correctable ECC Error Count   = %u\n", stats->cecc_count);
	printf("Uncorrectable ECC Error Count = %u\n", stats->uecc_count);
	printf("--------------------------------\n");
}

void print_status_xsdfec(struct xsdfec_status *status,
			const char *dev_name)
{
	if (!status) {
		fprintf(stderr, "Status is NULL ... exiting\n");
		return;
	}

	printf("-------- %s Stats ---------\n", dev_name);
	/* Print State */
	if (status->state == XSDFEC_INIT)
		printf(" State : Initialized\n");
	else if (status->state == XSDFEC_STARTED)
		printf(" State : Started\n");
	else if (status->state == XSDFEC_STOPPED)
		printf(" State : Stopped\n");
	else if (status->state == XSDFEC_NEEDS_RESET)
		printf(" State : Needs Reset\n");
	else
		printf(" State : Invalid\n");
	/* Print Activity */
	printf("Activity : %d\n", status->activity);
	printf("------------------------------\n");
}

void print_config_xsdfec(struct xsdfec_config *config,
			const char *dev_name)
{
	if (!config) {
		fprintf(stderr, "Status is NULL ... exiting\n");
		return;
	}

	printf("-------- %s Stats ---------\n", dev_name);
	/* Print Code */
	if (config->code == XSDFEC_TURBO_CODE)
		printf(" Code : Turbo\n");
	else if (config->code == XSDFEC_LDPC_CODE)
		printf(" Code : LDPC\n");
	else
		printf(" Code : Invalid\n");

	/* Print Order */
	if (config->order == XSDFEC_MAINTAIN_ORDER)
		printf(" Order : Maintain Order\n");
	else if (config->order == XSDFEC_OUT_OF_ORDER)
		printf(" Order : Out Of Order\n");
	else
		printf(" Order : Invalid\n");

	printf("------------------------------\n");
}

int start_xsdfec(int fd)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_START_DEV);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}

	return 0;
}

int stop_xsdfec(int fd)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_STOP_DEV);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}

	return 0;
}

int set_turbo_xsdfec(int fd, struct xsdfec_turbo *turbo)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!turbo) {
		fprintf(stderr, "%s : NULL turbo code pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	if ((rval = ioctl(fd, XSDFEC_SET_TURBO, turbo)) < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int get_turbo_xsdfec(int fd, struct xsdfec_turbo *turbo)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!turbo) {
		fprintf(stderr, "%s : NULL Turbo pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_GET_TURBO, turbo);
	if (rval < 0) {
		fprintf(stderr, "%s failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int set_irq_xsdfec(int fd, struct xsdfec_irq *irq)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!irq) {
		fprintf(stderr, "%s : NULL irq pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_SET_IRQ, irq);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int add_ldpc_xsdfec(int fd, struct xsdfec_ldpc_params *ldpc)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		rval = -EBADF;
		goto error;
	}

	if (!ldpc) {
		fprintf(stderr, "%s : NULL status pointer, file desc.=%d\n",
			__func__, fd);
		rval = -EINVAL;
		goto error;
	}

	rval = ioctl(fd, XSDFEC_ADD_LDPC_CODE_PARAMS, ldpc);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		goto error;
	}
	rval = 0;

error:
	return rval;
}

int set_default_config_xsdfec(int fd)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_SET_DEFAULT_CONFIG);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int get_stats_xsdfec(int fd, struct xsdfec_stats *stats)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!stats) {
		fprintf(stderr, "%s : NULL stats pointer, file desc.=%d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_GET_STATS, stats);
	if (rval < 0) {
		fprintf(stderr, "%s failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int clear_stats_xsdfec(int fd)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_CLEAR_STATS);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}
	return 0;
}

int set_order_xsdfec(int fd, enum xsdfec_order order)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_SET_ORDER, &order);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}

	return 0;
}

int set_bypass_xsdfec(int fd, bool bypass)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	rval = ioctl(fd, XSDFEC_SET_BYPASS, &bypass);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}

	return 0;
}

int is_active_xsdfec(int fd, bool *is_sdfec_active)
{
	int rval;

	if (fd < 0) {
		fprintf(stderr, "%s : Invalid file descriptor %d\n", __func__,
			fd);
		return -EBADF;
	}

	if (!is_sdfec_active) {
		fprintf(stderr,
			"%s : NULL status pointer, file descriptor is %d\n",
			__func__, fd);
		return -EINVAL;
	}

	rval = ioctl(fd, XSDFEC_IS_ACTIVE, is_sdfec_active);
	if (rval < 0) {
		fprintf(stderr, "%s : failed with %s\n", __func__,
			strerror(errno));
		return rval;
	}

	return 0;
}

int prepare_ldpc_code(struct xsdfec_user_ldpc_code_params *user_params,
		      struct xsdfec_user_ldpc_table_offsets *user_offsets,
		      struct xsdfec_ldpc_params *ldpc_params,
		      unsigned int code_id)
{
	unsigned int itr, ret;
	int len;
	size_t psize = getpagesize();
	uint32_t *ptr;

	if (!user_params || !user_offsets || !ldpc_params) {
		fprintf(stderr, "%s : Null input argument, error-%s\n",
			__func__, strerror(errno));
		return -EINVAL;
	}

	if (code_id >= 128) {
		fprintf(stderr, "%s : Invalid LDPC Code ID: %u\n",
                __func__, code_id);
        return -EINVAL;
	}

	memset(ldpc_params, 0, sizeof(*ldpc_params));

	ldpc_params->n = user_params->n;
	ldpc_params->k = user_params->k;
	ldpc_params->psize = user_params->psize;
	ldpc_params->nlayers = user_params->nlayers;
	ldpc_params->nqc = user_params->nqc;
	ldpc_params->nmqc = user_params->nmqc;
	ldpc_params->nm = user_params->nm;
	ldpc_params->norm_type = user_params->norm_type;
	ldpc_params->no_packing = user_params->no_packing;
	ldpc_params->special_qc = user_params->special_qc;
	ldpc_params->no_final_parity = user_params->no_final_parity;
	ldpc_params->max_schedule = user_params->max_schedule;

	ldpc_params->sc_off = user_offsets->sc_off;
	ldpc_params->la_off = user_offsets->la_off;
	ldpc_params->qc_off = user_offsets->qc_off;

	/* Prepare SC Table */
	if (LDPC_TABLE_DATA_SIZE * get_sc_table_size(user_params) > XSDFEC_SC_TABLE_DEPTH) {
		fprintf(stderr,
			"%s : SC Table entries for code %d exceeds reg space\n",
			__func__, code_id);
		return -EINVAL;
	}
	/* Allocate a memory pages for SC table */
	len = LDPC_TABLE_DATA_SIZE * get_sc_table_size(user_params);
	if (posix_memalign((void **)&ptr, psize, len)) {
		ret = -EINVAL;
		goto error2;
	}

	for (itr = 0; itr < (unsigned int)get_sc_table_size(user_params);
	     itr++) {
		ptr[itr] = user_params->sc_table[itr];
	}

	ldpc_params->sc_table = 0xFFFFFFFF & ((uint64_t) ptr);

	/* Prepare LA Table */
	if (LDPC_TABLE_DATA_SIZE * user_params->nlayers > XSDFEC_LA_TABLE_DEPTH) {
		fprintf(stderr,
			"%s : LA Table entries for code %d exceeds reg space\n",
			__func__, code_id);
		ret = -EINVAL;
		goto error2;
	}
	/* Allocate a memory pages for LA table */
	len = LDPC_TABLE_DATA_SIZE * user_params->nlayers;
	if (posix_memalign((void **)&ptr, psize, len)) {
		ret = -EINVAL;
		goto error2;
	}

	for (itr = 0; itr < user_params->nlayers; itr++) {
		ptr[itr] = user_params->la_table[itr];
	}

	ldpc_params->la_table = 0xFFFFFFFFULL & ((uint64_t) ptr);

	/* Prepare QC Table */
	if (LDPC_TABLE_DATA_SIZE * user_params->nqc > XSDFEC_QC_TABLE_DEPTH) {
		fprintf(stderr,
			"%s : QC Table entries for code %d exceeds reg space\n",
			__func__, code_id);
		ret = -EINVAL;
		goto error2;
	}
	/* Allocate a memory for QC table */
	len = LDPC_TABLE_DATA_SIZE * user_params->nqc;
	if (posix_memalign((void **)&ptr, psize, len)) {
		ret = -EINVAL;
		goto error2;
	}

	for (itr = 0; itr < user_params->nqc; itr++) {
		ptr[itr] = user_params->qc_table[itr];
	}

	ldpc_params->qc_table = 0xFFFFFFFFULL & ((uint64_t) ptr);

	ldpc_params->code_id = code_id;
	return 0;

error2:
	return ret;
}

/**
 * xsdfec_calculate_shared_ldpc_table_entry_size - Calculates shared code
 * table sizes.
 * @ldpc: Pointer to the LPDC Code Parameters
 * @table_sizes: Pointer to structure containing the calculated table sizes
 *
 * Calculates the size of shared LDPC code tables used for a specified LPDC code
 * parameters.
 */
static inline void xsdfec_calculate_shared_ldpc_table_entry_size(
	struct xsdfec_ldpc_params *ldpc,
	struct xsdfec_ldpc_param_table_sizes *table_sizes)
{
	/* Calculate the sc_size in 32 bit words */
	table_sizes->sc_size = (ldpc->nlayers + 3) >> 2;
	/* Calculate the la_size in 128 bit words */
	table_sizes->la_size = ((ldpc->nlayers << 2) + 15) >> 4;
	/* Calculate the qc_size in 128 bit words */
	table_sizes->qc_size = ((ldpc->nqc << 2) + 15) >> 4;
}

int update_lpdc_table_offsets(
	struct xsdfec_ldpc_params *ldpc_params,
	struct xsdfec_user_ldpc_table_offsets *user_offsets)
{
	struct xsdfec_ldpc_param_table_sizes table_sizes;

	xsdfec_calculate_shared_ldpc_table_entry_size(ldpc_params,
						      &table_sizes);
	user_offsets->sc_off = ldpc_params->sc_off + table_sizes.sc_size;
	user_offsets->la_off = ldpc_params->la_off + table_sizes.la_size;
	user_offsets->qc_off = ldpc_params->qc_off + table_sizes.qc_size;

	return 0;
}

int get_sc_table_size(struct xsdfec_user_ldpc_code_params *user_params)
{
	int sc_table_size;
	int rem;

	rem = (user_params->nlayers) % LDPC_TABLE_DATA_SIZE;
	sc_table_size = (user_params->nlayers) / LDPC_TABLE_DATA_SIZE;
	if (rem != 0) {
		sc_table_size++;
	}
	return sc_table_size;
}
