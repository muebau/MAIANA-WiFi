---
title: Web Flasher
description: Web Installer for Maiana Firmware
---

<style>
  .md-content__button {
    display: none;
  }
  .pick-variant select {
    background: transparent;
    width: 300px;
    padding: 1px;
    font-size: 16pt;
    border: 1px solid #ddd;
    height: 51px;
    border-radius: 15px;
  }
  .invisible {
    visibility: hidden;
  }
  .radios li {
    list-style: none;
    line-height: 2em;
  }
</style>

# Web Flasher

Flash or Find your device using next options:

1. Plug in your ESP32 to a USB port.
2. Hit "Install" and select the correct COM port.
3. Get firmware installed and connected in less than 3 minutes!
4. Connect to the Maiana wifi and configure the wifi network to connect your device to.

<p class="button-row" align="center">
<esp-web-install-button manifest="./manifest.json">
  <button slot="activate" class="md-button md-button--primary">INSTALL</button>
  <span slot="unsupported">Use Chrome Desktop</span>
  <span slot="not-allowed">Not allowed to use this on HTTP!</span>
  
</esp-web-install-button>
</p>
Powered by [ESP Web Tools](https://esphome.github.io/esp-web-tools/){target=_blank}

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>