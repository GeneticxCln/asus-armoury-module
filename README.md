# Universal Laptop Armoury Kernel Module

A Linux kernel module that provides control over gaming laptop features through ACPI interfaces across multiple brands including ASUS, MSI, Dell/Alienware, Lenovo, HP, Acer and generic gaming laptops.

## Features

- **GPU MUX Control**: Switch between integrated and discrete GPU modes
- **dGPU Disable**: Disable/enable the discrete GPU for power saving
- **eGPU Enable**: Control external GPU support
- **ACPI Integration**: Uses ACPI methods for hardware communication
- **Sysfs Interface**: Control features through `/sys/devices/LNXSYSTM:00/*/`

## Supported Devices

This module is designed for gaming laptops with compatible ACPI interfaces, including:

### ASUS Laptops
- ASUS ROG series (Strix, Zephyrus, Flow, etc.)
- ASUS TUF Gaming series
- Other ASUS gaming laptops with Armoury Crate support

### MSI Laptops
- MSI Gaming series
- MSI Stealth series
- MSI Raider series
- MSI Creator series with gaming features

### Dell/Alienware Laptops
- Alienware m15, m17, x14, x15, x17 series
- Dell G-series gaming laptops
- Dell XPS with discrete graphics

### Lenovo Laptops
- Lenovo Legion series (5, 7, Pro, Slim)
- Lenovo IdeaPad Gaming series
- ThinkPad P-series with discrete graphics

### HP Laptops
- HP Omen series
- HP Pavilion Gaming series
- HP ZBook with gaming features

### Acer Laptops
- Acer Predator series (Helios, Triton)
- Acer Nitro series
- Acer ConceptD with gaming features

### Generic Gaming Laptops
- Any laptop with compatible ACPI GPU switching methods
- Laptops with MUX switch support
- Laptops with dGPU disable functionality

## Prerequisites

Before building, ensure you have:
- Linux kernel headers: `sudo pacman -S linux-headers`
- Build tools: `sudo pacman -S base-devel`
- DKMS (optional): `sudo pacman -S dkms`

## Building and Installation

### Method 1: Manual Build
```bash
# Build the module
make

# Load the module
sudo make load

# Check if loaded successfully
make status

# Install permanently
sudo make install
```

### Method 2: DKMS Installation (Recommended)
```bash
# Copy module to DKMS source directory
sudo cp -r . /usr/src/universal-armoury-2.0.0/

# Add to DKMS
sudo dkms add -m universal-armoury -v 2.0.0

# Build and install
sudo dkms build -m universal-armoury -v 2.0.0
sudo dkms install -m universal-armoury -v 2.0.0

# Load the module
sudo modprobe universal-armoury
```

## Usage

Once loaded, the module creates sysfs entries for controlling supported features:

### GPU MUX Control
```bash
# Check current GPU MUX state (0=iGPU, 1=dGPU)
cat /sys/devices/LNXSYSTM:00/*/gpu_mux

# Switch to integrated GPU
echo 0 | sudo tee /sys/devices/LNXSYSTM:00/*/gpu_mux

# Switch to discrete GPU
echo 1 | sudo tee /sys/devices/LNXSYSTM:00/*/gpu_mux
```

### dGPU Disable Control
```bash
# Check dGPU disable state (0=enabled, 1=disabled)
cat /sys/devices/LNXSYSTM:00/*/dgpu_disable

# Disable dGPU
echo 1 | sudo tee /sys/devices/LNXSYSTM:00/*/dgpu_disable

# Enable dGPU
echo 0 | sudo tee /sys/devices/LNXSYSTM:00/*/dgpu_disable
```

### eGPU Enable Control
```bash
# Check eGPU enable state
cat /sys/devices/LNXSYSTM:00/*/egpu_enable

# Enable eGPU support
echo 1 | sudo tee /sys/devices/LNXSYSTM:00/*/egpu_enable
```

## Troubleshooting

### Module doesn't load
- Check if your system is supported: `dmesg | grep universal-armoury`
- Verify ACPI device presence: `ls /sys/bus/acpi/devices/ | grep -E 'ATK|ASUS|MSI|DELL|LEN|HPQ|ACR'`
- Check laptop vendor: `dmidecode -s system-manufacturer`

### Features not available
- Some features may not be supported on all models
- Check dmesg output for feature detection results
- Ensure BIOS/UEFI is up to date

### Permission denied
- Make sure to use `sudo` when writing to sysfs files
- Check if the module is properly loaded: `lsmod | grep universal-armoury`

## Uninstallation

### Manual removal
```bash
# Unload module
sudo make unload

# Remove from system
sudo make uninstall
```

### DKMS removal
```bash
# Unload module
sudo modprobe -r universal-armoury

# Remove from DKMS
sudo dkms remove -m universal-armoury -v 2.0.0 --all
sudo rm -rf /usr/src/universal-armoury-2.0.0/
```

## Development

### Building for different kernels
```bash
# Specify kernel version
make KERNEL_DIR=/lib/modules/6.1.0-1-MANJARO/build
```

### Debug mode
Add debug prints by modifying the source and rebuilding:
```bash
make clean
make
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on your hardware
5. Submit a pull request

## License

This module is licensed under the GPL v2 license. See the source code for full license text.

## Disclaimer

This software comes with no warranty. Use at your own risk. Always backup your system before installing kernel modules.

## Compatibility Notes

- Requires kernel version 4.19 or newer
- Some features may require specific BIOS versions
- GPU MUX switching typically requires a reboot to take effect
- Not all laptops support all features (depends on hardware and BIOS)
- ACPI method names may vary between laptop models
- Module automatically detects vendor and tries appropriate ACPI methods
- If standard methods fail, module attempts generic/alternative ACPI methods

## Vendor-Specific Notes

### ASUS
- Best support with most features available
- Uses ASUS-specific ACPI methods (MXDS, MXDM, DGPU, etc.)
- ROG and TUF series have the most complete feature support

### MSI
- Good support for GPU MUX and dGPU disable
- Uses MSI-specific ACPI methods (GMUX, SMUX, etc.)
- eGPU support varies by model

### Dell/Alienware
- Alienware laptops generally have good support
- Dell G-series support varies
- Uses Dell-specific ACPI methods

### Lenovo
- Legion series has good support
- Legion 5/7 Pro models work well
- Uses Lenovo-specific ACPI methods

### HP/Acer
- Support varies significantly by model
- Newer gaming models more likely to work
- Falls back to generic ACPI methods

