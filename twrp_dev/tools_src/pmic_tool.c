/*
 * pmic_tool.c - PMIC register read/write tool for Intel Moorefield (Shadycove)
 * Uses /sys/kernel/pmic_debug/pmic_debug/ sysfs interface
 *
 * Usage:
 *   pmic_tool read <addr>          - Read PMIC register
 *   pmic_tool write <addr> <data>  - Write PMIC register
 *   pmic_tool dump <start> <end>   - Dump register range
 *
 * License: GPL-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define PMIC_ADDR "/sys/kernel/pmic_debug/pmic_debug/addr"
#define PMIC_DATA "/sys/kernel/pmic_debug/pmic_debug/data"
#define PMIC_OPS  "/sys/kernel/pmic_debug/pmic_debug/ops"

static int write_sysfs(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return -1;
    }
    int ret = write(fd, value, strlen(value));
    close(fd);
    if (ret < 0) {
        fprintf(stderr, "Error writing to %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int read_sysfs(const char *path, char *buf, size_t len)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return -1;
    }
    int ret = read(fd, buf, len - 1);
    close(fd);
    if (ret < 0) {
        fprintf(stderr, "Error reading from %s: %s\n", path, strerror(errno));
        return -1;
    }
    buf[ret] = '\0';
    /* strip trailing newline */
    while (ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r'))
        buf[--ret] = '\0';
    return 0;
}

static int pmic_read(unsigned int addr, unsigned int *data)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "0x%02x", addr);
    if (write_sysfs(PMIC_ADDR, buf) < 0)
        return -1;

    if (write_sysfs(PMIC_OPS, "read") < 0)
        return -1;

    if (read_sysfs(PMIC_DATA, buf, sizeof(buf)) < 0)
        return -1;

    *data = (unsigned int)strtoul(buf, NULL, 0);
    return 0;
}

static int pmic_write(unsigned int addr, unsigned int data)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "0x%02x", addr);
    if (write_sysfs(PMIC_ADDR, buf) < 0)
        return -1;

    snprintf(buf, sizeof(buf), "0x%02x", data);
    if (write_sysfs(PMIC_DATA, buf) < 0)
        return -1;

    if (write_sysfs(PMIC_OPS, "write") < 0)
        return -1;

    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr, "PMIC Tool for Intel Moorefield (Shadycove)\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s read <addr>          - Read register\n", prog);
    fprintf(stderr, "  %s write <addr> <data>  - Write register\n", prog);
    fprintf(stderr, "  %s dump <start> <end>   - Dump range\n", prog);
    fprintf(stderr, "\nAddresses as hex (0x..) or decimal\n");
    fprintf(stderr, "\nKnown registers (Shadycove PMIC):\n");
    fprintf(stderr, "  0x00-0x02  IRQLVL1/MIRQLVL1 (interrupts)\n");
    fprintf(stderr, "  0x18-0x1A  GPIO regs\n");
    fprintf(stderr, "  0x20       ADC control\n");
    fprintf(stderr, "  0x56-0x5B  Charger control (BQ24261)\n");
    fprintf(stderr, "  0x58       VBUS detection\n");
    fprintf(stderr, "  0x63-0x69  Backlight/LED\n");
    fprintf(stderr, "  0x6E-0x73  Voltage regulators\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "read") == 0) {
        unsigned int addr = (unsigned int)strtoul(argv[2], NULL, 0);
        unsigned int data;
        if (pmic_read(addr, &data) < 0)
            return 1;
        printf("PMIC[0x%02X] = 0x%02X (%u)\n", addr, data, data);

    } else if (strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Write requires: <addr> <data>\n");
            return 1;
        }
        unsigned int addr = (unsigned int)strtoul(argv[2], NULL, 0);
        unsigned int data = (unsigned int)strtoul(argv[3], NULL, 0);
        if (pmic_write(addr, data) < 0)
            return 1;
        printf("PMIC[0x%02X] <- 0x%02X OK\n", addr, data);

    } else if (strcmp(argv[1], "dump") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Dump requires: <start> <end>\n");
            return 1;
        }
        unsigned int start = (unsigned int)strtoul(argv[2], NULL, 0);
        unsigned int end = (unsigned int)strtoul(argv[3], NULL, 0);
        if (end < start) {
            fprintf(stderr, "End addr must be >= start addr\n");
            return 1;
        }
        printf("PMIC register dump 0x%02X - 0x%02X:\n", start, end);
        printf("ADDR  DATA\n");
        printf("----  ----\n");
        for (unsigned int a = start; a <= end; a++) {
            unsigned int data;
            if (pmic_read(a, &data) < 0) {
                printf("0x%02X  ERROR\n", a);
            } else {
                printf("0x%02X  0x%02X", a, data);
                /* show binary for register inspection */
                printf("  [");
                for (int b = 7; b >= 0; b--)
                    printf("%c", (data & (1 << b)) ? '1' : '0');
                printf("]");
                printf("\n");
            }
        }

    } else {
        usage(argv[0]);
        return 1;
    }

    return 0;
}
