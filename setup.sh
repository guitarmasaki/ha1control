#!/bin/sh

sed -e 's/^.*ja_JP.UTF-8 UTF-8.*$/ja_JP.UTF-8 UTF-8/' /etc/locale.gen > /etc/locale.gen_new
mv /etc/locale.gen_new /etc/locale.gen
locale-gen
update-locale LANG=ja_JP.UTF-8
echo "Asia/Tokyo" > /etc/timezone
dpkg-reconfigure --frontend noninteractive tzdata
echo "ha1homeserver" > /etc/hostname
echo "dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait" > /boot/cmdline.txt
systemctl disable serial-getty@ttyAMA0.service

apt-get -y remove dphys-swapfile* 
rm /var/swap
apt-get -y -q update
apt-get -y -q dist-upgrade
apt-get -y -q install libssl-dev libcurl4-openssl-dev libgif-dev libjansson-dev telnet gawk zsh git cron-apt libavahi-compat-libdnssd-dev
apt-get -y -q --purge autoremove
apt-get -y clean

sed -e 's/^[ \t]*PermitRootLogin.*$/# &/' -e 's/^[ \t]*RSAAuthentication.*$/# &/' -e 's/^[ \t]*UsePAM.*$/# &/' -e '$aPermitRootLogin no' -e '$aRSAAuthentication no' -e '$aUsePAM no' -e '$aUseDNS no' -e '$a# PasswordAuthentication no' /etc/ssh/sshd_config > /etc/ssh/sshd_config_new
mv /etc/ssh/sshd_config_new /etc/ssh/sshd_config
echo -e 'APTCOMMAND=/usr/bin/apt-get\nMAILTO="root"\nMAILON="changes"\nDEBUG="changes"\nOPTIONS="-o quiet=1"\n' > /etc/cron-apt/config
echo -e 'autoclean -y\ndist-upgrade -d -y -o APT::Get::Show-Upgraded=true\n' > /etc/cron-apt/action.d/3-download

git clone https://github.com/mnakada/ha1control.git
(cd ha1control; make; make install)
git clone https://github.com/mnakada/HA1Observer.git
(cd HA1Observer; make install)
git clone https://github.com/mnakada/HA1HomeBridge.git
(cd HA1HomeBridge; make install)
sync

