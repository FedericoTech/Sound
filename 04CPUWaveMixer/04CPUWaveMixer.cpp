#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

const int SAMPLE_RATE = 44100; // sample rate in Hz
const int BYTES_PER_SAMPLE = 2; // 16-bit audio

int main() {
    // Load the WAV files into memory
    ifstream inFile1("output.wav", ios::binary);
    if (!inFile1) {
        cerr << "Error: could not open input file 1" << endl;
        return 1;
    }
    ifstream inFile2("output2.wav", ios::binary);
    if (!inFile2) {
        cerr << "Error: could not open input file 2" << endl;
        return 1;
    }

    // Read the WAV headers
    inFile1.seekg(0, ios::beg);
    inFile2.seekg(0, ios::beg);

    char chunkID[4];
    int chunkSize;
    char format[4];
    char subchunk1ID[4];
    int subchunk1Size;
    short audioFormat;
    short numChannels;
    int sampleRate;
    int byteRate;
    short blockAlign;
    short bitsPerSample;
    char subchunk2ID[4];
    int subchunk2Size;

    inFile1.read(chunkID, 4);
    inFile1.read((char*)&chunkSize, 4);
    inFile1.read(format, 4);
    inFile1.read(subchunk1ID, 4);
    inFile1.read((char*)&subchunk1Size, 4);
    inFile1.read((char*)&audioFormat, 2);
    inFile1.read((char*)&numChannels, 2);
    inFile1.read((char*)&sampleRate, 4);
    inFile1.read((char*)&byteRate, 4);
    inFile1.read((char*)&blockAlign, 2);
    inFile1.read((char*)&bitsPerSample, 2);
    inFile1.read(subchunk2ID, 4);
    inFile1.read((char*)&subchunk2Size, 4);

    inFile2.read(chunkID, 4);
    inFile2.read((char*)&chunkSize, 4);
    inFile2.read(format, 4);
    inFile2.read(subchunk1ID, 4);
    inFile2.read((char*)&subchunk1Size, 4);
    inFile2.read((char*)&audioFormat, 2);
    inFile2.read((char*)&numChannels, 2);
    inFile2.read((char*)&sampleRate, 4);
    inFile2.read((char*)&byteRate, 4);
    inFile2.read((char*)&blockAlign, 2);
    inFile2.read((char*)&bitsPerSample, 2);
    inFile2.read(subchunk2ID, 4);
    inFile2.read((char*)&subchunk2Size, 4);

    // Check that the WAV files have the same format and sample rate
    if (audioFormat != 1 || numChannels != 1 || sampleRate != SAMPLE_RATE || bitsPerSample != 8 * BYTES_PER_SAMPLE) {
        cerr << "Error: input files must be 16-bit mono WAV files with a sample rate of 44.1 kHz" << endl;
        return 1;
    }

    // Compute the number of samples and the total file size
    const int NUM_SAMPLES = subchunk2Size / BYTES_PER_SAMPLE;
    const int CHUNK_SIZE = 36 + subchunk2Size;

    // Merge the audio data
    vector<short> samples1(NUM_SAMPLES);
    vector<short> samples2(NUM_SAMPLES);
    //inFile


    // Read the audio data into the buffers
    for (int i = 0; i < NUM_SAMPLES; i++) {
        short sample1, sample2;
        inFile1.read((char*)&sample1, BYTES_PER_SAMPLE);
        inFile2.read((char*)&sample2, BYTES_PER_SAMPLE);
        samples1[i] = sample1;
        samples2[i] = sample2;
    }

    // Merge the audio data
    vector<short> mergedSamples(NUM_SAMPLES);
    for (int i = 0; i < NUM_SAMPLES; i++) {

        mergedSamples[i] = (samples1[i] + samples2[i]) >> 1;
        //mergedSamples[i] = (samples1[i] + samples2[i]);
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
    outFile.write((char*)&subchunk1Size, 4);
    outFile.write((char*)&audioFormat, 2);
    outFile.write((char*)&numChannels, 2);
    outFile.write((char*)&SAMPLE_RATE, 4);
    outFile.write((char*)&byteRate, 4);
    outFile.write((char*)&blockAlign, 2);
    outFile.write((char*)&bitsPerSample, 2);
    outFile.write("data", 4);
    outFile.write((char*)&subchunk2Size, 4);
    for (int i = 0; i < NUM_SAMPLES; i++) {
        outFile.write((char*)&mergedSamples[i], BYTES_PER_SAMPLE);
    }
    outFile.close();

    cout << "Merged audio data written to output.wav" << endl;

    return 0;
}