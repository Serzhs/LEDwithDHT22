#include "led-matrix.h"
#include "graphics.h"
#include "DHT22.h"

using namespace rgb_matrix;

#include <time.h>
#include <wiringPi.h>
#include <string>
#include <math.h>
#include <unistd.h>
#include <cstring>
#include <stdio.h>

#define DHT_PIN 8 // WiringPi 2 = BCM 27 = connector pin 13

int delayInS = 1;

int main(int argc, char *argv[])
{
    if (wiringPiSetup() == -1)
    {
        exit(1);
    }

    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;

    // LED options
    matrix_options.hardware_mapping = "regular";
    matrix_options.rows = 64;
    matrix_options.cols = 64;
    matrix_options.chain_length = 1;
    matrix_options.parallel = 1;
    matrix_options.brightness = 30;
    runtime_opt.drop_privileges = 0;
    runtime_opt.gpio_slowdown = 4;

    // Init LED matrix
    RGBMatrix *matrix = RGBMatrix::CreateFromFlags(&argc, &argv, &matrix_options, &runtime_opt);

    if (matrix == NULL)
    {
        return 1;
    }
    // Init DHT22 sensor
    TDHT22 *DHT22Sensor = new TDHT22(DHT_PIN);
    DHT22Sensor->Init(); // calls the wiringPiSetup

    // Set colors
    Color days_color(100, 10, 10);
    Color time_color(0, 100, 0);
    Color date_color(255, 255, 0);
    Color tmp_color(255, 100, 0);
    Color hum_color(0, 179, 255);

    // Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font dayFont;
    rgb_matrix::Font dateFont;
    rgb_matrix::Font timeFont;
    rgb_matrix::Font tmpFont;

    if (
        !dayFont.LoadFont("../fonts/8x13B.bdf") ||
        !dateFont.LoadFont("../fonts/6x13B.bdf") ||
        !timeFont.LoadFont("../fonts/texgyre-27.bdf") ||
        !tmpFont.LoadFont("../fonts/6x13B.bdf"))
    {
        fprintf(stderr, "Couldn't load One of necessary fonts");
        return 1;
    }

    // interactive session
    if (isatty(STDIN_FILENO))
    {
        printf("Enjoy you LED panel :) \n");
    }

    FrameCanvas *offscreen = matrix->CreateFrameCanvas();

    // Init time variables
    const char *weekdaysArr[7] = {"7diena", "1diena", "2diena", "3diena", "4diena", "5diena", "6diena"};
    char dayInfo[7];
    char dateInfo[255];
    char timeInfo[7];
    char tmpInfo[7];
    char humInfo[7];
    struct timespec next_time;
    next_time.tv_sec = time(NULL);
    next_time.tv_nsec = 0;
    struct tm tm;

    while (true)
    {
        struct TemperatureAndHumidityValues temperatureAndHumidityValues = getTemperatureAndHumidityValues(DHT22Sensor);

        offscreen->Fill(0, 0, 0);

        localtime_r(&next_time.tv_sec, &tm);

        // Get day info
        strftime(dayInfo, sizeof(dayInfo), "%w", &tm);
        // Get date info
        strftime(dateInfo, sizeof(dateInfo), "(%d.%m)", &tm);
        // Get time info
        strftime(timeInfo, sizeof(timeInfo), "%H:%M", &tm);
        // Get temperature info
        sprintf(tmpInfo, "%d C", temperatureAndHumidityValues.temperatureValue);
        // Get humidity info
        sprintf(humInfo, "%d%%", temperatureAndHumidityValues.humidityValue);

        // Draw day info
        rgb_matrix::DrawText(
            offscreen,
            dayFont,
            // center text
            32 - (((strlen(weekdaysArr[dayInfo[0] - '0'])) * dayFont.CharacterWidth('M')) / 2),
            1 + dayFont.baseline(),
            days_color,
            NULL,
            weekdaysArr[dayInfo[0] - '0'],
            0);

        // Draw date info
        rgb_matrix::DrawText(
            offscreen,
            dateFont,
            // center text
            32 - ((strlen(dateInfo) * dateFont.CharacterWidth('M')) / 2),
            2 + dayFont.baseline() + dateFont.baseline(),
            date_color,
            NULL,
            dateInfo,
            0);
        // Draw time info
        rgb_matrix::DrawText(
            offscreen,
            timeFont,
            2,
            5 + dateFont.baseline() + timeFont.baseline(),
            time_color,
            NULL,
            timeInfo,
            -1);
        // Draw temperature info
        rgb_matrix::DrawText(
            offscreen,
            tmpFont,
            2,
            51 + tmpFont.baseline(),
            tmp_color,
            NULL,
            tmpInfo,
            0);

        // Draw humidity info
        rgb_matrix::DrawText(
            offscreen,
            tmpFont,
            60 - (strlen(humInfo) * tmpFont.CharacterWidth('M')),
            51 + tmpFont.baseline(),
            hum_color,
            NULL,
            humInfo,
            1);

        // Wait until we're ready to show it.
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_time, NULL);

        // Atomic swap with double buffer
        offscreen = matrix->SwapOnVSync(offscreen);

        // Update time
        next_time.tv_sec += delayInS;

        // change to 1s
        delay(delayInS * 1000);
    }

    // Finished. Shut down the RGB matrix.
    delete matrix;
    delete DHT22Sensor;

    return 0;
}
