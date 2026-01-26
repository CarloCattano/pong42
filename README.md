## Setting up
### get the required addons into of addon folder

```bash
cd ~/{YOUR-openframeworks-Folder}/addons
git clone https://github.com/neilmendoza/ofxPostProcessing
```

```
make

export XDG_SESSION_TYPE=x11
export DISPLAY=:0
export XDG_RUNTIME_DIR=/run/user/$(id -u)

make RunRelease
```

- [x] Post Processing (GPU)
- [x] Implement GUI for debugging on site
- [x] Ascii effect on shader instead of CPU
- [x] Particles on GPU
