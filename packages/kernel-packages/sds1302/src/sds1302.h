#ifndef SDS1302_H_
#define SDS1302_H_

#undef S_DEBUG
#define S_DEBUG

#ifdef S_DEBUG

#define DRV_NAME     "rtc-sds1302"
#define S_DBG(fmt, args...) printk(KERN_DEBUG "%s: " fmt, DRV_NAME, ##args)
#define S_ALERT(fmt, args...) printk(KERN_ALERT "%s: " fmt, DRV_NAME, ##args)

#else

#define S_DBG(fmt, args...) do {} while (0)
#define S_ALERT(fmt, args...) do {} while (0)

#endif


#endif /* SDS1302_H_ */