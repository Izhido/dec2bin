# dec2bin

Convert very large decimal numbers into binary numbers.

Usage: dec2bin [arguments]

Arguments:
 -i : Path to input decimal number file
 -o : Path to output binary number file
 -s : Offset into input file (starting index)
 -l : Amount of digits to read from file (length)
 -t : Minimum amount of time that dec2bin will run (hh:mm:ss)
 -c : Path to cache file storing results for a partial run

The input is expected to be a continuous stream of ASCII-encoded numeric digits (0x30 to 0x39). The output is a single binary blob representing the final conversion, in 8-bit bytes, MSB first. If the resulting number does not fit in a byte boundary, the result will be padded to the left with up to seven 0 bits.

For a single run - that is, when you run dec2bin exactly once with a small file, 10MB or less - be sure to specify -i and -o, the minimum required arguments:

    dec2bin -i pi-million.txt -o pi-million.bin

If you need to extract only a section of a file, use -s and -l to specify the starting point and the length of that section:

    dec2bin -i pi-billion.txt -o pi-billion.bin -s 2 -l 1000000

If the amount of digits you need to convert exceed 10MB, while the program can still process them in a single run, the amount of time required might be too large - it's been estimated that the program needs about 9 months continuously running to convert a 1GB file, running in a 8-core Intel Core i9, 3.6MHz. In that case, we recommend you to use -t and -c to perform successive, controlled partial runs. This way, if dec2bin gets interrupted in one partial run, you only need to respecify the arguments, and the program will resume from the last partial run until time runs out, or the number is completed:

    dec2bin -i pi-billion.txt -o pi-billion.bin -s 2 -l 1000000000 -t 8:00:00 -c pi-billion_cache.bin 

Partial runs must be manually started. If you specify -t, dec2bin will only run for that amount of time, and stop immediately. You are responsible to continue running dec2bin with the required arguments until the conversion is completed.

Source code is standard C++. Packaged inside an Xcode console app project; however, it can be compiled for any platform that supports minimum, basic C++11 features. 
