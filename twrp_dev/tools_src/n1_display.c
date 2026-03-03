/*
 * n1_display.c - Nokia N1 Display Control Tool
 * Controls backlight, GPIO pins, and display panel via sysfs/devmem
 *
 * Usage:
 *   n1_display bl <0-255>             - Set backlight brightness
 *   n1_display bl                     - Read current brightness
 *   n1_display gpio <pin> [0|1]       - Read/set GPIO pin
 *   n1_display gpio export <pin>      - Export GPIO pin
 *   n1_display panel on               - Power on display panel
 *   n1_display panel off              - Power off display panel
 *   n1_display panel reset            - Reset display panel
 *   n1_display status                 - Show display status
 *
 * Key GPIOs for Nokia N1:
 *   189 - disp_bias_en (display bias enable)
 *   190 - disp0_rst (display reset, active low)
 *   191 - disp0_touch_rst (touch reset)
 *
 * License: GPL-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#define BL_PATH "/sys/class/backlight/psb-bl"
#define GPIO_PATH "/sys/class/gpio"

/* Nokia N1 display GPIOs */
#define GPIO_DISP_BIAS_EN   189
#define GPIO_DISP_RST       190
#define GPIO_TOUCH_RST      191

static int write_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    int ret = write(fd, value, strlen(value));
    close(fd);
    return (ret > 0) ? 0 : -1;
}

static int read_file(const char *path, char *buf, size_t len)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    int ret = read(fd, buf, len - 1);
    close(fd);
    if (ret < 0) return -1;
    buf[ret] = '\0';
    while (ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r'))
        buf[--ret] = '\0';
    return 0;
}

static int read_int(const char *path)
{
    char buf[32];
    if (read_file(path, buf, sizeof(buf)) < 0) return -1;
    return atoi(buf);
}

/* === Backlight === */
static int cmd_backlight(int argc, char *argv[])
{
    if (argc < 3) {
        /* Read current */
        int cur = read_int(BL_PATH "/brightness");
        int max_val = read_int(BL_PATH "/max_brightness");
        int actual = read_int(BL_PATH "/actual_brightness");
        printf("Backlight:\n");
        printf("  brightness:        %d\n", cur);
        printf("  actual_brightness: %d\n", actual);
        printf("  max_brightness:    %d\n", max_val);
        return 0;
    }

    int val = atoi(argv[2]);
    if (val < 0) val = 0;
    if (val > 255) val = 255;

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    if (write_file(BL_PATH "/brightness", buf) < 0) {
        fprintf(stderr, "Failed to set backlight\n");
        return 1;
    }
    printf("Backlight set to %d/255\n", val);
    return 0;
}

/* === GPIO === */
static int gpio_export(int pin)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", pin);
    return write_file(GPIO_PATH "/export", buf);
}

static int gpio_direction(int pin, const char *dir)
{
    char path[128];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", pin);
    return write_file(path, dir);
}

static int gpio_set(int pin, int value)
{
    char path[128];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    return write_file(path, value ? "1" : "0");
}

static int gpio_get(int pin)
{
    char path[128];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    return read_int(path);
}

static int cmd_gpio(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: n1_display gpio <pin> [0|1]\n");
        fprintf(stderr, "       n1_display gpio export <pin>\n");
        return 1;
    }

    if (strcmp(argv[2], "export") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: n1_display gpio export <pin>\n");
            return 1;
        }
        int pin = atoi(argv[3]);
        if (gpio_export(pin) < 0) {
            fprintf(stderr, "Failed to export GPIO %d\n", pin);
            return 1;
        }
        printf("GPIO %d exported\n", pin);
        return 0;
    }

    int pin = atoi(argv[2]);
    if (argc >= 4) {
        /* Set */
        int val = atoi(argv[3]);
        gpio_direction(pin, "out");
        if (gpio_set(pin, val) < 0) {
            fprintf(stderr, "Failed to set GPIO %d\n", pin);
            return 1;
        }
        printf("GPIO %d = %d\n", pin, val);
    } else {
        /* Read */
        int val = gpio_get(pin);
        if (val < 0) {
            fprintf(stderr, "Failed to read GPIO %d (not exported?)\n", pin);
            return 1;
        }
        printf("GPIO %d = %d\n", pin, val);
    }
    return 0;
}

/* === Panel control === */
static void msleep(int ms)
{
    usleep(ms * 1000);
}

static int panel_ensure_gpios(void)
{
    int gpios[] = { GPIO_DISP_BIAS_EN, GPIO_DISP_RST, GPIO_TOUCH_RST };
    char path[128];

    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof(path), GPIO_PATH "/gpio%d", gpios[i]);
        if (access(path, F_OK) != 0) {
            printf("Exporting GPIO %d...\n", gpios[i]);
            gpio_export(gpios[i]);
            msleep(50);
        }
        gpio_direction(gpios[i], "out");
    }
    return 0;
}

static int cmd_panel(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: n1_display panel <on|off|reset>\n");
        return 1;
    }

    panel_ensure_gpios();

    if (strcmp(argv[2], "on") == 0) {
        printf("Panel power on sequence:\n");

        printf("  1. Assert reset (LOW)\n");
        gpio_set(GPIO_DISP_RST, 0);
        msleep(10);

        printf("  2. Enable bias\n");
        gpio_set(GPIO_DISP_BIAS_EN, 1);
        msleep(20);

        printf("  3. Deassert reset (HIGH)\n");
        gpio_set(GPIO_DISP_RST, 1);
        msleep(50);

        printf("  4. Deassert touch reset\n");
        gpio_set(GPIO_TOUCH_RST, 1);
        msleep(20);

        printf("  5. Setting backlight to max\n");
        write_file(BL_PATH "/brightness", "255");

        printf("Panel ON sequence complete.\n");
        printf("Note: DSI init commands must be sent by the display driver.\n");

    } else if (strcmp(argv[2], "off") == 0) {
        printf("Panel power off sequence:\n");

        printf("  1. Backlight off\n");
        write_file(BL_PATH "/brightness", "0");
        msleep(10);

        printf("  2. Assert reset\n");
        gpio_set(GPIO_DISP_RST, 0);
        msleep(10);

        printf("  3. Disable bias\n");
        gpio_set(GPIO_DISP_BIAS_EN, 0);
        msleep(10);

        printf("  4. Assert touch reset\n");
        gpio_set(GPIO_TOUCH_RST, 0);

        printf("Panel OFF complete.\n");

    } else if (strcmp(argv[2], "reset") == 0) {
        printf("Panel reset sequence:\n");

        printf("  1. Assert reset\n");
        gpio_set(GPIO_DISP_RST, 0);
        msleep(20);

        printf("  2. Deassert reset\n");
        gpio_set(GPIO_DISP_RST, 1);
        msleep(100);

        printf("Panel reset complete. DSI re-init may be needed.\n");

    } else {
        fprintf(stderr, "Unknown panel command: %s\n", argv[2]);
        return 1;
    }

    return 0;
}

/* === Status === */
static int cmd_status(void)
{
    printf("=== Nokia N1 Display Status ===\n\n");

    /* Backlight */
    int bl = read_int(BL_PATH "/brightness");
    int bl_max = read_int(BL_PATH "/max_brightness");
    int bl_actual = read_int(BL_PATH "/actual_brightness");
    printf("Backlight: %d/%d (actual: %d)\n", bl, bl_max, bl_actual);

    /* GPIOs */
    printf("\nDisplay GPIOs:\n");
    int gpios[] = { GPIO_DISP_BIAS_EN, GPIO_DISP_RST, GPIO_TOUCH_RST };
    const char *names[] = { "disp_bias_en", "disp0_rst", "disp0_touch_rst" };
    char path[128], buf[64];

    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", gpios[i]);
        int val = read_int(path);
        snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", gpios[i]);
        if (read_file(path, buf, sizeof(buf)) < 0)
            strcpy(buf, "N/A");
        printf("  GPIO-%d (%s): %s val=%d\n", gpios[i], names[i], buf,
               val >= 0 ? val : -1);
    }

    /* DRM connectors */
    printf("\nDRM status:\n");
    char *connectors[] = {
        "/sys/class/drm/card0-DSI-1/status",
        "/sys/class/drm/card0-DSI-2/status",
        NULL
    };
    for (int i = 0; connectors[i]; i++) {
        if (read_file(connectors[i], buf, sizeof(buf)) == 0)
            printf("  %s: %s\n", connectors[i], buf);
    }

    /* Framebuffer */
    printf("\nFramebuffer:\n");
    if (read_file("/sys/class/graphics/fb0/virtual_size", buf, sizeof(buf)) == 0)
        printf("  fb0 virtual_size: %s\n", buf);
    if (read_file("/sys/class/graphics/fb0/bits_per_pixel", buf, sizeof(buf)) == 0)
        printf("  fb0 bpp: %s\n", buf);

    /* GPU power state */
    printf("\nGPU:\n");
    if (read_file("/sys/class/drm/card0/power/runtime_status", buf, sizeof(buf)) == 0)
        printf("  runtime_status: %s\n", buf);

    return 0;
}

/* === Main === */
static void usage(const char *prog)
{
    printf("Nokia N1 Display Control Tool\n\n");
    printf("Usage:\n");
    printf("  %s bl [0-255]              Set/read backlight\n", prog);
    printf("  %s gpio <pin> [0|1]        Read/set GPIO\n", prog);
    printf("  %s gpio export <pin>       Export GPIO pin\n", prog);
    printf("  %s panel <on|off|reset>    Panel power control\n", prog);
    printf("  %s status                  Show display status\n", prog);
    printf("\nKey GPIOs:\n");
    printf("  189 = disp_bias_en\n");
    printf("  190 = disp0_rst (active low)\n");
    printf("  191 = disp0_touch_rst\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "bl") == 0)
        return cmd_backlight(argc, argv);
    else if (strcmp(argv[1], "gpio") == 0)
        return cmd_gpio(argc, argv);
    else if (strcmp(argv[1], "panel") == 0)
        return cmd_panel(argc, argv);
    else if (strcmp(argv[1], "status") == 0)
        return cmd_status();
    else {
        usage(argv[0]);
        return 1;
    }
}
