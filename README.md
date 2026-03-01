# VArchive

VArchive is a professional, full-featured archive manager built specifically for the **VAXP-OS** ecosystem. Designed as a modern alternative , VArchive leverages GTK4 and libadwaita to deliver a stunning glassmorphism user interface while providing comprehensive archive management capabilities.

## Features

- **Extensive Format Support**: Seamlessly handle over 40 archive and compression formats.
  - **Read & Write**: ZIP, TAR (GZ, BZ2, XZ, ZST, LZ), 7Z, CPIO, AR, ARJ, ZOO, and more.
  - **Read-Only**: RAR, ISO, DEB, RPM, CAB, LHA, DMG, Snap, Squashfs, Stuffit, and more.
- **Modern Interface**: A gorgeous, hardware-accelerated UI built with GTK4 and tailored to the VAXP-OS glassmorphism design language, featuring custom header bars, animated progress overlays, and an intuitive file tree view.
- **Smart Architecture**: Built on top of the robust `libarchive` library for excellent performance and security, gracefully falling back to external CLI tools (`p7zip`, `unrar`, `unar`, etc.) for propriety or specialized formats.
- **Security**: Built-in password protection and AES-256 encryption support for creating and extracting secure archives (ZIP, 7z, RAR).
- **Core Operations**: Easily browse archive contents, extract specific files, navigate directory hierarchies, view archive properties, and create new archives with selective compression levels.

## Requirements

- GTK4 (>= 4.6)
- libadwaita (>= 1.2)
- libarchive (>= 3.4)
- GLib / GIO

### Optional Dependencies (for extended format support)
- `p7zip-full`
- `unrar` / `unar`
- `brotli`, `lrzip`, `lzop`, `arj`, `zoo`, `ncompress`, `bzip3`

## Building and Installation

VArchive utilizes the Meson build system.

```bash
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

---

This project is part of the VAXP ecosystem.
