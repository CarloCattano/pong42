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



build opencv cuda dnn
```
cd ~/dev/opencv_cuda/opencv
rm -rf build
mkdir build && cd build


cmake .. \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D OPENCV_GENERATE_PKGCONFIG=ON \
  -D OPENCV_EXTRA_MODULES_PATH=~/dev/opencv_cuda/opencv_contrib/modules \
  -D WITH_CUDA=ON \
  -D OPENCV_DNN_CUDA=ON \
  -D WITH_CUDNN=ON \
  -D CUDA_ARCH_BIN="6.1" \
  -D CUDA_ARCH_PTX="6.1" \
  -D WITH_GSTREAMER=ON \
  -D WITH_FFMPEG=ON \
  -D BUILD_TESTS=OFF \
  -D BUILD_EXAMPLES=OFF
  ```
