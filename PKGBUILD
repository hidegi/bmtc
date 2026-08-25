# Maintainer: hidegi <nachv2010@gmail.com>
pkgname=bmtc-git
pkgver=r1.0.0
pkgrel=1
pkgdesc="CLI tool for inspecting and converting BMT (Binary Multi-data Tree) files"
arch=('x86_64' 'aarch64')
url="https://github.com/hidegi/bmtc"
license=('zlib')
makedepends=('cmake' 'git')
provides=('bmtc')
conflicts=('bmtc')
source=(
    "${pkgname}::git+https://github.com/hidegi/bmtc.git"
    "bmt::git+https://github.com/hidegi/BMT-C-API.git"
    "nlohmann_json::git+https://github.com/nlohmann/json.git#tag=v3.11.3"
)
sha256sums=('SKIP' 'SKIP' 'SKIP')

pkgver() {
    cd "${pkgname}"
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cmake -B build -S "${pkgname}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
        -DFETCHCONTENT_SOURCE_DIR_BMT="${srcdir}/bmt" \
        -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON="${srcdir}/nlohmann_json"
    cmake --build build
}

package() {
    install -Dm755 "${srcdir}/build/bmtc" "${pkgdir}/usr/bin/bmtc"
}
