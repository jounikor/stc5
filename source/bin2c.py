#!/usr/bin/python

import sys
import os

#
#


class conput:
    def __init__(self):
        return

    def write( self,str ):
        sys.stdout.write( str )

    def close( self ):
        sys.stdout.flush()



def bin2c( lab, b, n, o ):
    o.write("unsigned char %(str)s[%(len)d] = {\n" % {"str":lab,"len":n})

    i = 0

    while i < n:
        if i % 8 == 0:
            o.write("\t")

        o.write("0x%(luku)02x" % {"luku":b[i]})

        if i < n-1:
            if i % 8 == 7:
                o.write(",\n")
            else:
                o.write(",")

        i = i+1


    o.write("\n};\n")


#
#

def main():
    if (len(sys.argv) < 3) or (len(sys.argv) > 4) :
        print "**Usage: {0} name infile [outfile]".format(sys.argv[0])
        return

    #
    fi = open(sys.argv[2],"rb")
    fi.seek(0,os.SEEK_END)
    flen = fi.tell()
    fi.seek(0,os.SEEK_SET)

    # do not read attribute data, just gfx
    ib = bytearray(flen)
    fi.readinto(ib)
    fi.close()

    # check output method
    if len(sys.argv) == 4:
        fo = open(sys.argv[3],"wb")
    else:
        fo = conput()

    # do
    bin2c(sys.argv[1],ib,flen,fo)
    fo.close()


    # done
    return 0



#
#

if __name__ == "__main__":
    main()



