/*
 * Stylus Bridge - translates KEY_PAGEUP/KEY_PAGEDOWN from
 * Xiaomi Smart Pen BLE keyboard into configurable output
 * via uinput.
 *
 * Default mode: BTN_STYLUS/BTN_STYLUS2 for fusion with NVTCapacitivePen.
 * Other modes: volume, dpad up/down, media next/prev.
 *
 * Mode is read from persist.vendor.stylus.button_mode property.
 * Device matched by vendor/product ID (not name).
 *
 * Uses inotify to wait for pen — zero CPU when pen absent.
 * read() blocks when pen idle — zero CPU when no buttons pressed.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <cutils/properties.h>

#define LOG_TAG "StylusBridge"
#include <log/log.h>

#define DEVICE_NAME       "xiaomi-stylus-bridge"
#define INPUT_DIR         "/dev/input"
#define MAX_PATH          256
#define PROP_BUTTON_MODE  "persist.vendor.stylus.button_mode"

/* Xiaomi Smart Pen BLE identifiers */
#define PEN_VENDOR     0x1915
#define PEN_PRODUCT_V1 0xEAEA
#define PEN_PRODUCT_V2 0x4D81

/* Button modes */
#define MODE_DEFAULT 0  /* BTN_STYLUS / BTN_STYLUS2 */
#define MODE_UPDOWN  1  /* KEY_UP / KEY_DOWN */
#define MODE_VOLUME  2  /* KEY_VOLUMEUP / KEY_VOLUMEDOWN */
#define MODE_MUSIC   3  /* KEY_NEXTSONG / KEY_PREVIOUSSONG */

static void signal_handler(int sig) {
    ALOGE("Caught signal %d, exiting", sig);
    _exit(sig);
}

/* Mode strings must match arrays.xml stylus_button_values in pad_parts */

static int is_pen_device(int fd) {
    struct input_id id;
    if (ioctl(fd, EVIOCGID, &id) < 0)
        return 0;
    return id.vendor == PEN_VENDOR &&
           (id.product == PEN_PRODUCT_V1 || id.product == PEN_PRODUCT_V2);
}

static int try_open_pen(void) {
    DIR *dir = opendir(INPUT_DIR);
    if (!dir)
        return -1;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0)
            continue;

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", INPUT_DIR, ent->d_name);

        int fd = open(path, O_RDONLY);
        if (fd < 0)
            continue;

        if (is_pen_device(fd)) {
            closedir(dir);
            int grabbed = ioctl(fd, EVIOCGRAB, 1) == 0;
            if (!grabbed)
                ALOGE("EVIOCGRAB failed: %s", strerror(errno));
            ALOGI("Found pen: %s (%s)", path, grabbed ? "grabbed" : "not grabbed");
            return fd;
        }
        close(fd);
    }
    closedir(dir);
    return -1;
}

static int wait_for_pen(void) {
    int fd = try_open_pen();
    if (fd >= 0)
        return fd;

    ALOGI("Waiting for pen (inotify)...");

    int ifd = inotify_init();
    if (ifd < 0) {
        ALOGE("inotify_init failed: %s", strerror(errno));
        return -1;
    }

    int wd = inotify_add_watch(ifd, INPUT_DIR, IN_CREATE);
    if (wd < 0) {
        ALOGE("inotify_add_watch failed: %s", strerror(errno));
        close(ifd);
        return -1;
    }

    char buf[512];
    while (1) {
        int len = read(ifd, buf, sizeof(buf));
        if (len <= 0)
            break;

        usleep(200000);

        fd = try_open_pen();
        if (fd >= 0) {
            inotify_rm_watch(ifd, wd);
            close(ifd);
            return fd;
        }
    }

    close(ifd);
    return -1;
}

static int get_mode(void) {
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get(PROP_BUTTON_MODE, value, "default");
    if (strcmp(value, "updown") == 0) return MODE_UPDOWN;
    if (strcmp(value, "volume") == 0) return MODE_VOLUME;
    if (strcmp(value, "music") == 0)  return MODE_MUSIC;
    return MODE_DEFAULT;
}

static int create_uinput_device(int mode) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Cannot open /dev/uinput: %s", strerror(errno));
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    if (mode == MODE_DEFAULT) {
        ioctl(fd, UI_SET_EVBIT, EV_ABS);
        ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS);
        ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2);
        ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);
    } else {
        ioctl(fd, UI_SET_KEYBIT, KEY_UP);
        ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
        ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEUP);
        ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
        ioctl(fd, UI_SET_KEYBIT, KEY_NEXTSONG);
        ioctl(fd, UI_SET_KEYBIT, KEY_PREVIOUSSONG);
    }

    struct uinput_user_dev udev;
    memset(&udev, 0, sizeof(udev));
    strncpy(udev.name, DEVICE_NAME, UINPUT_MAX_NAME_SIZE - 1);
    udev.id.bustype = BUS_VIRTUAL;
    udev.id.vendor  = PEN_VENDOR;
    udev.id.product = 0xEAEB;
    udev.id.version = 1;

    if (mode == MODE_DEFAULT) {
        udev.absmin[ABS_PRESSURE] = 0;
        udev.absmax[ABS_PRESSURE] = 255;
    }

    if (write(fd, &udev, sizeof(udev)) < 0) {
        ALOGE("write uinput_user_dev failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        ALOGE("UI_DEV_CREATE failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static void send_event(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    (void)write(fd, &ev, sizeof(ev));
}

static void get_keys_for_mode(int mode, int *up_key, int *down_key) {
    switch (mode) {
    case MODE_UPDOWN:
        *up_key = KEY_UP;        *down_key = KEY_DOWN;          break;
    case MODE_VOLUME:
        *up_key = KEY_VOLUMEUP;  *down_key = KEY_VOLUMEDOWN;    break;
    case MODE_MUSIC:
        *up_key = KEY_NEXTSONG;  *down_key = KEY_PREVIOUSSONG;  break;
    default:
        *up_key = BTN_STYLUS2;   *down_key = BTN_STYLUS;        break;
    }
}

int main(void) {
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);

    ALOGI("Starting (pid=%d)", getpid());

    while (1) {
        int evfd = wait_for_pen();
        if (evfd < 0) {
            ALOGE("Failed to find pen, retrying...");
            sleep(3);
            continue;
        }

        int current_mode = get_mode();
        int uifd = create_uinput_device(current_mode);
        if (uifd < 0) {
            close(evfd);
            ALOGE("uinput failed, retrying...");
            sleep(3);
            continue;
        }

        ALOGI("Bridging active (mode=%d)", current_mode);

        struct input_event ev;
        ssize_t n;
        while ((n = read(evfd, &ev, sizeof(ev))) == sizeof(ev)) {
            if (ev.type != EV_KEY)
                continue;

            /* Check if mode changed */
            int mode = get_mode();
            if (mode != current_mode) {
                ALOGI("Mode changed %d -> %d, recreating uinput", current_mode, mode);
                ioctl(uifd, UI_DEV_DESTROY);
                close(uifd);
                uifd = create_uinput_device(mode);
                if (uifd < 0) {
                    ALOGE("uinput recreate failed, reconnecting");
                    break; /* outer cleanup guards uifd < 0 */
                }
                current_mode = mode;
                usleep(100000); /* let Android register new device */
            }

            int up_key, down_key;
            get_keys_for_mode(current_mode, &up_key, &down_key);

            int code = -1;
            if (ev.code == KEY_PAGEUP)
                code = up_key;
            else if (ev.code == KEY_PAGEDOWN)
                code = down_key;

            if (code < 0)
                continue;

            send_event(uifd, EV_KEY, code, ev.value);
            if (current_mode == MODE_DEFAULT)
                send_event(uifd, EV_ABS, ABS_PRESSURE, ev.value ? 255 : 0);
            send_event(uifd, EV_SYN, SYN_REPORT, 0);
        }

        ALOGI("Read loop exited: n=%zd errno=%d (%s)", n, errno, strerror(errno));
        if (uifd >= 0) {
            ioctl(uifd, UI_DEV_DESTROY);
            close(uifd);
        }
        close(evfd);
    }

    return 0;
}
