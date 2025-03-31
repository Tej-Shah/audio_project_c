#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>     // For usleep()
#include <sys/select.h> // For non-blocking input (cross-platform)

#define SAMPLE_RATE 44100
#define FREQUENCY 440.0
#define AMPLITUDE 0.5
#define BUFFER_SIZE 4096
#define WAV_HEADER_SIZE 44

typedef struct {
    float phase;
    int playing;
} AudioData;

// Function to check for user input without blocking
int check_keyboard_input() {
    struct timeval tv = {0, 0};  // No wait time (non-blocking)
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);  // Monitor standard input (keyboard)
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

// Function to write a valid WAV header
void write_wav_header(FILE *file) {
    uint8_t header[WAV_HEADER_SIZE] = {
        'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,  // PCM, 2 channels
        0, 0, 0, 0,  // Sample rate placeholder
        0, 0, 0, 0,  // Byte rate placeholder
        4, 0, 16, 0,  // Block align and bits per sample
        'd', 'a', 't', 'a', 0, 0, 0, 0  // Data chunk
    };

    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 2 * 2;  // 2 channels * 16 bits (2 bytes)
    memcpy(&header[24], &sampleRate, 4);
    memcpy(&header[28], &byteRate, 4);

    fwrite(header, 1, WAV_HEADER_SIZE, file);
}

// Function to update the WAV header with the correct data size
void update_wav_header(FILE *file, uint32_t dataSize) {
    fseek(file, 40, SEEK_SET);
    fwrite(&dataSize, sizeof(uint32_t), 1, file);

    fseek(file, 4, SEEK_SET);
    uint32_t chunkSize = 36 + dataSize;
    fwrite(&chunkSize, sizeof(uint32_t), 1, file);
}

// Function to generate and write audio samples
void generate_audio_data(FILE *file, AudioData *data, uint32_t *totalDataSize) {
    float buffer[BUFFER_SIZE * 2];
    float phaseStep = 2.0 * M_PI * FREQUENCY / SAMPLE_RATE;

    for (int i = 0; i < BUFFER_SIZE * 2; i += 2) {
        if (data->playing) {
            float sampleValue = AMPLITUDE * sinf(data->phase);
            buffer[i] = sampleValue;     // Left channel
            buffer[i + 1] = sampleValue; // Right channel
            data->phase += phaseStep;

            if (data->phase >= 2 * M_PI) {
                data->phase -= 2 * M_PI;
            }
        } else {
            buffer[i] = 0.0f;
            buffer[i + 1] = 0.0f;
        }
    }

    for (int i = 0; i < BUFFER_SIZE * 2; i++) {
        int16_t sample = (int16_t)(buffer[i] * 32767);
        fwrite(&sample, sizeof(int16_t), 1, file);
    }

    *totalDataSize += BUFFER_SIZE * 2 * sizeof(int16_t);
}

int main() {
    FILE *file = fopen("output.wav", "wb");
    if (file == NULL) {
        printf("Error opening file for writing.\n");
        return -1;
    }

    write_wav_header(file);  // Write initial header

    AudioData data = {0, 1}; // Start with sound playing
    uint32_t totalDataSize = 0;

    printf("Press SPACE to toggle sound, Q to quit.\n");

    while (1) {
        generate_audio_data(file, &data, &totalDataSize);

        // Check for user input without blocking
        if (check_keyboard_input()) {
            char input = getchar();
            if (input == ' ') {
                data.playing = !data.playing;
                printf("%s sound\n", data.playing ? "Starting" : "Stopping");
            } else if (input == 'q' || input == 'Q') {
                break;
            }
        }

        usleep(5000); // Small delay to allow smooth execution
    }

    update_wav_header(file, totalDataSize); // Fix header with actual data size

    fclose(file);
    printf("WAV file created successfully: output.wav\n");
    return 0;
}
