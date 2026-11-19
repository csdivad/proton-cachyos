#!/bin/sh
# Assemble the onnxruntime tree piper-phonemize expects (include/*.h and
# lib/libonnxruntime.so*) from Microsoft's NuGet package. The upstream
# download in piper-phonemize comes from microsoft/onnxruntime's GitHub
# releases, which disappeared on 2026-11-17; NuGet still serves the same
# Microsoft 1.14.1 binaries.
#
# Usage: fetch-onnxruntime.sh <dest> <x86_64|aarch64> <version> <sha256> <url>
set -eu

dest=$1
arch=$2
version=$3
sha256=$4
url=$5

case "$arch" in
    x86_64)  nuget_arch=linux-x64 ;;
    aarch64) nuget_arch=linux-arm64 ;;
    *) echo "unsupported arch: $arch" >&2; exit 1 ;;
esac

nupkg="$dest/.onnxruntime.nupkg"
unpack="$dest/.unpack"

rm -rf "$nupkg" "$unpack" "$dest/include" "$dest/lib"
mkdir -p "$dest/include" "$dest/lib" "$unpack"

echo ":: fetching onnxruntime $version from nuget.org ($nuget_arch)..." >&2
wget -q --no-use-server-timestamps -O "$nupkg" "$url"
echo "$sha256  $nupkg" | sha256sum -c - >/dev/null

unzip -q -o "$nupkg" 'build/native/include/*' "runtimes/$nuget_arch/native/libonnxruntime.so" -d "$unpack"
cp "$unpack/build/native/include/"*.h "$dest/include/"
cp "$unpack/runtimes/$nuget_arch/native/libonnxruntime.so" "$dest/lib/libonnxruntime.so.$version"
ln -sfn "libonnxruntime.so.$version" "$dest/lib/libonnxruntime.so"

rm -rf "$nupkg" "$unpack"
touch "$dest/.ready"
