## What is `dbus-vis-web`

This program (which runs both in a [web page](https://quadpixels.github.io/openbmc_ipmi_timeline_vis/dbus-vis-web/index.html) and in Electron.js) visualizes PCAP files captured on an OpenBMC system in a time-line format.

This tool can be thought of as a visual version of Wireshark with a focus on OpenBMC-related packet captures. It visualizes:
- DBus packet capture obtained through `busctl capture`.
  - DBus packet captures expose an abundance of information about the operation of an OpenBMC system.
- MCTP packet capture obtained through `tcpdump -i mctpi2cX`.
- Boost ASIO handler logs obtained by enabling the `BOOST_ASIO_ENABLE_HANDLER_TRACKING` flag.

It parses the packet capture file using a WASM module compiled from CXX code with [EmScripten](https://emscripten.org/).

The UI is inspired by CPU & GPU performance profilers and allows the user to pan, zoom and highlight the timeframe, as check simple statistics such as the number of messages and aggregate time.

## How to use

* For the web version, click this page: [https://quadpixels.github.io/openbmc_ipmi_timeline_vis/dbus-vis-web/index.html](https://quadpixels.github.io/openbmc_ipmi_timeline_vis/dbus-vis-web/index.html)

* For the Electron.js version:
  Clone the code base and:

  1. `npm install`

  2. `npm start`

![Image](./scrnshot.png)

<hr/>

<span style="font-size:1.2em; color:#ccc; font-family:monospace">Bonus feature</span>
<span style="font-size:0.7em; color:#ccc; font-family:monospace">
A voxel scene modeled with <a href="https://ephtracy.github.io" style="color:#aaa">MagicaVoxel</a> and rendered with GLES (which also gets compiled to WebGL by EmScripten and runs in a browser) can be activated by clicking the 2 buttons located on the bottom-left of the UI :D
</span>