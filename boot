#!/bin/bash

RESET=$(tput sgr0)
GREEN=$(tput setaf 2)
RED=$(tput setaf 1)

# Path to the INI file
INI_FILE="$HOME/.config/bootloader/bootloader.ini"

# Check if the file exists
if [ ! -f "$INI_FILE" ]; then
    echo "Error: INI file $INI_FILE not found"
    exit 1
fi

# Variables to track current section and the desired value
current_section=""
CMD1=""

# Read the INI file line by line
while IFS='=' read -r key value; do
    # Trim leading/trailing whitespace from key and value
    key=$(echo "$key" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    value=$(echo "$value" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

    # Skip empty lines or comments (fixed regex with escaped semicolon)
    if [ -z "$key" ] || [[ "$key" =~ ^# ]] || [[ "$key" =~ ^\; ]]; then
        continue
    fi

    # Check for section headers
    if [[ "$key" =~ ^\[.*\]$ ]]; then
        current_section=$(echo "$key" | sed 's/\[\(.*\)\]/\1/')
        continue
    fi

    # Look for 'distro' in the [selection_title] section
    if [ "$current_section" = "selection_title" ] && [ "$key" = "distro" ]; then
        CMD1="$value"
    fi
done < "$INI_FILE"

start_service() {
    local service="$1"
    local delay=${2:-0.2}  # default delay in seconds
    local status=$(( RANDOM % 10 ))  # randomized failed chance

    printf "Starting %-40s" "$service..."
    sleep "$delay"  # Simulate processing time

    if [ "$status" -eq 0 ]; then
        echo -e "[ ${RED}FAILED${RESET} ]"
        return 1
    else
        echo -e "[ ${GREEN}OK${RESET} ]"
        return 0
    fi
}

run_startup() {
    clear
    echo "Welcome to ${CMD1:-Unknown Distro}"  # Fallback if CMD1 is unset
    echo "Booting system..."
    echo "----------------------------------------"
    sleep 1

    # Services list (add more if you like)
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
        start_service "$service" "$(printf "0.%d" $(( RANDOM % 3 + 1 )))"  # Random delay between 0.1 and 0.3s
    done

    echo "----------------------------------------"
    sleep 1
    echo -e "${GREEN}System boot completed.${RESET}"
    echo "Opening Display..."
    sleep 2  # Give it a moment before clearing or proceeding
    clear
}

run_startup

