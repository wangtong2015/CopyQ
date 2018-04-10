#!/bin/bash
# Build with Travis CI.

set -exuo pipefail

version=$(git describe --match='v*')

root=$PWD
mkdir -p build
cd build

qt_bin="/usr/local/opt/qt5/bin"

"$qt_bin/qmake" \
    CONFIG+=release \
    CONFIG+=tests \
    DEFINES+=COPYQ_VERSION=\\\\\\\"\"$version\\\\\\\"\" \
    ..

make
make package_plugins
make package_translations
make package_themes

# Create "CopyQ.app/"
"$qt_bin/macdeployqt" CopyQ.app -verbose=2 -always-overwrite

executable="CopyQ.app/Contents/MacOS/copyq"

# Fix paths for included Qt libraries.
git clone https://github.com/iltommi/macdeployqtfix.git
python macdeployqtfix/macdeployqtfix.py "$executable" /usr/local/Cellar/qt5

# Test the app before deployment.
"$executable" --help
"$executable" --version
"$executable" --info

# Test paths and features.
ls "$("$executable" info plugins)/"
ls "$("$executable" info themes)/"
ls "$("$executable" info translations)/"
test "$("$executable" info has-global-shortcuts)" -eq "1"

# Run tests (retry once on error).
export COPYQ_TESTS_SKIP_COMMAND_EDIT=1
"$executable" tests ||
    "$executable" tests

# Create "CopyQ.dmg".
"$qt_bin/macdeployqt" CopyQ.app -verbose=2 -dmg -no-plugins

cd "$root"
