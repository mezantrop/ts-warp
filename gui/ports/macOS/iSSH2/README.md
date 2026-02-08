# iSSH2-head

iSSH2-head is a fork of [iSSH](https://github.com/Frugghi/iSSH2) bash script with modifications for compiling bleeding edge versions of Libssh2 (and OpenSSL) for iOS, macOS, watchOS and tvOS.

<a href="https://www.buymeacoffee.com/mezantrop" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>

- Libssh2: [Website](http://www.libssh2.org) | [Documentation](http://www.libssh2.org/docs.html) | [Changelog](http://www.libssh2.org/changes.html)
- OpenSSL: [Website](http://www.openssl.org) | [Documentation](http://www.openssl.org/docs/) | [Changelog](http://www.openssl.org/news/)

## Requirements

- Xcode
- Xcode Command Line Tools

## How to use

1. Download the script
2. Run `iSSH2-head.sh` passing `--platform=PLATFORM --min-version=VERS` or `--xcodeproj=PATH --target=TARGET` as options (for example: `./iSSH2.sh --platform=iphoneos --min-version=8.0`)
3. Grab a cup of coffee while waiting

## Script help

```sh
Usage: iSSH2-head.sh [options]

Download and build bleeding edge OpenSSL and Libssh2 libraries.

Options:
  -a, --archs=ARCHS
    target architectures (platform-specific)

  -p, --platform=PLATFORM
    target platform: iphoneos|macosx|appletvos|watchos

  -v, --min-version=VERS    set platform minimum version to VERS
  -s, --sdk-version=VERS    use SDK version VERS
  -x, --xcodeproj=PATH      get info from the project (requires --target)
  -t, --target=TARGET       get info from the target (requires --xcodeproj)
      --build-only-openssl  build OpenSSL and skip Libssh2
      --no-clean            do not clean build folder
      --no-bitcode          do not embed bitcode
  -h, --help                display this help and exit

You must specify either:
  --xcodeproj + --target
or:
  --platform + --min-version

Supported architectures by platform:
  macosx:     x86_64 | arm64
  iphoneos:   arm64 | arm64e | armv7 | armv7s
  watchos:    armv7k | arm64_32
  appletvos:  arm64

Examples:
  iSSH2-head.sh --platform=macosx --min-version=11 --archs="arm64 x86_64"
  iSSH2-head.sh --xcodeproj path/to/MyApp.xcodeproj --target MyApp
```

## License

- Original iSSH2 code by Tommaso Madonia is licensed under **MIT License** (see LICENSE file).
- All modifications by Mikhail Zakharov are licensed under **BSD-2-Clause License** (see LICENSE file).
