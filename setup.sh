#!/usr/bin/env bash
# =============================================================================
# Embedded Development Environment Setup — Ubuntu / Debian
# Covers: STM32 toolchain, OpenOCD, GDB, CMake, static analysis, testing,
#         documentation, code quality tools, Python tools, and CI helpers
# =============================================================================

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log()    { echo -e "${GREEN}[+]${NC} $*"; }
info()   { echo -e "${BLUE}[i]${NC} $*"; }
warn()   { echo -e "${YELLOW}[!]${NC} $*"; }
error()  { echo -e "${RED}[x]${NC} $*"; exit 1; }

check_root() {
    if [[ $EUID -eq 0 ]]; then
        error "Do not run this script as root. It will call sudo when needed."
    fi
}

# =============================================================================
# 1. SYSTEM UPDATE
# =============================================================================
update_system() {
    log "Updating package lists..."
    sudo apt-get update -qq
    sudo apt-get upgrade -y -qq
}

# =============================================================================
# 2. ARM CROSS-COMPILATION TOOLCHAIN
# =============================================================================
install_toolchain() {
    log "Installing ARM cross-compilation toolchain..."

    sudo apt-get install -y \
        gcc-arm-none-eabi \
        gdb-multiarch \
        binutils-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib

    # Verify installation
    arm-none-eabi-gcc --version | head -1 && log "arm-none-eabi-gcc OK"
    arm-none-eabi-gdb --version | head -1 && log "arm-none-eabi-gdb OK"

    # gdb-multiarch is preferred for FreeRTOS thread awareness plugins
    # Symlink if needed for scripts that call arm-none-eabi-gdb directly
    if ! command -v arm-none-eabi-gdb &>/dev/null; then
        sudo ln -sf "$(which gdb-multiarch)" /usr/local/bin/arm-none-eabi-gdb
        log "Symlinked gdb-multiarch -> arm-none-eabi-gdb"
    fi
}

# =============================================================================
# 3. BUILD SYSTEM
# =============================================================================
install_build_tools() {
    log "Installing build tools (CMake, Ninja, Make)..."

    sudo apt-get install -y \
        cmake \
        ninja-build \
        make

    # Check CMake version — need 3.20+ for modern embedded CMake patterns
    CMAKE_VERSION=$(cmake --version | head -1 | awk '{print $3}')
    CMAKE_MAJOR=$(echo "$CMAKE_VERSION" | cut -d. -f1)
    CMAKE_MINOR=$(echo "$CMAKE_VERSION" | cut -d. -f2)

    if [[ "$CMAKE_MAJOR" -lt 3 ]] || { [[ "$CMAKE_MAJOR" -eq 3 ]] && [[ "$CMAKE_MINOR" -lt 20 ]]; }; then
        warn "CMake $CMAKE_VERSION is older than 3.20. Installing latest via pip..."
        pip3 install --user cmake --upgrade
        export PATH="$HOME/.local/bin:$PATH"
        cmake --version | head -1
    else
        log "CMake $CMAKE_VERSION OK"
    fi

    ninja --version && log "Ninja OK"
}

# =============================================================================
# 4. OpenOCD — Flash and debug over SWD/JTAG
# =============================================================================
install_openocd() {
    log "Installing OpenOCD..."

    sudo apt-get install -y openocd

    # Verify and show version
    openocd --version 2>&1 | head -1 && log "OpenOCD OK"

    # Add user to plugdev and dialout groups for USB/serial access without sudo
    sudo usermod -aG plugdev "$USER"
    sudo usermod -aG dialout "$USER"
    log "Added $USER to plugdev and dialout groups (re-login required to take effect)"

    # Install udev rules for ST-Link
    if [[ ! -f /etc/udev/rules.d/49-stlinkv2.rules ]]; then
        log "Installing ST-Link udev rules..."
        sudo tee /etc/udev/rules.d/49-stlinkv2.rules > /dev/null <<'EOF'
# ST-Link V1
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3744", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
# ST-Link V2
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
# ST-Link V2.1 / Nucleo
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
# ST-Link V3
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374f", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3753", \
    MODE="660", GROUP="plugdev", TAG+="uaccess"
EOF
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        log "ST-Link udev rules installed"
    fi
}

# =============================================================================
# 5. STATIC ANALYSIS TOOLS
# =============================================================================
install_static_analysis() {
    log "Installing static analysis tools..."

    sudo apt-get install -y \
        cppcheck \
        clang \
        clang-tidy \
        clang-format

    cppcheck --version && log "cppcheck OK"
    clang-tidy --version | head -1 && log "clang-tidy OK"
    clang-format --version && log "clang-format OK"
}

# =============================================================================
# 6. UNIT TESTING — Unity + CMock
# =============================================================================
install_unity_cmock() {
    log "Installing Unity and CMock (C unit testing frameworks)..."

    UNITY_DIR="$HOME/embedded-tools/Unity"
    CMOCK_DIR="$HOME/embedded-tools/CMock"

    mkdir -p "$HOME/embedded-tools"

    if [[ ! -d "$UNITY_DIR" ]]; then
        git clone --depth 1 https://github.com/ThrowTheSwitch/Unity.git "$UNITY_DIR"
        log "Unity cloned to $UNITY_DIR"
    else
        info "Unity already exists at $UNITY_DIR, skipping"
    fi

    if [[ ! -d "$CMOCK_DIR" ]]; then
        git clone --depth 1 https://github.com/ThrowTheSwitch/CMock.git "$CMOCK_DIR"
        log "CMock cloned to $CMOCK_DIR"
    else
        info "CMock already exists at $CMOCK_DIR, skipping"
    fi

    # Ruby is required for CMock's mock generation scripts
    sudo apt-get install -y ruby
    ruby --version && log "Ruby (for CMock codegen) OK"
}

# =============================================================================
# 7. DOCUMENTATION — Doxygen + Graphviz
# =============================================================================
install_documentation_tools() {
    log "Installing documentation tools..."

    sudo apt-get install -y \
        doxygen \
        graphviz

    doxygen --version && log "Doxygen OK"
    dot -V 2>&1 | head -1 && log "Graphviz OK"
}

# =============================================================================
# 8. PYTHON ENVIRONMENT
# =============================================================================
install_python_tools() {
  # Install pipx for standalone Python CLI tools
  sudo apt-get install -y pipx
  pipx ensurepath

  # Install pre-commit via pipx (it's a CLI tool, perfect fit for pipx)
  pipx install pre-commit

  # Create a dedicated venv for embedded dev tools
  python3 -m venv ~/.venv/embedded
  source ~/.venv/embedded/bin/activate

  # Install packages into the venv
  pip install \
      pyserial \
      intelhex \
      pyelftools \
      pyyaml \
      compiledb

  deactivate

}

# =============================================================================
# 9. SERIAL TERMINAL — for UART debugging
# =============================================================================
install_serial_tools() {
    log "Installing serial terminal tools..."

    sudo apt-get install -y \
        minicom \
        picocom \
        screen

    log "Serial tools OK — use 'picocom -b 115200 /dev/ttyACM0' for Nucleo virtual COM"
}

# =============================================================================
# 10. VERSION CONTROL EXTRAS
# =============================================================================
install_git_tools() {
    log "Installing Git and extras..."

    sudo apt-get install -y \
        git \
        git-lfs

    git --version && log "Git OK"
    git lfs version && log "Git LFS OK"

    # Configure useful Git globals if not already set
    if [[ -z "$(git config --global user.name 2>/dev/null)" ]]; then
        warn "Git user.name not set. Set it with: git config --global user.name 'Your Name'"
    fi
    if [[ -z "$(git config --global user.email 2>/dev/null)" ]]; then
        warn "Git user.email not set. Set it with: git config --global user.email 'you@example.com'"
    fi

    # Set up useful Git aliases
    git config --global alias.lg "log --oneline --graph --decorate --all"
    git config --global alias.st "status --short"
    git config --global alias.unstage "reset HEAD --"
    log "Git aliases configured (lg, st, unstage)"
}

# =============================================================================
# 11. VS CODE EXTENSIONS (optional — only if VS Code is installed)
# =============================================================================
install_vscode_extensions() {
    if command -v code &>/dev/null; then
        log "VS Code detected — installing recommended extensions..."

        EXTENSIONS=(
            "ms-vscode.cpptools"              # C/C++ IntelliSense
            "ms-vscode.cmake-tools"           # CMake integration
            "marus25.cortex-debug"            # Cortex-M debug with OpenOCD
            "dan-c-underwood.arm"             # ARM assembly syntax highlighting
            "zixuanwang.linkerscript"         # Linker script syntax highlighting
            "cschlosser.doxdocgen"            # Doxygen comment generation
            "streetsidesoftware.code-spell-checker"  # Catch typos in comments/docs
            "mhutchie.git-graph"              # Visual Git graph
            "eamodio.gitlens"                 # Git blame and history
        )

        for EXT in "${EXTENSIONS[@]}"; do
            code --install-extension "$EXT" --force 2>/dev/null && log "Installed: $EXT"
        done
    else
        info "VS Code not found — skipping extension installation"
        info "Install VS Code from https://code.visualstudio.com/ then re-run this block"
    fi
}

# =============================================================================
# 12. MISC UTILITIES
# =============================================================================
install_misc_utils() {
    log "Installing miscellaneous utilities..."

    # xxd, hexdump: inspecting binary firmware files
    # stlink-tools:  st-flash / st-util, an alternative to OpenOCD for simple flashing
    sudo apt-get install -y \
        curl \
        wget \
        unzip \
        xxd \
        hexdump \
        stlink-tools

    st-flash --version 2>&1 | head -1 && log "st-flash OK"
}

# =============================================================================
# 13. GENERATE PROJECT CONFIG FILES
# =============================================================================
generate_config_files() {
    log "Generating project configuration file templates..."

    CONFIG_DIR="$HOME/embedded-tools/templates"
    mkdir -p "$CONFIG_DIR"

    # -------------------------------------------------------------------------
    # .clang-format
    # -------------------------------------------------------------------------
    cat > "$CONFIG_DIR/.clang-format" <<'EOF'
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Allman
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: false
SortIncludes: true
IncludeBlocks: Regroup
EOF
    log "Generated .clang-format -> $CONFIG_DIR/.clang-format"

    # -------------------------------------------------------------------------
    # .clang-tidy
    # -------------------------------------------------------------------------
    cat > "$CONFIG_DIR/.clang-tidy" <<'EOF'
Checks: >
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  performance-*,
  portability-*,
  readability-identifier-naming,
  -cert-err34-c,
  -bugprone-easily-swappable-parameters

WarningsAsErrors: ""

CheckOptions:
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE
  - key: readability-identifier-naming.TypedefCase
    value: lower_case
  - key: readability-identifier-naming.TypedefSuffix
    value: _t
EOF
    log "Generated .clang-tidy -> $CONFIG_DIR/.clang-tidy"

    # -------------------------------------------------------------------------
    # .pre-commit-config.yaml
    # -------------------------------------------------------------------------
    cat > "$CONFIG_DIR/.pre-commit-config.yaml" <<'EOF'
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
        args: ['--maxkb=500']
      - id: mixed-line-ending
        args: ['--fix=lf']

  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v17.0.6
    hooks:
      - id: clang-format
        types: [c, c++]
EOF
    log "Generated .pre-commit-config.yaml -> $CONFIG_DIR/.pre-commit-config.yaml"

    # -------------------------------------------------------------------------
    # .gitignore for embedded CMake project
    # -------------------------------------------------------------------------
    cat > "$CONFIG_DIR/.gitignore" <<'EOF'
# Build artifacts
build/
cmake-build-*/
*.elf
*.bin
*.hex
*.map
*.lst
*.axf

# Generated files
*.d
*.o
*.a
*.su

# IDE
.vscode/
.idea/
*.ioc
*.ioc.bak

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
install_manifest.txt
compile_commands.json

# Doxygen
docs/html/
docs/latex/

# Python
__pycache__/
*.pyc
.venv/

# OS
.DS_Store
Thumbs.db
EOF
    log "Generated .gitignore -> $CONFIG_DIR/.gitignore"

    # -------------------------------------------------------------------------
    # Doxyfile (minimal)
    # -------------------------------------------------------------------------
    cat > "$CONFIG_DIR/Doxyfile" <<'EOF'
PROJECT_NAME           = "STM32 Firmware Stack"
PROJECT_VERSION        = "0.1.0"
OUTPUT_DIRECTORY       = docs
INPUT                  = src include
FILE_PATTERNS          = *.c *.h
RECURSIVE              = YES
EXTRACT_ALL            = YES
EXTRACT_STATIC         = YES
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HAVE_DOT               = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
UML_LOOK               = YES
TEMPLATE_RELATIONS     = YES
JAVADOC_AUTOBRIEF      = YES
OPTIMIZE_OUTPUT_FOR_C  = YES
WARN_IF_UNDOCUMENTED   = YES
QUIET                  = YES
EOF
    log "Generated Doxyfile -> $CONFIG_DIR/Doxyfile"

    # -------------------------------------------------------------------------
    # GitHub Actions workflow
    # -------------------------------------------------------------------------
    mkdir -p "$CONFIG_DIR/.github/workflows"
    cat > "$CONFIG_DIR/.github/workflows/build.yml" <<'EOF'
name: Build and Analyze

on:
  push:
    branches: [ main, develop, 'phase/**' ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install ARM toolchain
        run: |
          sudo apt-get update -qq
          sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi \
            libnewlib-arm-none-eabi cmake ninja-build cppcheck

      - name: Configure (bootloader)
        run: cmake -B build/bootloader -S bootloader -G Ninja \
               -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake

      - name: Build (bootloader)
        run: cmake --build build/bootloader

      - name: Configure (application)
        run: cmake -B build/app -S app -G Ninja \
               -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake

      - name: Build (application)
        run: cmake --build build/app

      - name: Binary size report
        run: |
          echo "### Binary Sizes" >> $GITHUB_STEP_SUMMARY
          echo '```' >> $GITHUB_STEP_SUMMARY
          arm-none-eabi-size build/bootloader/*.elf build/app/*.elf >> $GITHUB_STEP_SUMMARY
          echo '```' >> $GITHUB_STEP_SUMMARY

      - name: Static analysis (cppcheck)
        run: |
          cppcheck --enable=warning,performance,portability \
                   --error-exitcode=1 \
                   --suppress=missingIncludeSystem \
                   src/ bsp/

      - name: Upload build artifacts
        uses: actions/upload-artifact@v4
        with:
          name: firmware-binaries
          path: |
            build/**/*.elf
            build/**/*.bin
            build/**/*.hex
EOF
    log "Generated GitHub Actions workflow -> $CONFIG_DIR/.github/workflows/build.yml"

    info "All config templates saved to $CONFIG_DIR"
    info "Copy them into your project root when you create it"
}

# =============================================================================
# 14. SUMMARY
# =============================================================================
print_summary() {
    echo ""
    echo -e "${GREEN}============================================================${NC}"
    echo -e "${GREEN}  Installation complete${NC}"
    echo -e "${GREEN}============================================================${NC}"
    echo ""
    echo -e "${BLUE}Installed:${NC}"
    echo "  ARM toolchain      arm-none-eabi-gcc, gdb-multiarch, binutils"
    echo "  Build system       CMake, Ninja, Make"
    echo "  Debug/Flash        OpenOCD, st-flash, ST-Link udev rules"
    echo "  Static analysis    cppcheck, clang-tidy, clang-format"
    echo "  Unit testing       Unity, CMock (~/embedded-tools/)"
    echo "  Documentation      Doxygen, Graphviz"
    echo "  Python tools       pyserial, intelhex, pyelftools, pre-commit"
    echo "  Serial terminals   minicom, picocom, screen"
    echo "  Git extras         git-lfs, aliases (lg, st, unstage)"
    echo "  Config templates   ~/embedded-tools/templates/"
    echo ""
    echo -e "${YELLOW}Action required:${NC}"
    echo "  1. Log out and back in for group changes to take effect"
    echo "     (plugdev + dialout for USB/serial access without sudo)"
    echo "  2. Copy templates from ~/embedded-tools/templates/ into your project"
    echo "  3. Run 'pre-commit install' inside your project after copying"
    echo "     .pre-commit-config.yaml"
    echo "  4. Install VS Code manually if not already installed:"
    echo "     https://code.visualstudio.com/"
    echo "     Then re-run install_vscode_extensions() or run this script again"
    echo ""
    echo -e "${BLUE}Key paths:${NC}"
    echo "  Unity              ~/embedded-tools/Unity"
    echo "  CMock              ~/embedded-tools/CMock"
    echo "  Config templates   ~/embedded-tools/templates"
    echo ""
    echo -e "${BLUE}Quick verify:${NC}"
    echo "  arm-none-eabi-gcc --version"
    echo "  openocd --version"
    echo "  cmake --version"
    echo "  cppcheck --version"
    echo "  doxygen --version"
    echo ""
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    check_root

    echo -e "${BLUE}"
    echo "  ╔═══════════════════════════════════════════════════╗"
    echo "  ║   STM32 Embedded Dev Environment Setup            ║"
    echo "  ║   Ubuntu / Debian                                 ║"
    echo "  ╚═══════════════════════════════════════════════════╝"
    echo -e "${NC}"

    update_system
    install_toolchain
    install_build_tools
    install_openocd
    install_static_analysis
    install_unity_cmock
    install_documentation_tools
    install_python_tools
    install_serial_tools
    install_git_tools
    install_misc_utils
    install_vscode_extensions
    generate_config_files
    print_summary
}

main "$@"
