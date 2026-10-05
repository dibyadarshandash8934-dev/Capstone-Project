#ifndef VNS_IOCTL_H
#define VNS_IOCTL_H

#include <linux/ioctl.h>

/* VNS magic number */
#define VNS_IOCTL_MAGIC 'V'

/* IOCTL commands */
#define VNS_IOCTL_GET_STATS     _IOR(VNS_IOCTL_MAGIC, 1, struct vns_stats)
#define VNS_IOCTL_RESET_STATS   _IO(VNS_IOCTL_MAGIC, 2)
#define VNS_IOCTL_GET_STATUS    _IOR(VNS_IOCTL_MAGIC, 3, struct vns_status)
#define VNS_IOCTL_SET_STATUS    _IOW(VNS_IOCTL_MAGIC, 4, struct vns_status)
#define VNS_IOCTL_GET_NAT_ENTRY _IOR(VNS_IOCTL_MAGIC, 5, struct vns_nat_entry)
#define VNS_IOCTL_RESET_NAT     _IO(VNS_IOCTL_MAGIC, 6)

/* Maximum lengths */
#define VNS_MAX_HOSTNAME_LEN    64
#define VNS_MAX_IP_STR_LEN      16

/* Protocol types */
#define VNS_PROTO_TCP   6
#define VNS_PROTO_UDP   17
#define VNS_PROTO_ICMP  1

/* NAT states */
#define VNS_NAT_NEW       0
#define VNS_NAT_ACTIVE    1
#define VNS_NAT_ESTABLISHED 2
#define VNS_NAT_CLOSED    3
#define VNS_NAT_EXPIRED   4

/* Driver status flags */
#define VNS_STATUS_RUNNING    0x01
#define VNS_STATUS_ERROR      0x02

/* Statistics structure */
struct vns_stats {
    unsigned long long total_packets;
    unsigned long long successful_packets;
    unsigned long long failed_packets;
    unsigned long long snat_packets;
    unsigned long long dnat_packets;
    unsigned long long reverse_nat_packets;
    unsigned long long active_nat_mappings;
    unsigned long long active_port_forward_rules;
    unsigned long long total_processing_time_ns;
};

/* Driver status structure */
struct vns_status {
    unsigned int status_flags;
    unsigned int active_connections;
    unsigned int nat_table_entries;
    unsigned int port_forward_rules;
    unsigned int simulator_running;
    char simulator_version[32];
};

/* NAT entry structure for kernel/user communication */
struct vns_nat_entry {
    unsigned char protocol;
    unsigned int private_ip;
    unsigned short private_port;
    unsigned int public_ip;
    unsigned short public_port;
    unsigned int destination_ip;
    unsigned short destination_port;
    unsigned char state;
    unsigned long long created_time;
    unsigned long long last_activity;
};

#endif /* VNS_IOCTL_H */