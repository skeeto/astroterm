# ✨ starsaver

![Test Status](https://github.com/da-luce/starsaver/actions/workflows/ci.yml/badge.svg)
![Codecov App](https://private-user-images.githubusercontent.com/152432831/340411587-e90313f4-9d3a-4b63-8b54-cfe14e7ec20d.svg?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MzQ1NjYyNjEsIm5iZiI6MTczNDU2NTk2MSwicGF0aCI6Ii8xNTI0MzI4MzEvMzQwNDExNTg3LWU5MDMxM2Y0LTlkM2EtNGI2My04YjU0LWNmZTE0ZTdlYzIwZC5zdmc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjQxMjE4JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI0MTIxOFQyMzUyNDFaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT1iNmM0ZWNjNzA5ZDdmM2M5OGFjZGI4NDIyODM5MmYxN2M3NWY5MDAzN2QwNGNiZGRkMDM2MjVlNjI3Yjk2NmEwJlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCJ9.kPnqrMGue2upMZ2ZSnpc76vHX1OMzXP-a70-Bjm4feo)

Stellar magic, now in your terminal! ✨🪐 View the live position stars, planets, constellations, and more, all rendered right the command line—no telescope required! 🌌

![Screenshot of Starsaver](/assets/screenshot.png)

_<p align="center">Stars above Boston around 2 PM on December 17, 2024</p>_

- [✨ starsaver](#-starsaver)
  - [Building](#building)
    - [Requirements](#requirements)
    - [Installation](#installation)
  - [Usage](#usage)
    - [Options](#options)
    - [Example](#example)
  - [Development](#development)
    - [Known issues](#known-issues)
    - [Testing](#testing)
  - [Citations](#citations)
  - [Data Sources](#data-sources)

---

## Building

> Ncurses detection is spotty on some systems, and you may need to install
> [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) in order
> for Meson to find it. You may install it via [Homebrew](https://formulae.brew.sh/formula/ncurses) on macOS.

### Requirements

- Unix-like environment (Linux, macOS, WSL, etc.)
- C compiler
- [`ncurses`](https://invisible-island.net/ncurses/announce.html) library
- [`meson`](https://github.com/mesonbuild/meson) 1.4.0 or newer
- [`ninja`](https://github.com/ninja-build/ninja) 1.8.2 or newer
- Some common CLI tools (_these are checked for automatically during install_)
  - [`wget`](https://www.gnu.org/software/wget/) or [`curl`](https://curl.se/)
  - [`xxd`](https://linux.die.net/man/1/xxd)
  - [`sed`](https://www.gnu.org/software/sed/manual/sed.html)

### Installation

Clone the repository and enter the project directory:

```sh
git clone https://github.com/da-luce/starsaver && cd starsaver
```

Run the install script:

```sh
sh install.sh
```

You may now run the generated `./build/starsaver` binary or add the `starsaver` command system wide via `meson install -C build`. Pressing <kbd>q</kbd> or <kbd>ESC</kbd> will exit the display.

---

## Usage

### Options

The `--help` flag displays all supported options.

### Example

Say we wanted to view the sky at 5:00 AM (Eastern) on July 16, 1969—the morning
of the Apollo 11 launch at the Kennedy Space Center in Florida. We would run:

```sh
starsaver --latitude 28.573469 --longitude -80.651070 --datetime 1969-7-16T9:32:00
```

If we then wanted to display all stars with a magnitude brighter than or equal
to 5.0 and add color, we would add `--threshold 5.0 --color` as options.

If you simply want the current time, don't specify the `--datetime` option and
_starsaver_ will use the system time. For your current location, you will still
have to specify the `--lat` and `--long` options.

For more options and help run `starsaver -h` or `starsaver --help`.

> ℹ️ Use a tool like [LatLong](https://www.latlong.net/) to get your latitude and longitude.

> ℹ️ Star magnitudes decrease as apparent brightness increases, i.e. to show more stars, increase the threshold.

---

## Development

### [Known issues](https://github.com/da-luce/starsaver/issues)

### Testing

Run `meson test` within the build directory. To get a coverage report, subsequently run `ninja coverage`.

---

## Citations

Many thanks to the following resources, which were invaluable to the development of this project.

- [Map Projections-A Working Manual By JOHN P. SNYDER](https://pubs.usgs.gov/pp/1395/report.pdf)
- [Wikipedia](https://en.wikipedia.org)
- [Atractor](https://www.atractor.pt/index-_en.html)
- [Jon Voisey's Blog: Following Kepler](https://jonvoisey.net/blog/)
- [Celestial Programming: Greg Miller's Astronomy Programming Page](https://astrogreg.com/convert_ra_dec_to_alt_az.html)

---

## Data Sources

- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation figures: [Stellarium](https://stellarium.org/)
- Planet orbital elements: [NASA Jet Propulsion Laboratory](https://ssd.jpl.nasa.gov/planets/approx_pos.html)
- Planet magnitudes: [Computing Apparent Planetary Magnitudes for The Astronomical Almanac](https://arxiv.org/abs/1808.01973)