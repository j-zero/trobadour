#!/bin/bash
podman run -v ./mt32-pi/:/root/src/mt32-pi -w /root/src/mt32-pi -i -t mt32-pi-toolchain make all
