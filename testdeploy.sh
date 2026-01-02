#!/bin/sh

cd build
sudo cmake --install . --prefix=/usr
systemctl --user daemon-reload
systemctl --user stop xdg-desktop-portal-uni
systemctl --user stop xdg-desktop-portal-uni
systemctl --user restart xdg-desktop-portal