#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>

using namespace std;

int main(int argc, const char * argv[])
{
    try
    {
        if (argc < 3 || argc > 5)
        {
            cout << "Usage: dec2bin path/to/input/number.txt path/to/output/binary/number.bin [numeric offset into input file] [amount of bytes to read into file]" << endl;
        }
        cout << "Opening " << argv[1] << endl;
        ifstream input_file(argv[1], ios_base::binary);
        input_file.seekg(0, ios_base::end);
        auto file_size = input_file.tellg();
        if (file_size <= 0)
        {
            throw runtime_error(string(argv[1]) + " - empty or absent input file");
        }
        long offset = 0;
        if (argc >= 4)
        {
            offset = atol(argv[3]);
            if (offset <= 0)
            {
                throw runtime_error("Offset " + string(argv[2]) + " is not a positive number");
            }
            if (offset >= file_size)
            {
                throw runtime_error("Offset " + string(argv[2]) + " exceeds the size of the input file");
            }
        }
        input_file.seekg(offset);
        long number_size = (size_t)file_size - offset;
        if (argc == 5)
        {
            number_size = atol(argv[4]);
            if (number_size <= 0)
            {
                throw runtime_error("Amount " + string(argv[3]) + " is not a positive number");
            }
            if (number_size + offset >= file_size)
            {
                throw runtime_error("Amount " + string(argv[3]) + " plus offset " + string(argv[4]) + " exceeds the size of the input file");
            }
        }
        vector<unsigned char> input_buffer(number_size);
        cout << "Reading " << argv[1] << endl;
        input_file.read((char*)(input_buffer.data()), number_size);
        for (size_t i = 0; i < number_size; i++)
        {
            auto c = input_buffer[i];
            if (c < '0' || c > '9')
            {
                throw runtime_error("Character " + to_string(i + offset) + " in " + argv[1] + " is not a number (" + to_string(c) + ")");
            }
            input_buffer[i] = c - '0';
        }
        cout << "Processing number with " << number_size << " digits" << endl;
        auto output_buffer_size = (size_t)ceil(number_size / 4);
        vector<uint32_t> output_buffer(output_buffer_size);
        auto start_time = chrono::high_resolution_clock::now();
        size_t start = output_buffer_size - 1;
        for (size_t i = 0; i < input_buffer.size(); i++)
        {
            uint64_t carry = input_buffer[i];
            for (size_t j = output_buffer_size - 1; j >= start; j--)
            {
                uint64_t value = (uint64_t)output_buffer[j] * 10 + carry;
                carry = value / 4294967296;
                output_buffer[j] = (uint32_t)(value % 4294967296);
            }
            if (carry > 0)
            {
                start--;
                output_buffer[start] = (uint32_t)carry;
            }
        }
        auto finish_time = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish_time - start_time;
        cout << "Processing time: " << elapsed.count() << " seconds" << endl;
        cout << "Writing " << argv[2] << endl;
        ofstream output_file(argv[2], ios_base::binary);
        size_t shift = 24;
        if (output_buffer[start] < 256)
        {
            shift = 0;
        }
        else if (output_buffer[start] < 65536)
        {
            shift = 8;
        }
        else if (output_buffer[start] < 16777216)
        {
            shift = 16;
        }
        while (start < output_buffer_size)
        {
            output_file.put((output_buffer[start] >> shift) & 255);
            if (shift == 0)
            {
                start++;
                shift = 24;
            }
            else
            {
                shift -= 8;
            }
        }
        output_file.close();
    }
    catch (exception& e)
    {
        cout << "Error: " << e.what() << endl;
    }
}
