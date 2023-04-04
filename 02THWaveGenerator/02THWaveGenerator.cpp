#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>

constexpr float TWO_PI = 6.28318530718;

const int NUM_THREADS = 8; // Number of threads to use for parallel processing

constexpr int DURATION = 4440;                        // length in seconds
constexpr int SAMPLE_RATE = 22050; // 44100;                  // Sample rate of the audio file
constexpr int NUM_SAMPLES = SAMPLE_RATE * DURATION * 2; // Total number of samples in the audio file
constexpr int BYTES_PER_SAMPLE = 2;                 // Number of bytes per sample (16-bit audio)
constexpr short NUM_CHANNELS = 1;                   // Number of channels Mono audio
constexpr int BYTE_RATE = SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE;    // Byte rate
constexpr short BLOCK_ALIGN = NUM_CHANNELS * BYTES_PER_SAMPLE;              // Block align
constexpr short BITS_PER_SAMPLE = 16 * BYTES_PER_SAMPLE;                    // Bits per sample
constexpr int SUBCHUNK_SIZE = NUM_SAMPLES * BYTES_PER_SAMPLE;

constexpr short AUDIO_FORMAT = 1; // PCM audio

constexpr int FREQUENCY = 200;                      // wave frequency


int main() {
    std::ofstream outFile("R:\THoutput.wav", std::ios::out | std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: could not open output file" << std::endl;
        return 1;
    }

    // Write the WAV header
    const int SUBCHUNK_SIZE = NUM_SAMPLES * BYTES_PER_SAMPLE;
    const int CHUNK_SIZE = 36 + SUBCHUNK_SIZE;

    outFile << "RIFF"; // Chunk ID
    outFile.write(reinterpret_cast<const char*>(&CHUNK_SIZE), 4); // Chunk size
    outFile << "WAVE"; // Format
    outFile << "fmt "; // Subchunk 1 ID
    const int Subchunk = 16;
    outFile.write(reinterpret_cast<const char*>(&Subchunk), 4); // Subchunk 1 size
    outFile.write(reinterpret_cast<const char*>(&AUDIO_FORMAT), 2); // Audio format
    outFile.write(reinterpret_cast<const char*>(&NUM_CHANNELS), 2); // Number of channels
    outFile.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4); // Sample rate
    outFile.write(reinterpret_cast<const char*>(&BYTE_RATE), 4); // Byte rate
    outFile.write(reinterpret_cast<const char*>(&BLOCK_ALIGN), 2); // Block align
    outFile.write(reinterpret_cast<const char*>(&BITS_PER_SAMPLE), 2); // Bits per sample
    outFile << "data"; // Subchunk 2 ID
    outFile.write(reinterpret_cast<const char*>(&SUBCHUNK_SIZE), 4); // Subchunk 2 size

    // Get the current time
    auto start_time = std::chrono::high_resolution_clock::now();

    const int chunkSize = NUM_SAMPLES / NUM_THREADS;

    short (*output)[chunkSize] = new short[NUM_THREADS][chunkSize];
    std::vector<std::thread> threads(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {

        int startIndex = i * chunkSize;
        
        int endIndex = (i == NUM_THREADS - 1) ? NUM_SAMPLES : startIndex + chunkSize;

        threads[i] = std::thread(
            [startIndex, endIndex, chunkSize](short* output) {

                for (int j = 0; j < chunkSize && j < endIndex - startIndex; j++) {
                    
                    const double t = static_cast<double>(j) / SAMPLE_RATE;      // time in seconds

                    const double sample = 32760 * sin(TWO_PI * FREQUENCY * t);  // 16-bit amplitude

                    output[j] = static_cast<short>(sample);                     // convert to 16-bit integer
                }
            },
            output[i]
        );
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }

    
    
    for (int i = 0; i < NUM_THREADS; i++) {
        outFile.write(reinterpret_cast<const char*>(output[i]), chunkSize * 2);
    }
    

    outFile.close();

    // Get the current time again
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the elapsed time
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Print the elapsed time
    std::cout << "Elapsed time: " << elapsed_time << " ms" << std::endl;

    return 0;
}