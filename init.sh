#!/bin/bash
git clone https://github.com/Scondo/mt32-pi.git --recursive
cd mt32-pi
git submodule update --recursive --remote
