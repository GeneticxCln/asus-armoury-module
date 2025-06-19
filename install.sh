#!/bin/bash

# ASUS Armoury Kernel Module Installation Script
# Copyright (C) 2024

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_NAME="universal-armoury"
MODULE_VERSION="2.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root!"
        print_info "Please run as a regular user. Sudo will be used when needed."
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    print_info "Checking prerequisites..."
    
    # Check for kernel headers
    if ! pacman -Qs linux-headers >/dev/null 2>&1; then
        print_warning "Linux kernel headers not found. Installing..."
        sudo pacman -S --needed linux-headers || {
            print_error "Failed to install kernel headers"
            exit 1
        }
    fi
    
    # Check for build tools
    if ! pacman -Qs base-devel >/dev/null 2>&1; then
        print_warning "Build tools not found. Installing..."
        sudo pacman -S --needed base-devel || {
            print_error "Failed to install build tools"
            exit 1
        }
    fi
    
    print_success "Prerequisites check completed"
}

# Check if supported gaming laptop
check_gaming_laptop() {
    print_info "Checking laptop vendor and model..."
    
    VENDOR=$(dmidecode -s system-manufacturer 2>/dev/null || echo "Unknown")
    PRODUCT=$(dmidecode -s system-product-name 2>/dev/null || echo "Unknown")
    
    print_info "Detected: $VENDOR $PRODUCT"
    
    SUPPORTED=false
    
    # Check for supported vendors
    if echo "$VENDOR" | grep -qi "asus\|msi\|dell\|alienware\|lenovo\|hp\|acer"; then
        SUPPORTED=true
    fi
    
    # Check for gaming laptop indicators in product name
    if echo "$PRODUCT" | grep -qi "rog\|tuf\|legion\|gaming\|predator\|nitro\|alienware\|omen\|pavilion"; then
        SUPPORTED=true
    fi
    
    if [ "$SUPPORTED" = true ]; then
        print_success "Compatible gaming laptop detected"
    else
        print_warning "This laptop may not be fully supported."
        print_warning "The module will still attempt to work with generic ACPI methods."
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_info "Installation cancelled."
            exit 0
        fi
    fi
}

# Build the module
build_module() {
    print_info "Building kernel module..."
    
    cd "$SCRIPT_DIR"
    make clean >/dev/null 2>&1 || true
    
    if make; then
        print_success "Module built successfully"
    else
        print_error "Failed to build module"
        exit 1
    fi
}

# Install with DKMS if available
install_dkms() {
    if command -v dkms >/dev/null 2>&1; then
        print_info "DKMS found. Installing with DKMS..."
        
        # Copy source to DKMS directory
        sudo mkdir -p "/usr/src/${MODULE_NAME}-${MODULE_VERSION}"
        sudo cp -r "$SCRIPT_DIR"/* "/usr/src/${MODULE_NAME}-${MODULE_VERSION}/"
        
        # Add, build, and install with DKMS
        sudo dkms add -m "$MODULE_NAME" -v "$MODULE_VERSION" 2>/dev/null || true
        sudo dkms build -m "$MODULE_NAME" -v "$MODULE_VERSION"
        sudo dkms install -m "$MODULE_NAME" -v "$MODULE_VERSION"
        
        print_success "Module installed with DKMS"
        return 0
    else
        print_warning "DKMS not available. Using manual installation."
        return 1
    fi
}

# Manual installation
install_manual() {
    print_info "Installing module manually..."
    
    cd "$SCRIPT_DIR"
    sudo make install
    
    print_success "Module installed manually"
}

# Load the module
load_module() {
    print_info "Loading kernel module..."
    
    if sudo modprobe "$MODULE_NAME"; then
        print_success "Module loaded successfully"
        
        # Check if module is working
        if lsmod | grep -q "$MODULE_NAME"; then
            print_success "Module is active"
            
            # Show available features
            print_info "Checking for available features..."
            sleep 2
            dmesg | tail -10 | grep -i "universal-armoury" || true
        else
            print_warning "Module loaded but not visible in lsmod"
        fi
    else
        print_error "Failed to load module"
        print_info "Check dmesg for error messages: dmesg | tail -20"
        exit 1
    fi
}

# Create systemd service for auto-loading
create_systemd_service() {
    print_info "Creating systemd service for auto-loading..."
    
    cat << EOF | sudo tee /etc/modules-load.d/universal-armoury.conf >/dev/null
# Load Universal Laptop Armoury module at boot
universal-armoury
EOF
    
    print_success "Auto-load configuration created"
}

# Show usage information
show_usage() {
    print_success "Installation completed successfully!"
    echo
    print_info "Usage examples:"
    echo "  Check GPU MUX state: cat /sys/devices/LNXSYSTM:00/*/gpu_mux"
    echo "  Switch to iGPU: echo 0 | sudo tee /sys/devices/LNXSYSTM:00/*/gpu_mux"
    echo "  Switch to dGPU: echo 1 | sudo tee /sys/devices/LNXSYSTM:00/*/gpu_mux"
    echo
    print_info "See README.md for complete usage instructions."
    echo
    print_warning "Note: GPU MUX changes typically require a reboot to take effect."
}

# Main installation process
main() {
    echo "========================================="
    echo "Universal Laptop Armoury Module Installation"
    echo "========================================="
    echo
    
    check_root
    check_prerequisites
    check_gaming_laptop
    build_module
    
    if install_dkms; then
        # DKMS installation successful
        true
    else
        install_manual
    fi
    
    load_module
    create_systemd_service
    show_usage
}

# Handle script interruption
trap 'print_error "Installation interrupted"; exit 1' INT TERM

# Run main function
main "$@"

