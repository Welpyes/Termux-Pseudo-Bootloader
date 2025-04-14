#/bin/env sh

bin="/data/data/com.termux/files/usr/bin"
mkdir $HOME/.config/bootloader -p

curl -L "https://github.com/Welpyes/Termux-Pseudo-Bootloader/releases/download/Release/bootloader" -o /data/data/com.termux/files/usr/bin/bootloader

curl -o $HOME/.config/bootloader/bootloader.yaml "https://raw.githubusercontent.com/Welpyes/Termux-Pseudo-Bootloader/refs/heads/main/bootloader.yaml"

curl -o $HOME/.config/bootloader/boot "https://raw.githubusercontent.com/Welpyes/Termux-Pseudo-Bootloader/refs/heads/main/boot"

chmod +x $HOME/.config/bootloader/boot
chmod +x $bin/bootloader

read -p "edit your config file. (enter to continue)" prompt
nano $HOME/.config/bootloader/bootloader.yaml


echo "to uninstall just use ./.config/bootloader/uninstall.sh"

cat > $HOME/.config/bootloader/uninstall.sh << EOF
#!/data/data/com.termux/files/usr/bin/bash
rm -f /data/data/com.termux/files/usr/bin/bootloader
rm -rf $HOME/.config/bootloader
rm -f $HOME/.config/bootloader/uninstall.sh
echo "Uninstallation complete!"
EOF
chmod +x $HOME/.config/bootloader/uninstall.sh
