#!/bin/bash

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

# Function to simulate starting a service
start_service() {
    local service="$1"
    local delay="$2"
    echo -n "Starting $service... "
    sleep "$delay"
    if [ $((RANDOM % 10)) -eq 0 ]; then  # 10% chance of failure for realism
        echo -e "[ ${RED}FAILED${RESET} ]"
        return 1
    else
        echo -e "[ ${GREEN}OK${RESET} ]"
        return 0
    fi
}

# Function to run the startup sequence
run_startup() {
    # Get the distro name from YAML
    CONFIG_FILE="$HOME/.config/bootloader/bootloader.yaml"
    DISTRO=$(yq e '.login.options.label' "$CONFIG_FILE" 2>/dev/null || echo "Unknown Distro")

    clear
    echo "Welcome to $DISTRO"  # Use the parsed label instead of CMD1
    echo "Booting system..."
    echo "----------------------------------------"
    sleep 1

    # Services list
    SERVICES=(
        "kernel modules"
        "network interfaces"
        "mounting filesystems"
        "udev device manager"
        "system clock synchronization"
        "dbus service"
        "ssh daemon"
        "cron scheduler"
        "pulseaudio sound server"
        "graphical target"
        "syslog daemon"
        "firewalld service"
        "bluetooth daemon"
        "avahi mDNS/DNS-SD stack"
        "cups printing service"
        "apparmor profiles"
        "selinux policy enforcement"
        "rngd entropy daemon"
        "lvm volume activation"
        "swap space initialization"
        "auditd logging service"
        "postfix mail server"
        "network manager"
        "gdm display manager"
        "docker container runtime"
        "systemd-tmpfiles setup"
        "plymouth boot splash"
        "smartd disk monitoring"
        "rsyslog logging service"
        "acpid power management"
        "chronyd time service"
        "sssd authentication service"
        "polkit authorization service"
        "laptop-mode tools"
        "irqbalance daemon"
        "systemd-oomd out-of-memory daemon"
        "anacron scheduler"
        "dmraid device mapper"
        "fstrim SSD trimming"
        "systemd-backlight service"
        "systemd-resolved DNS resolver"
        "openvpn service"
        "rpcbind NFS service"
        "zfs filesystem support"
        "snapd service"
        "thermald thermal daemon"
        "ufw firewall"
        "fail2ban intrusion prevention"
        "systemd-networkd network service"
        "power-profiles-daemon"
    )

    # Simulate service startup
    for service in "${SERVICES[@]}"; do
        start_service "$service" "$(printf "0.%d" $(( RANDOM % 3 + 1 )))"
    done

    echo "----------------------------------------"
    sleep 1
    echo -e "${GREEN}System boot completed.${RESET}"
    echo "Opening Display..."
    sleep 2
    clear
}

# Run the startup sequence
run_startup

# Launch proot in the background and start the display manager
(sleep 1 && bash "$HOME/.config/bootloader/proot" > /dev/null 2>&1) &
env dm
