# gbooks-scrapper

<img src="/scrappy.jpg" height=150px />

Google books scrapper made with C

## Download

- [Linux v1.0.1](https://github.com/GelaniNijraj/gbooks-scrapper/releases/download/v1.0.1/gbooks-scrapper)
- [Windows v1.0.1](https://github.com/GelaniNijraj/gbooks-scrapper/releases/download/v1.0.1/gbooks-scrapper.exe)

## Build from source

```
make
make install
```

## Usage

```
Usage: gbooks-scrapper [OPTIONS]
A tool to scrape pages as images from books.google.com

  -i, --id=ID               ID of the book
  -I, --info                get book information only
  -c, --complete            download complete book
  -t, --target-location     download files location
  -s, --start-page=PAGE     start download from page PAGE
  -e, --end-page=PAGE       download pages upto PAGE
  -v, --verbose             show status messages
  -h, --help                get this message
```

## Issues

Report any issues, bugs or feature requests at [issues](https://github.com/GelaniNijraj/gbooks-scrapper/issues) page.

## License

Copyright (c) 2017 Nijraj Gelani

This software is distributed under [MIT license](http://www.opensource.org/licenses/mit-license.php).
