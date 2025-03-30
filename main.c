#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define FREQUENCY 100.0
#define AMPLITUDE 0.5
#define BUFFER_SIZE 4096
#define WAV_HEADER_SIZE 44

typedef struct {
    float phase;
    int playing;
} AudioData;

void generate_audio_data(FILE *file, AudioData *data) {
    float buffer[BUFFER_SIZE * 2];  // Stereo buffer (left & right)
    float phaseStep = 2.0 * M_PI * FREQUENCY / SAMPLE_RATE;

    // Generate audio data: sine wave
    for (int i = 0; i < BUFFER_SIZE * 2; i += 2) {
        if (data->playing) {
            float sampleValue = AMPLITUDE * sinf(data->phase);
            buffer[i] = sampleValue;     // Left channel
            buffer[i + 1] = sampleValue; // Right channel
            data->phase += phaseStep;
        } else {
            buffer[i] = 0.0f;
            buffer[i + 1] = 0.0f;
        }
    }

    // Write audio data to the file (in little-endian 16-bit signed format)
    for (int i = 0; i < BUFFER_SIZE * 2; i++) {
        int16_t sample = (int16_t)(buffer[i] * 32767);
        fwrite(&sample, sizeof(int16_t), 1, file);
    }
}

// Write the WAV file header
void write_wav_header(FILE *file) {
    uint8_t header[WAV_HEADER_SIZE] = {
        'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' 
    };

    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 2 * 2;  // 2 channels, 2 bytes per sample
    uint16_t blockAlign = 4;  // 2 channels * 2 bytes
    uint16_t bitsPerSample = 16;

    // Fill the header with the necessary information
    uint32_t chunkSize = 36 + BUFFER_SIZE * 2 * sizeof(int16_t);
    memcpy(&header[4], &chunkSize, 4);  // Chunk size
    memcpy(&header[16], &sampleRate, 4);  // Sample rate
    memcpy(&header[24], &byteRate, 4);   // Byte rate
    memcpy(&header[32], &blockAlign, 2);  // Block align
    memcpy(&header[34], &bitsPerSample, 2);  // Bits per sample

    fwrite(header, 1, WAV_HEADER_SIZE, file);
}

void toggle_audio_playback(AudioData *data) {
    char input;
    printf("Press SPACE to toggle sound, Q to quit: ");
    while (1) {
        input = getchar();
        if (input == ' ') {
            data->playing = !data->playing;
            printf("%s sound\n", data->playing ? "Starting" : "Stopping");
        } else if (input == 'q' || input == 'Q') {
            break;
        }
    }
}

int main() {
    FILE *file = fopen("output.wav", "wb");
    if (file == NULL) {
        printf("Error opening file for writing.\n");
        return -1;
    }

    AudioData data = {0, 0};

    write_wav_header(file); // Write the WAV header first
    toggle_audio_playback(&data);  // Start the input loop for toggling sound

    // Generate audio data and write it to the file
    for (int i = 0; i < SAMPLE_RATE; i++) {
        generate_audio_data(file, &data);
    }

    fclose(file);
    printf("WAV file created: output.wav\n");
    return 0;
}
