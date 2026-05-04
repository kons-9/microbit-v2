win
```
# 管理者
usbipd list
usbipd bind --busid <id>
usbipd attach --wsl --busid <id>
```

wsl
```
sudo apt install usbutils
lsusb

sudo vim /etc/udev/rules.d/99-pyocd.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```