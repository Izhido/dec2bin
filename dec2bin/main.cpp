#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <sstream>

using namespace std;

int main(int argc, const char * argv[])
{
    try
    {
        string input_file_name;
        string output_file_name;
        size_t input_start = 0;
        size_t input_length = 0;
        size_t seconds = 0;
        string cache_file_name;
        auto show_help = (argc == 1);
        for (auto i = 1; i < argc; i++)
        {
            string argument(argv[i]);
            if (argument == "-i")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Input file name not specified");
                }
                input_file_name = argv[i];
            }
            else if (argument == "-o")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Output file name not specified");
                }
                output_file_name = argv[i];
            }
            else if (argument == "-s")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Offset not specified");
                }
                stringstream stream(argv[i], ios_base::in);
                stream >> input_start;
                if (stream.fail())
                {
                    throw runtime_error("Offset " + string(argv[i]) + " is not a positive number");
                }
            }
            else if (argument == "-l")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Amount not specified");
                }
                stringstream stream(argv[i], ios_base::in);
                stream >> input_length;
                if (stream.fail())
                {
                    throw runtime_error("Amount " + string(argv[i]) + " is not a positive number");
                }
            }
            else if (argument == "-t")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Time not specified");
                }
                string time = argv[i];
                vector<size_t> components;
                components.emplace_back();
                for (size_t j = 0; j < time.length(); j++)
                {
                    auto c = time[j];
                    if (c >= '0' && c <= '9')
                    {
                        components[components.size() - 1] = components[components.size() - 1] * 10 + (c - '0');
                    }
                    else if (c == ':')
                    {
                        if (components.size() >= 3)
                        {
                            throw runtime_error("Too many components present in time expression");
                        }
                        components.emplace_back();
                    }
                    else
                    {
                        throw runtime_error("Character '" + string(1, c) + "' unexpected in time expression");
                    }
                }
                if (components.size() == 3)
                {
                    seconds = components[0] * 3600 + components[1] * 60 + components[2];
                }
                else if (components.size() == 2)
                {
                    seconds = components[0] * 60 + components[1];
                }
                else
                {
                    seconds = components[0];
                }
            }
            else if (argument == "-c")
            {
                i++;
                if (i >= argc)
                {
                    throw runtime_error("Cache file name not specified");
                }
                cache_file_name = argv[i];
            }
            else if (argument == "-h")
            {
                show_help = true;
            }
            else
            {
                throw runtime_error("Argument " + argument + " not recognized");
            }
        }
        if (show_help)
        {
            cout << "Usage: dec2bin [arguments]" << endl;
            cout << "Arguments:" << endl;
            cout << " -i : Path to input decimal number file" << endl;
            cout << " -o : Path to output binary number file" << endl;
            cout << " -s : Offset into input file (starting index)" << endl;
            cout << " -l : Amount of digits to read from file (length)" << endl;
            cout << " -t : Minimum amount of time that dec2bin will run (hh:mm:ss)" << endl;
            cout << " -c : Path to cache file storing results for a partial run" << endl;
            return EXIT_FAILURE;
        }
        cout << "Opening " << input_file_name << endl;
        ifstream input_file(input_file_name, ios_base::binary);
        input_file.seekg(0, ios_base::end);
        size_t input_file_size = input_file.tellg();
        if (input_file_size == 0)
        {
            throw runtime_error(input_file_name + " - empty or absent input file");
        }
        if (input_start >= input_file_size)
        {
            throw runtime_error("Offset " + to_string(input_start) + " exceeds the size of the input file");
        }
        if (input_start + input_length > input_file_size)
        {
            throw runtime_error("Amount " + to_string(input_length) + " plus offset " + to_string(input_start) + " exceeds the size of the input file");
        }
        input_file.seekg(input_start);
        auto number_size = input_length;
        if (number_size == 0)
        {
            number_size = input_file_size - input_start;
        }
        vector<unsigned char> input_buffer(number_size);
        cout << "Reading " << input_file_name << endl;
        input_file.read((char*)(input_buffer.data()), number_size);
        if (input_file.fail())
        {
            throw runtime_error("Could not read data from " + input_file_name);
        }
        for (size_t i = 0; i < number_size; i++)
        {
            auto c = input_buffer[i];
            if (c < '0' || c > '9')
            {
                throw runtime_error("Character '" + string(1, c) + "' in " + input_file_name + " is not a number");
            }
            input_buffer[i] = c - '0';
        }
        input_file.close();
        auto output_buffer_size = (size_t)ceil(number_size / sizeof(uint32_t)) + 1;
        cout << "Allocating " << output_buffer_size * sizeof(uint32_t) << " bytes for output" << endl;
        vector<uint32_t> output_buffer(output_buffer_size);
        size_t iteration = 0;
        size_t start = output_buffer_size - 1;
        size_t finish = start;
        if (cache_file_name.length() > 0)
        {
            cout << "Opening cache at " << cache_file_name << endl;
            ifstream cache_file(cache_file_name, ios_base::binary);
            if (!cache_file.fail())
            {
                cache_file.seekg(0, ios_base::end);
                size_t cache_file_size = cache_file.tellg();
                size_t cache_length = 2 * sizeof(size_t) + output_buffer_size * sizeof(uint32_t);
                if (cache_file_size != cache_length)
                {
                    throw runtime_error("Cache file " + cache_file_name + " is not exactly " + to_string(cache_length) + " bytes in size");
                }
                cache_file.seekg(0);
                cout << "Reading cache file " << cache_file_name << endl;
                cache_file.read((char*)&iteration, sizeof(size_t));
                if (cache_file.fail())
                {
                    throw runtime_error("Could not read iteration number from cache file " + cache_file_name);
                }
                if (iteration >= number_size)
                {
                    throw runtime_error("Iteration number " + to_string(iteration) + " in cache file " + cache_file_name + " exceeds the size of the input file");
                }
                cache_file.read((char*)&start, sizeof(size_t));
                if (cache_file.fail())
                {
                    throw runtime_error("Could not read binary number start from cache file " + cache_file_name);
                }
                if (start > finish)
                {
                    throw runtime_error("Binary number start " + to_string(start) + " in cache file " + cache_file_name + " exceeds the size of the output number buffer");
                }
                cout << "Iteration number: " << iteration << " of " << number_size << endl;
                cout << "Binary number start: " << start << " of " << finish << endl;
                cache_file.read((char*)(output_buffer.data()), output_buffer_size * sizeof(uint32_t));
                if (cache_file.fail())
                {
                    throw runtime_error("Could not read output number data from cache file " + cache_file_name);
                }
            }
        }
        cout << "Processing number with " << number_size << " digits" << endl;
        auto start_time = chrono::high_resolution_clock::now();
        while (iteration < number_size)
        {
            uint64_t carry = input_buffer[iteration];
            for (size_t i = finish; i >= start; i--)
            {
                uint64_t value = (uint64_t)output_buffer[i] * 10 + carry;
                carry = value / 4294967296;
                output_buffer[i] = (uint32_t)(value % 4294967296);
            }
            if (carry > 0)
            {
                start--;
                output_buffer[start] = (uint32_t)carry;
            }
            iteration++;
            if (seconds > 0)
            {
                auto iteration_time = chrono::high_resolution_clock::now();
                chrono::duration<double> elapsed = iteration_time - start_time;
                if (elapsed.count() > seconds)
                {
                    break;
                }
            }
        }
        auto finish_time = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish_time - start_time;
        cout << "Processing time: " << elapsed.count() << " seconds" << endl;
        if (iteration < number_size)
        {
            cout << "Iteration number: " << iteration << " of " << number_size << endl;
            cout << "Binary number start: " << start << " of " << finish << endl;
            cout << "Writing cache file " << cache_file_name << endl;
            ofstream cache_file(cache_file_name, ios_base::binary);
            if (cache_file.fail())
            {
                throw runtime_error("Could not open cache file " + cache_file_name);
            }
            cache_file.write((char*)&iteration, sizeof(size_t));
            if (cache_file.fail())
            {
                throw runtime_error("Could not write iteration number to cache file " + cache_file_name);
            }
            cache_file.write((char*)&start, sizeof(size_t));
            if (cache_file.fail())
            {
                throw runtime_error("Could not write binary number start to cache file " + cache_file_name);
            }
            cache_file.write((char*)(output_buffer.data()), output_buffer_size * sizeof(uint32_t));
            cache_file.close();
            cout << "Cache file " << cache_file_name << " written" << endl;
        }
        else
        {
            cout << "Writing " << output_file_name << endl;
            ofstream output_file(output_file_name, ios_base::binary);
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
            cout << output_file_name << " written"  << endl;;
        }
        return EXIT_SUCCESS;
    }
    catch (exception& e)
    {
        cout << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
