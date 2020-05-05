This program visualizes IPMI traffic dumped on a BMC running OpenBMC. It allows the user to pan, zoom and highlight the timeline, inspect the distribution of the time it takes ipmid to process the message, as well as generate commands that can talk to `ipmid` or `ipmitool` to replay those IPMI requests.

Its inputs are DBus traffic dumps that involve DBus signals and method calls launched against the IPMI daemon (ipmid). To be specific, there are two possible ways:

- For the "legacy" interface:
  - A request is sent to IPMID by a DBus signal "ReceivedMessage". (The "ReceivedMessage" signal name actually means the bridge daemon (`btbridged`) "receives" a message from the "host".)
  - The response is a method call "sendMessage" which means the bridge daemon sends a message response back to the "host".

- For the "new" interface:
  - A request is a DBus method call "execute".
  - The IPMI response is just the method response of this method call.

The input to this program is a text dump of DBus messages that may be obtained by the "dbus-monitor" command.

This program is based on Electron; it should be compatible with Windows, Linux, Mac and ChromeOS's Linux environment.

To build and run, a user would first need to install node.js and npm (Node.js package manager). To install node.js on a Ubuntu/Debian-based system:

1. `curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -`

2. `sudo apt install -y nodejs`

3. `sudo apt install npm`

If `npm` and `node.js` are installed correctly you would probably see:

```
$ node --version
v10.20.1
$ npm --version
6.14.4
```

With `npm` and `node.js` installed, enter the root directory of this project, and run the following commands:

1. `npm install`

2. `npm start`

1440x900 (WSXGA+) or higher screen resolution (or "logic screen size" for high-DPI displays) is recommended.

![Image](./scrnshot.png)
