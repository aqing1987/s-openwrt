#include <linux/kernel.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/printk.h>

#define NETLINK_ASSOC_DEBUG_PORT_ID 0x10
#define MAX_PAYLOAD 1024

static struct sock *nlsk = NULL;

static void nl_send_msg(char flag) {
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *skb_out = NULL;
	int msg_size;
	char msg[MAX_PAYLOAD];
	int res;
	int i = 0;

	memset(msg, 0, sizeof(msg));
	msg[0] = flag;
	for (i = 1; i < MAX_PAYLOAD - 1; ++i)
		msg[i] = 'x';
	msg_size = MAX_PAYLOAD;

	/* create a new netlink message */
	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
		pr_err("Failed to allocated new skb\n");
		return;
	}

	/* add a netlink message to an skb */
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	if (nlh == NULL) {
		pr_err("nlmsg_put err\n");
		nlmsg_free(skb_out);
		return;
	}

	/*
	 * set netlink control body
	 */

	/* from kernel */
	NETLINK_CB(skb_out).portid = 0;
	/* if dest team is kernle or one process, set it to 0 */
	NETLINK_CB(skb_out).dst_group = 0; 

	memcpy(nlmsg_data(nlh), msg, msg_size);

	/*
	 * You're not allowed to free the skb after you've sent it. nlmsg_unicast() will take care of that.
	 *
	 * The reason is fairly simple: once you send the message it can be queued in the netlink socket for a
	 * while before anyone reads it. Just because nlmsg_unicast() returned it doesn't mean that the
	 * other side of the socket already got the message. If you free it before it's received you end up with a
	 * freed message in the queue, which causes the crash when the kernel tries to deliver it.
	 *
	 * Simply allocate a new skb for every message.
	 * unicast a message to a single socket
	 */
	res = nlmsg_unicast(nlsk, skb_out, NETLINK_ASSOC_DEBUG_PORT_ID);
	if (res < 0)
		pr_err("nlmsg_unicast err:  %d\n", res);
}

static void nl_callback(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	int i = 0;

	if (skb == NULL) {
		pr_err("skb is NULL\n");
		return;
	}

	pr_info("enter: %s\n", __func__);

	nlh = (struct nlmsghdr *)skb->data;
	pr_info("Netlink rx msg payload: %s\n", (char *)nlmsg_data(nlh));

	for (i = 0; i < 5; ++i) {
		nl_send_msg((i%26) + 65);
	}
}

static void create_netlink(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = 1,
		.input = &nl_callback,
		.cb_mutex = NULL,
	};

	int i = 0;

	// find a valid id from 10 to 31
	for (i = 10; i < 31; ++i) {
		nlsk = netlink_kernel_create(&init_net, i, &cfg);
		if (nlsk) {
			pr_info("NETLINK NUM = %d\n", i);
			return;
		}
	}
}

static int __init nltest_init(void)
{
	pr_info("%s init\n", __func__);
	create_netlink();
	return 0;
}

static void __exit nltest_exit(void)
{
	if (nlsk) {
		netlink_kernel_release(nlsk);
		nlsk = NULL;
	}

	pr_info("%s exit\n", __func__);
}

module_init(nltest_init);
module_exit(nltest_exit);

MODULE_LICENSE("GPL");
