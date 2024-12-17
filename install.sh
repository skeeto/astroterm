#!/bin/sh
 
SCRIPT_DIR="$(dirname "$0")" # directory where the script is located

download_bsc5() {
    BSC5_URL="http://tdc-www.harvard.edu/catalogs/BSC5"
    BSC5_LOCATION="data/BSC5"
    DOWNLOAD_LOCATION="$SCRIPT_DIR/$BSC5_LOCATION"

    if [ -f "$DOWNLOAD_LOCATION" ]; then
        echo "BSC5 file already exists at $DOWNLOAD_LOCATION. Skipping download."
        return
    fi
    if command -v curl > /dev/null 2>&1; then
        echo "Using curl to download BSC5..."
        curl -L -o  "$DOWNLOAD_LOCATION" "$BSC5_URL"
    elif command -v wget > /dev/null 2>&1; then
        echo "Using wget to download BSC5..."
        wget -O "$DOWNLOAD_LOCATION" "$BSC5_URL"
    else
        echo "Error: Neither curl nor wget is installed. Please install one of them, or download BSC5 manually: $BSC5_URL"
        exit 1
    fi

    echo "Download completed successfully and saved to $DOWNLOAD_LOCATION"
}

build_with_meson() {
    BUILD_DIR="$SCRIPT_DIR/build"

    if [ "$GITHUB_ACTIONS" = "true" ]; then
        MESON_FLAGS=" --buildtype debugoptimized -Db_coverage=true"
        echo "Running as a GitHub Action. Adding flags: $MESON_FLAGS"
    else
        MESON_FLAGS=""
    fi

    # Run Meson configure step
    echo "Configuring project with Meson..."
    meson setup $MESON_FLAGS "$BUILD_DIR" "$SCRIPT_DIR" --wipe

    # Run Meson build step
    echo "Building project with Ninja..."
    meson compile -C "$BUILD_DIR"
}


download_bsc5
build_with_meson

