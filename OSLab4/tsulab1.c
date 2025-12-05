#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nobahu");
MODULE_DESCRIPTION("Tomsk State University Module");
MODULE_VERSION("2.0");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS
#endif

#define PROC_FS_NAME "tsulab"
#define TARGET_TIMESTAMP 1771286400


static struct proc_dir_entry *proc_file = NULL;

static int calculate(void)
{
	struct timespec64 now;
	long seconds_left;
	int days_left;

	ktime_get_real_ts64(&now);
	seconds_left = TARGET_TIMESTAMP - now.tv_sec;
	if (seconds_left <= 0) {
		return 0;
	}

	days_left = seconds_left / 86400;
	if(seconds_left % 86400 != 0) {
		days_left++;
	}
	return days_left;
}

static ssize_t proc_file_read(struct file *filePtr, char __user *buffer, 
				size_t bufferLength, loff_t *offset)
{
	char message[100];
	int days_left;

	if(*offset > 0) {
		return 0;
	}
	days_left = calculate();

	snprintf(message, sizeof(message), "Days until China new year: %d days\n ", days_left);

	if(copy_to_user(buffer, message, strlen(message))) {
		return -EFAULT;
	}
	*offset = strlen(message);
	return *offset;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_ops = { .proc_read = proc_file_read,};
#else
static const struct file_operations proc_file_ops = {
	.read = proc_file_read,
};
#endif

static int __init tsuInit(void)
{
	proc_file = proc_create(PROC_FS_NAME, 0444,NULL,&proc_file_ops);

	if(!proc_file) {
		pr_err("Failed to create /proc/%s\n",PROC_FS_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "Welcome to the Tomsk State University\n");
	return 0;
}

static void __exit tsuExit(void)
{
	proc_remove(proc_file);

	printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsuInit);
module_exit(tsuExit);
