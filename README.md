# XPAutoThrottle

Thanks to [OpenALs Example Repo](https://github.com/6639835/xplane-plugin-example/) for the intro to XP plugins and sample scripts!

## Download Pre-compiled Binaries

1. **Latest Release**: Go to the [Releases page](../../releases) to download the latest stable version

Each release includes:
- `win.xpl` - Windows (64-bit)
- `mac.xpl` - macOS Universal (Intel + Apple Silicon)
- `lin.xpl` - Linux (64-bit)
- `XPAutoThrottle.zip` - All platforms

### Automatic Release Process

The GitHub Actions workflow automatically:
- ✅ Builds for Windows, macOS, and Linux on every commit
- ✅ Runs comprehensive cross-platform tests
- ✅ Creates releases when the `VERSION` file is updated
- ✅ Packages all platforms into convenient downloads
- ✅ Generates installation instructions

**Simply update the `VERSION` file and push to trigger a new release!**

## 🛠 Installation

### Option 1: Use Pre-compiled Binaries (Recommended)

1. Download the appropriate `.xpl` file for your platform from the [Releases page](../../releases)
2. Create the plugin directory: `X-Plane/Resources/plugins/XPAutoThrottle/`
3. Copy the `.xpl` file to this directory
4. Restart X-Plane

### Option 2: Manual Compilation

If you need to modify the code or prefer to compile yourself:

#### Prerequisites
- **CMake** 3.16 or later
- **C++ compiler** with C++17 support

#### Quick Build
```bash
# Clone the repository
git clone https://github.com/idanoo/XPAutoThrottle
cd XPAutoThrottle

# Build for your platform
python build_all.py

# Or build with cleanup
python build_all.py clean
```

#### Platform-Specific Build Commands
```bash
# Windows
python build_all.py --platform windows

# macOS  
python build_all.py --platform mac

# Linux
python build_all.py --platform linux
```

The compiled plugin will be in the `build/` directory.

## 🎮 Usage

1. **Load X-Plane** with the plugin installed
2. **Load any aircraft** - the plugin will automatically play a sound
3. **Check Log.txt** for plugin status messages and OpenAL information
4. **Each aircraft load** will play the sound at a progressively higher pitch

### Creating a New Release

1. **Update the `VERSION` file** with the new version number (e.g., `1.1.0`)
2. **Commit and push** the change to the main branch
3. **GitHub Actions will automatically**:
   - Build all platforms
   - Create a git tag
   - Generate a GitHub release
   - Upload all binaries
