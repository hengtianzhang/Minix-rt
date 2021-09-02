/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Device tables which are exported to userspace via
 * scripts/mod/file2alias.c.  You must keep that file in sync with this
 * header.
 */

#ifndef __OF_MOD_DEVICETABLE_H_
#define __OF_MOD_DEVICETABLE_H_

/*
 * Struct used for matching a device
 */
struct of_device_id {
	char	name[32];
	char	type[32];
	char	compatible[128];
	const void *data;
};

#endif /* !__OF_MOD_DEVICETABLE_H_ */
