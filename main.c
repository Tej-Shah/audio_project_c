#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>     // For usleep()
#include <sys/select.h> // For non-blocking input (cross-platform)

#define SAMPLE_RATE 44100
#define AMPLITUDE 0.5
#define BUFFER_SIZE 4096
#define WAV_HEADER_SIZE 44
#define NOTE_DURATION 0.5  // Each note lasts 0.5 seconds

// Frequency mapping for numbers 0-7 (C Major scale)
const float FREQUENCIES[8] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};

typedef struct {
    float phase;
    float frequency;
} AudioData;

// Function to check for user input (blocking)
char get_user_input()
{
    char input;
    printf("Press 0-7 to play a note, Q to quit: ");
    input = getchar();
    while (getchar() != '\n'); // Clear input buffer
    return input;
}

// Function to write a valid WAV header
void write_wav_header(FILE *file)
{
    uint32_t sampleRate;
    uint32_t byteRate;
    
    sampleRate = SAMPLE_RATE;
    byteRate = SAMPLE_RATE * 2 * 2;     // 2 channels * 16 bits (2 bytes)
    uint8_t header[WAV_HEADER_SIZE] = {
        'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,  // PCM, 2 channels
        0, 0, 0, 0,  // Sample rate placeholder
        0, 0, 0, 0,  // Byte rate placeholder
        4, 0, 16, 0,  // Block align and bits per sample
        'd', 'a', 't', 'a', 0, 0, 0, 0  // Data chunk
    };

    memcpy(&header[24], &sampleRate, 4);
    memcpy(&header[28], &byteRate, 4);

    fwrite(header, 1, WAV_HEADER_SIZE, file);
}

// Function to update the WAV header with the correct data size
void update_wav_header(FILE *file, uint32_t dataSize)
{
    fseek(file, 40, SEEK_SET);
    fwrite(&dataSize, sizeof(uint32_t), 1, file);

    fseek(file, 4, SEEK_SET);
    uint32_t chunkSize = 36 + dataSize;
    fwrite(&chunkSize, sizeof(uint32_t), 1, file);
}

// Function to generate and write a single note
void generate_note(FILE *file, AudioData *data, uint32_t *totalDataSize)
{
    float buffer[BUFFER_SIZE * 2];
    uint32_t samplesToGenerate;
    float phaseStep;
    int samplesThisLoop;
    float sampleValue;
    int16_t sample;

    samplesToGenerate = (uint32_t)(SAMPLE_RATE * NOTE_DURATION);
    phaseStep = 2.0 * M_PI * data->frequency / SAMPLE_RATE;
    while (samplesToGenerate > 0) {
        samplesThisLoop = (samplesToGenerate > BUFFER_SIZE) ? BUFFER_SIZE : samplesToGenerate;
        
        for (int i = 0; i < samplesThisLoop * 2; i += 2) {
            sampleValue = AMPLITUDE * sinf(data->phase);
            buffer[i] = sampleValue;     // Left channel
            buffer[i + 1] = sampleValue; // Right channel
            data->phase += phaseStep;

            if (data->phase >= 2 * M_PI) {
                data->phase -= 2 * M_PI;
            }
        }

        for (int i = 0; i < samplesThisLoop * 2; i++) {
            sample = (int16_t)(buffer[i] * 32767);
            fwrite(&sample, sizeof(int16_t), 1, file);
        }

        *totalDataSize += samplesThisLoop * 2 * sizeof(int16_t);
        samplesToGenerate -= samplesThisLoop;
    }
}

int main()
{
    FILE *file = fopen("output.wav", "wb");
    uint32_t totalDataSize;
    char input;
    if (file == NULL)
    {
        printf("Error opening file for writing.\n");
        return (-1);
    }

    write_wav_header(file);  // Write initial header

    AudioData data = {0, FREQUENCIES[0]}; // Start at C4
    totalDataSize = 0;

    printf("Press 0-7 to play a note then press enter. Q to quit.\n");

    while (1) {
        input = get_user_input();

        if (input >= '0' && input <= '7') {
            data.frequency = FREQUENCIES[input - '0'];
            printf("Playing %.2f Hz\n", data.frequency);
            generate_note(file, &data, &totalDataSize);
        } else if (input == 'q' || input == 'Q') {
            break;
        }
    }

    update_wav_header(file, totalDataSize); // Fix header with actual data size

    fclose(file);
    printf("WAV file created successfully: output.wav\n");
    return 0;
}
