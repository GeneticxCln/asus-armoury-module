// SPDX-License-Identifier: GPL-2.0
/*
 * Universal Laptop Armoury Kernel Module
 * Support for ASUS, MSI, Alienware, Lenovo Legion and other gaming laptop features
 *
 * Copyright (C) 2025
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/leds.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/dmi.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>

#define DRIVER_NAME "universal-armoury"
#define DRIVER_VERSION "2.0.0"

/* ACPI method names - ASUS */
#define ASUS_ACPI_GET_BIOS_SETTINGS    "GBMD"
#define ASUS_ACPI_SET_BIOS_SETTINGS    "SBMD"
#define ASUS_ACPI_GET_GPU_MUX_STATE    "MXDS"
#define ASUS_ACPI_SET_GPU_MUX_STATE    "MXDM"
#define ASUS_ACPI_GET_DGPU_DISABLE     "DGPU"
#define ASUS_ACPI_SET_DGPU_DISABLE     "SDGP"
#define ASUS_ACPI_GET_EGPU_ENABLE      "EGPU"
#define ASUS_ACPI_SET_EGPU_ENABLE      "SEGP"
#define ASUS_ACPI_GET_GPU_STATE        "GPUS"
#define ASUS_ACPI_SET_GPU_STATE        "SGPU"

/* ACPI method names - MSI */
#define MSI_ACPI_GET_GPU_MUX_STATE     "GMUX"
#define MSI_ACPI_SET_GPU_MUX_STATE     "SMUX"
#define MSI_ACPI_GET_DGPU_DISABLE      "GDIS"
#define MSI_ACPI_SET_DGPU_DISABLE      "SDIS"

/* ACPI method names - Alienware/Dell */
#define DELL_ACPI_GET_GPU_MUX_STATE    "GFXS"
#define DELL_ACPI_SET_GPU_MUX_STATE    "SFXS"
#define DELL_ACPI_GET_DGPU_DISABLE     "GDDS"
#define DELL_ACPI_SET_DGPU_DISABLE     "SDDS"

/* ACPI method names - Lenovo */
#define LENOVO_ACPI_GET_GPU_MUX_STATE  "LGPU"
#define LENOVO_ACPI_SET_GPU_MUX_STATE  "SLGP"
#define LENOVO_ACPI_GET_DGPU_DISABLE   "LDGP"
#define LENOVO_ACPI_SET_DGPU_DISABLE   "SLDG"

/* Generic ACPI method names */
#define GENERIC_ACPI_GET_GPU_STATE     "_GPU"
#define GENERIC_ACPI_SET_GPU_STATE     "SGPU"
#define GENERIC_ACPI_GET_MUX_STATE     "GMUX"
#define GENERIC_ACPI_SET_MUX_STATE     "SMUX"

/* Device IDs - Expanded for multiple brands */
static const struct acpi_device_id universal_armoury_device_ids[] = {
    /* ASUS devices */
    {"ATK4001", 0},
    {"ATK4002", 0},
    {"ASUS0010", 0},
    {"ASUSTEK", 0},
    /* MSI devices */
    {"MSI0001", 0},
    {"MICRO-STAR", 0},
    {"MSG0001", 0},
    /* Dell/Alienware devices */
    {"DELL0001", 0},
    {"AWK0001", 0},
    {"ALIW0001", 0},
    /* Lenovo devices */
    {"LCFC0001", 0},
    {"LEN0001", 0},
    {"IDEA0001", 0},
    /* HP devices */
    {"HPQ0001", 0},
    {"HPQI0001", 0},
    /* Acer devices */
    {"ACER0001", 0},
    {"ACR0001", 0},
    /* Generic ACPI devices */
    {"PNP0C02", 0},
    {"ACPI0003", 0},
    {"", 0},
};
MODULE_DEVICE_TABLE(acpi, universal_armoury_device_ids);

/* DMI matching for supported systems - Expanded */
static const struct dmi_system_id universal_armoury_quirks[] = {
    /* ASUS systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK COMPUTER INC."),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "ASUS"),
        },
    },
    /* MSI systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Micro-Star International Co., Ltd."),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "MSI"),
        },
    },
    /* Dell/Alienware systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Dell Inc."),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Alienware"),
        },
    },
    /* Lenovo systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Lenovo"),
        },
    },
    /* HP systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "HP"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Hewlett-Packard"),
        },
    },
    /* Acer systems */
    {
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
        },
    },
    /* Generic gaming laptop indicators */
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "ROG"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "TUF"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "Legion"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "Gaming"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "Predator"),
        },
    },
    {
        .matches = {
            DMI_MATCH(DMI_PRODUCT_NAME, "Nitro"),
        },
    },
    {}
};

/* Laptop vendor types */
enum laptop_vendor {
    VENDOR_UNKNOWN = 0,
    VENDOR_ASUS,
    VENDOR_MSI,
    VENDOR_DELL_ALIENWARE,
    VENDOR_LENOVO,
    VENDOR_HP,
    VENDOR_ACER,
    VENDOR_GENERIC
};

struct universal_armoury {
    struct acpi_device *acpi_dev;
    struct platform_device *platform_dev;
    struct device *hwmon_dev;
    struct input_dev *input_dev;
    
    /* Vendor identification */
    enum laptop_vendor vendor;
    char vendor_name[32];
    char product_name[64];
    
    /* BIOS settings */
    bool gpu_mux_supported;
    bool dgpu_disable_supported;
    bool egpu_supported;
    bool power_profile_supported;
    bool fan_control_supported;
    
    /* Current states */
    int gpu_mux_state;
    int dgpu_disable_state;
    int egpu_state;
    int power_profile_state;
    
    /* ACPI method names for this vendor */
    const char *get_gpu_mux_method;
    const char *set_gpu_mux_method;
    const char *get_dgpu_disable_method;
    const char *set_dgpu_disable_method;
    const char *get_egpu_enable_method;
    const char *set_egpu_enable_method;
};

static struct universal_armoury *universal_armoury_dev;

/* Vendor detection function */
static enum laptop_vendor detect_laptop_vendor(struct universal_armoury *dev)
{
    const char *vendor = dmi_get_system_info(DMI_SYS_VENDOR);
    const char *product = dmi_get_system_info(DMI_PRODUCT_NAME);
    
    if (vendor) {
        strscpy(dev->vendor_name, vendor, sizeof(dev->vendor_name));
    }
    if (product) {
        strscpy(dev->product_name, product, sizeof(dev->product_name));
    }
    
    if (vendor) {
        if (strstr(vendor, "ASUS") || strstr(vendor, "ASUSTeK")) {
            return VENDOR_ASUS;
        } else if (strstr(vendor, "MSI") || strstr(vendor, "Micro-Star")) {
            return VENDOR_MSI;
        } else if (strstr(vendor, "Dell") || strstr(vendor, "Alienware")) {
            return VENDOR_DELL_ALIENWARE;
        } else if (strstr(vendor, "LENOVO") || strstr(vendor, "Lenovo")) {
            return VENDOR_LENOVO;
        } else if (strstr(vendor, "HP") || strstr(vendor, "Hewlett-Packard")) {
            return VENDOR_HP;
        } else if (strstr(vendor, "Acer")) {
            return VENDOR_ACER;
        }
    }
    
    /* Check product name for gaming laptop indicators */
    if (product) {
        if (strstr(product, "ROG") || strstr(product, "TUF") ||
            strstr(product, "Legion") || strstr(product, "Gaming") ||
            strstr(product, "Predator") || strstr(product, "Nitro")) {
            return VENDOR_GENERIC;
        }
    }
    
    return VENDOR_UNKNOWN;
}

/* Set vendor-specific ACPI method names */
static void set_vendor_acpi_methods(struct universal_armoury *dev)
{
    switch (dev->vendor) {
    case VENDOR_ASUS:
        dev->get_gpu_mux_method = ASUS_ACPI_GET_GPU_MUX_STATE;
        dev->set_gpu_mux_method = ASUS_ACPI_SET_GPU_MUX_STATE;
        dev->get_dgpu_disable_method = ASUS_ACPI_GET_DGPU_DISABLE;
        dev->set_dgpu_disable_method = ASUS_ACPI_SET_DGPU_DISABLE;
        dev->get_egpu_enable_method = ASUS_ACPI_GET_EGPU_ENABLE;
        dev->set_egpu_enable_method = ASUS_ACPI_SET_EGPU_ENABLE;
        break;
    case VENDOR_MSI:
        dev->get_gpu_mux_method = MSI_ACPI_GET_GPU_MUX_STATE;
        dev->set_gpu_mux_method = MSI_ACPI_SET_GPU_MUX_STATE;
        dev->get_dgpu_disable_method = MSI_ACPI_GET_DGPU_DISABLE;
        dev->set_dgpu_disable_method = MSI_ACPI_SET_DGPU_DISABLE;
        dev->get_egpu_enable_method = NULL; /* Not commonly supported */
        dev->set_egpu_enable_method = NULL;
        break;
    case VENDOR_DELL_ALIENWARE:
        dev->get_gpu_mux_method = DELL_ACPI_GET_GPU_MUX_STATE;
        dev->set_gpu_mux_method = DELL_ACPI_SET_GPU_MUX_STATE;
        dev->get_dgpu_disable_method = DELL_ACPI_GET_DGPU_DISABLE;
        dev->set_dgpu_disable_method = DELL_ACPI_SET_DGPU_DISABLE;
        dev->get_egpu_enable_method = NULL;
        dev->set_egpu_enable_method = NULL;
        break;
    case VENDOR_LENOVO:
        dev->get_gpu_mux_method = LENOVO_ACPI_GET_GPU_MUX_STATE;
        dev->set_gpu_mux_method = LENOVO_ACPI_SET_GPU_MUX_STATE;
        dev->get_dgpu_disable_method = LENOVO_ACPI_GET_DGPU_DISABLE;
        dev->set_dgpu_disable_method = LENOVO_ACPI_SET_DGPU_DISABLE;
        dev->get_egpu_enable_method = NULL;
        dev->set_egpu_enable_method = NULL;
        break;
    default:
        /* Try generic methods */
        dev->get_gpu_mux_method = GENERIC_ACPI_GET_MUX_STATE;
        dev->set_gpu_mux_method = GENERIC_ACPI_SET_MUX_STATE;
        dev->get_dgpu_disable_method = GENERIC_ACPI_GET_GPU_STATE;
        dev->set_dgpu_disable_method = GENERIC_ACPI_SET_GPU_STATE;
        dev->get_egpu_enable_method = NULL;
        dev->set_egpu_enable_method = NULL;
        break;
    }
}

/* Helper function to execute ACPI methods */
static int universal_armoury_acpi_evaluate_method(struct acpi_device *adev,
                                                const char *method_name,
                                                u32 arg, u32 *result)
{
    struct acpi_object_list input;
    union acpi_object in_obj;
    struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
    union acpi_object *out_obj;
    acpi_status status;
    int ret = 0;

    /* Validate input parameters */
    if (!adev || !method_name) {
        return -EINVAL;
    }

    input.count = 1;
    input.pointer = &in_obj;
    in_obj.type = ACPI_TYPE_INTEGER;
    in_obj.integer.value = arg;

    status = acpi_evaluate_object(adev->handle, (char *)method_name,
                                &input, &output);

    if (ACPI_SUCCESS(status)) {
        out_obj = output.pointer;
        if (out_obj) {
            if (out_obj->type == ACPI_TYPE_INTEGER && result) {
                *result = (u32)out_obj->integer.value;
            } else if (out_obj->type != ACPI_TYPE_INTEGER) {
                dev_warn(&adev->dev, "ACPI method %s returned non-integer type: %d\n",
                         method_name, out_obj->type);
                ret = -EPROTO;
            }
            kfree(output.pointer);
        } else {
            dev_err(&adev->dev, "ACPI method %s returned NULL output\n", method_name);
            ret = -ENODATA;
        }
    } else {
        dev_err(&adev->dev, "ACPI method %s failed with status 0x%x\n",
                method_name, status);
        ret = -EIO;
    }

    return ret;
}

/* GPU MUX control */
static ssize_t gpu_mux_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    u32 result;
    int ret;

    if (!armoury || !armoury->gpu_mux_supported || !armoury->get_gpu_mux_method)
        return -ENODEV;

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_gpu_mux_method,
                                               0, &result);
    if (ret)
        return ret;

    armoury->gpu_mux_state = result;
    return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t gpu_mux_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    int value, ret;

    if (!armoury || !armoury->gpu_mux_supported || !armoury->set_gpu_mux_method)
        return -ENODEV;

    if (!buf || count == 0)
        return -EINVAL;

    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid input for gpu_mux: %s\n", buf);
        return ret;
    }

    if (value < 0 || value > 1) {
        dev_err(dev, "gpu_mux value must be 0 or 1, got: %d\n", value);
        return -EINVAL;
    }

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->set_gpu_mux_method,
                                               value, NULL);
    if (ret)
        return ret;

    armoury->gpu_mux_state = value;
    return count;
}

/* dGPU disable control */
static ssize_t dgpu_disable_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    u32 result;
    int ret;

    if (!armoury || !armoury->dgpu_disable_supported || !armoury->get_dgpu_disable_method)
        return -ENODEV;

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_dgpu_disable_method,
                                               0, &result);
    if (ret)
        return ret;

    armoury->dgpu_disable_state = result;
    return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t dgpu_disable_store(struct device *dev,
                                 struct device_attribute *attr,
                                 const char *buf, size_t count)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    int value, ret;

    if (!armoury || !armoury->dgpu_disable_supported || !armoury->set_dgpu_disable_method)
        return -ENODEV;

    if (!buf || count == 0)
        return -EINVAL;

    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid input for dgpu_disable: %s\n", buf);
        return ret;
    }

    if (value < 0 || value > 1) {
        dev_err(dev, "dgpu_disable value must be 0 or 1, got: %d\n", value);
        return -EINVAL;
    }

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->set_dgpu_disable_method,
                                               value, NULL);
    if (ret)
        return ret;

    armoury->dgpu_disable_state = value;
    return count;
}

/* eGPU enable control */
static ssize_t egpu_enable_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    u32 result;
    int ret;

    if (!armoury || !armoury->egpu_supported || !armoury->get_egpu_enable_method)
        return -ENODEV;

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_egpu_enable_method,
                                               0, &result);
    if (ret)
        return ret;

    armoury->egpu_state = result;
    return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t egpu_enable_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    int value, ret;

    if (!armoury || !armoury->egpu_supported || !armoury->set_egpu_enable_method)
        return -ENODEV;

    if (!buf || count == 0)
        return -EINVAL;

    ret = kstrtoint(buf, 10, &value);
    if (ret) {
        dev_err(dev, "Invalid input for egpu_enable: %s\n", buf);
        return ret;
    }

    if (value < 0 || value > 1) {
        dev_err(dev, "egpu_enable value must be 0 or 1, got: %d\n", value);
        return -EINVAL;
    }

    ret = universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->set_egpu_enable_method,
                                               value, NULL);
    if (ret)
        return ret;

    armoury->egpu_state = value;
    return count;
}

/* Device attributes */
static DEVICE_ATTR_RW(gpu_mux);
static DEVICE_ATTR_RW(dgpu_disable);
static DEVICE_ATTR_RW(egpu_enable);

/* Vendor information attributes */
static ssize_t vendor_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    if (!armoury || !armoury->vendor_name[0])
        return scnprintf(buf, PAGE_SIZE, "Unknown\n");
    return scnprintf(buf, PAGE_SIZE, "%s\n", armoury->vendor_name);
}

static ssize_t product_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    if (!armoury || !armoury->product_name[0])
        return scnprintf(buf, PAGE_SIZE, "Unknown\n");
    return scnprintf(buf, PAGE_SIZE, "%s\n", armoury->product_name);
}

static ssize_t supported_features_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct universal_armoury *armoury = adev->driver_data;
    if (!armoury)
        return scnprintf(buf, PAGE_SIZE, "gpu_mux:0 dgpu_disable:0 egpu_enable:0\n");
    return scnprintf(buf, PAGE_SIZE, "gpu_mux:%d dgpu_disable:%d egpu_enable:%d\n",
                   armoury->gpu_mux_supported,
                   armoury->dgpu_disable_supported,
                   armoury->egpu_supported);
}

static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(product);
static DEVICE_ATTR_RO(supported_features);

static struct attribute *universal_armoury_attrs[] = {
    &dev_attr_gpu_mux.attr,
    &dev_attr_dgpu_disable.attr,
    &dev_attr_egpu_enable.attr,
    &dev_attr_vendor.attr,
    &dev_attr_product.attr,
    &dev_attr_supported_features.attr,
    NULL
};

static const struct attribute_group universal_armoury_attr_group = {
    .attrs = universal_armoury_attrs,
};

/* Probe supported features - Universal version */
static void universal_armoury_probe_features(struct universal_armoury *armoury)
{
    u32 result;
    const char *vendor_str;

    switch (armoury->vendor) {
    case VENDOR_ASUS:
        vendor_str = "ASUS";
        break;
    case VENDOR_MSI:
        vendor_str = "MSI";
        break;
    case VENDOR_DELL_ALIENWARE:
        vendor_str = "Dell/Alienware";
        break;
    case VENDOR_LENOVO:
        vendor_str = "Lenovo";
        break;
    case VENDOR_HP:
        vendor_str = "HP";
        break;
    case VENDOR_ACER:
        vendor_str = "Acer";
        break;
    case VENDOR_GENERIC:
        vendor_str = "Generic Gaming Laptop";
        break;
    default:
        vendor_str = "Unknown";
        break;
    }

    dev_info(&armoury->acpi_dev->dev, "Detected %s laptop: %s %s\n",
             vendor_str, armoury->vendor_name, armoury->product_name);

    /* Test GPU MUX support */
    if (armoury->get_gpu_mux_method &&
        !universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_gpu_mux_method,
                                               0, &result)) {
        armoury->gpu_mux_supported = true;
        armoury->gpu_mux_state = result;
        dev_info(&armoury->acpi_dev->dev, "GPU MUX control supported\n");
    }

    /* Test dGPU disable support */
    if (armoury->get_dgpu_disable_method &&
        !universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_dgpu_disable_method,
                                               0, &result)) {
        armoury->dgpu_disable_supported = true;
        armoury->dgpu_disable_state = result;
        dev_info(&armoury->acpi_dev->dev, "dGPU disable control supported\n");
    }

    /* Test eGPU support */
    if (armoury->get_egpu_enable_method &&
        !universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                               armoury->get_egpu_enable_method,
                                               0, &result)) {
        armoury->egpu_supported = true;
        armoury->egpu_state = result;
        dev_info(&armoury->acpi_dev->dev, "eGPU control supported\n");
    }

    if (!armoury->gpu_mux_supported && !armoury->dgpu_disable_supported && !armoury->egpu_supported) {
        /* Try alternative/generic ACPI methods */
        const char *alt_methods[] = {"GMUX", "_GPU", "DGPU", "SGPU", "MXDS", "MXDM", NULL};
        int i;
        
        dev_warn(&armoury->acpi_dev->dev, "No supported features found. Trying alternative ACPI methods...\n");
        
        for (i = 0; alt_methods[i]; i++) {
            if (!universal_armoury_acpi_evaluate_method(armoury->acpi_dev,
                                                       alt_methods[i], 0, &result)) {
                dev_info(&armoury->acpi_dev->dev, "Found working ACPI method: %s\n", alt_methods[i]);
                if (strstr(alt_methods[i], "MUX") || strstr(alt_methods[i], "MXD")) {
                    armoury->gpu_mux_supported = true;
                    armoury->get_gpu_mux_method = alt_methods[i];
                } else if (strstr(alt_methods[i], "GPU")) {
                    armoury->dgpu_disable_supported = true;
                    armoury->get_dgpu_disable_method = alt_methods[i];
                }
                break;
            }
        }
    }
}

static int universal_armoury_add(struct acpi_device *adev)
{
    struct universal_armoury *armoury;
    int ret;

    armoury = devm_kzalloc(&adev->dev, sizeof(*armoury), GFP_KERNEL);
    if (!armoury)
        return -ENOMEM;

    armoury->acpi_dev = adev;
    adev->driver_data = armoury;
    universal_armoury_dev = armoury;

    /* Detect laptop vendor */
    armoury->vendor = detect_laptop_vendor(armoury);
    set_vendor_acpi_methods(armoury);

    /* Check if this is a supported system */
    if (!dmi_check_system(universal_armoury_quirks)) {
        dev_warn(&adev->dev, "System not in compatibility list, but trying anyway...\n");
    }

    /* Probe available features */
    universal_armoury_probe_features(armoury);

    /* Create sysfs attributes */
    ret = sysfs_create_group(&adev->dev.kobj, &universal_armoury_attr_group);
    if (ret) {
        dev_err(&adev->dev, "Failed to create sysfs attributes: %d\n", ret);
        return ret;
    }

    dev_info(&adev->dev, "Universal Armoury driver loaded successfully for %s %s\n",
             armoury->vendor_name, armoury->product_name);
    return 0;
}

static void universal_armoury_remove(struct acpi_device *adev)
{
    sysfs_remove_group(&adev->dev.kobj, &universal_armoury_attr_group);
    universal_armoury_dev = NULL;
    dev_info(&adev->dev, "Universal Armoury driver unloaded\n");
}

static struct acpi_driver universal_armoury_driver = {
    .name = DRIVER_NAME,
    .class = DRIVER_NAME,
    .ids = universal_armoury_device_ids,
    .ops = {
        .add = universal_armoury_add,
        .remove = universal_armoury_remove,
    },
};

static int __init universal_armoury_init(void)
{
    int ret;

    pr_info("Universal Laptop Armoury driver v%s loading\n", DRIVER_VERSION);
    pr_info("Supports: ASUS, MSI, Dell/Alienware, Lenovo, HP, Acer and generic gaming laptops\n");

    ret = acpi_bus_register_driver(&universal_armoury_driver);
    if (ret) {
        pr_err("Failed to register ACPI driver: %d\n", ret);
        return ret;
    }

    return 0;
}

static void __exit universal_armoury_exit(void)
{
    acpi_bus_unregister_driver(&universal_armoury_driver);
    pr_info("Universal Laptop Armoury driver unloaded\n");
}

module_init(universal_armoury_init);
module_exit(universal_armoury_exit);

MODULE_AUTHOR("Alex <alex@example.com>");
MODULE_DESCRIPTION("Universal Laptop Armoury control driver for gaming laptops");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

/* ACPI device aliases */
MODULE_ALIAS("acpi*:ATK4001:*");  /* ASUS */
MODULE_ALIAS("acpi*:ATK4002:*");  /* ASUS */
MODULE_ALIAS("acpi*:ASUS0010:*"); /* ASUS */
MODULE_ALIAS("acpi*:MSI0001:*");  /* MSI */
MODULE_ALIAS("acpi*:DELL0001:*"); /* Dell */
MODULE_ALIAS("acpi*:AWK0001:*");  /* Alienware */
MODULE_ALIAS("acpi*:LEN0001:*");  /* Lenovo */
MODULE_ALIAS("acpi*:HPQ0001:*");  /* HP */
MODULE_ALIAS("acpi*:ACR0001:*");  /* Acer */
MODULE_ALIAS("acpi*:PNP0C02:*");  /* Generic */

