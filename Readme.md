# ShairportQt

An AirPlay Audio-Receiver for your Personal Computer or ARM-SoC (e.g. Raspberry Pi).
Supports Windows (x64, ARM64), Linux (x64, ARM64) and Raspbian.

Play audio content from your iPhone, iPad, iPod or iTunes on your PC with ShairportQt.
AirPlay lets you wirelessly stream what's on your iOS device whenever you see the AirPlay symbol.

This desktop-software was originally only available for Windows.
But now it's based on `Qt` and is therefore completely portable/cross platform. Additionally many bugs had been fixed - compared to the
previous version (Shairport4w).

Please download pre compiled binaries from [`Releases`](https://github.com/Frank-Friemel/ShairportQt/releases).
When being asked by your Firewall you should grant access to your LAN if secure. Protect ShairportQt with a password
to be sure nobody is misusing this service.

## Installation

ShairportQt consists of a single executable file. So, there's not much to install. Nevertheless I would like to provide
some hints for the several Operating Systems which worked for me. Updating your existing installation please
do the same. 

Once you've downloaded the
[zip](https://github.com/Frank-Friemel/ShairportQt/releases) ... just extract the folder which matches your
OS and follow these instructions:

#### Windows (x64 / ARM64)

Just copy the file `ShairportQt.exe` to your filesystem and create a Desktop-Link. That's it.

Shairport depends on Apple's [`Bonjour`](https://support.apple.com/kb/DL999). Just start `ShairportQt`... it will guide you in case `Bonjour`
is not installed on your machine. Please retry to start `ShairportQt.exe` after
installation of `Bonjour`.

#### Linux (x64 / ARM64)

The installation for Linux depends a bit on your Linux distribution.
I myself am using [`Manjaro-Linux`](https://manjaro.org/)
which worked out of the box and I would expect the same for all KDE based distributions. You'll need at least `Qt 6.7` installed on your machine.

May be anybody of the Linux Pros can give advice how to improve the installation experience with a `.deb` or `.rpm`
package. Comments are appreciated.

The [releases](https://github.com/Frank-Friemel/ShairportQt/releases) package contains an installation script. So, for now you need to unpack the
[zip](https://github.com/Frank-Friemel/ShairportQt/releases) file, open a terminal, change-directory to
`Linux_x64` and start script `install.sh` as superuser:

```shell
chmod a+x install.sh
sudo ./install.sh
```

Afterwards `ShairportQt` application should be available from your start menu.

#### Raspbian (arm64)

The [releases](https://github.com/Frank-Friemel/ShairportQt/releases) package contains an installation script. So, for now you need to unpack the
[zip](https://github.com/Frank-Friemel/ShairportQt/releases) file, open a terminal, change-directory to
`Raspbian` and start script `install.sh` as superuser:

```shell
chmod a+x install.sh
sudo ./install.sh
```

Afterwards `ShairportQt` application should be available from your start menu.

## Problem reports

When you have issues with `ShairportQt` please provide the following information:

- Operating System you're using.
- detailed steps how to reproduce.
- a log file. Which may be enabled from the advanced options dialog. The log file `ShairportQt.log` will be created within your home folder. Alternatively start `ShairportQt` with command line option `-log`.

## Building

`ShairportQt` is a `CMake` project (C++17). You'll need a complete C++ development environment and these packages:

- `openssl`
- `spdlog`
- `sockpp`
- `qtbase` (Qt 6)
- `gtest` / `gmock`

I recommend to use [`vcpkg`](https://github.com/microsoft/vcpkg) in order to get them.
It works very well on Linux and Windows.

### Windows (Visual Studio)

#### Prerequisites

1. **Visual Studio 2022** (or later) with the *Desktop development with C++* workload.

2. **vcpkg** - install once and integrate:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```
   Set the environment variable so CMake finds it automatically:
   ```powershell
   setx VCPKG_ROOT C:\vcpkg
   ```

3. **Install dependencies** via vcpkg:
   ```powershell
   # For x64
   C:\vcpkg\vcpkg install openssl spdlog sockpp gtest --triplet x64-windows-static
   # For ARM64
   C:\vcpkg\vcpkg install openssl spdlog sockpp gtest --triplet arm64-windows-static
   ```

4. **Qt 6** - install via the [Qt online installer](https://download.qt.io/official_releases/online_installers/).
   Select the *MSVC 2022 64-bit* component (and/or *MSVC 2022 ARM64* for ARM64 builds).
   Note the install path and set `CMAKE_PREFIX_PATH` so CMake finds Qt:
   ```powershell
   # For x64
   setx CMAKE_PREFIX_PATH C:\Qt\6.8.1\msvc2022_64
   # For ARM64
   setx CMAKE_PREFIX_PATH C:\Qt\6.8.1\msvc2022_arm64
   ```

5. **Apple Bonjour SDK** - the Bonjour header (`dns_sd.h`) is already included in the repository under `lib/Bonjour/`.
   At runtime you need [Bonjour for Windows](https://support.apple.com/kb/DL999) installed,
   which provides `dnssd.dll`.

#### Option A - Open as CMake project in Visual Studio

1. Open Visual Studio and choose **Open a local folder**, then select the `ShairportQt` directory.
2. Visual Studio will detect `CMakeLists.txt` and configure automatically.
   If vcpkg is integrated, dependencies are found via the toolchain file.
3. Select the desired configuration (`x64-Release`, `x64-Debug`, `arm64-Release` or `arm64-Debug`) from the toolbar.
4. Build with **Build > Build All** (`Ctrl+Shift+B`).
5. The executable is located under `out/build/<config>/ShairportQt.exe`.

#### Option B - Command line

```cmd
git clone https://github.com/Frank-Friemel/ShairportQt.git
cd ShairportQt

REM x64 build
cmake -B build -S . ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
  -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ^
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

REM ARM64 build (cross-compile from x64 host)
cmake -B build-arm64 -S . -A ARM64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=arm64-windows-static ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.1\msvc2022_arm64 ^
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 --config Release
```

The resulting binary is `build\Release\ShairportQt.exe` (or `build-arm64\Release\ShairportQt.exe` for ARM64).

To run the unit tests:

```powershell
build\Release\ShairportQtTest.exe
```

### Linux

You may use Visual Studio Code or build from the command line.

#### Using vcpkg

```shell
git clone https://github.com/Frank-Friemel/ShairportQt.git
cd ShairportQt
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

#### Using system packages (Debian/Ubuntu)

```shell
sudo apt install cmake build-essential \
  libssl-dev libspdlog-dev libgtest-dev libgmock-dev libasound2-dev \
  libavahi-compat-libdnssd-dev qt6-base-dev libqt6dbus6

# install sockpp from source
git clone --depth 1 -b v1.0.0 https://github.com/fpagliughi/sockpp.git
cd sockpp
cmake -B build . -DCMAKE_INSTALL_PREFIX=/usr -DSOCKPP_BUILD_STATIC=ON -DSOCKPP_WITH_OPENSSL=ON
cmake --build build
sudo cmake --install build
cd ..

# build ShairportQt
git clone https://github.com/Frank-Friemel/ShairportQt.git
cd ShairportQt
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Credits

Thanks to James Laird who implemented the original version of "Shairport".
Special thanks to Japanese translator [maborosohin](https://github.com/maboroshin).
Localization: English, German, Japanese, Spanish, Catalan, French, Italian.

### Screenshots

<p float="left" align="center">

<img src="img/screen.png" width="40%" height="40%">

<img src="img/airplay.png" width="40%" height="40%">

</p>

### Usage Hints

Click on the time marker to the right of the progress bar to toggle between different display modes.

ShairportQt offers a tray icon which may be
disabled. If the function `Show "Now Playing" in Tray` is switched on, title information will only appear in
the tray if the main window is not visible on the desktop.
To completely hide/restore the main window from the taskbar you have to click
on the tray icon (for `Windows` users, it's a double click). A tray menu
will show up when you right click on the tray icon.

The multimedia buttons at the bottom of the main window are being used to remotely control
your connected device. This also applies to the volume buttons, so it's
*not* your local volume which will increase/decrease.

ShairportQt allows you to start multiple process-instances with different configurations. All you need to
do is to provide a name for your instance configuration by applying the command line parameter
`-config=MyConfigurationName`. The allowed characters for the configuration-name
are limited to characters `A-Z`, `a-z` and `0-9`.

### Avahi (aka Bonjour)

For Windows you may need to download and install [`Bonjour`](https://support.apple.com/kb/DL999). 
When being asked during the installation ... just dismiss the option to automatically update Bonjour's files in the background. 
Which may save you from having another unnecessary process running on your machine.
A desktop-link for Bonjour is also completely unnecessary.

On my Raspbian ... I had to install `libavahi-compat-libdnssd-dev`.

```sh
sudo apt install libavahi-compat-libdnssd-dev
```

For other Linux distributions, the package name is different:

- **Fedora:** `avahi-compat-libdns_sd`

Also you may need to enable/start the `avahi-daemon`:

```sh
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
```

On some Linux distributions you may have to install `avahi` via their own desktop installation tool. Please see my
Video [Installation of ShairportQt on Suse](https://youtu.be/UIfek93D5Hw).

