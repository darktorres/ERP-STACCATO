FROM ubuntu:22.04

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# Set up user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    make \
    gcc \
    g++ \
    clang \
    clang-tools \
    gdb \
    ccache \
    pkg-config \
    ninja-build \
    # Qt development dependencies
    qt5-qmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    libqt5sql5-mysql \
    libqt5network5 \
    libqt5xml5 \
    libqt5charts5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    # Additional Qt modules
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    libqt5printsupport5 \
    # Development tools
    git \
    curl \
    wget \
    unzip \
    zip \
    vim \
    nano \
    tree \
    jq \
    sudo \
    # SSL and networking
    libssl-dev \
    libcurl4-openssl-dev \
    ca-certificates \
    # X11 and display support (for GUI testing)
    xvfb \
    x11-apps \
    libxcb-xinerama0 \
    # Clean up
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Install Qt 5.15.2 from official installer (for latest version)
RUN mkdir -p /opt/Qt && cd /tmp \
    # Download Qt online installer script alternative - using system Qt for now
    # The system Qt 5.15.2 should be sufficient for development
    && echo "Using system Qt installation"

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && mkdir -p /etc/sudoers.d \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Set up ccache
RUN mkdir -p /home/$USERNAME/.ccache \
    && echo "max_size = 2G" > /home/$USERNAME/.ccache/ccache.conf \
    && echo "sloppiness = pch_defines,time_macros" >> /home/$USERNAME/.ccache/ccache.conf \
    && chown -R $USERNAME:$USERNAME /home/$USERNAME/.ccache

# Configure environment for Qt development
ENV QT_SELECT=qt5
ENV QTDIR=/usr/lib/x86_64-linux-gnu/qt5
ENV PATH=$QTDIR/bin:$PATH
ENV LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
ENV PKG_CONFIG_PATH=$QTDIR/lib/pkgconfig:$PKG_CONFIG_PATH

# Set up development environment
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CCACHE_DIR=/home/$USERNAME/.ccache

# Create workspace directory
RUN mkdir -p /workspace && chown $USERNAME:$USERNAME /workspace

# Install additional development tools
RUN curl -fsSL https://get.docker.com | sh \
    && usermod -aG docker $USERNAME

# Set up Git configuration template
RUN git config --system --add safe.directory /workspace

# Install modern CMake (optional, for potential future use)
RUN wget -qO- https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-x86_64.tar.gz | tar -xz -C /opt \
    && ln -s /opt/cmake-3.27.7-linux-x86_64/bin/* /usr/local/bin/

# Create convenience scripts
RUN echo '#!/bin/bash\n\
cd /workspace\n\
echo "Configuring project with qmake..."\n\
qmake Loja.pro\n\
echo "Building project..."\n\
make -j$(nproc)\n\
' > /usr/local/bin/build-project && chmod +x /usr/local/bin/build-project

RUN echo '#!/bin/bash\n\
cd /workspace\n\
echo "Cleaning build files..."\n\
make clean 2>/dev/null || true\n\
rm -f Makefile* .qmake.stash\n\
echo "Clean completed."\n\
' > /usr/local/bin/clean-project && chmod +x /usr/local/bin/clean-project

# Set up environment for X11 forwarding (for GUI applications)
ENV DISPLAY=:99
RUN echo '#!/bin/bash\n\
Xvfb :99 -screen 0 1024x768x16 &\n\
export DISPLAY=:99\n\
exec "$@"\n\
' > /usr/local/bin/xvfb-run-gui && chmod +x /usr/local/bin/xvfb-run-gui

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog

# Switch to the created user
USER $USERNAME

# Set up workspace
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]