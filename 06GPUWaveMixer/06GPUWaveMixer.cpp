//04GPUWaveMixer.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

constexpr int NUM_SAMPLES = 195804000; // Total number of samples in the audio file
constexpr int BYTES_PER_SAMPLE = 2; // Number of bytes per sample (16-bit audio)
constexpr int SAMPLE_RATE = 22050; // Sample rate of the audio file

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in int wave1;
    layout (location = 1) in int wave2;
    out int wave_mix;
    void main()
    {
        int a1 = (wave1 << 16) >> 16;   //we take the first sample to the end of the int and back to bring the sign!!
        int a2 = wave1 >> 16;           //we take the second sample to the beginning of the int

        int b1 = (wave2 << 16) >> 16;   //we take the first sample to the end of the int and back to bring the sign!!
        int b2 = wave2 >> 16;           //we take the second sample to the beginning of the int


        int sum1 = (a1 + b1) >> 1;      //we sum the sample of the first sound with the sample of the second sound and divided the result by 2
        int sum2 = (a2 + b2) >> 1;      //we sum the sample of the first sound with the sample of the second sound and divided the result by 2

        wave_mix = sum2 << 16 | (0x0000FFFF & sum1);    //now we pack the two samples back to the int
    }
)";

int printError()
{
    std::cout << glewGetErrorString(glGetError()) << std::endl;
    glfwTerminate();
    exit(1);
}

// Read the audio data into the buffers
std::vector<short> buffer1(NUM_SAMPLES);
std::vector<short> buffer2(NUM_SAMPLES);
std::vector<short> pMergedBuffer(NUM_SAMPLES);

GLint success;
GLchar infoLog[512];

bool loadWav(const char* filename, std::vector<short>& buffer)
{
    std::ifstream inFile(filename, std::ios::binary);

    //if any of the files isn't present...
    if (!inFile) {
        std::cerr << "Error: could not open input file" << std::endl;
        return false;
    }
    inFile.ignore(44); // Ignore the WAV header
    inFile.read(reinterpret_cast<char *>(buffer.data()), NUM_SAMPLES * BYTES_PER_SAMPLE);

    inFile.close();

    return true;
}

GLuint createShader(const char* vertexShaderSource)
{
    // Create the shader program and attach the vertex and fragment shaders
    GLuint shaderProgram = glCreateProgram();

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof infoLog, NULL, infoLog);
        std::cout << "Vertex shader compilation failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return -1;
    }

    // Attach the vertex shader to the program
    glAttachShader(shaderProgram, vertexShader);

    //we capture the output varying
    const GLchar* feedbackVaryings[] = { "wave_mix" };
    glTransformFeedbackVaryings(
        shaderProgram,
        1, //number of varying variables
        feedbackVaryings, //names of the varying variables //(const GLchar* const*)(&"outValue"),
        GL_INTERLEAVED_ATTRIBS
    );

    //link program
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, sizeof infoLog, NULL, infoLog);
        std::cout << "Error linking shader program:\n" << infoLog << std::endl;
        glfwTerminate();
        return -1;
    }

    //we can get rid of the compiled shader
    glDeleteShader(vertexShader);

    return shaderProgram;
}

GLuint createBuffer(GLuint shaderProgram, const void* data, const char* attribName) {
    GLuint waveHandle;

    glGenBuffers(1, &waveHandle);
    glBindBuffer(GL_ARRAY_BUFFER, waveHandle);
    glBufferData(GL_ARRAY_BUFFER, NUM_SAMPLES * BYTES_PER_SAMPLE, data, GL_STATIC_DRAW);

    GLint inputAttrib = glGetAttribLocation(shaderProgram, attribName);
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribIPointer(
        inputAttrib,
        1,          // the array is unidimensional 
        GL_INT,     // the array is typed integer
        0,          // stride to the next vertex attribute, 0 = tightly packed 
        0           // offset to the first component
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0); //we unbind the buffer

    return waveHandle;
}

int saveWave(const char* filename, const char* data, size_t dataSize)
{
    // Write the merged audio data to a new file
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: could not open output file" << std::endl;
        return false;
    }
    int chunkSize = 36 + dataSize;
    outFile.write("RIFF", 4);
    outFile.write(reinterpret_cast<char *>(&chunkSize), 4);
    outFile.write("WAVE", 4);
    outFile.write("fmt ", 4);
    outFile.write("\x10\x00\x00\x00", 4); // Subchunk1Size
    outFile.write("\x01\x00", 2); // AudioFormat

    const short NUM_CHANNELS = 1; // Mono audio
    outFile.write(reinterpret_cast<const char*>(&NUM_CHANNELS), 2);
    outFile.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4);

    const int BYTE_RATE = SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE; // Byte rate
    outFile.write(reinterpret_cast<const char*>(&BYTE_RATE), 4);

    const short BLOCK_ALIGN = NUM_CHANNELS * BYTES_PER_SAMPLE; // Block align
    outFile.write(reinterpret_cast<const char*>(&BLOCK_ALIGN), 2);

    const short BITS_PER_SAMPLE = 16 * BYTES_PER_SAMPLE; // Bits per sample
    outFile.write(reinterpret_cast<const char*>(&BITS_PER_SAMPLE), 2);
    outFile.write("data", 4);
    outFile.write(reinterpret_cast<const char*>(&dataSize), 4);
    outFile.write(data, dataSize);
    outFile.close();

    return true;
}

int main()
{
    // Initialize GLFW and create a window
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
        
    //we create a minimal window
    GLFWwindow* window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW and create the compute shader program
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_VENDOR) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;
    //std::cout << glGetString(GL_EXTENSIONS) << std::endl;

    GLuint shaderProgram = createShader(vertexShaderSource);

    glUseProgram(shaderProgram);

    //if any of the files aren't present...
    if (!loadWav("output.wav", buffer1) 
        || !loadWav("output2.wav", buffer2)) {

        std::cerr << "Error: could not open input file" << std::endl;
        return 1;
    }

    // Allocate and initialize the buffers on the GPU
    GLuint wave1Handle = createBuffer(shaderProgram, &buffer1[0], "wave1");
    GLuint wave2Handle = createBuffer(shaderProgram, &buffer2[0], "wave2");

    // setup a buffer for retriving 
    GLuint tbo;
    glGenBuffers(1, &tbo);
    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, NUM_SAMPLES * BYTES_PER_SAMPLE, nullptr, GL_STATIC_READ);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);

    // Get the current time
    auto start_time = std::chrono::high_resolution_clock::now();

    //we disable the raster stage (fragment shader) as we don't need it.
    glEnable(GL_RASTERIZER_DISCARD);

    // Begin transform feedback
    glBeginTransformFeedback(GL_POINTS);

    // Draw the points
    glDrawArrays(GL_POINTS, 0, NUM_SAMPLES/2);

    // End transform feedback
    glEndTransformFeedback();

    glFlush();

    //we enable back the raster stage (fragment shader)
    glDisable(GL_RASTERIZER_DISCARD);

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, NUM_SAMPLES * BYTES_PER_SAMPLE, &pMergedBuffer[0]);

    // Get the current time again
    auto end_time = std::chrono::high_resolution_clock::now();

    saveWave("output3.wav", reinterpret_cast<char*>(pMergedBuffer.data()), NUM_SAMPLES * BYTES_PER_SAMPLE);

    // Calculate the elapsed time
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Print the elapsed time
    std::cout << "Elapsed time: " << elapsed_time << " ms" << std::endl;

    // Clean up resources
    

    buffer1.clear();
    buffer2.clear();
    pMergedBuffer.clear();
    glDeleteBuffers(1, &tbo);
    glDeleteBuffers(1, &wave2Handle);
    glDeleteBuffers(1, &wave1Handle);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}