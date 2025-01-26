# Windows Setup Instructions (MSYS2 + GCC + Make)

This guide helps you install MSYS2, configure a GCC toolchain, and build a C application on Windows using a Makefile that relies on POSIX shell syntax (e.g., `[ ! -d … ]`).

---

## 1. Install MSYS2

1. **Download the installer** from [msys2.org](https://www.msys2.org/).
2. **Run the installer** and follow the on-screen instructions.

---

## 2. Open the MSYS2 Shell

After installation, you can open various MSYS2 shells from the Start menu:
- **MSYS2 MSYS** (POSIX environment)
- **MSYS2 UCRT64** (native UCRT environment, still includes Bash)

For this Makefile, which uses POSIX shell features, you can use **either** shell, but you’ll likely run `make` from the **MSYS2 MSYS** shell.

---

## 3. Update MSYS2

1. Open your chosen MSYS2 shell.
2. Run:
   ```bash
   pacman -Syu
   ```
3. If prompted to close and reopen the shell (after updating core components), do so.
4. Then run:
   ```bash
   pacman -Su
   ```
This ensures everything is fully up to date.

---

## 4. Install Required Packages

To build with GCC and Make, install:

- `mingw-w64-ucrt-x86_64-gcc` (UCRT-based GCC compiler)
- `make` (MSYS2 version of GNU Make, supports POSIX shell commands)
- `git` (needed if your Makefile clones repositories)

From the shell, run:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc make git
```

> **Note:**
> - **`mingw-w64-ucrt-x86_64-make`** is a purely native Make (no POSIX shell). Not recommended if your Makefile uses POSIX commands.
> - Installing `make` (the MSYS2 version) ensures Bash scripting in the Makefile works properly.

---

## 5. (Optional) Add to System PATH

If you want to call `gcc` or `make` directly from Command Prompt or PowerShell—without opening the MSYS2 shell—add the following to your global Windows PATH:
```
C:\msys64\ucrt64\bin
```
1. Open **Settings** → **System** → **About** → **Advanced system settings**.
2. Click **Environment Variables…**.
3. In **Path**, add `C:\msys64\ucrt64\bin`.

> **Tip:**
> - You can compile in the MSYS2 shell, or add the toolchain to PATH for broader use.
> - **Important**: POSIX shell commands (like `[ ! -d … ]`) will **not** run in `cmd.exe` or PowerShell. Keep using the MSYS2 shell for your builds.

---

## 6. Verify the Installation

From your MSYS2 shell, verify by running:
```bash
gcc --version
make --version
git --version
```
You should see version information for each tool.

---

## 7. Build Your Project

1. In the MSYS2 shell, navigate to your project directory.
2. Run:
   ```bash
   make
   ```
   or a specific target such as:
   ```bash
   make all
   make test
   ```

Your Makefile can now use `gcc`, POSIX shell commands, and other MSYS2 utilities as expected.

---

**Done!** You’re ready to compile C applications in Windows using MSYS2 and GCC.

