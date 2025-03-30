# Building dirdoc

This document provides instructions on how to build, test, and maintain the dirdoc project. dirdoc uses a Makefile to automate dependency management, compilation, and testing. The project leverages Cosmopolitan Libc to produce a universal binary that can run on macOS, Linux, and Windows (with a few extra steps).

---

## Creating a Release

The project is configured with GitHub Actions to automatically build and publish releases when you push a version tag. Here's how to create a new release:

### 1. Ensure Everything is Ready

Before creating a release:

- Make sure all desired changes are committed and pushed to the main branch.
- Verify that the code builds and tests pass locally:
  ```bash
  make clean
  make all
  make test
  ```

### 2. Create and Push a Version Tag

The GitHub workflow is triggered when a tag with a version number format (e.g., `1.0.0`, `v1.2.3`) is pushed to the repository:

```bash
# 1. Create the tag locally (choose an appropriate version number)
git tag 1.0.0

# 2. Push the tag to GitHub
git push origin 1.0.0
```

### 3. Monitor the Release Process

After pushing the tag:

1. GitHub Actions will automatically start the release workflow
2. The workflow will:
   - Build the project
   - Create a release binary for Linux/macOS
   - Create a Windows-compatible .exe file
   - Create a GitHub Release with these binaries attached

You can monitor the progress of the workflow in the "Actions" tab of your GitHub repository.

### 4. Verify the Release

Once the workflow completes successfully:

1. Go to the "Releases" section of your GitHub repository
2. Verify that the new release appears with the correct version number
3. Confirm that both the `dirdoc` and `dirdoc.exe` binaries are attached to the release

---

## Prerequisites

Before building the project, ensure that your environment has the following:

- **Operating System:**  
  - **Linux/macOS:** Follow the instructions below.
  - **Windows:** **Building directly on Windows is a pain.**  
    **It is strongly recommended to install [Windows Subsystem for Linux (WSL)](https://docs.microsoft.com/en-us/windows/wsl/install) and follow the Linux build instructions.**  
    *(Note: The pre-built universal binary runs on Windows; only the build process is problematic on native Windows.)*

- **Tools:**
  - **make:**  
    - **Linux:** Usually available via your package manager.  
      - For Ubuntu/Debian-based systems, run:  
        ```bash
        sudo apt-get update && sudo apt-get install build-essential make
        ```  
      - For Fedora/RHEL-based systems, run:  
        ```bash
        sudo dnf install make gcc
        ```
    - **macOS:** Install Xcode Command Line Tools by running:  
      ```bash
      xcode-select --install
      ```  
      Alternatively, install [Homebrew](https://brew.sh/) and then run:  
      ```bash
      brew install make
      ```
    - **Windows (via WSL):**  
      Follow the Linux instructions inside your WSL environment.

  - **C Compiler:**  
    The project uses `$(DEPS_DIR)/cosmocc/bin/cosmocc` from Cosmopolitan Libc, which is automatically set up by the Makefile.

  - **curl:**  
    - **Linux:**  
      ```bash
      sudo apt-get install curl    # Debian/Ubuntu
      sudo dnf install curl        # Fedora/RHEL
      ```
    - **macOS:**  
      curl is typically pre-installed. Otherwise, use Homebrew:  
      ```bash
      brew install curl
      ```
    - **Windows (via WSL):**  
      Use the Linux instructions.

  - **unzip:**  
    - **Linux:**  
      ```bash
      sudo apt-get install unzip   # Debian/Ubuntu
      sudo dnf install unzip       # Fedora/RHEL
      ```
    - **macOS:**  
      unzip is typically pre-installed, or install via Homebrew:  
      ```bash
      brew install unzip
      ```
    - **Windows (via WSL):**  
      Use the Linux instructions.

  - **Git (optional, for cloning the repository):**  
    - **Linux:**  
      ```bash
      sudo apt-get install git     # Debian/Ubuntu
      sudo dnf install git         # Fedora/RHEL
      ```
    - **macOS:**  
      Git is typically available via the Xcode Command Line Tools. Alternatively, install via Homebrew:  
      ```bash
      brew install git
      ```
    - **Windows (via WSL):**  
      Use the Linux instructions.

> **Note:** The Makefile automatically downloads and sets up the Cosmopolitan dependency into the `deps` folder.

---

## Build Instructions

### 1. Clone the Repository

If you haven’t already, clone the repository and change into the project directory:

```bash
git clone https://github.com/chrisnourse/dirdoc.git
cd dirdoc
```

### 2. Build the Application

Run the following command to compile the project:

```bash
make all
```

This target will:
- **Download Dependencies:** Fetch and unzip Cosmopolitan Libc (if not already present).
- **Compile the Code:** Build the source files in the `src/` directory.
- **Output Binary:** Create the executable `dirdoc` in the `build/` directory.

After a successful build, you should see messages similar to:

- `✅ Build completed successfully`
- `📍 Binary location: build/dirdoc`
- `🚀 Run ./build/dirdoc --help for usage`

### 3. Running the Application

Once built, you can run dirdoc to see its help message:

```bash
./build/dirdoc --help
```

### 4. Running Tests

The project includes a test suite located in the `tests/` directory. To build and run the tests, execute:

```bash
make test
```

This command compiles the test binary (`dirdoc_test`) and runs it, verifying the functionality of various project components.

### 5. Building a Temporary Test Binary

For manual inspection or debugging, you can build a temporary test binary:

```bash
make build_temp
```

This creates the `temp_test` binary in the `build/` directory. To run it:

```bash
./build/temp_test
```

When finished, remove temporary test files with:

```bash
make clean_temp
```

### 6. Cleaning Up Build Artifacts

- **Clean Compiled Files Only:**

  ```bash
  make clean
  ```

  This removes the contents of the `build/` directory while preserving downloaded dependencies.

- **Full Cleanup (Artifacts and Dependencies):**

  ```bash
  make super_clean
  ```

  This command removes both the `build/` and `deps/` directories.

### 7. Displaying Available Make Targets

For a summary of all available make targets and their descriptions, run:

```bash
make help
```

---

## Cross-Platform Considerations

- **Unix-like Systems (Linux/macOS/BSD):**
  - Ensure the binary is executable. If necessary, run:
  
    ```bash
    chmod +x build/dirdoc
    ```

- **Windows:**
  - The build produces a universal binary. You may need to rename the file by appending `.exe` (i.e., `dirdoc.exe`) for proper execution.

---

## Troubleshooting

- **Dependency Errors:**
  - If the Cosmopolitan dependency does not download or extract correctly, verify that `curl` and `unzip` are installed.
  
- **Permission Issues:**
  - On Unix-like systems, if you experience permission issues running the binary, adjust its permissions using `chmod`.

- **Rebuilding:**
  - If you encounter build errors, it may help to run a full cleanup and rebuild:
  
    ```bash
    make super_clean
    make all
    ```

---
