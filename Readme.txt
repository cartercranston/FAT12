Carter Cranston

Calling 
    $ make
will compile my code into four executables: diskinfo, disklist, diskget, and diskput.

    $ ./diskinfo disk.IMA
will print information about disk.IMA

    $ ./disklist disk.IMA
will print the contents of disk.IMA


    $ ./diskget disk.IMA foo.txt
will copy /FOO.TXT to the current directory as foo.txt

    $ ./diskput disk.IMA foo.txt
will copy ./foo.txt to /FOO.TXT on the disk.
    $ ./diskput disk.IMA sub1/foo.txt
will copy ./foo.txt to /SUB1/FOO.TXT on the disk, provided there is room in the current sector of SUB1.
