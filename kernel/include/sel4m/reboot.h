/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_REBOOT_H_
#define __MINIX_RT_REBOOT_H_

enum reboot_mode {
	REBOOT_COLD = 0,
	REBOOT_WARM,
	REBOOT_HARD,
	REBOOT_SOFT,
	REBOOT_GPIO,
};
extern enum reboot_mode reboot_mode;

extern void machine_restart(char *cmd);

extern void (*arm_pm_restart)(enum reboot_mode reboot_mode, const char *cmd);
extern void (*pm_power_off)(void);

#endif /* !__MINIX_RT_REBOOT_H_ */
