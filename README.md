# Build
`gcc *.c *.h -o elfhash && chmod +x elfhash`

# Usage
```
./elfhash <options> file

Options:
  -l  list elf contents

  -r  rehash gnu.hash

  -f  oldname -t newname 
      replace the dynamic symbol from oldname to newname and rehash the elf

  -s  name_to_search 
      search the symbol name

  -h  display this message
```