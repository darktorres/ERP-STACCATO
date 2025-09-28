# Staccato ERP Development Container

This devcontainer provides a complete development environment for the Staccato ERP Qt C++ application.

## 🚀 Quick Start

1. **Prerequisites:**
   - Docker Desktop installed and running
   - VS Code with the "Dev Containers" extension

2. **Open in Container:**
   - Open this project in VS Code
   - Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
   - Select "Dev Containers: Reopen in Container"
   - Wait for the container to build (first time may take 10-15 minutes)

## 🛠️ Development Environment

### Included Tools
- **Qt 5.15.2** - Complete Qt development framework
- **GCC/Clang** - Modern C++ compilers with C++17 support
- **qmake** - Qt's build system
- **ccache** - Compiler cache for faster rebuilds
- **Git** - Version control
- **VS Code Extensions** - C++, CMake, and development tools

### Build Commands

```bash
# Quick build script
build-project

# Manual build process
qmake Loja.pro
make -j$(nproc)

# Clean build files
clean-project
```

### Database Configuration

This devcontainer focuses on Qt C++ development only. You'll need to connect to an external MySQL database:

### Connection String Example
```cpp
QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
db.setHostName("your-mysql-host");  // e.g., "localhost", "192.168.1.100"
db.setDatabaseName("staccato");
db.setUserName("your-username");
db.setPassword("your-password");
```

### Database Setup Options
1. **Local MySQL Server** - Install MySQL on your host machine
2. **Docker MySQL** - Run MySQL in a separate container
3. **Cloud Database** - Use services like AWS RDS, Google Cloud SQL, etc.
4. **XAMPP/WAMP** - For Windows development environment

## 🏗️ Architecture Notes

### Cross-Platform Considerations
- This devcontainer uses Linux (Ubuntu 22.04) for development
- The original codebase targets Windows with MSVC
- Linux build uses GCC/Clang instead of MSVC
- Some Windows-specific dependencies may need alternatives:
  - ACBr libraries (Brazilian fiscal compliance)
  - Windows-specific OpenSSL/cURL builds

### Third-Party Dependencies
The following dependencies are included in the `3rdparty/` directory:
- **QtXlsxWriter** - Excel file generation
- **QSimpleUpdater** - Application auto-update
- **LimeReport 1.5.68** - Report generation
- **OpenSSL** - Cryptographic operations
- **cURL** - HTTP client functionality
- **ACBr** - Brazilian compliance (may need adaptation for Linux)

### GUI Applications
- X11 forwarding is configured for GUI applications
- Use `xvfb-run-gui` prefix for headless GUI testing
- Display is set to `:99` by default

## 🔧 Customization

### Adding VS Code Extensions
Edit `.devcontainer/devcontainer.json` and add to the `extensions` array:
```json
"extensions": [
  "your.extension.id"
]
```

### Modifying Build Environment
- Edit `.devcontainer/Dockerfile` to add system packages
- Modify `.devcontainer/docker-compose.yml` for service configuration
- Update environment variables in `devcontainer.json`

### Database Configuration
- Connect to external MySQL database
- Configure connection settings in your application

## 🐛 Troubleshooting

### Container Won't Start
```bash
# Rebuild container
docker-compose -f .devcontainer/docker-compose.yml build --no-cache

# Check logs
docker-compose -f .devcontainer/docker-compose.yml logs
```

### Build Errors
```bash
# Clear Qt cache
rm -rf ~/.cache/qmake

# Clean and rebuild
clean-project
build-project
```

### GUI Application Issues
```bash
# Start X server manually
Xvfb :99 -screen 0 1024x768x16 &
export DISPLAY=:99

# Test GUI
xvfb-run-gui ./your-qt-app
```

## 📁 Directory Structure

```
.devcontainer/
├── devcontainer.json    # Main configuration
├── Dockerfile          # Development environment
├── docker-compose.yml  # Services (app + database)
└── README.md           # This file

3rdparty/               # Third-party libraries
src/                    # Source code
ui/                     # Qt Designer forms
tests/                  # Test suite
initdb.sql             # Database schema (for external setup)
Loja.pro               # Main project file
```

## 🤝 Contributing

1. Make sure your changes work in the devcontainer
2. Test both debug and release builds
3. Verify database migrations work correctly
4. Run the test suite: `cd tests && qmake tests.pro && make && ./tests`

## 📋 Known Limitations

- ACBr libraries may need Linux equivalents
- Windows-specific features may require adaptation
- Some Qt modules might have different versions than production
- File paths use forward slashes (Linux) vs backslashes (Windows)

## 🔄 Migration from Windows Development

If migrating from Windows development:
1. Update file paths to use forward slashes
2. Check for Windows-specific dependencies
3. Verify database connection strings
4. Test all critical functionality in the Linux environment
5. Consider setting up CI/CD for both Linux and Windows builds