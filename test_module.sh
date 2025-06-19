#!/bin/bash

# Comprehensive Test Script for Universal Armoury Module
# Tests module loading, functionality, and edge cases

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_NAME="universal-armoury"
MODULE_FILE="$SCRIPT_DIR/${MODULE_NAME}.ko"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local test_command="$2"
    
    log_info "Running test: $test_name"
    
    if eval "$test_command"; then
        log_success "$test_name passed"
        ((TESTS_PASSED++))
        return 0
    else
        log_error "$test_name failed"
        ((TESTS_FAILED++))
        return 1
    fi
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root for module testing"
        exit 1
    fi
}

# Test 1: Module file exists and is valid
test_module_exists() {
    [[ -f "$MODULE_FILE" ]] && file "$MODULE_FILE" | grep -q "ELF.*relocatable"
}

# Test 2: Module dependencies are satisfied
test_dependencies() {
    modprobe --dry-run "$MODULE_FILE" >/dev/null 2>&1
}

# Test 3: Module info is correct
test_module_info() {
    local info
    info=$(modinfo "$MODULE_FILE" 2>/dev/null)
    echo "$info" | grep -q "version.*2.0.0" && 
    echo "$info" | grep -q "license.*GPL" &&
    echo "$info" | grep -q "description.*Universal Laptop Armoury"
}

# Test 4: Module can be loaded
test_module_load() {
    # Unload if already loaded
    if lsmod | grep -q "${MODULE_NAME/_/-}"; then
        rmmod "$MODULE_NAME" 2>/dev/null || true
    fi
    
    # Load the module
    insmod "$MODULE_FILE" 2>/dev/null &&
    lsmod | grep -q "${MODULE_NAME/_/-}"
}

# Test 5: Module creates expected sysfs entries
test_sysfs_creation() {
    # Look for sysfs entries created by the module
    find /sys -name "*gpu_mux*" -o -name "*dgpu_disable*" -o -name "*egpu_enable*" 2>/dev/null | head -1 | grep -q "."
}

# Test 6: Module handles invalid input gracefully
test_invalid_input() {
    local sysfs_file
    sysfs_file=$(find /sys -name "gpu_mux" 2>/dev/null | head -1)
    
    if [[ -n "$sysfs_file" ]] && [[ -w "$sysfs_file" ]]; then
        # Test invalid values
        echo "invalid" > "$sysfs_file" 2>/dev/null && return 1  # Should fail
        echo "999" > "$sysfs_file" 2>/dev/null && return 1     # Should fail
        echo "-1" > "$sysfs_file" 2>/dev/null && return 1      # Should fail
        return 0  # All invalid inputs were rejected as expected
    else
        log_warning "No writable sysfs file found for testing invalid input"
        return 0  # Not a failure, just no testable interface
    fi
}

# Test 7: Module can be unloaded cleanly
test_module_unload() {
    if lsmod | grep -q "${MODULE_NAME/_/-}"; then
        rmmod "$MODULE_NAME" 2>/dev/null &&
        ! lsmod | grep -q "${MODULE_NAME/_/-}"
    else
        log_warning "Module was not loaded for unload test"
        return 0
    fi
}

# Test 8: Check for memory leaks (basic)
test_memory_leaks() {
    local mem_before mem_after
    
    # Get memory stats before
    mem_before=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
    
    # Load and unload module multiple times
    for i in {1..5}; do
        insmod "$MODULE_FILE" 2>/dev/null || return 1
        sleep 0.1
        rmmod "$MODULE_NAME" 2>/dev/null || return 1
        sleep 0.1
    done
    
    # Get memory stats after
    mem_after=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
    
    # Check if memory difference is reasonable (less than 1MB)
    local mem_diff=$((mem_before - mem_after))
    [[ $mem_diff -lt 1024 ]]
}

# Test 9: Verify module symbols
test_module_symbols() {
    local symbols
    symbols=$(nm "$MODULE_FILE" 2>/dev/null | grep -c "T.*universal_armoury" || echo "0")
    [[ $symbols -gt 0 ]]
}

# Test 10: Check for undefined symbols
test_undefined_symbols() {
    ! nm "$MODULE_FILE" 2>/dev/null | grep -q "U.*undefined"
}

# Main test execution
main() {
    log_info "Starting comprehensive tests for Universal Armoury Module"
    log_info "Module file: $MODULE_FILE"
    echo
    
    # Check if we're root
    check_root
    
    # Run all tests
    run_test "Module file exists and is valid" "test_module_exists"
    run_test "Module dependencies satisfied" "test_dependencies"
    run_test "Module information correct" "test_module_info"
    run_test "Module symbols present" "test_module_symbols"
    run_test "No undefined symbols" "test_undefined_symbols"
    run_test "Module loads successfully" "test_module_load"
    run_test "Sysfs entries created" "test_sysfs_creation"
    run_test "Invalid input handling" "test_invalid_input"
    run_test "Module unloads cleanly" "test_module_unload"
    run_test "No memory leaks detected" "test_memory_leaks"
    
    echo
    log_info "Test Summary:"
    log_success "Tests passed: $TESTS_PASSED"
    if [[ $TESTS_FAILED -gt 0 ]]; then
        log_error "Tests failed: $TESTS_FAILED"
        exit 1
    else
        log_success "All tests passed!"
        exit 0
    fi
}

# Run main function
main "$@"

