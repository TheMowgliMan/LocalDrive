# Goals

The stated goals of the LocalDrive project, in two parts: 1) file index, 2) database, and 3) server.

## Part One: File Index
The first part is a file index.

It must support:

1. a linked design to allow fast searching and sorting
2. functions for creating, modifying and deleting databases
3. functions for getting arrays of nodes sorted alphabetically, by filesize, and fileage
4. functions for searching for nodes
4. functions for loading and saving the nodes from a file

## Part Two: Database
The second part is a wrapper for accessing files stored on-disk via the File Index.

It must support:

1. adding files to the primary disk and the file index in a compressed format (.xz)
2. finding files via the file index or directly and loading them, optionally as compressed (.zip) or default uncompressed
3. using a cache in RAM of uncompressed files (that accounts for OOM errors)
4. using a high-speed cache drive of uncompressed files
5. archicing the main drive to the backup drive
6. add multiple file owners
7. dynamically loading/unloading `file_index`es
8. functions for adding, getting, and removing files, directories, and file owners

## Part Three: Server
The third part is a server that allows fetching the files remotely and with basic security.

It must support:

1. using all of the features of the database remotely
2. encryption to keep the usernames, passwords, and files secure while in transfer, yet with a public system to allow arbitrary hosts to log in