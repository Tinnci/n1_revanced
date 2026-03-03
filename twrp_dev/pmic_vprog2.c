/*
 * pmic_vprog2 - Enable/disable VPROG2 regulator via Intel SCU IPC
 * Usage: pmic_vprog2 <on|off>
 *
 * Uses /dev/mid_ipc ioctl to write PMIC registers.
 * Tries multiple PMIC types (Shadycove: 0xAD, Whiskey Cove: 0xD7)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

/* Intel SCU IPC structures - from Intel MID IPC driver */
struct scu_ipc_data {
    uint32_t count;    /* number of registers */
    uint16_t addr[5];  /* register addresses */
    uint8_t  data[5];  /* register data */
};

/* ioctl commands for /dev/mid_ipc */
/* Based on Linux _IOW macro: _IOW('I', cmd, struct scu_ipc_data) */
/* sizeof(struct scu_ipc_data) = 4 + 10 + 5 = 19, padded to 20 */
/* _IOW = (1<<30) | (size<<16) | (type<<8) | nr */
#define IPC_REGISTER_READ    (0x40000000 | (20 << 16) | ('I' << 8) | 0)
#define IPC_REGISTER_WRITE   (0x40000000 | (20 << 16) | ('I' << 8) | 1)
#define IPC_REGISTER_UPDATE  (0x40000000 | (20 << 16) | ('I' << 8) | 2)

static int ipc_read(int fd, uint16_t addr, uint8_t *val)
{
    struct scu_ipc_data d;
    memset(&d, 0, sizeof(d));
    d.count = 1;
    d.addr[0] = addr;
    d.data[0] = 0;

    /* For read, use _IOWR instead: both read and write direction */
    /* _IOWR = (3<<30) | (size<<16) | (type<<8) | nr */
    unsigned long cmd = (3UL << 30) | (20UL << 16) | ('I' << 8) | 0;
    int ret = ioctl(fd, cmd, &d);
    if (ret == 0)
        *val = d.data[0];
    return ret;
}

static int ipc_write(int fd, uint16_t addr, uint8_t val)
{
    struct scu_ipc_data d;
    memset(&d, 0, sizeof(d));
    d.count = 1;
    d.addr[0] = addr;
    d.data[0] = val;

    int ret = ioctl(fd, IPC_REGISTER_WRITE, &d);
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <on|off|read|dump>\n", argv[0]);
        return 1;
    }

    int fd = open("/dev/mid_ipc", O_RDWR);
    if (fd < 0) {
        perror("open /dev/mid_ipc");
        return 1;
    }

    if (strcmp(argv[1], "dump") == 0) {
        /* Dump all potentially relevant PMIC registers */
        uint16_t regs[] = {
            0xAB, 0xAC, 0xAD, /* Shadycove VPROG1/2/3 */
            0xD5, 0xD6, 0xD7, /* Whiskey Cove VPROG1/2/3 */
            0x141, 0x151,      /* Rev 8 PMIC VPROG2 regs */
        };
        for (int i = 0; i < sizeof(regs)/sizeof(regs[0]); i++) {
            uint8_t val = 0;
            int ret = ipc_read(fd, regs[i], &val);
            printf("PMIC reg 0x%04X: 0x%02X (ret=%d)\n", regs[i], val, ret);
        }
        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s read <hex_addr>\n", argv[0]);
            close(fd);
            return 1;
        }
        uint16_t addr = (uint16_t)strtoul(argv[2], NULL, 16);
        uint8_t val = 0;
        int ret = ipc_read(fd, addr, &val);
        printf("PMIC reg 0x%04X: 0x%02X (ret=%d)\n", addr, val, ret);
        close(fd);
        return ret;
    }

    int on = (strcmp(argv[1], "on") == 0) ? 1 : 0;

    /* Try Shadycove PMIC first (type 4): register 0xAD */
    /* ON: 0xC1 (enable + voltage), OFF: 0x00 */
    uint8_t current_val = 0;
    int ret;

    /* Read current value first */
    ret = ipc_read(fd, 0xAD, &current_val);
    printf("Shadycove VPROG2 reg 0xAD current: 0x%02X (ret=%d)\n",
           current_val, ret);

    if (ret == 0) {
        uint8_t new_val = on ? 0xC1 : 0x00;
        printf("Writing 0x%02X to reg 0xAD...\n", new_val);
        ret = ipc_write(fd, 0xAD, new_val);
        printf("Write result: %d\n", ret);

        /* Verify */
        ret = ipc_read(fd, 0xAD, &current_val);
        printf("After write: 0x%02X (ret=%d)\n", current_val, ret);
    }

    /* Also try Whiskey Cove PMIC: register 0xD7 */
    ret = ipc_read(fd, 0xD7, &current_val);
    printf("\nWhiskey Cove VPROG2 reg 0xD7 current: 0x%02X (ret=%d)\n",
           current_val, ret);

    if (ret == 0 && argc > 2 && strcmp(argv[2], "--all") == 0) {
        uint8_t new_val = on ? 0x36 : 0x24;
        printf("Writing 0x%02X to reg 0xD7...\n", new_val);
        ret = ipc_write(fd, 0xD7, new_val);
        printf("Write result: %d\n", ret);
    }

    close(fd);
    printf("\nDone. Check regulator state:\n");
    printf("  cat /sys/class/regulator/regulator.4/state\n");
    return 0;
}
