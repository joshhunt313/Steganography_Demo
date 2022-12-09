HOW TO COMPILE:
    make

HOW TO DELETE EXTRANEOUS FILES:
    make clean

HOW TO RUN:
    Encoding: ./stegan -e [image] [hidden_image]
    where "hidden_image" is the file that will be split into
    the least significant bits of "image"

    Decoding: ./stegan -d [image]
    where "image" is the file to decode
