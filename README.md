dnafx-editor
============

This is an open source editor (still experimental and WIP) for the [Harley Benton](https://harleybenton.com) DNAfx GiT series, namely:

* [DNAfx GiT Core](https://harleybenton.com/product/dnafx-git-core/)
* [DNAfx GiT](https://harleybenton.com/product/dnafx-git/) (untested)
* [DNAfx GiT Advanced](https://harleybenton.com/product/dnafx-git-advanced/) (untested)

It is **NOT** affiliated with, nor endorsed by, Harley Benton, and is not aimed to compete with or deprecate the official editor. It's very simply an effort that I started because I needed it, and that I'm sharing with the hope that others may find it useful (and hopefully contribute back to the project, should they find ways to extend/enhance it).

It's also an _experimental_ tool. I'm not responsible for any damage this may do to your device or presets. You use it at your own risk: I'm testing this regularly on my device and it seems to be working as expected, but of course I don't know what may happen in different environments.

The following sections provide some motivations and objectives for this repo. Scroll further below for info on how to compile and test this, or for information on how you can help, should you want to.

# Motivation

I recently acquired a DNAfx GiT Core device on [Thomann](https://www.thomann.de/it/harley_benton_dnafx_git_core.htm), since as a guitar player I was intrigued by its alleged amp modeling capabilities and its very chear price tag. You can learn more about it in [this informative video review](https://www.youtube.com/watch?v=Uj87A27qPUQ) by Ola Englund.

The device is very customizable, with 200 presets that you can tweak to your will in its 9 effect blocks, each with their own plethora of settings (and there's a lot of them). You can either edit those directly on the device (which is a bit awkward to do there, with its small buttons and knobs), or using a software editor, that controls the device via USB.

That said, unfortunately Harley Benton only provides an editor for Windows and macOS. No matter how hard I tried, I couldn't get the Windows version to see my device neither via Wine (which was expected) nor using virtual machines: I tried both qemu and VirtualBox, using both Windows 11 and Windows 7 emulation, with no results at all. This meant that, if I wanted to be able to edit my DNAfx device from Linux, I had to either find someone who had written an editor for it, or write one myself.

At the time of writing, I found two related efforts: [hbdnafx-git](https://github.com/bjgillet/hbdnafx-git) by @bjgillet and [DNAfx_GiT_CLI](https://github.com/jblackiex/DNAfx_GiT_CLI/) @jblackiex. Both efforts are very early stages, and don't provide much functionality yet, but they were tremendously helpful to me to start figuring out how to reverse engineer the USB protocol the official editor uses to talk to the device, which is what led to the code in this repo. While those projects are written in Python, the code in this repo is written in C.

# Objective

The plan, sooner or later, is to come up with a more or less complete editor for Linux, that provides feature parity with the official editor, and possibly something more. The list below, which is just a summary on the top of my head and not exhaustive, provides an overview of what I'd like to have in the app. Where you see a checkmark, that feature should already be working in the current version (YMMV, of course).

- [x] Connecting to the device via USB
- [x] Retrieving the list of presets from the device
- [x] Retrieving the list of "extras" from the device (note: apparently it's the list of custom IR's that were uploaded to the device?)
- [x] Saving device presets to binary and PHB (JSON, official editor) formats
- [x] Loading presets from disk, in binary or PHB format
- [x] Converting between binary and PHB preset formats
- [x] Uploading custom presets to specific slots on the device
- [ ] Renaming presets (I have examples from USB captures, but they don't work yet)
- [ ] Tweaking individual settings in existing presets (active state, effect, values, etc.)
- [ ] Uploading custom IR files to use as CAB elements
- [ ] Interacting with the looper functionality, if possible (@jblackiex is working on that in his repo)
- [ ] Interacting with the rhythm/tap functionality, if possible
- [ ] Intercepting asynchronous events from the device (e.g., hardware preset edits, triggers, etc.)
- [x] Interactive console for doing things
- [ ] Support for network protocols for doing things (e.g., netcat/telnet, WebSockets, HTTP, etc.)
- [ ] Support for Bluetooth commands for doing things
- [ ] Interactive GUI to mimic the official editor functionality (GTK? SDL2?)
- [ ] Offline mode for editing presets even without access to the device

# Dependencies

To compile this editor, you'll need to satisfy the following dependencies:

* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
* [GLib](https://docs.gtk.org/glib/)
* [libusb](https://libusb.info/)
* [Jansson](https://github.com/akheron/jansson)

Notice that, at the time of writing, only Linux is supported as a target. While all dependencies should be cross platform, meaning support could be extended to other systems as well, I don't see an immediate need for that (especially considering an official editor already exists for Windows and macOS).

# Compile

Once you have installed all the dependencies, just use:

	make

If nothing went wrong, you'll end up with a `dnafx-editor` executable.

# USB device configuration

Out of the box, good chances are that your system will only allow you to read/write to the device using root. [This tutorial](https://github.com/bjgillet/hbdnafx-git/blob/main/doc/01_QualifyingtheUSBdeviceonLinux.pdf) on the [hbdnafx-git](https://github.com/bjgillet/hbdnafx-git) provides more context, and some info on how to qualify the USB device accordingly.

Should those instructions fail for you (as they did for me), you can use this simpler set of rules which seem to work for me:

	lminiero@lminiero ~ $ cat /usr/lib/udev/rules.d/99-usb-dnafx.rules
	SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5703", MODE="0666"

If that still doesn't work, you can use `sudo` to launch the editor, although of course that's not recommended.

# Examples

Pass `-h` or `--help` to `dnafx-editor` for a complete list of command line arguments you can pass to the tool.

If you launch it with no arguments:

	./dnafx-editor

it will try to connect to the device via USB, "greet" it (send what we figured out to be some initialization messages), retrieve the list of presets from the device, and finally get the list of "extras". Since by default the tool doesn't launch in interactive mode, the tool will automatically close when done.

To save the presets from your device to disk, use `-s` to provide a target folder, e.g.:

	./dnafx-editor -s ./presets/

This should result in 200 `.bhb` files (each of 184 bytes) in that folder.

You can launch the editor in "offline" mode too, with `-o`, which is particularly helpful when you just want to parse or convert a preset. This command, for instance, lets you parse one of the presets we just downloaded (in my seyup, preset `001` is `US Clean`):

	./dnafx-editor -o -b ./presets/001-US\ Clean.bhb

You can convert presets from binary to PHB format and back using the tool as well: `-b` and `-p` allow you to specify the preset to import (in binary or PHB format respectively), while `-B` and `-P` allow you to specify the target file in the respective format as well. This command, for instance, will convert our `US Clean` binary preset to PHB, which is a JSON file and the format that the official editor uses:

	./dnafx-editor -o -b ./presets/001-US\ Clean.bhb -P ./presets/001-US\ Clean.phb

When talking to the device, you can quickly change the currently active preset using the `-c` flag, which is much faster than just going up/down using the hardware buttons. This command, for instance, switches to preset number `35`, which is `POP METAL` in my setup:

	./dnafx-editor -c 35

If you don't want to do the full setup every time you launch the app, you can pass `-IGE` (which is a combination of `-I`, `-G` and `-E`) to skip the init/presets/extra dance:

	./dnafx-editor -IGE -c 35

If you have custom presets, either because you downloaded one somewhere or you edited one of the existing ones locally, you can upload it on one of the slots on the device. To do that, you first need to provide the preset you want to upload (using either `-b` or `-p`, as we've seen before), and then specify the slot to put the preset into (1-200) via `-u`. This command, for instance, will upload a "Gary Moore" preset I downloaded in PHB format on [guitarpatches](https://guitarpatches.com/patches.php?unit=DNAfxGiT) to slot 200:

	./dnafx-editor -IGE -u 200 -p ../presets/GARY\ MOORE.phb

Of course, you can always restore one of the original presets by just re-uploading them again to their original spot (assuming you backed them up first with `-s`).

All those features can also be performed interactively, if you launch the tool in interactive mode with `-i`. When you do that, the tool won't shut down after performing the things you asked to do in the command line arguments, but will wait for your commands. The following is an examples of how to import the "Gary Moore" preset programmatically, for instance:

	import-preset phb "../presets/GARY\ MOORE.phb"

Type `help` for a more comprehensive list of the currently supported CLI commands.

This is, in summary, what the tool allows you to do for now. Hopefully further revisions will add more features.

# Want to help?

The easiest way to help is to, first of all, just use the tool, and make sure it does what it should. This would be particularly helpful with versions of the device different from my own, since I only have the Core but have never tried it with the regular or Advanced versions (which should in theory work the same way, as the official editor is the same, but I can't vouch for that).

Besides that, reverse engineering the protocol has been a ton of fun, so far, but it's also been hard, especially considering I'm new to libusb, and to USB reverse engineering in the first place. Having access to some Wireshark pcapng dumps of USB exchanges between the official editor and a DNAfx device helped, but it's still not enough to figure out what the messages actually mean, and what the device may be expecting/sending at any given time. As such, if there's a feature you'd like the tool to have but that's still missing, try providing Wireshark pcapng captures that include messages related to what you want to do: e.g., start a capture on Windows (Wireshark supports USB captures via USBPcap), launch the editor, trigger the command you want to replicate, see it have effect, and stop the capture and save it to a pcapng file (which at this point will ideally have more details on the USB messaging that triggered that action).

Of course, if you're a C developer and/or are familiar with libusb, a fresh look at the code or even pull requests to contribute fixes/enhancements would be always more than welcome, especially if you have ways to test the code with a device of your own.
