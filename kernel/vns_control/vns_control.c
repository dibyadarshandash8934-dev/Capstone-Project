#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "vns_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VNS Capstone Project");
MODULE_DESCRIPTION("Virtual NAT Gateway Simulator Control Interface");
MODULE_VERSION("1.0.0");

#define VNS_DEVICE_NAME "vns_control"
#define VNS_CLASS_NAME "vns"
#define VNS_MAX_NAT_ENTRIES 1024
#define VNS_MAX_PF_RULES 256

/* Driver state */
struct vns_driver_state {
    struct mutex lock;
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct device *device;
    
    /* Statistics */
    unsigned long long total_packets;
    unsigned long long successful_packets;
    unsigned long long failed_packets;
    unsigned long long snat_packets;
    unsigned long long dnat_packets;
    unsigned long long reverse_nat_packets;
    unsigned long long total_processing_time_ns;
    
    /* NAT table (simplified for kernel) */
    unsigned int nat_table_entries;
    unsigned int port_forward_rules;
    
    /* Status */
    unsigned int status_flags;
    unsigned int active_connections;
    unsigned int simulator_running;
    char simulator_version[32];
};

static struct vns_driver_state *vns_state = NULL;

/* Helper functions */
static unsigned long long get_time_ns(void) {
    struct timespec64 ts;
    ktime_get_real_ts64(&ts);
    return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* File operations */
static int vns_open(struct inode *inode, struct file *filp) {
    if (!vns_state) {
        return -ENODEV;
    }
    
    mutex_lock(&vns_state->lock);
    vns_state->status_flags |= 0x01; /* Running */
    mutex_unlock(&vns_state->lock);
    
    pr_info("VNS: Device opened\n");
    return 0;
}

static int vns_release(struct inode *inode, struct file *filp) {
    if (!vns_state) {
        return -ENODEV;
    }
    
    mutex_lock(&vns_state->lock);
    vns_state->status_flags &= ~0x01; /* Not running */
    mutex_unlock(&vns_state->lock);
    
    pr_info("VNS: Device closed\n");
    return 0;
}

static ssize_t vns_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    char *buffer;
    int len;
    ssize_t ret;
    
    if (!vns_state) {
        return -ENODEV;
    }
    
    if (*f_pos > 0) {
        return 0; /* EOF */
    }
    
    mutex_lock(&vns_state->lock);
    
    buffer = kmalloc(512, GFP_KERNEL);
    if (!buffer) {
        mutex_unlock(&vns_state->lock);
        return -ENOMEM;
    }
    
    len = snprintf(buffer, 512,
        "VNS Control Interface\n"
        "====================\n"
        "Simulator Version: %s\n"
        "Status: %s\n"
        "Active Connections: %u\n"
        "NAT Table Entries: %u\n"
        "Port Forward Rules: %u\n"
        "Total Packets: %llu\n"
        "Successful: %llu\n"
        "Failed: %llu\n"
        "SNAT: %llu\n"
        "DNAT: %llu\n"
        "Reverse NAT: %llu\n"
        "Avg Processing Time: %llu ns\n",
        vns_state->simulator_version,
        (vns_state->status_flags & 0x01) ? "Running" : "Stopped",
        vns_state->active_connections,
        vns_state->nat_table_entries,
        vns_state->port_forward_rules,
        vns_state->total_packets,
        vns_state->successful_packets,
        vns_state->failed_packets,
        vns_state->snat_packets,
        vns_state->dnat_packets,
        vns_state->reverse_nat_packets,
        (vns_state->total_packets > 0) ? 
            (vns_state->total_processing_time_ns / vns_state->total_packets) : 0
    );
    
    mutex_unlock(&vns_state->lock);
    
    len = strlen(buffer);
    if (*f_pos >= len) {
        kfree(buffer);
        return 0;
    }
    
    if (count > len - *f_pos) {
        count = len - *f_pos;
    }
    
    if (copy_to_user(buf, buffer + *f_pos, count)) {
        kfree(buffer);
        return -EFAULT;
    }
    
    *f_pos += count;
    ret = count;
    kfree(buffer);
    return ret;
}

static ssize_t vns_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    char *buffer;
    int ret;
    
    if (!vns_state) {
        return -ENODEV;
    }
    
    if (count > 256) {
        return -EINVAL;
    }
    
    buffer = kmalloc(count + 1, GFP_KERNEL);
    if (!buffer) {
        return -ENOMEM;
    }
    
    if (copy_from_user(buffer, buf, count)) {
        kfree(buffer);
        return -EFAULT;
    }
    
    buffer[count] = '\0';
    
    /* Simple command parsing */
    mutex_lock(&vns_state->lock);
    if (strncmp(buffer, "reset", 5) == 0) {
        vns_state->total_packets = 0;
        vns_state->successful_packets = 0;
        vns_state->failed_packets = 0;
        vns_state->snat_packets = 0;
        vns_state->dnat_packets = 0;
        vns_state->reverse_nat_packets = 0;
        vns_state->total_processing_time_ns = 0;
        pr_info("VNS: Statistics reset via write\n");
        ret = count;
    } else if (strncmp(buffer, "status", 6) == 0) {
        vns_state->simulator_running = 1;
        vns_state->status_flags |= 0x01;
        pr_info("VNS: Simulator started via write\n");
        ret = count;
    } else if (strncmp(buffer, "stop", 4) == 0) {
        vns_state->simulator_running = 0;
        vns_state->status_flags &= ~0x01;
        pr_info("VNS: Simulator stopped via write\n");
        ret = count;
    } else {
        ret = -EINVAL;
    }
    mutex_unlock(&vns_state->lock);
    
    kfree(buffer);
    return ret;
}

static long vns_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    void __user *argp = (void __user *)arg;
    int ret = 0;
    
    if (!vns_state) {
        return -ENODEV;
    }
    
    switch (cmd) {
        case VNS_IOCTL_GET_STATS: {
            struct vns_stats stats;
            mutex_lock(&vns_state->lock);
            stats.total_packets = vns_state->total_packets;
            stats.successful_packets = vns_state->successful_packets;
            stats.failed_packets = vns_state->failed_packets;
            stats.snat_packets = vns_state->snat_packets;
            stats.dnat_packets = vns_state->dnat_packets;
            stats.reverse_nat_packets = vns_state->reverse_nat_packets;
            stats.active_nat_mappings = vns_state->nat_table_entries;
            stats.active_port_forward_rules = vns_state->port_forward_rules;
            stats.total_processing_time_ns = vns_state->total_processing_time_ns;
            mutex_unlock(&vns_state->lock);
            
            if (copy_to_user(argp, &stats, sizeof(stats))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case VNS_IOCTL_RESET_STATS: {
            mutex_lock(&vns_state->lock);
            vns_state->total_packets = 0;
            vns_state->successful_packets = 0;
            vns_state->failed_packets = 0;
            vns_state->snat_packets = 0;
            vns_state->dnat_packets = 0;
            vns_state->reverse_nat_packets = 0;
            vns_state->total_processing_time_ns = 0;
            mutex_unlock(&vns_state->lock);
            pr_info("VNS: Statistics reset via ioctl\n");
            break;
        }
        
        case VNS_IOCTL_GET_STATUS: {
            struct vns_status status;
            mutex_lock(&vns_state->lock);
            status.status_flags = vns_state->status_flags;
            status.active_connections = vns_state->active_connections;
            status.nat_table_entries = vns_state->nat_table_entries;
            status.port_forward_rules = vns_state->port_forward_rules;
            status.simulator_running = vns_state->simulator_running;
            strncpy(status.simulator_version, vns_state->simulator_version, 31);
            status.simulator_version[31] = '\0';
            mutex_unlock(&vns_state->lock);
            
            if (copy_to_user(argp, &status, sizeof(status))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case VNS_IOCTL_SET_STATUS: {
            struct vns_status status;
            if (copy_from_user(&status, argp, sizeof(status))) {
                ret = -EFAULT;
                break;
            }
            mutex_lock(&vns_state->lock);
            vns_state->status_flags = status.status_flags;
            vns_state->simulator_running = status.simulator_running;
            if (status.simulator_version[0]) {
                strncpy(vns_state->simulator_version, status.simulator_version, 31);
                vns_state->simulator_version[31] = '\0';
            }
            mutex_unlock(&vns_state->lock);
            break;
        }
        
        case VNS_IOCTL_GET_NAT_ENTRY: {
            /* In a real implementation, this would look up the NAT table */
            /* For now, return a placeholder */
            struct vns_nat_entry entry = {0};
            if (copy_to_user(argp, &entry, sizeof(entry))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case VNS_IOCTL_RESET_NAT: {
            mutex_lock(&vns_state->lock);
            vns_state->nat_table_entries = 0;
            vns_state->port_forward_rules = 0;
            mutex_unlock(&vns_state->lock);
            pr_info("VNS: NAT table reset via ioctl\n");
            break;
        }
        
        default:
            ret = -ENOTTY;
    }
    
    return ret;
}

static const struct file_operations vns_fops = {
    .owner = THIS_MODULE,
    .open = vns_open,
    .release = vns_release,
    .read = vns_read,
    .write = vns_write,
    .unlocked_ioctl = vns_ioctl,
    .compat_ioctl = vns_ioctl,
};

/* Module initialization */
static int __init vns_control_init(void) {
    int ret;
    dev_t dev_num;
    
    pr_info("VNS: Initializing Virtual NAT Gateway Simulator Control Module\n");
    
    /* Allocate driver state */
    vns_state = kmalloc(sizeof(struct vns_driver_state), GFP_KERNEL);
    if (!vns_state) {
        pr_err("VNS: Failed to allocate driver state\n");
        return -ENOMEM;
    }
    
    memset(vns_state, 0, sizeof(struct vns_driver_state));
    mutex_init(&vns_state->lock);
    strcpy(vns_state->simulator_version, "1.0.0");
    
    /* Allocate device number */
    ret = alloc_chrdev_region(&vns_state->dev_num, 0, 1, VNS_DEVICE_NAME);
    if (ret < 0) {
        pr_err("VNS: Failed to allocate device number\n");
        goto err_free_state;
    }
    
    /* Initialize cdev */
    cdev_init(&vns_state->cdev, &vns_fops);
    vns_state->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&vns_state->cdev, vns_state->dev_num, 1);
    if (ret < 0) {
        pr_err("VNS: Failed to add cdev\n");
        goto err_unregister_chrdev;
    }
    
    /* Create device class */
    vns_state->class = class_create(THIS_MODULE, VNS_CLASS_NAME);
    if (IS_ERR(vns_state->class)) {
        ret = PTR_ERR(vns_state->class);
        pr_err("VNS: Failed to create class\n");
        goto err_cdev_del;
    }
    
    /* Create device node */
    vns_state->device = device_create(vns_state->class, NULL, vns_state->dev_num, NULL, VNS_DEVICE_NAME);
    if (IS_ERR(vns_state->device)) {
        ret = PTR_ERR(vns_state->device);
        pr_err("VNS: Failed to create device\n");
        goto err_class_destroy;
    }
    
    pr_info("VNS: Device created at /dev/%s\n", VNS_DEVICE_NAME);
    pr_info("VNS: Major: %d, Minor: %d\n", MAJOR(vns_state->dev_num), MINOR(vns_state->dev_num));
    
    return 0;
    
err_class_destroy:
    class_destroy(vns_state->class);
err_cdev_del:
    cdev_del(&vns_state->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(vns_state->dev_num, 1);
err_free_state:
    kfree(vns_state);
    vns_state = NULL;
    return ret;
}

static void __exit vns_control_exit(void) {
    pr_info("VNS: Unloading Virtual NAT Gateway Simulator Control Module\n");
    
    if (vns_state) {
        device_destroy(vns_state->class, vns_state->dev_num);
        class_destroy(vns_state->class);
        cdev_del(&vns_state->cdev);
        unregister_chrdev_region(vns_state->dev_num, 1);
        kfree(vns_state);
        vns_state = NULL;
    }
    
    pr_info("VNS: Module unloaded\n");
}

module_init(vns_control_init);
module_exit(vns_control_exit);