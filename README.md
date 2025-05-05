## Setting up

### get the required addons into of addon folder

```bash
cd ~/{YOUR-openframeworks-Folder}/addons
git clone https://github.com/neilmendoza/ofxPostProcessing
```

### then go to examples/ for example and clone the repo

```bash
git clone https://github.com/CarloCattano/pong42  --recursive
cd pong42
projectGenerator .
```

### Add this line to config.make if you generate a new one

```bash
PROJECT_CFLAGS += -I/usr/include/boost -Ilibs/websocketpp
```

```bash
make
make RunRelease
```

### TODO's

- Azure kinect testing [https://github.com/prisonerjohn/ofxAzureKinect](https://github.com/prisonerjohn/ofxAzureKinect) for skeletal tracking / hand tracking

- audio reactivity [https://github.com/kylemcdonald/ofxFft](https://github.com/kylemcdonald/ofxFft)

- info:

  - Instructions
  - QR code credits

- [x] Post Processing (GPU)
- [x] Websockets adaptor for openframeworks from scratch using websocketpp

- [x] Implement GUI for debugging on site

- [x] Game loop:

  - [x] Finish game logic
  - [x] restart

- [x] Ascii effect on shader instead of CPU
- [x] Particles on GPU

~- abstract parameters to config file~
