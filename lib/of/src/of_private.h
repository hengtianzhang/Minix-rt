/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __OF_SRC_PRIVATE_H_
#define __OF_SRC_PRIVATE_H_
/*
 * Private symbols used by OF support code
 *
 * Paul Mackerras	August 1996.
 * Copyright (C) 1996-2005 Paul Mackerras.
 */

/**
 * struct alias_prop - Alias property in 'aliases' node
 * @link:	List node to link the structure in aliases_lookup list
 * @alias:	Alias property name
 * @np:		Pointer to device_node that the alias stands for
 * @id:		Index value from end of alias name
 * @stem:	Alias string without the index
 *
 * The structure represents one alias property of 'aliases' node as
 * an entry in aliases_lookup list.
 */
struct alias_prop {
	struct list_head link;
	const char *alias;
	struct device_node *np;
	int id;
	char stem[0];
};

extern struct list_head aliases_lookup;

static inline int of_property_notify(int action, struct device_node *np,
				     struct property *prop, struct property *old_prop)
{
	return 0;
}

/**
 * General utilities for working with live trees.
 *
 * All functions with two leading underscores operate
 * without taking node references, so you either have to
 * own the devtree lock or work on detached trees only.
 */
struct device_node *__of_node_dup(const struct device_node *np,
				  const char *full_name);

struct device_node *__of_find_node_by_path(struct device_node *parent,
						const char *path);
struct device_node *__of_find_node_by_full_path(struct device_node *node,
						const char *path);

extern const void *__of_get_property(const struct device_node *np,
				     const char *name, int *lenp);
extern int __of_add_property(struct device_node *np, struct property *prop);
extern int __of_add_property_sysfs(struct device_node *np,
		struct property *prop);
extern int __of_remove_property(struct device_node *np, struct property *prop);
extern void __of_remove_property_sysfs(struct device_node *np,
		struct property *prop);
extern int __of_update_property(struct device_node *np,
		struct property *newprop, struct property **oldprop);
extern void __of_update_property_sysfs(struct device_node *np,
		struct property *newprop, struct property *oldprop);

extern int __of_attach_node_sysfs(struct device_node *np);
extern void __of_detach_node(struct device_node *np);
extern void __of_detach_node_sysfs(struct device_node *np);

extern void __of_sysfs_remove_bin_file(struct device_node *np,
				       struct property *prop);

/* illegal phandle value (set when unresolved) */
#define OF_PHANDLE_ILLEGAL	0xdeadbeef

/* iterators for transactions, used for overlays */
/* forward iterator */
#define for_each_transaction_entry(_oft, _te) \
	list_for_each_entry(_te, &(_oft)->te_list, node)

/* reverse iterator */
#define for_each_transaction_entry_reverse(_oft, _te) \
	list_for_each_entry_reverse(_te, &(_oft)->te_list, node)

#endif /* !__OF_SRC_PRIVATE_H_ */
