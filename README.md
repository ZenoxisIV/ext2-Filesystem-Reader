# ext2 Filesystem Reader

## Features
- Path Enumeration
    - If the program is called with only one argument, then the program prints out **all absolute paths** of all files and directories contained in the input filesystem.

- Filesystem Object Extraction
    - If the second argument described above is present, then the program interprets this argument as an **absolute path**.
    - If the path corresponds to a **valid file** in the input filesystem, then the program extracts the said file.
    - If the path corresponds to a **valid directory** in the input filesystem, then the program creates a directory in the host machine’s current working directory called `output`.

## How to Run the Program

### Initial Setup
1. Open the terminal inside the repository.
2. Run the following command to initialize the project (do this only at the initial run):
    ```bash
    make
    ```

### Running the Program
3. Execute the following command in the terminal:
    ```bash
    sudo ./ext2op path/to/ext2/filesystem path/to/extract
    ```
    Replace `path/to/ext2/filesystem` with the path to the desired ext2 filesystem. You can also provide an **optional** path argument for extraction operations.

## Cleaning Outdated Makes

To clean outdated make files, use the following command in the terminal inside our repository:

```bash
make clean
```

## Contributors
* Ivan Cassidy Cadiang
* Diego Montenejo
* Rohan Solas
