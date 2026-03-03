/*
 * Minimal PMIC VPROG2 control - no libc, raw syscalls only
 * Compile: gcc -static -nostdlib -nostartfiles -march=bonnell -o pmic_vprog2 pmic_vprog2_raw.c
 */

/* x86_64 syscall numbers */
#define SYS_read    0
#define SYS_write   1
#define SYS_open    2
#define SYS_close   3
#define SYS_ioctl   16
#define SYS_exit    60

/* Open flags */
#define O_RDWR      2

static long syscall1(long nr, long a1) {
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static long syscall2(long nr, long a1, long a2) {
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}

static long syscall3(long nr, long a1, long a2, long a3) {
    long ret;
    register long r10 __asm__("r10") = 0;
    (void)r10;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

static int my_open(const char *path, int flags) {
    return (int)syscall2(SYS_open, (long)path, (long)flags);
}

static int my_close(int fd) {
    return (int)syscall1(SYS_close, (long)fd);
}

static int my_ioctl(int fd, unsigned long cmd, void *arg) {
    return (int)syscall3(SYS_ioctl, (long)fd, (long)cmd, (long)arg);
}

static void my_write(int fd, const char *buf, int len) {
    syscall3(SYS_write, (long)fd, (long)buf, (long)len);
}

static void my_exit(int code) {
    syscall1(SYS_exit, (long)code);
}

/* Simple string output */
static int my_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void puts_str(const char *s) {
    my_write(1, s, my_strlen(s));
}

/* Convert byte to hex string */
static void byte_to_hex(unsigned char b, char *out) {
    const char hex[] = "0123456789ABCDEF";
    out[0] = hex[b >> 4];
    out[1] = hex[b & 0xF];
}

static void print_hex_byte(unsigned char b) {
    char buf[5] = "0x";
    byte_to_hex(b, buf + 2);
    buf[4] = '\n';
    my_write(1, buf, 5);
}

/* SFI IPC data structure */
struct scu_ipc_data {
    unsigned int count;
    unsigned short addr[5];
    unsigned char data[5];
} __attribute__((packed));

/* ioctl commands: Raw numbers from kernel disassembly of scu_ipc_ioctl
 * The handler does: ebx = cmd - 0xB0, then switch on ebx
 * So cmd 0xB0 = case 0, 0xB1 = case 1, 0xB8 = case 8
 * These are NOT encoded with _IOC macro in this Intel BSP! */
#define IPC_READ   0xB0
#define IPC_WRITE  0xB1

/* Alternate: try _IOC encoded versions too */
#define IPC_DATA_SIZE 19
#define IPC_READ_IOC   ((3UL << 30) | ((unsigned long)IPC_DATA_SIZE << 16) | ('I' << 8) | 0)
#define IPC_WRITE_IOC  ((1UL << 30) | ((unsigned long)IPC_DATA_SIZE << 16) | ('I' << 8) | 1)

static void zero_mem(void *p, int n) {
    char *c = (char *)p;
    for (int i = 0; i < n; i++) c[i] = 0;
}

static int ipc_read_reg(int fd, unsigned short addr, unsigned char *val) {
    struct scu_ipc_data d;
    zero_mem(&d, sizeof(d));
    d.count = 1;
    d.addr[0] = addr;
    
    int ret = my_ioctl(fd, IPC_READ, &d);
    if (ret == -22) {  /* EINVAL - try alternate size */
        zero_mem(&d, sizeof(d));
        d.count = 1;
        d.addr[0] = addr;
        ret = my_ioctl(fd, IPC_READ2, &d);
    }
    *val = d.data[0];
    return ret;
}

static int ipc_write_reg(int fd, unsigned short addr, unsigned char val) {
    struct scu_ipc_data d;
    zero_mem(&d, sizeof(d));
    d.count = 1;
    d.addr[0] = addr;
    d.data[0] = val;
    
    int ret = my_ioctl(fd, IPC_WRITE, &d);
    if (ret == -22) {  /* EINVAL - try alternate size */
        zero_mem(&d, sizeof(d));
        d.count = 1;
        d.addr[0] = addr;
        d.data[0] = val;
        ret = my_ioctl(fd, IPC_WRITE2, &d);
    }
    return ret;
}

static void print_reg(int fd, const char *name, unsigned short addr) {
    unsigned char val = 0;
    int ret = ipc_read_reg(fd, addr, &val);
    puts_str(name);
    puts_str(" (0x");
    char abuf[5];
    byte_to_hex((addr >> 8) & 0xFF, abuf);
    byte_to_hex(addr & 0xFF, abuf + 2);
    abuf[4] = 0;
    puts_str(abuf);
    puts_str("): ");
    if (ret == 0) {
        print_hex_byte(val);
    } else {
        puts_str("err=");
        /* print signed int */
        char nbuf[12];
        int neg = (ret < 0);
        int v = neg ? -ret : ret;
        int i = 0;
        do { nbuf[i++] = '0' + (v % 10); v /= 10; } while (v > 0);
        if (neg) nbuf[i++] = '-';
        /* reverse */
        for (int j = 0; j < i/2; j++) {
            char t = nbuf[j]; nbuf[j] = nbuf[i-1-j]; nbuf[i-1-j] = t;
        }
        nbuf[i++] = '\n';
        my_write(1, nbuf, i);
    }
}

void _start(void) {
    int fd = my_open("/dev/mid_ipc", O_RDWR);
    if (fd < 0) {
        puts_str("Cannot open /dev/mid_ipc\n");
        my_exit(1);
    }

    puts_str("=== PMIC Register Dump ===\n");
    /* Shadycove PMIC */
    print_reg(fd, "VPROG1 Shady", 0xAB);
    print_reg(fd, "VPROG2 Shady", 0xAC);
    print_reg(fd, "VPROG2 Shady", 0xAD);
    print_reg(fd, "VPROG3 Shady", 0xAE);
    /* Whiskey Cove */
    print_reg(fd, "VPROG1 Whisk", 0xD5);
    print_reg(fd, "VPROG2 Whisk", 0xD6);
    print_reg(fd, "VPROG2 Whisk", 0xD7);
    /* Rev 8 */
    print_reg(fd, "VPROG2 Rev8a", 0x141);
    print_reg(fd, "VPROG2 Rev8b", 0x151);

    puts_str("\n=== Trying to enable VPROG2 ===\n");
    
    /* Try Shadycove: write 0xC1 to 0xAD */
    puts_str("Writing 0xC1 to 0xAD (Shadycove)...\n");
    int ret = ipc_write_reg(fd, 0xAD, 0xC1);
    if (ret == 0) {
        puts_str("OK!\n");
    } else {
        puts_str("Failed, trying 0xD7 (WhiskeyCove)...\n");
        ret = ipc_write_reg(fd, 0xD7, 0x36);
        if (ret == 0) {
            puts_str("OK!\n");
        } else {
            puts_str("Failed both\n");
        }
    }
    
    /* Read back */
    puts_str("\n=== After write ===\n");
    print_reg(fd, "VPROG2 0xAD ", 0xAD);
    print_reg(fd, "VPROG2 0xD7 ", 0xD7);

    my_close(fd);
    my_exit(0);
}
