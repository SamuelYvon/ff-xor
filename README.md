# ff-xor

Flag-Finder with XOR cipher brute force

When you have a random binary and you know it contains a flag, maybe it's worth going the easy way and brute forcing
it :)

This simple, portable utility should help you brute force simple XOR-ciphered flags.

## Usage and options

### Usage

```shell
ff-xor [-k keysize] [-A after] path_to_binary search_string
```


### Options

- `-A`: the number of characters to print after the search string (when found)
- `-k`: the number of bytes for the key (only accepts 1 for now)

Positional arguments must be the path to the binary and the search string, in that order.

Example:

```shell
$ff-xor -A 20 /home/syvon/firmware.bin FLAG-
Parsing file /home/syvon/firmware.bin
Found 1 results.
=============================
Result 1:
Addr: 119793
XOR: 79
Search: FLAG-M4thG3n1u$O".;' 
```

For now it only supports 1 byte key but I might get around adding bigger keys, although the larger the keys become
the less worthwhile this tool becomes to use.