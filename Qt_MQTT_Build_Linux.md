# Building Qt MQTT Module on Linux (Qt 5.15.0)

## Prerequisites

1. **System Update**:
```bash
sudo apt-get update
sudo apt-get upgrade
```

2. **Install Build Dependencies**:
```bash
sudo apt-get install -y \
    git \
    perl \
    build-essential \
    cmake \
    ninja-build \
    libgl1-mesa-dev \
    libx11-dev \
    libxkbcommon-dev \
    libwayland-dev \
    libxrandr-dev \
    libxss-dev \
    libxi-dev
```

## Build Steps

### 1. Create Qt Directory
```bash
mkdir -p ~/Qt/5.15.0
cd ~/Qt/5.15.0
```

### 2. Clone Qt 5 Repository
```bash
git clone https://github.com/qt/qt5.git
cd qt5
git checkout v5.15.0
perl init-repository
```

### 3. Configure the Build
```bash
mkdir build
cd build

../configure -prefix ~/Qt/5.15.0/5.15.0 \
    -release \
    -opensource \
    -confirm-license \
    -nomake tests \
    -nomake examples \
    -mqtt
```

### 4. Build Qt
```bash
# Build using all CPU cores
make -j$(nproc)

# Install
make install
```

## Environment Configuration

1. **Add to .bashrc**:
```bash
# Open .bashrc
nano ~/.bashrc

# Add these lines
export Qt5_DIR=~/Qt/5.15.0/5.15.0
export PATH=$Qt5_DIR/bin:$PATH
export LD_LIBRARY_PATH=$Qt5_DIR/lib:$LD_LIBRARY_PATH

# Reload configuration
source ~/.bashrc
```

## Project Configuration

In your .pro file, add:
```qmake
QT += mqtt
```

## Troubleshooting
- Ensure all dependencies are installed
- Check disk space (minimum 30GB recommended)
- Build process can take several hours
- Verify Qt version compatibility

## Alternative Methods
1. Use Qt's online installer
2. Use distribution package manager
3. Consider third-party MQTT libraries

## Notes
- This guide is for Qt 5.15.0
- Commercial license may be required for full MQTT functionality
- Build times vary by system specifications
