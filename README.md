# k4 SideChain 🎛️

A free **sidechain effect** for FL Studio (and any VST3 music software) on **Windows** and **Mac**.

Three effects in one plugin — pick whichever you need:
- **Compressor** – the classic EDM "pumping" duck
- **Gate** – chops the sound on/off in time with a trigger
- **Envelope Follower** – the volume rides up and down with a trigger

It also has a **live waveform display** in the background so you can *see* the sidechain working: the bright shape is your processed sound, the faint shape behind it is the original. Click the display to pause it.

---

## ⬇️ Download

Get the latest version from the **[Releases page](https://github.com/kr4wford/k4sidechain/releases/latest)** and download the file for your computer:

| Your computer | Download this |
|---|---|
| **Windows** | `k4SideChain-Windows.zip` |
| **Mac (Apple Silicon — M1/M2/M3/M4)** | `k4SideChain-macOS-AppleSilicon.zip` |

> Not sure which Mac you have? Click the  Apple menu → **About This Mac**. If it says **Apple M1/M2/M3/M4**, you're on Apple Silicon.

---

## 🪟 Install on Windows

1. **Unzip** the downloaded file. You'll get a folder named **`k4 SideChain.vst3`**.
2. **Copy that folder** into:
   ```
   C:\Program Files\Common Files\VST3
   ```
3. Open **FL Studio** → **Options → Manage plugins → Find more plugins**.
4. "k4 SideChain" now appears in your plugin list. 🎉

---

## 🍎 Install on Mac (Apple Silicon)

1. **Unzip** the downloaded file. You'll get **`k4 SideChain.vst3`**.
2. In Finder, press **Go → Go to Folder…**, paste this and hit Enter, then drop the file in:
   ```
   ~/Library/Audio/Plug-Ins/VST3
   ```
3. **One-time unblock step.** Because this is a free plugin not signed by Apple, macOS blocks it at first. To allow it, open the **Terminal** app, paste the line below, and press Enter:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"k4 SideChain.vst3"
   ```
   *(You only ever do this once.)*
4. Open **FL Studio** → **Options → Manage plugins → Find more plugins**.
5. "k4 SideChain" appears in your list. 🎉

---

## 🎚️ How to use it

1. Add **k4 SideChain** to the track you want to duck (e.g. your **bass** or **pads**).
2. Choose a **Mode** at the top (Compressor / Gate / Envelope Follower).
3. Send your **kick** (or whatever the trigger is) into the plugin's **sidechain input**.
   In FL Studio: open the plugin wrapper's options and set the sidechain source to your kick track.
   - No sidechain set up? No problem — it reacts to its own sound so you still hear the effect.
4. Tweak **Threshold / Attack / Release** to taste, and watch the background waveform pump.

---

## ❓ Troubleshooting

- **It's not showing up in FL Studio** → run **Manage plugins → Find more plugins** again, and double-check the `.vst3` folder is in the exact location listed above.
- **(Mac) It still won't load** → re-do the one-time Terminal "unblock" step above — that's almost always the fix.
- **I have an older Intel Mac** → this download is built for Apple Silicon. Message me and I'll make an Intel/Universal version.

---

## 🛠️ Building from source (for developers only)

You do **not** need this to use the plugin — just download from Releases above. But if you want to build it yourself:

```bash
git clone https://github.com/kr4wford/k4sidechain.git
cd k4sidechain
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE is downloaded automatically — you don't need to install it first. Requires **CMake 3.22+** and a C++ compiler (Visual Studio 2022 on Windows, Xcode on Mac).
