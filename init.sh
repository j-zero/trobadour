#!/bin/bash
git clone https://github.com/Scondo/mt32-pi.git --recursive
cd mt32-pi
git submodule update --recursive --remote
cp ./external/circle-stdlib/libs/circle/addon/wlan/hostap/wpa_supplicant/defconfig ./external/circle-stdlib/libs/circle/addon/wlan/hostap/wpa_supplicant/.config
cp ./external/circle-stdlib/libs/circle/addon/wlan/hostap/hostapd/defconfig ./external/circle-stdlib/libs/circle/addon/wlan/hostap/hostapd/.config
