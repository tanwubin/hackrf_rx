#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <itpp/itbase.h>
#include <sys/time.h>
#include <unistd.h>
// #include <hackrf.h>
// #include "src/receiver/receiver.h"
#include <libhackrf/hackrf.h>

#define DEBUG 1

typedef enum
{
    TRANSCEIVER_MODE_OFF = 0,
    TRANSCEIVER_MODE_RX = 1,
    TRANSCEIVER_MODE_TX = 2,
    TRANSCEIVER_MODE_SS = 3,
} transceiver_mode_t;

typedef enum
{
    HW_SYNC_MODE_OFF = 0,
    HW_SYNC_MODE_ON = 1,
} hw_sync_mode_t;

#define SAMP_RATE 10e6
#define FREQ_CENTRE 900e6
#define LENGTH 153600
// #define LENGTH 5
#define CAPLENGTH LENGTH // number of complex samples o capture.

/********global************/

int8_t hackrf_rx_buf[CAPLENGTH * 2]; // used for capture_data() and hackrf rx callback
double cap_buf[CAPLENGTH * 2];       // Convert to complex
size_t hackrf_rx_count;

int rx_callback(hackrf_transfer *transfer)
{
    printf("=============================\n");
    size_t bytes_to_write;
    size_t hackrf_rx_count_new = hackrf_rx_count + transfer->valid_length;
    printf("After This Receive,Data Len:%ld = %ld(his) + %d(now)\n", hackrf_rx_count_new, hackrf_rx_count, transfer->valid_length);
    int count_left = (CAPLENGTH * 2) - hackrf_rx_count_new;
    printf("Left to receive:%d\n", count_left);
    if (count_left <= 0)
    {
        printf("!Write (CAPLENGTH * 2) - hackrf_rx_count(his total cnt) = %ld\n", (CAPLENGTH * 2) - hackrf_rx_count);
        bytes_to_write = transfer->valid_length + count_left;
    }
    else
    {
        bytes_to_write = transfer->valid_length;
    }
    printf("Now  receive:%7d\nHad received:%7ld\nNow    write:%7ld\n", transfer->valid_length, hackrf_rx_count, bytes_to_write);
    if (bytes_to_write != 0)
    {
        memcpy(hackrf_rx_buf + hackrf_rx_count, transfer->buffer, bytes_to_write);
        // for (size_t i=0; i<bytes_to_write; i++) {
        //     //  hackrf_rx_buf[hackrf_rx_count+i] = transfer->buffer[i];
        //     printf("%5d", transfer->buffer[i]);
        // }
        hackrf_rx_count = hackrf_rx_count + bytes_to_write;
    }

    return (0);
}

int rx(hackrf_device *dev)
{
    struct timeval time_start;
    int result = 0;
    gettimeofday(&time_start, NULL);

#ifdef DEBUG
    printf("time_start:%ld\n", 1000000 * time_start.tv_sec + time_start.tv_usec);
#endif
    result = hackrf_start_rx(dev, rx_callback, NULL);
    while ((hackrf_is_streaming(dev) == HACKRF_TRUE))
    {
        // struct timeval time_now;
        // gettimeofday(&time_now, NULL);

        //         if ((1000000 * (time_now.tv_sec - time_start.tv_sec) + time_now.tv_usec - time_start.tv_usec) > 3000000)
        //         {
        // #ifdef DEBUG
        //             printf("time_now : %ld\n", time_now.tv_usec);
        //             printf("time_gap : %ld\n", 1000000 * (time_now.tv_sec - time_start.tv_sec) + time_now.tv_usec - time_start.tv_usec);
        // #endif
        //             break;
        //         }
        if (hackrf_rx_count == (CAPLENGTH * 2))
        {
            printf("=============================\n");
            printf("receive caplength complex sample.\n");
            break;
        }
        result = hackrf_is_streaming(dev);

        // Convert to complex
        for (int t = 0; t < CAPLENGTH; t++)
        {
            // printf("hackrf_is_streaming:%d\n", result);
            cap_buf[t << 1] = (((double)hackrf_rx_buf[(t << 1)]) - 0.0) / 128.0;
            cap_buf[(t << 1) + 1] = (((double)hackrf_rx_buf[(t << 1) + 1]) - 0.0) / 128.0;
            // printf("%d   %d\n", hackrf_rx_buf[(t << 1)], hackrf_rx_buf[(t << 1) + 1]);
            printf("%f + %fi\n", cap_buf[t << 1], cap_buf[(t << 1) + 1]);
        }
    }

    result = hackrf_stop_rx(dev);
    if (result != HACKRF_SUCCESS)
    {
        fprintf(stderr, "hackrf_stop_rx() failed: (%d)\n", result);
    }
    else
    {
        fprintf(stderr, "hackrf_stop_rx() done\n");
    }
    result = hackrf_close(dev);
    if (result != HACKRF_SUCCESS)
    {
        fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
    }
    else
    {
        fprintf(stderr, "hackrf_close() done\n");
    }
    // hackrf_exit();
    fprintf(stderr, "hackrf_exit() done\n");
    return result;
}

int config_hackrf(hackrf_device **dev, const int16_t gain)
{
    unsigned int lna_gain = 28; // default value
    unsigned int vga_gain = 20; // default value
    if (gain != -9999)
        vga_gain = (gain / 2) * 2;

    int result = hackrf_init();
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_init() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }

    result = hackrf_open(dev);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_open() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }

    double sampling_rate = SAMP_RATE;
    // Sampling frequency
    // result = hackrf_set_sample_rate_manual(*dev, sampling_rate, 1);
    result = hackrf_set_sample_rate(*dev, sampling_rate);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }

    result = hackrf_set_baseband_filter_bandwidth(*dev, 10000000*0.75);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }

    result = hackrf_set_vga_gain(*dev, vga_gain);
    result |= hackrf_set_lna_gain(*dev, lna_gain);

    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_set_vga_gain hackrf_set_lna_gain failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }

    // Center frequency
    result = hackrf_set_freq(*dev, FREQ_CENTRE);
    if (result != HACKRF_SUCCESS)
    {
        printf("config_hackrf hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)result), result);
        return (result);
    }
    return (result);
}

int main()
{
    hackrf_device *hackrf_dev = NULL;
    int result = 0;

    if (config_hackrf(&hackrf_dev, 20) == 0)
    {
        printf("HACKRF device FOUND!\n");
    }
    else
    {
        printf("HACKRF device not FOUND!\n");
    }

    rx(hackrf_dev);
}
