#!/bin/bash

STARTX_FILE="$HOME/.config/bootloader/startx"



cd $PATH

pkg i wget ncurses-utils -y

wget "https://github.com/Welpyes/Termux-Pseudo-Bootloader/releases/download/Release/bootloader" 

chmod +x $PATH/bootloader

mkdir "$HOME/.config/bootloader" -p

cd "$HOME/.config/bootloader"

wget "https://raw.githubusercontent.com/Welpyes/Termux-Pseudo-Bootloader/refs/heads/main/bootloader.ini"

wget "https://raw.githubusercontent.com/Welpyes/Termux-Pseudo-Bootloader/refs/heads/main/startx"

chmod +x startx

cd $HOME 

# File checker(just for debuggings)
if [ ! -f "$STARTX_FILE" ]; then
    echo "Error: $STARTX_FILE not found"
    exit 1
fi

read -p "Enter your username: " username
if [ -z "$username" ]; then
    echo "Error: Username cannot be empty"
    exit 1
fi

read -p "Enter your desktop environment (e.g., bspwm, xfce4-session): " desktop_env
if [ -z "$desktop_env" ]; then
    echo "Error: Desktop environment cannot be empty"
    exit 1
fi

# backups
cp "$STARTX_FILE" "$STARTX_FILE.bak"
echo "Backup created at $STARTX_FILE.bak"

# i aint using regex dawg 😭🙏
sed -i "s|su - -c \"env DISPLAY=:0 \"|su - $username -c \"env DISPLAY=:0 $desktop_env\"|" "$STARTX_FILE"

if [ $? -eq 0 ]; then
    echo "Successfully updated $STARTX_FILE with username '$username' and desktop environment '$desktop_env'"
else
    echo "Error: Failed to update $STARTX_FILE"
    exit 1
fi

grep "su - $username -c \"env DISPLAY=:0 $desktop_env\"" "$STARTX_FILE" > /dev/null
if [ $? -eq 0 ]; then
    echo "Verification: Change applied correctly"
else
    echo "Warning: Change not found in $STARTX_FILE. Check manually."
fi
