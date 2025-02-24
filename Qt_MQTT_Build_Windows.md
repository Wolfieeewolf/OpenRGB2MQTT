# Building Qt MQTT Module on Windows (Qt 5.15.0)

## Prerequisites

1. **Required Software**:
   - Visual Studio 2019 or 2022 (Community Edition is fine)
   - Git for Windows
   - Python 3.x
   - Perl (recommended: Strawberry Perl)

2. **Download and Install**:
   - Visual Studio: Download from [Microsoft's website](https://visualstudio.microsoft.com/downloads/)
   - Git: Download from [Git for Windows](https://git-scm.com/download/win)
   - Python: Download from [Python's official site](https://www.python.org/downloads/windows/)
   - Strawberry Perl: Download from [Strawberry Perl website](http://strawberryperl.com/)

## Build Steps

### 1. Open Developer Command Prompt
- Open "Developer Command Prompt for VS" (search in Start menu)

### 2. Clone Qt 5 Repository
```cmd
mkdir C:\Qt\5.15.0
cd C:\Qt\5.15.0
git clone https://github.com/qt/qt5.git
cd qt5
git checkout v5.15.0
perl init-repository
```

### 3. Configure the Build
```cmd
mkdir build
cd build
..\configure.bat -prefix C:\Qt\5.15.0\5.15.0 ^
    -release ^
    -opensource ^
    -confirm-license ^
    -nomake tests ^
    -nomake examples ^
    -mqtt
```

### 4. Build Qt
```cmd
nmake
nmake install
```

## Post-Build Configuration

1. **Add to PATH**:
   - Add `C:\Qt\5.15.0\5.15.0\bin` to your system PATH

2. **Qt Creator Configuration**:
   - Open Qt Creator
   - Go to Tools > Options > Build & Run
   - Add the new Qt version in the "Qt Versions" tab

## Project Configuration

In your .pro file, add:
```qmake
QT += mqtt
```

## Troubleshooting
- Ensure all prerequisites are installed
- Check that PATH includes all necessary directories
- Verify Visual Studio C++ components are installed
- Allocate at least 30GB of disk space
- Build process can take several hours

## Notes
- This guide is for Qt 5.15.0
- Commercial license may be required for full MQTT functionality
- Consider alternative MQTT libraries if needed
