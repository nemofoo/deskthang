# DeskThang Host Application

## Prerequisites

### Serial Port Access
To communicate with the device over serial port, you need appropriate permissions. On Linux systems, add your user to the `dialout` group:

```bash
sudo usermod -a -G dialout $USER
```

**Note:** You'll need to log out and log back in for the group changes to take effect. You can verify it worked by running `groups` and checking that `dialout` appears in the list.

## Building

```bash
cd host
zig build
```

## Usage

```bash
# Show test patterns
deskthang test 1    # Show checkerboard pattern
deskthang test 2    # Show color bars pattern
deskthang test 3    # Show gradient pattern

# Send image
deskthang image image.png  # Send 240×240 PNG image
```

## Development

See [docs/host.md](../docs/host.md) for detailed development documentation. 