# ✨cstar

View stars in your terminal!

![](/assets/screenshot.png)

## Command Line Options

| Option                            | Description                                       | Default               |
|-----------------------------------|---------------------------------------------------|-----------------------|
| **`--latitude, -a lat`**          | Latitude of observation in radians                | Boston, MA            |
| **`--longitude, -o long`**        | Longitude of observation in radians               | Boston, MA            |
| **`--julian-date, -j jd`**        | Julian date of observation                        | Current system time   |
| **`--threshold, -t thresh`**      | Only stars brighter than this will be rendered    | 3.0                   |
| **`--label-thresh, -l thresh`**   | Only stars brighter than this will be labeled     | 0.5                   |
| **`--fps, -f fps`**               | Frames per second                                 | 24                    |
| **`--animation-mult, -m mult`**   | Real time animation speed multiplier              | 1.0                   |
| **`--no-unicode`**                | Only use ASCII characters                         |                       |
| **`--grid`**                      | Draw an azimuthal grid                            |                       |

> ⓘ Use a tool like https://www.aavso.org/jd-calculator to convert Gregorian Calendar  Dates to Julian Dates

> ⓘ Star magnitudes decrease as apparent brightness increases

## Building

Compile with gcc:
```
gcc star.c bit.c term.c coord.c astro.c misc.c parse_BSC5.c -lm -lncursesw
```
_You only need to link `-lncurses` if not using unicode_

## Data Sources
- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star Names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation Figure: [Stellarium](https://stellarium.org/)