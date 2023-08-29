#include "led-matrix.h"
#include "graphics.h"
#include "DHT22.h"

using namespace rgb_matrix;

#include <wiringPi.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <unistd.h>
#include <cstring>

#define DHT_PIN 8         // WiringPi 2 = BCM 27 = connector pin 13
#define DHT_BUTTON_PIN 25 // WiringPi 25

int getDayIndex()
{
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    struct std::tm *localTime = std::localtime(&currentTime);
    return localTime->tm_wday;
}

std::string getFormattedDate()
{
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    char buffer[10];
    std::strftime(buffer, sizeof(buffer), "%d.%m", std::localtime(&currentTime));

    return buffer;
}

std::string getFormattedTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    char buffer[6];
    std::strftime(buffer, sizeof(buffer), "%H:%M", std::localtime(&currentTime));

    return buffer;
}

int main(int argc, char *argv[])
{
    if (wiringPiSetup() == -1)
    {
        exit(1);
    }

    int initial_brightness = 30;
    int brightness = initial_brightness;

    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;

    // LED options
    matrix_options.hardware_mapping = "regular";
    matrix_options.rows = 64;
    matrix_options.cols = 64;
    matrix_options.chain_length = 1;
    matrix_options.parallel = 1;
    matrix_options.brightness = initial_brightness;
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

    // init buttons
    pinMode(DHT_BUTTON_PIN, INPUT);

    // Set colors
    Color days_color(100, 10, 10);
    Color time_color(0, 100, 0);
    Color brigtness_color(255, 255, 255);
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

    char tmpInfo[7];
    char humInfo[7];

    while (true)
    {
        int buttonState = digitalRead(DHT_BUTTON_PIN);
        offscreen->Clear();

        // brightness button
        if (buttonState == HIGH)
        {
            matrix->SetBrightness(brightness); = brightness == 100 ? 10 : brightness + 10;
            matrix->SetBrightness(brightness);

            rgb_matrix::DrawText(
                offscreen,
                dateFont,
                32 - (9 * dateFont.CharacterWidth('M') / 2),
                20,
                brigtness_color,
                NULL,
                "Brigtness",
                0);

            rgb_matrix::DrawText(
                offscreen,
                dayFont,
                32 - ((strlen(std::to_string(brightness).c_str()) * dayFont.CharacterWidth('M')) / 2),
                35,
                brigtness_color,
                NULL,
                std::to_string(brightness).c_str(),
                0);

            offscreen = matrix->SwapOnVSync(offscreen);

            delay(500);

            continue;
        }

        struct TemperatureAndHumidityValues temperatureAndHumidityValues = getTemperatureAndHumidityValues(DHT22Sensor);

        int dayIndex = getDayIndex();
        std::string formattedDate = getFormattedDate();
        std::string formattedTime = getFormattedTime();

        // Get temperature info
        sprintf(tmpInfo, "%d C", temperatureAndHumidityValues.temperatureValue);
        // Get humidity info
        sprintf(humInfo, "%d%%", temperatureAndHumidityValues.humidityValue);

        // // Draw day info
        rgb_matrix::DrawText(
            offscreen,
            dayFont,
            // center text
            32 - (((strlen(weekdaysArr[dayIndex])) * dayFont.CharacterWidth('M')) / 2),
            1 + dayFont.baseline(),
            days_color,
            NULL,
            weekdaysArr[dayIndex],
            0);

        // // Draw date info
        rgb_matrix::DrawText(
            offscreen,
            dateFont,
            // center text
            32 - ((formattedDate.length() * dateFont.CharacterWidth('M')) / 2),
            2 + dayFont.baseline() + dateFont.baseline(),
            date_color,
            NULL,
            formattedDate.c_str(),
            0);

        // // Draw time info
        rgb_matrix::DrawText(
            offscreen,
            timeFont,
            2,
            5 + dateFont.baseline() + timeFont.baseline(),
            time_color,
            NULL,
            formattedTime.c_str(),
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

        // Atomic swap with double buffer
        offscreen = matrix->SwapOnVSync(offscreen);

        delay(100);
    }

    // Finished. Shut down the RGB matrix.
    delete matrix;
    delete DHT22Sensor;

    return 0;
}
