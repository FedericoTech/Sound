//04GPUWaveMixer.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

constexpr int DURATION = 4440;                        // length in seconds
constexpr int SAMPLE_RATE = 22050; // 44100;                  // Sample rate of the audio file
constexpr int NUM_SAMPLES = SAMPLE_RATE * DURATION * 2; // Total number of samples in the audio file
constexpr int BYTES_PER_SAMPLE = 2;                 // Number of bytes per sample (16-bit audio)
constexpr short NUM_CHANNELS = 1;                   // Number of channels Mono audio
constexpr int BYTE_RATE = SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE;    // Byte rate
constexpr short BLOCK_ALIGN = NUM_CHANNELS * BYTES_PER_SAMPLE;              // Block align
constexpr short BITS_PER_SAMPLE = 16 * BYTES_PER_SAMPLE;                    // Bits per sample
constexpr int SUBCHUNK_SIZE = NUM_SAMPLES * BYTES_PER_SAMPLE;
                  
constexpr int FREQUENCY = 200;                      // wave frequency

const char* vertexShaderSource = R"(
    #version 330 core
    uniform float sample_rate;
    uniform float frequency;
    uniform int period;

    out int wave_output;
    
    void main()
    {
        const float TWO_PI = 6.28318530718; 
        const float MAX_AMPLITUDE = 32760; //max 16bit value to prevent distorsions

        //we use gl_VertexID to track time but we limit it by the period so that we avoid overflow
        float i = float((gl_VertexID % period) * 2);

        //sample A  present second
        float t = i / sample_rate;                                              // time in seconds
        int sampleA = int(MAX_AMPLITUDE * sin(TWO_PI * frequency * t)) << 16;   // 16-bit amplitude

        //sample B  a second ahead
        t = (i + 1) / sample_rate;                                              // next second
        int sampleB = int(MAX_AMPLITUDE * sin(TWO_PI * frequency * t)) ;        // 16-bit amplitude

        wave_output = sampleA | (0x0000FFFF & sampleB); // we pack the two samples together back to the CPU

    }
)";

int printError()
{
    std::cout << glewGetErrorString(glGetError()) << std::endl;
    glfwTerminate();
    exit(1);
}

GLint success;
GLchar infoLog[512];

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
    const GLchar* feedbackVaryings[] = { "wave_output" };
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

int saveWave(const char* filename, const char* data)
{
    // Write the merged audio data to a new file
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: could not open output file" << std::endl;
        return false;
    }
    int chunkSize = 36 + SUBCHUNK_SIZE;
    outFile << "RIFF";                                                  // Chunk ID
    outFile.write(reinterpret_cast<char*>(&chunkSize), 4);              // Chunk size
    outFile << "WAVE";                                                  // Format
    outFile << "fmt ";                                                  // Subchunk 1 ID
    outFile.write("\x10\x00\x00\x00", 4);                               // Subchunk1Size
    outFile.write("\x01\x00", 2);                                       // AudioFormat PCM audio
    outFile.write(reinterpret_cast<const char*>(&NUM_CHANNELS), 2);     // Number of channels Mono audio
    outFile.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4);      // Sample rate
    outFile.write(reinterpret_cast<const char*>(&BYTE_RATE), 4);        // Byte rate
    outFile.write(reinterpret_cast<const char*>(&BLOCK_ALIGN), 2);      // Block align
    outFile.write(reinterpret_cast<const char*>(&BITS_PER_SAMPLE), 2);  // Bits per sample
    outFile << "data";                                                  // Subchunk 2 ID
    outFile.write(reinterpret_cast<const char*>(&SUBCHUNK_SIZE), 4);    // Subchunk 2 size
    outFile.write(data, SUBCHUNK_SIZE);
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

    GLint sample_rate = glGetUniformLocation(shaderProgram, "sample_rate");
    glUniform1f(sample_rate, static_cast<const GLfloat>(SAMPLE_RATE));

    GLint frequency = glGetUniformLocation(shaderProgram, "frequency");
    glUniform1f(frequency, static_cast<const GLfloat>(FREQUENCY));

    GLint period = glGetUniformLocation(shaderProgram, "period");
    glUniform1i(period, SAMPLE_RATE / FREQUENCY);


    // setup a buffer for retriving the data
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


    // Retrieve the transformed data from the buffer
    short* output = new short[NUM_SAMPLES];

    ///short output[NUM_SAMPLES];

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, NUM_SAMPLES * BYTES_PER_SAMPLE, output);

    // Get the current time again
    auto end_time = std::chrono::high_resolution_clock::now();
    /*
    for (int i = 0; i < 10; i += 2) {
        printf("%#04x %#04x\n", output[i], output[i + 1]);
    }
    */
    
    //saveWave("R:\GPUoutput.wav", reinterpret_cast<char*>(output)); //it can't be save in the in-memory drive
    saveWave("GPUoutput.wav", reinterpret_cast<char*>(output));

    // Clean up resources

    delete[] output;
    glDeleteBuffers(1, &tbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    


    // Calculate the elapsed time
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Print the elapsed time
    std::cout << "Elapsed time: " << elapsed_time << " ms" << std::endl;

    return 0;
}