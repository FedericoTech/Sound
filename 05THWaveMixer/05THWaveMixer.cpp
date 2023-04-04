#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

using namespace std;

const int NUM_SAMPLES = 44100; // Total number of samples in the audio file
const int BYTES_PER_SAMPLE = 2; // Number of bytes per sample (16-bit audio)
const int CHUNK_SIZE = NUM_SAMPLES * BYTES_PER_SAMPLE + 36; // Size of the data chunk in the WAV file
const int SAMPLE_RATE = 44100; // Sample rate of the audio file
const int NUM_THREADS = 4; // Number of threads to use for parallel processing

void mergeBuffers(const vector<short>& buffer1, const vector<short>& buffer2, vector<short>& mergedBuffer, int startIndex, int endIndex)
{
    for (int i = startIndex; i < endIndex; i++) {
        mergedBuffer[i] = buffer1[i] + buffer2[i];
    }
}

int main()
{
    // Read the audio data into the buffers
    ifstream inFile1("output.wav", ios::binary);
    ifstream inFile2("output2.wav", ios::binary);
    if (!inFile1 || !inFile2) {
        cerr << "Error: could not open input file" << endl;
        return 1;
    }
    inFile1.ignore(44); // Ignore the WAV header
    inFile2.ignore(44); // Ignore the WAV header
    vector<short> buffer1(NUM_SAMPLES);
    vector<short> buffer2(NUM_SAMPLES);
    inFile1.read((char*)&buffer1[0], NUM_SAMPLES * BYTES_PER_SAMPLE);
    inFile2.read((char*)&buffer2[0], NUM_SAMPLES * BYTES_PER_SAMPLE);

    // Merge the audio data using multiple threads
    vector<short> mergedBuffer(NUM_SAMPLES);
    vector<thread> threads(NUM_THREADS);
    int chunkSize = NUM_SAMPLES / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; i++) {
        int startIndex = i * chunkSize;
        int endIndex = (i == NUM_THREADS - 1) ? NUM_SAMPLES : startIndex + chunkSize;
        threads[i] = thread(mergeBuffers, ref(buffer1), ref(buffer2), ref(mergedBuffer), startIndex, endIndex);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }

    // Write the merged audio data to a WAV file
    ofstream outFile("output3.wav", ios::binary);
    if (!outFile) {
        cerr << "Error: could not open output file" << endl;
        return 1;
    }
    outFile.write("RIFF", 4);
    outFile.write((char*)&CHUNK_SIZE, 4);
    outFile.write("WAVE", 4);
    outFile.write("fmt ", 4);
    outFile.write("\x10\0\0\0", 4); // Subchunk1Size (16 bytes)
    outFile.write("\x01\0", 2); // AudioFormat (PCM)
    outFile.write("\x01\0", 2); // NumChannels (1)
    outFile.write((char*)&SAMPLE_RATE, 4);
    const int ByteRate = SAMPLE_RATE * BYTES_PER_SAMPLE;
    outFile.write((char*)&ByteRate, 4); // ByteRate
    outFile.write("\x02\0", 2); // BlockAlign (2 bytes per sample)
    outFile.write("\x10\0", 2); // BitsPerSample (

    outFile.write("data", 4);

    const int Subchunk2Size = NUM_SAMPLES * BYTES_PER_SAMPLE;
    outFile.write((char*)&Subchunk2Size, 4); // Subchunk2Size
    outFile.write((char*)&mergedBuffer[0], NUM_SAMPLES * BYTES_PER_SAMPLE);

    return 0;
}