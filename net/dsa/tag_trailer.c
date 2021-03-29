// SPDX-License-Identifier: GPL-2.0+
/*
 * net/dsa/tag_trailer.c - Trailer tag format handling
 * Copyright (c) 2008-2009 Marvell Semiconductor
 */

#include <linux/etherdevice.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "dsa_priv.h"

static struct sk_buff *trailer_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct dsa_port *dp = dsa_slave_to_port(dev);
	u8 *trailer;

	trailer = skb_put(skb, 4);
	trailer[0] = 0x80;
#ifdef CONFIG_NET_DSA_RTL83XX
	trailer[1] = dp->index;
#else
	trailer[1] = 1 << dp->index;
#endif /*CONFIG_NET_DSA_RTL83XX  */
	trailer[2] = 0x10;
	trailer[3] = 0x00;

	return skb;
}

static struct sk_buff *trailer_rcv(struct sk_buff *skb, struct net_device *dev,
				   struct packet_type *pt)
{
	u8 *trailer;
	int source_port;

	if (skb_linearize(skb))
		return NULL;

	trailer = skb_tail_pointer(skb) - 4;
#ifdef CONFIG_NET_DSA_RTL83XX
	if (trailer[0] != 0x80 || (trailer[1] & 0x80) != 0x00 ||
	    (trailer[2] & 0xef) != 0x00 || trailer[3] != 0x00)
		return NULL;

	if (!(trailer[1] & 0x40))
		skb->offload_fwd_mark = 1;

	source_port = trailer[1] & 0x3f;
#else
	if (trailer[0] != 0x80 || (trailer[1] & 0xf8) != 0x00 ||
	    (trailer[2] & 0xef) != 0x00 || trailer[3] != 0x00)
		return NULL;

	source_port = trailer[1] & 7;
#endif /*CONFIG_NET_DSA_RTL83XX  */

	skb->dev = dsa_master_find_slave(dev, 0, source_port);
	if (!skb->dev)
		return NULL;

	if (pskb_trim_rcsum(skb, skb->len - 4))
		return NULL;

	return skb;
}

static const struct dsa_device_ops trailer_netdev_ops = {
	.name	= "trailer",
	.proto	= DSA_TAG_PROTO_TRAILER,
	.xmit	= trailer_xmit,
	.rcv	= trailer_rcv,
	.overhead = 4,
	.tail_tag = true,
};

MODULE_LICENSE("GPL");
MODULE_ALIAS_DSA_TAG_DRIVER(DSA_TAG_PROTO_TRAILER);

module_dsa_tag_driver(trailer_netdev_ops);
