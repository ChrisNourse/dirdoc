# dirdoc

<img src="dirdoc_logo.png" alt="drawing" width="200"/>

dirdoc (directory documentor) is a command-line tool designed to quickly and easily package an entire codebase into a single (or multiple) Markdown file. This packaged document includes a comprehensive directory structure, file contents with syntax highlighting, and token/size statistics. It is especially useful for providing entire codebases to LLMs for analysis, bug fixing, or feature enhancements.

## Features

- **Comprehensive Documentation:** Automatically scans a directory and generates a Markdown document detailing the directory layout and file contents.
- **Syntax Highlighting:** Uses language detection based on file extensions for enhanced readability in Markdown code blocks.
- **Token and Size Statistics:** Displays estimated token counts and file size metrics for insights into the generated document.
- **.gitignore Integration:** Honors .gitignore files, letting you exclude specific files or directories.
- **Output Splitting:** Can split the output into multiple files if the generated document exceeds a specified size.
- **Flexible Modes:** Optionally generate structure-only documentation or include file contents.

## Universal Binary

Pre-built binaries for dirdoc are available in the [GitHub Releases](https://github.com/ChrisNourse/dirdoc/releases) section. Download the binary.

- **macOS, Linux, BSD:**  
  Ensure the binary is executable. If needed, run:
  ```bash
  chmod +x dirdoc
  ```

- **Windows:**  
 Obtain the binary from GitHub Releases, you may need to rename it by appending `.exe` (e.g., `dirdoc` → `dirdoc.exe`) for proper execution.

Once prepared, you can run the binary directly:
```bash
dirdoc --help
```

- **Windows:**  
```powershell
dirdoc.exe --help
```

## Installation (Using the Pre-built Binary)

### 1. Download the Binary

Visit the [GitHub Releases](https://github.com/ChrisNourse/dirdoc/releases) page and download the binary for your operating system.

### 2. Run dirdoc from Anywhere

To conveniently run `dirdoc` from any folder on your system, choose one of the following methods:

#### On Linux and macOS

- **Option 1:** Copy the binary to a directory in your PATH (e.g., `/usr/local/bin`):
  ```bash
  sudo cp dirdoc /usr/local/bin/dirdoc
  ```
- **Option 2:** Add the folder containing `dirdoc` to your PATH. For example, if you place it in `~/dirdoc`, add the following to your `~/.bashrc` or `~/.bash_profile`:
  ```bash
  export PATH="$PATH:$HOME/dirdoc"
  ```
  Then reload your profile:
  ```bash
  source ~/.bashrc
  ```

#### On Windows

- **Option 1:** Copy `dirdoc.exe` to a directory already in your PATH (e.g., `C:\Windows\System32`).
- **Option 2:** Add the folder containing `dirdoc.exe` to your PATH environment variable:
  1. Open **System Properties** (Right-click **This PC** > **Properties** > **Advanced system settings**).
  2. Click **Environment Variables**.
  3. Edit the **Path** variable and add the folder containing `dirdoc.exe`.
  4. Click **OK** to apply.

## Usage

After installation, run the application to display the help message:
```bash
dirdoc --help
```

### Example Commands

- **Generate full documentation:**
  ```bash
  dirdoc /path/to/dir
  ```

- **Specify a custom output file:**
  ```bash
  dirdoc -o custom_documentation.md /path/to/dir
  ```

- **Ignore .gitignore rules:**
  ```bash
  dirdoc --no-gitignore /path/to/dir
  ```

- **Generate structure-only documentation:**
  ```bash
  dirdoc --structure-only /path/to/dir
  ```

- **Enable split output with a custom size limit:**
  ```bash
  dirdoc -sp /path/to/dir
  dirdoc -sp -l 10 /path/to/dir
  ```

- **Include .git folders in the documentation:**
  ```bash
  dirdoc --include-git /path/to/dir
  ```

## Use Cases

dirdoc is ideal for:
- Packaging an entire codebase into a single document for LLM analysis.
- Sharing a self-contained reference of your codebase with collaborators.
- Quickly debugging or reviewing a project with both structure and contents in one file.
- Generating documentation that is easy to inspect, share, or version-control.

## Contributing

Contributions are welcome! Please open issues or submit pull requests with your suggestions or improvements.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements

- Built with [Cosmopolitan Libc](https://github.com/jart/cosmopolitan).
- For more on the Cosmo projects, visit:
  - [Cosmopolitan Documentation](https://github.com/jart/cosmopolitan/blob/master/docs/README.md)
- The code is human guided written by AI. Thanks Claude and chatGPT!
- The logo is human generated. Thanks Whit!
---
